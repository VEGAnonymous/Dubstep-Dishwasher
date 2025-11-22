#include "Teensy/Effects/Flanger.h"
#include "Teensy/Utilities/Utilities.h"

/* PRIVATE */

/*

enum Params : ParamID { MIX, RATE, DEPTH, FEEDBACK };

DelayLine delayLine;
Wavetable LFO;
float mix, rate, depth, feedback;
float wetSig = 0;

*/

/* PUBLIC */

Flanger::Flanger(float mix, float rate, float depth, float feedback) 
: delayLine(15, 30), LFO(rate, WavetableType::SINE) { setMix(mix); setRate(rate); setDepth(depth); setFeedback(feedback); }

void Flanger::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void Flanger::setRate(float rate) { this->rate = std::clamp(rate, 0.0f, 20.0f); LFO.setFreq(rate); } // Hz, [0.0, 20.0]
void Flanger::setDepth(float depth) { this->depth = std::clamp(depth, 0.0f, 1.0f); } // [0.0, 1.0]
void Flanger::setFeedback(float feedback) { this->feedback = std::clamp(feedback, -0.95f, 0.95f); } // [-0.95, 0.95]
void Flanger::setParam(ParamID param, float value) {
    switch (param) {
        case MIX: setMix(value); break;
        case RATE: setRate(value); break;
        case DEPTH: setDepth(value); break;
        case FEEDBACK: setFeedback(value); break;
    }
}
float Flanger::getParam(ParamID param) const {
    switch (param) {
        case MIX: return mix;
        case RATE: return rate;
        case DEPTH: return depth;
        case FEEDBACK: return feedback;
        default: return 0.0f;
    }
}

void Flanger::process(const float* in, float* out, size_t n) {
    const float* in_ptr = in;
    float* out_ptr = out;

    for (size_t i = 0; i < n; ++i) {
        delayLine.setDelayTime(15.0f + (LFO.next() * 10.0f * depth)); // Modulate delay with LFO, 5-25 ms
        wetSig = delayLine.read(); // Read from delay line

        delayLine.write(*in_ptr + (feedback * wetSig)); // Feedback and write new sample
        
        *out_ptr++ = dryWetMix(*in_ptr++, wetSig, mix); // Mix
    }
}