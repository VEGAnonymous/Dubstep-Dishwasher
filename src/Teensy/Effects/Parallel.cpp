#include "Teensy/Effects/Parallel.h"

/*  ADDRESSING SCHEME
    CHAIN A
    0-12:    Effect 1 parameter space
    13-16:   Effect 1 command space
    17-29:   Effect 2 parameter space
    30-33:   Effect 2 command space
    ...
    CHAIN B
    85-97:   Effect 1 parameter space
    98-101:   Effect 1 command space
    102-114:  Effect 2 parameter space
    115-118: Effect 2 command space
    ...
    254: Mix
    255: Mode
*/

/* PRIVATE */

/* 

enum Params : ParamID { MIX = 254, MODE = 255 }; // Meta parameters
enum LocalCommand : uint8_t { // Command slots per block
    ADD = 13,
    REMOVE = 14,
    REORDER = 15,
    BYPASS = 16
};

static constexpr uint8_t MAX_CHAIN_EFFECTS = 5; // For the preservation of my sanity
static constexpr uint8_t PARAMS_PER_EFFECT = 17; // Max 13 parameters + 4 command slots
static constexpr uint8_t CHAIN_BLOCK = MAX_CHAIN_EFFECTS * PARAMS_PER_EFFECT; // = 85
// Total addressing space = 2 * CHAIN_BLOCK = 170 (< 255)

float mix; ParallelMode mode;

struct Slot { // Store info about each slot in a chain
    bool occupied = false;
    EffectID id = 255;
    EffectName name = EffectName::GAIN;
};

AudioChain chainA, chainB; // Audio chains
std::array<Slot, MAX_CHAIN_EFFECTS> slotsA, slotsB; // Chain effect slot info
float* signalA = nullptr; float* signalB = nullptr; // Wet signal buffers

*/

// Addressing / routing helpers
bool Parallel::isChainB(ParamID pid) { return pid >= CHAIN_BLOCK; }
uint8_t Parallel::localIndex(ParamID pid) { return pid % CHAIN_BLOCK; }
uint8_t Parallel::effectIndex(ParamID pid) { return (localIndex(pid) / PARAMS_PER_EFFECT) % MAX_CHAIN_EFFECTS; }
uint8_t Parallel::paramIndex(ParamID pid) { return localIndex(pid) % PARAMS_PER_EFFECT; }

/* PUBLIC */

Parallel::Parallel(float mix, ParallelMode mode) {
    // Initialize slots to empty
    std::fill(slotsA.begin(), slotsA.end(), Slot{});
    std::fill(slotsB.begin(), slotsB.end(), Slot{});

    signalA = (float*)extmem_malloc(BUFFER_SIZE * sizeof(float)); if (!signalA) while(1){}; memset(signalA, 0, BUFFER_SIZE * sizeof(float));
    signalB = (float*)extmem_malloc(BUFFER_SIZE * sizeof(float)); if (!signalB) while(1){}; memset(signalB, 0, BUFFER_SIZE * sizeof(float));
    
    setMix(mix); setMode(mode);
}
Parallel::~Parallel() { if (signalA) extmem_free(signalA); if (signalB) extmem_free(signalB); }

void Parallel::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void Parallel::setMode(ParallelMode mode) { this->mode = mode; }
void Parallel::setParam(ParamID pid, float value) {
    // Reminder: pid = id2 slot in Command packet struct
    if (pid >= CHAIN_BLOCK * 2 && pid < MIX) return; // Reject out of range
    switch (pid) {
        case MIX: setMix(value); break;
        case MODE: setMode(static_cast<ParallelMode>(value)); break;
        default: // Command space
            bool chainB_ = isChainB(pid); // Select the chain to index into
            auto &chain = chainB_ ? chainB : chainA;
            auto &slots = chainB_ ? slotsB : slotsA;

            uint8_t effectIdx = effectIndex(pid);
            uint8_t param = paramIndex(pid);
            
            if (effectIdx >= MAX_CHAIN_EFFECTS) return;

            switch (param) { // id2, identify which command to perform on chain
                case LocalCommand::ADD: {
                    if (slots[effectIdx].occupied) break; // No slots available
                    if ((uint8_t)value > (uint8_t)EffectName::LIMITER) return; // DNE

                    EffectName name = static_cast<EffectName>((uint8_t)value);
                    if (auto* effect = chain.addEffect(name)) {
                        slots[effectIdx] = { 
                            true, // Set occupied
                            effect->getID(),
                            name
                        };
                    } 
                } break;
                case LocalCommand::REMOVE: {
                    if (!slots[effectIdx].occupied) break;

                    chain.removeEffect(slots[effectIdx].id);
                    slots[effectIdx] = Slot{}; // Reset slot
                } break;
                case LocalCommand::REORDER: {
                    if (slots[effectIdx].occupied)
                        // Pass id2, value -> id1, id2
                        chain.reorderEffect(slots[effectIdx].id, std::clamp<size_t>((size_t)value, 0, MAX_CHAIN_EFFECTS - 1));
                } break;       
                case LocalCommand::BYPASS: {
                    if (slots[effectIdx].occupied)
                        if (auto* effect = chain.getEffect(slots[effectIdx].id)) effect->setBypass(value >= 0.5f);
                } break;
                default: { // Parameter slot 0–11            
                    if (slots[effectIdx].occupied)
                        if (auto* effect = chain.getEffect(slots[effectIdx].id)) effect->setParam(param, value);
                } break;
            } // param
    } // pid
} // setParam

float Parallel::getParam(ParamID param) const {
    switch (param) {
        case MIX: return mix;
        case MODE: return (float)mode;
        // Other things? Probably not...
        default: return 0.0f;
    }
}

void Parallel::process(const float* in, float* out, size_t n) {
    // Process chains independently
    chainA.processChain(in, signalA, n);
    chainB.processChain(in, signalB, n);
    switch (mode) { // Mixdown
        case ParallelMode::SUM:
            for (size_t i = 0; i < n; ++i) out[i] = dryWetMix(in[i], (signalA[i] + signalB[i]) * 0.5f, mix);
            break;
        case ParallelMode::CROSSFADE: 
            for (size_t i = 0; i < n; ++i) out[i] = dryWetMix(signalA[i], signalB[i], mix);
            break;
    }
}