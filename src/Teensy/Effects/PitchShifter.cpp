#include "Teensy/Effects/PitchShifter.h"

// Partially based on Kilohearts grain-based pitch shifter
// https://kilohearts.com/products/pitch_shifter

/* PRIVATE */

/*

enum Params : ParamID { MIX, PITCH_SHIFT, GRAIN_SIZE, GRAIN_OVERLAP, JITTER };
static constexpr size_t bufSize = (SAMPLE_RATE * 501) / 1000; // 500ms
const size_t maxGrains = 16;

float mix, pitchShift, grainSize, grainOverlap, jitter;
float grainSizeSamples, grainInterval, pitchRatio, overlapGain;

float* inBuf = nullptr;
size_t writePos = 0;
float grainCounter = 0.0f;

struct Grain {
    bool active = false;
    float startPos;
    float playhead;
};

std::vector<Grain> grains; // Grain pool

Random jitterer; // Jitter noise generator

*/

void PitchShifter::spawnGrain() {
    Grain* freeGrain = nullptr;
    for (auto& grain : grains) if (!grain.active) { freeGrain = &grain; break; }
    // If none, steal oldest grain
    if (freeGrain == nullptr) {
        for (auto& grain : grains) if (grain.active) { freeGrain = &grain; break; }
    }

    // Calculate start position
    float grainStartPos = (float)writePos - grainSizeSamples;
    grainStartPos += jitterer.next() * ((grainSizeSamples * 0.25f) * (jitter * jitter)); // Add random start jitter
    float bufSizef = (float)bufSize;
    while (grainStartPos < 0.0f) grainStartPos += bufSizef;
    if (grainStartPos >= bufSizef) grainStartPos = fmodf(grainStartPos, bufSizef);

    // Init
    freeGrain->active = true;
    freeGrain->startPos = grainStartPos;
    freeGrain->playhead = 0.0f;
}

float PitchShifter::processGrain(Grain& grain) {
    float readPos = grain.startPos + grain.playhead; // Current position in grain
    float bufSizef = (float)bufSize;
    if (readPos >= bufSizef) readPos = fmodf(readPos, bufSizef);
    else if (readPos < 0.0f) readPos = fmodf(readPos + bufSizef, bufSizef);

    float envelopeValue = getEnvelopeValue(grain.playhead / grainSizeSamples, EnvelopeType::HANN);

    grain.playhead += pitchRatio * (1.0f + (0.02f * jitterer.next() * (jitter * jitter))); // Advance playhead at rate according to pitch shift
    if (grain.playhead >= grainSizeSamples) grain.active = false; // Free if done
    
    return lerp(inBuf, readPos, bufSizef) * envelopeValue;
}

void PitchShifter::updateInterval() {
    if (grainSizeSamples <= 0.0f) grainInterval = 1.0f;
    else grainInterval = (grainSizeSamples * (1.0f - grainOverlap)) / pitchRatio;
    arm_sqrt_f32(1.0f - grainOverlap, &overlapGain);
}

/* PUBLIC */

PitchShifter::PitchShifter(float mix, float pitchShift, float grainSize, float grainOverlap, float jitter) 
: jitterer(5.0f, RandomMode::PERLIN) {
    setMix(mix); setPitchShift(pitchShift); setGrainSize(grainSize); setGrainOverlap(grainOverlap); setJitter(jitter);
    grains.resize(maxGrains);
    inBuf = (float*)extmem_malloc(bufSize * sizeof(float)); 
    if (!inBuf) while(1){}; 
    memset(inBuf, 0, bufSize * sizeof(float));
}
PitchShifter::~PitchShifter() { if (inBuf) extmem_free(inBuf); }

void PitchShifter::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void PitchShifter::setPitchShift(float pitchShift) { 
    this->pitchShift = std::clamp(pitchShift, -24.0f, 24.0f); // semitones, [-24.0, 24.0]
    pitchRatio = exp2f(pitchShift / 12.0f);
    updateInterval();
}
void PitchShifter::setGrainSize(float grainSize) { // ms, [20.0, 500.0]
    this->grainSize = std::clamp(grainSize, 20.0f, 200.0f); 
    grainSizeSamples = msSamples(grainSize);
    updateInterval();
}
void PitchShifter::setGrainOverlap(float grainOverlap) { // [0.25, 0.75]
    this->grainOverlap = std::clamp(grainOverlap, 0.25f, 0.75f);
    updateInterval();
}
void PitchShifter::setJitter(float jitter) { this->jitter = std::clamp(jitter, 0.0f, 1.0f); } // [0.0, 1.0]
void PitchShifter::setParam(ParamID param, float value) {
    switch (param) {
        case MIX: setMix(value); break;
        case PITCH_SHIFT: setPitchShift(value); break;
        case GRAIN_SIZE: setGrainSize(value); break;
        case GRAIN_OVERLAP: setGrainOverlap(value); break;
        case JITTER: setJitter(value); break;
    }
}
float PitchShifter::getParam(ParamID param) const {
    switch (param) {
        case MIX: return mix;
        case PITCH_SHIFT: return pitchShift;
        case GRAIN_SIZE: return grainSize;
        case GRAIN_OVERLAP: return grainOverlap;
        case JITTER: return jitter;
        default: return 0.0f;
    }
}

void PitchShifter::process(const float* in, float* out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        inBuf[writePos] = in[i]; // Write input to circular buffer
        ++grainCounter;
        
        if (grainCounter >= grainInterval) { // Time to spawn a grain!
            spawnGrain();
            grainCounter = 0.0f;
        }
        
        // Process active grains and accumulate output
        float wetSig = 0.0f;
        for (auto& grain : grains) if (grain.active) { wetSig += processGrain(grain); }
        wetSig *= overlapGain;
        out[i] = dryWetMix(in[i], wetSig, mix); // Mix

        ++writePos;
        if (writePos >= bufSize) writePos = 0;
    }
}