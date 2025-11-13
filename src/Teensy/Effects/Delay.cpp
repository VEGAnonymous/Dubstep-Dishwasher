#include "Teensy/Effects/Delay.h"
#include "Teensy/Utilities/Utilities.h"

/* PRIVATE */

/*

enum Params : ParamID { MIX, DELAY_TIME, FEEDBACK };

static constexpr float maxDelayTime = 500.0f; // ms
float mix, feedback;
DelayLine delayLine;

*/

/* PUBLIC */

Delay::Delay(float mix, float delayTime, float feedback) :
delayLine(delayTime, maxDelayTime) { setMix(mix); setFeedback(feedback); }

void Delay::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void Delay::setDelayTime(float delayTime) { delayLine.setDelayTime(std::clamp(delayTime, 1.0f, maxDelayTime)); } // ms, [1.0, 500.0]
void Delay::setFeedback(float feedback) { this->feedback = std::clamp(feedback, -0.95f, 0.95f); } // [-0.95, 0.95]
void Delay::setParam(ParamID param, float value) {
    switch (param) {
        case MIX: setMix(value); break;
        case DELAY_TIME: setDelayTime(value); break;
        case FEEDBACK: setFeedback(value); break;
    }
}

void Delay::process(const float* in, float* out, size_t n) {
    const float* in_ptr = in;
    float* out_ptr = out;

    float delaySig;
    for (size_t i = 0; i < n; ++i) {
        delaySig = delayLine.read(); // Read from delay line
        delayLine.write(*in_ptr + (delaySig * feedback)); // Feedback and write new sample
        *out_ptr++ = dryWetMix(*in_ptr++, delaySig, mix); // Mix
    }
}