#include "Teensy/Effects/Granulator.h"
#include "Teensy/Utilities/Utilities.h"

// HACK: Works, but produces clicks with larger positionRand since grains may end up reading from indices overwritten by write head
// Non-clicking usage: No reversed grains + any positionRand, OR reversed grains + no/small positionRand

/* PRIVATE */

/*
enum Params : ParamID { MIX, POSITION, POSITION_RAND, RATE, RATE_RAND, LENGTH, LENGTH_RAND, 
                        TUNE, TUNE_RAND, LEVEL, LEVEL_RAND, REVERSE_CHANCE, ENVELOPE_TYPE };

static constexpr size_t bufSize = 3 * (size_t)SAMPLE_RATE; // 3s running buffer
static constexpr int maxGrains = 32;

float mix, position, rate, length, tune, level, reverseChance;
float positionRand, rateRand, lengthRand, tuneRand, levelRand;
EnvelopeType envType;

float* inBuf = nullptr; size_t writePos = 0;
float grainCounter = 0.0f;

struct Grain {
    bool active = false;
    uint32_t startPos;
    float playhead;
    uint32_t length;
    float rate;
    float level;
    bool reverse;
};
std::vector<Grain> grains; // Grain pool

*/

void Granulator::spawnGrain() {
    // Find free grain
    Grain* freeGrain = nullptr;
    for (auto& grain : grains) if (!grain.active) { freeGrain = &grain; break; }
    // If none, steal oldest grain
    if (freeGrain == nullptr) {
        for (auto& grain : grains) if (grain.active) { freeGrain = &grain; break; }
    }
    
    // Calculate grain parameters
    float positionFactor = std::clamp(position + (uniform() * positionRand), 0.0f, 1.0f);
    int grainStartPos = (int)writePos - (int)(positionFactor * (bufSize - 1));
    if (grainStartPos < 0) grainStartPos += bufSize;
    
    float grainLength = std::clamp(length * (1.0f + (uniform() * lengthRand)), 5.0f, 1000.0f);
    int grainLengthSamples = std::min((int)(msSamples(grainLength)), (int)bufSize - 1);

    float grainLevel = std::clamp(level + (uniform() * 0.25f * levelRand), 0.0f, 1.0f) * 0.5f;
    
    bool reversed = ((uniform() + 1.0f) / 2.0f) <= reverseChance;

    float tuneOffset = tune * (1.0f + (uniform() * tuneRand));
    float grainRate = powf(2.0f, tuneOffset / 12.0f);
    
    // Init
    freeGrain->active = true;
    freeGrain->startPos = grainStartPos;
    freeGrain->playhead = 0;
    freeGrain->length = grainLengthSamples;
    freeGrain->rate = grainRate;
    freeGrain->level = grainLevel;
    freeGrain->reverse = reversed;
}

float Granulator::processGrain(Grain& grain) {
    // Calculate read position in input buffer
    float offset = grain.reverse ? ((grain.length - 1) - grain.playhead) : grain.playhead;
    float readPos = fmodf((float)grain.startPos + offset, (float)bufSize);
    if (readPos < 0) readPos += bufSize;
    
    float envelopeValue = getEnvelopeValue((float)grain.playhead / (float)grain.length, envType); // Envelope
    
    grain.playhead += grain.rate; // Advance grain playhead at rate
    if (grain.playhead >= grain.length) grain.active = false; // Free if done
    
    return lerp(inBuf, readPos, bufSize) * envelopeValue * grain.level;
}

/* PUBLIC */

Granulator::Granulator(float mix, float position, float positionRand, float time, float timeRand , 
                       float length, float lengthRand, float tune, float tuneRand, float level, 
                       float levelRand, float reverseChance, EnvelopeType envType) {
    setMix(mix); setPosition(position); setPositionRand(positionRand); setRate(time); setRateRand(timeRand); setLength(length); 
    setLengthRand(lengthRand); setTune(tune); setTuneRand(tuneRand); setLevel(level); setLevelRand(levelRand); 
    setReverseChance(reverseChance); setEnvelopeType(envType);
    grains.resize(maxGrains);
    inBuf = (float*)extmem_malloc(bufSize * sizeof(float)); if (!inBuf) while(1){}; memset(inBuf, 0, bufSize * sizeof(float));
}
Granulator::~Granulator() { if (inBuf) extmem_free(inBuf); }

