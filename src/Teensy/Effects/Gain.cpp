#include "Teensy/Effects/Gain.h"
#include "Teensy/Utilities/Utilities.h"

#include <stddef.h>

/* PRIVATE */

/*

enum Params : ParamID { GAIN, CLIP };

float gainFactor; bool clip;

*/

/* PUBLIC */

Gain::Gain(float gainDB) { setGain(gainDB); }

void Gain::setGain(float gainDB) { this->gainFactor = dbAmp(std::clamp(gainDB, -60.0f, 24.0f)); } // dB, [-60.0, 24.0]
void Gain::setClip(bool clip) { this->clip = clip; }
void Gain::setParam(ParamID param, float value) {
    switch (param) {
        case GAIN: setGain(value); break;
        case CLIP: setClip(value > 0.5f); break;
    }
}

void Gain::process(const float* in, float* out, size_t n) {
    const float* in_ptr = in;
    float* out_ptr = out;

    for (size_t i = 0; i < n; ++i) {
        float wetSig = *in_ptr++ * gainFactor;
        if (clip) { // Optional hard clip
            if (wetSig > 1.0f) { wetSig = 1.0f; }
            else if (wetSig < -1.0f) { wetSig = -1.0f; }
        }

        *out_ptr++ = wetSig;
    } 
};