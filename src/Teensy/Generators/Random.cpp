#include "Teensy/Generators/Random.h"

/* PRIVATE */

/*

float freq, phase = 0.0f, currentVal = 0.0f, nextVal = 0.0f;
RandomMode mode;
float (Random::*algorithm)() = nullptr;

*/

// Noise algorithms
float Random::perlin() { // Not really perlin, but smooth noise
    phase += freq / SAMPLE_RATE;
    if (phase >= 1.0f) {
        phase -= 1.0f;
        currentVal = nextVal;
        nextVal = uniform();
    }
    float smooth = phase * phase * (3.0f - (2.0f * phase)); // smoothstep(x) -> 3x^2 - 2x^3
    return currentVal + (smooth * (nextVal - currentVal)); // lerp
}
float Random::sampleHold() {
    phase += freq / SAMPLE_RATE;
    if (phase >= 1.0f) {
        phase -= 1.0f;
        currentVal = uniform();
    }
    return currentVal;
}
float Random::binary() {
    phase += freq / SAMPLE_RATE;
    if (phase >= 1.0f) {
        phase -= 1.0f;
        currentVal = (rand() & 1) ? 1.0f : -1.0f;
    }
    return currentVal;
}

/* PUBLIC */

Random::Random(float freq, RandomMode mode) { setFreq(freq); setMode(mode); }

void Random::setFreq(float freq) { this->freq = freq; } // Hz
float Random::getFreq() const { return this->freq; }
void Random::setPhase(float phase) { this->phase = phase; }
float Random::getPhase() const { return this->phase; }
void Random::setMode(RandomMode mode) {
    this->mode = mode;
    switch (mode) {
        case RandomMode::PERLIN: algorithm = &Random::perlin; break;
        case RandomMode::SAMPLE_HOLD: algorithm = &Random::sampleHold; break;
        case RandomMode::BINARY: algorithm = &Random::binary; break;
    }
}

float Random::next() { return (this->*algorithm)(); }

// HACK: SUPER hacky way to force advance the phase for Random modulators at control rate (dt)
// Literally just the algorithms above but rehashed lmao
float Random::tick(float dt) {
    phase += freq * dt;
    
    // Phase wrap and update
    if (phase >= 1.0f) {
        phase -= 1.0f;
        
        switch (mode) {
            case RandomMode::PERLIN:
                currentVal = nextVal;
                nextVal = uniform();
                break;
            case RandomMode::SAMPLE_HOLD: currentVal = uniform(); break;
            case RandomMode::BINARY: currentVal = (rand() & 1) ? 1.0f : -1.0f; break;
        }
    }
    
    // Evaluate without phase advancement
    switch (mode) {
        case RandomMode::PERLIN: {
            float smooth = phase * phase * (3.0f - (2.0f * phase));
            return currentVal + (smooth * (nextVal - currentVal));
            break;
        }
        case RandomMode::SAMPLE_HOLD: return currentVal; break;
        case RandomMode::BINARY: return currentVal; break;
        default: return 0.0f; // impossibru
    }
}