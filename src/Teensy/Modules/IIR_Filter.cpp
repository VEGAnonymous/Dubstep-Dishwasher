#include "Teensy/Modules/IIR_Filter.h"

/* PROTECTED */

/*

enum Params : ParamID { MIX };
float mix;

*/

/* PUBLIC */

void IIR_Filter::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); }; // [0.0, 1.0]
void IIR_Filter::setParam(ParamID param, float value) {
    switch (param) {
        case MIX: setMix(value); break;
        // Subclasses can call IIR_Filter::setParam(param, value)
    }
}

float IIR_Filter::processSample(float x) { return LCCDE(x); }
void IIR_Filter::process(const float* in, float* out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        out[i] = dryWetMix(in[i], LCCDE(in[i]), mix);
    }
}