void Granulator::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void Granulator::setPosition(float position) { this->position = std::clamp(position, 0.0f, 1.0f); } // [0.0, 1.0]
void Granulator::setPositionRand(float positionRand) { this->positionRand = std::clamp(positionRand, 0.0f, 1.0f); } // [0.0, 1.0]
void Granulator::setRate(float rate) { this->rate = std::clamp(rate, 1.0f, 500.0f); } // ms, [1.0, 500.0]
void Granulator::setRateRand(float rateRand) { this->rateRand = std::clamp(rateRand, 0.0f, 1.0f); } // [0.0, 1.0]
void Granulator::setLength(float length) { this->length = std::clamp(length, 5.0f, 500.0f); } // ms, [5.0, 500.0]
void Granulator::setLengthRand(float lengthRand) { this->lengthRand = std::clamp(lengthRand, 0.0f, 1.0f); } // [0.0, 1.0]
void Granulator::setTune(float tune) { this->tune = std::clamp(tune, -24.0f, 24.0f); } // semitones, [-24.0, 24.0]
void Granulator::setTuneRand(float tuneRand) { this->tuneRand = std::clamp(tuneRand, 0.0f, 1.0f); } // [0.0, 1.0]
void Granulator::setReverseChance(float reverseChance) { this->reverseChance = std::clamp(reverseChance, 0.0f, 1.0f); } // [0.0, 1.0]
void Granulator::setLevel(float level) { this->level = std::clamp(level, 0.0f, 1.0f); } // [0.0, 1.0]
void Granulator::setLevelRand(float levelRand) { this->levelRand = std::clamp(levelRand, 0.0f, 1.0f); } // [0.0, 1.0]
void Granulator::setEnvelopeType(EnvelopeType envType) { this->envType = envType; }
void Granulator::setParam(ParamID param, float value) {
    switch (param) {
        case MIX: setMix(value); break;
        case POSITION: setPosition(value); break;
        case POSITION_RAND: setPositionRand(value); break;
        case RATE: setRate(value); break;
        case RATE_RAND: setRateRand(value); break;
        case LENGTH: setLength(value); break;
        case LENGTH_RAND: setLengthRand(value); break;
        case TUNE: setTune(value); break;
        case TUNE_RAND: setTuneRand(value); break;
        case LEVEL: setLevel(value); break;
        case LEVEL_RAND: setLevelRand(value); break;
        case REVERSE_CHANCE: setReverseChance(value); break;
        case ENVELOPE_TYPE: setEnvelopeType(static_cast<EnvelopeType>(value)); break;
    }
}
float Granulator::getParam(ParamID param) const {
    switch (param) {
        case MIX: return mix;
        case POSITION: return position;
        case POSITION_RAND: return positionRand;
        case RATE: return rate;
        case RATE_RAND: return rateRand;
        case LENGTH: return length;
        case LENGTH_RAND: return lengthRand;
        case TUNE: return tune;
        case TUNE_RAND: return tuneRand;
        case LEVEL: return level;
        case LEVEL_RAND: return levelRand;
        case REVERSE_CHANCE: return reverseChance;
        case ENVELOPE_TYPE: return (float)envType;
        default: return 0.0f;
    }
}

void Granulator::process(const float* in, float* out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        inBuf[writePos] = in[i];
        ++grainCounter;
        
        float timeSamples = msSamples(rate + (rate * rateRand * uniform())); // Compute interval for next grain
        if (grainCounter >= timeSamples) { // Time to spawn a grain!
            spawnGrain();
            grainCounter = 0.0f;
        }
        
        // Process active grains and accumulate output
        float wetSig = 0.0f;
        for (auto& grain : grains) if (grain.active) { wetSig += processGrain(grain); }
        out[i] = dryWetMix(in[i], wetSig, mix); // Mix

        ++writePos;
        if (writePos >= bufSize) writePos = 0;
    }
}