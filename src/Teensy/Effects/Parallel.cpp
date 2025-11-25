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

enum Params : ParamID { MIX, MODE };

enum Chain : uint8_t { A, B };
enum ChainCommand : uint8_t { 
    ADD, 
    REMOVE, 
    REORDER, 
    BYPASS, 
    SET_PARAM 
};

static constexpr uint8_t MAX_CHAIN_EFFECTS = 5;

float mix; ParallelMode mode;

AudioChain chainA, chainB; // Audio chains
float* signalA = nullptr; float* signalB = nullptr; // Wet signal buffers

*/

/* PUBLIC */

Parallel::Parallel(float mix, ParallelMode mode) {
    signalA = (float*)extmem_malloc(BUFFER_SIZE * sizeof(float)); if (!signalA) while(1){}; memset(signalA, 0, BUFFER_SIZE * sizeof(float));
    signalB = (float*)extmem_malloc(BUFFER_SIZE * sizeof(float)); if (!signalB) while(1){}; memset(signalB, 0, BUFFER_SIZE * sizeof(float));
    
    setMix(mix); setMode(mode);
}
Parallel::~Parallel() { if (signalA) extmem_free(signalA); if (signalB) extmem_free(signalB); }

bool Parallel::isParallel() const { return true; }

void Parallel::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void Parallel::setMode(ParallelMode mode) { this->mode = mode; }
void Parallel::setParam(ParamID param, float value) {
    switch (param) {
        case MIX: setMix(value); break;
        case MODE: setMode(static_cast<ParallelMode>(value)); break;
        default: break;
    }
}
float Parallel::getParam(ParamID param) const {
    switch (param) {
        case MIX: return mix;
        case MODE: return (float)mode;
        default: return 0.0f;
    }
}

void Parallel::chainCommand(uint8_t chainSelect, uint8_t command, EffectID effectId, ParamID paramId, float value) {
    // Select chain
    AudioChain& chain = (chainSelect == Chain::B) ? chainB : chainA;

    size_t effectID = static_cast<EffectID>(effectId);
    size_t paramID = static_cast<ParamID>(paramId);

    switch (command) {
        case ChainCommand::ADD: {
            EffectName name = static_cast<EffectName>(static_cast<uint8_t>(value));
            if (name == EffectName::PARALLEL) return; // No thanks
            chain.addEffect(name);
            break;
        }
        case ChainCommand::REMOVE: {
            chain.removeEffect(effectID);
            break;
        }
        case ChainCommand::REORDER: {
            size_t toIndex = static_cast<size_t>(value);
            chain.reorderEffect(effectID, toIndex);
            break;
        } 
        case ChainCommand::BYPASS: {
            Effect* effect = chain.getEffect(effectID);
            if (effect) effect->setBypass(value >= 0.5f);
            break;
        }
        case ChainCommand::SET_PARAM: {
            Effect* effect = chain.getEffect(effectID);
            if (effect) effect->setParam(paramID, value);
            break;
        }
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