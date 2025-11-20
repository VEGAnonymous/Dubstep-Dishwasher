#include "Teensy/Effects/Gate.h"
#include "Teensy/Utilities/Utilities.h"

#include "arm_math.h"

/* PRIVATE */

/* 

enum Params : ParamID { MIX, THRESHOLD, ATTACK_TIME, RELEASE_TIME, HOLD_TIME, INVERT };

float mix, threshold, attackTime, releaseTime, holdTime; bool invert;
float attackCoeff, releaseCoeff; 

DelayLine inBuffer;
static constexpr float L = 50.0f, L_samples = L * SAMPLE_RATE / 1000.0f; // RMS window size
float rms = 1e-6f, gainSmoothed = 0.0f;
size_t holdSamples, holdCounter = 0;

*/

/* PUBLIC */

Gate::Gate(float mix, float threshold, float attack, float release, float hold, bool invert)
    : inBuffer(1.0f, L + 1.0f) {
    setMix(mix); setThreshold(threshold); setAttackTime(attack); setReleaseTime(release); setHoldTime(hold); setInvert(invert); }

void Gate::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void Gate::setThreshold(float threshold) { this->threshold = std::clamp(threshold, -100.0f, 0.0f); } // dB, [-100.0, 0.0]
void Gate::setAttackTime(float attackTime) { // ms, [0.01, 250.0]
    this->attackTime = attackTime; 
    attackCoeff = exp(-2.2f / (std::clamp(attackTime, 0.01f, 250.0f) * SAMPLE_RATE / 1000.0f)); 
} 
void Gate::setReleaseTime(float releaseTime) { // ms, [0.01, 1500.0]
    this->releaseTime = releaseTime;
    releaseCoeff = exp(-2.2f / (std::clamp(releaseTime, 0.01f, 1500.0f) * SAMPLE_RATE / 1000.0f)); 
} 
void Gate::setHoldTime(float holdTime) { // ms, [1.0, 1500.0]
    this->holdTime = holdTime;
    holdSamples = (size_t)(std::clamp(holdTime, 1.0f, 1500.0f) * SAMPLE_RATE / 1000.0f); 
} 
void Gate::setInvert(bool invert) { this->invert = invert; }
void Gate::setParam(ParamID param, float value) {
    switch (param) {
        case MIX: setMix(value); break;
        case THRESHOLD: setThreshold(value); break;
        case ATTACK_TIME: setAttackTime(value); break;
        case RELEASE_TIME: setReleaseTime(value); break;
        case HOLD_TIME: setHoldTime(value); break;
        case INVERT: setInvert(value > 0.5f); break;
    }
}
float Gate::getParam(ParamID param) const {
    switch (param) {
        case MIX: return mix;
        case THRESHOLD: return threshold;
        case ATTACK_TIME: return attackTime;
        case RELEASE_TIME: return releaseTime;
        case HOLD_TIME: return holdTime;
        case INVERT: return invert ? 1.0f : 0.0f;
        default: return 0.0f;
    }
}

void Gate::process(const float* in, float* out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        // Compute RMS recursively
        const float x_i = in[i], x_L = inBuffer.read();
        inBuffer.write(x_i); 
        arm_sqrt_f32((rms * rms) + (((x_i * x_i) - (x_L * x_L)) / L_samples), &rms);
        float rmsDB = ampDB(std::max(rms, 1e-6f));

        // To gate or not to gate
        bool gateOpen = (rmsDB > threshold);
        if (gateOpen) holdCounter = holdSamples;
        else if (holdCounter > 0) { holdCounter--; gateOpen = true; }

        float staticGain = gateOpen ? 1.0f : 0.0f;
        if (invert) staticGain = 1.0f - staticGain;

        // Gain smoothing (one-pole IIR LPF)
        if (staticGain > gainSmoothed) gainSmoothed += (1.0f - attackCoeff) * (staticGain - gainSmoothed); // Attack
        else gainSmoothed += (1.0f - releaseCoeff) * (staticGain - gainSmoothed); // Release

        float wetSig = x_i * gainSmoothed;
        out[i] = dryWetMix(x_i, wetSig, mix); // Mix
    }
}