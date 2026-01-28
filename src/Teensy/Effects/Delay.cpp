#include "Teensy/Effects/Delay.h"
#include "Teensy/Utilities/Utilities.h"

/* PRIVATE */

/*

enum Params : ParamID { MIX, DELAY_TIME, FEEDBACK };

static constexpr float maxDelayTime = 500.0f; // ms
static constexpr int fadeLength = 32;

float mix, delayTime, delaySamples, feedback;
DelayLine delayLine;

float oldDelaySamples;
int fadeCounter = 0; bool isCrossfading = false;

*/

/* PUBLIC */

Delay::Delay(float mix, float delayTime, float feedback) :
delayLine(delayTime, maxDelayTime) { 
    setMix(mix); setFeedback(feedback); 
    delaySamples = msSamples(delayTime);
}

void Delay::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void Delay::setDelayTime(float delayTime) { // ms, [1.0, 500.0]
    float newSamples = msSamples(std::clamp(delayTime, 1.0f, maxDelayTime));
    if (newSamples != delaySamples) {
        // Begin a new crossfade
        oldDelaySamples = delaySamples;
        delaySamples = newSamples;
        fadeCounter = 0; isCrossfading = true;
    }
} 
void Delay::setFeedback(float feedback) { this->feedback = std::clamp(feedback, -0.95f, 0.95f); } // [-0.95, 0.95]
void Delay::setParam(ParamID param, float value) {
    switch (param) {
        case MIX: setMix(value); break;
        case DELAY_TIME: setDelayTime(value); break;
        case FEEDBACK: setFeedback(value); break;
    }
}
float Delay::getParam(ParamID param) const {
    switch (param) {
        case MIX: return mix;
        case DELAY_TIME: return delayTime;
        case FEEDBACK: return feedback;
        default: return 0.0f;
    }
}

void Delay::process(const float* in, float* out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        float delaySig;
        if (isCrossfading) { // Delay time changed
            float t = fadeCounter / float(fadeLength);
            float tapA = delayLine.read(oldDelaySamples), tapB = delayLine.read(delaySamples);
            delaySig = lerp(tapA, tapB, t); // Crossfade to new delay time
            if (++fadeCounter >= fadeLength) isCrossfading = false;
        } else delaySig = delayLine.read(delaySamples);

        delayLine.write(in[i] + delaySig * feedback); // Feedback and write new sample
        out[i] = dryWetMix(in[i], delaySig, mix); // Mix
    }
}
