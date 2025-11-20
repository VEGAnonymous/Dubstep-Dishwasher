#include "Teensy/Effects/Compressor.h"
#include "Teensy/Utilities/Utilities.h"

#include "arm_math.h"

/* PRIVATE */

/*

enum Params : ParamID { MIX, THRESHOLD, RATIO, KNEE, ATTACK_TIME, RELEASE_TIME, MAKEUP_GAIN, AUTO_MAKEUP };

float mix, threshold, ratio, knee, attackTime, releaseTime, makeupGain; bool autoMakeup;
float attackCoeff, releaseCoeff, makeupCoeff;

DelayLine inBuffer; 
static constexpr float L = 50.0f, L_samples = L * SAMPLE_RATE / 1000.0f; // RMS window size
float rms = 1e-6f, gainSmoothed = 0.0f;
float reductionSmoothed = 0.0f, autoMakeupGain = 0.0f;

*/

float Compressor::computeReduction(float rmsDB) {
    // https://www.desmos.com/calculator/wkmkrmn9le
    float gDB = 0.0f;
    if (rmsDB < (threshold - (knee / 2.0f))) { gDB = rmsDB; } // Below threshold, linear
    else if (rmsDB > (threshold + (knee / 2.0f))) { gDB = threshold + ((rmsDB - threshold) / ratio); } // Above threshold, attenuate
    else { gDB = rmsDB + ((((1.0f / ratio) - 1.0f) * pow(rmsDB - threshold + (knee / 2.0f), 2.0f)) / (2.0f * knee)); } // Soft knee
    
    return gDB - rmsDB;
}

/* PUBLIC */

Compressor::Compressor(float mix, float threshold, float ratio, float knee, float attack, float release, 
                       float makeupGain, bool autoMakeup)
    : inBuffer(1.0f, L + 1.0f) {
    setMix(mix); setThreshold(threshold); setRatio(ratio); setKnee(knee); 
    setAttackTime(attack); setReleaseTime(release); setMakeupGain(makeupGain); setAutoMakeup(autoMakeup);
    inBuffer.setDelayTime(L);
    makeupCoeff = exp(-2.2f / (100.0f * SAMPLE_RATE / 1000.0f)); // Auto-makeup smoothing factor
}

void Compressor::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void Compressor::setThreshold(float threshold) { this->threshold = std::clamp(threshold, -100.0f, 0.0f); } // dB, [-100.0, 0.0]
void Compressor::setRatio(float ratio) { this->ratio = std::clamp(ratio, 1.0f, 100.0f); } // [1.0, 100.0]
void Compressor::setKnee(float knee) { this->knee = std::clamp(knee, 0.0f, 40.0f); } // dB, [0.0, 40.0]
void Compressor::setAttackTime(float attackTime) { // ms, [0.01, 250.0]
    this->attackTime = attackTime;
    attackCoeff = exp(-2.2f / (std::clamp(attackTime, 0.01f, 250.0f) * SAMPLE_RATE / 1000.0f)); 
} 
void Compressor::setReleaseTime(float releaseTime) { // ms, [10.0, 2500.0]
    this->releaseTime = releaseTime;
    releaseCoeff = exp(-2.2f / (std::clamp(releaseTime, 10.0f, 2500.0f) * SAMPLE_RATE / 1000.0f)); 
} 
void Compressor::setMakeupGain(float makeupGain) { this->makeupGain = std::clamp(makeupGain, -72.0f, 36.0f); } // dB, [-72.0, 36.0]
void Compressor::setAutoMakeup(bool autoMakeup) { 
    this->autoMakeup = autoMakeup;
    if (autoMakeup) { reductionSmoothed = 0.0f; autoMakeupGain = 0.0f; }
}
void Compressor::setParam(ParamID param, float value) {
    switch (param) {
        case MIX: setMix(value); break;
        case THRESHOLD: setThreshold(value); break;
        case RATIO: setRatio(value); break;
        case KNEE: setKnee(value); break;
        case ATTACK_TIME: setAttackTime(value); break;
        case RELEASE_TIME: setReleaseTime(value); break;
        case MAKEUP_GAIN: setMakeupGain(value); break;
        case AUTO_MAKEUP: setAutoMakeup(value > 0.5f); break;
    }
}
float Compressor::getParam(ParamID param) const {
    switch (param) {
        case MIX: return mix;
        case THRESHOLD: return threshold;
        case RATIO: return ratio;
        case KNEE: return knee;
        case ATTACK_TIME: return attackTime;
        case RELEASE_TIME: return releaseTime;
        case MAKEUP_GAIN: return makeupGain;
        case AUTO_MAKEUP: return autoMakeup ? 1.0f : 0.0f;
        default: return 0.0f;
    }
}

void Compressor::process(const float* in, float* out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        // Compute RMS recursively
        const float x_i = in[i], x_L = inBuffer.read();
        inBuffer.write(x_i); 
        arm_sqrt_f32((rms * rms) + (((x_i * x_i) - (x_L * x_L)) / L_samples), &rms);
        float rmsDB = ampDB(std::max(rms, 1e-6f));

        // Gain computation
        float gDB = computeReduction(rmsDB);

        // Gain smoothing (one-pole IIR LPF)
        if (gDB <= gainSmoothed) { gainSmoothed = (attackCoeff * gainSmoothed) + ((1.0f - attackCoeff) * gDB); } // Attack
        else { gainSmoothed = (releaseCoeff * gainSmoothed) + ((1.0f - releaseCoeff) * gDB); } // Release

        // Makeup gain
        if (autoMakeup) { 
            reductionSmoothed = (makeupCoeff * reductionSmoothed) + ((1.0f - makeupCoeff) * gDB); // Makeup smoothing
            autoMakeupGain = -reductionSmoothed;
        }
        float makeupDB = autoMakeup ? autoMakeupGain : makeupGain;
        float g = dbAmp(std::clamp(gainSmoothed + makeupDB, -60.0f, 20.0f));
        float y_i = x_i * g;

        // Hard clip just in case (e.g., limiter use case)
        if (y_i > 1.0f) { y_i = 1.0f; }
        else if (y_i < -1.0f) { y_i = -1.0f; }

        out[i] = dryWetMix(x_i, y_i, mix); // Mix
    }
}