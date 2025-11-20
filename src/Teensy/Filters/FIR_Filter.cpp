#include "Teensy/Filters/FIR_Filter.h"
#include "Teensy/Utilities/Utilities.h"

/* PRIVATE */

/* 

enum Params : ParamID { MIX };

float mix;
const float* h; // Pointer to filter kernel (stored in PROGMEM)
size_t M; // Filter order
std::vector<float> z; // Circular double buffer (filter state)
size_t z_i = 0; // State pointer

*/
        
/* PUBLIC */

FIR_Filter::FIR_Filter(float mix, const float* h, size_t M) : h(h), M(M), z(M * 2, 0.0f), z_i(0) { setMix(mix); }

void FIR_Filter::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void FIR_Filter::setParam(ParamID param, float value) { 
    switch (param) {
        case MIX: setMix(value); break;
    }
}
float FIR_Filter::getParam(ParamID param) const { 
    switch (param) {
        case MIX: return mix;
        default: return 0.0f;
    }
}


void FIR_Filter::process(const float* in, float* out, size_t n) { // Filter via convolution
    const float* in_ptr = in;
    float* out_ptr = out;

    for (size_t i = 0; i < n; ++i) {
        z[z_i] = *in_ptr; z[z_i + M] = *in_ptr; // Write input in double-buffered state

        // Convolve via inner product
        float wetSig = 0.0f;
        for (size_t j = 0; j < M; ++j) wetSig += pgm_read_float_near(h + j) * z[z_i + j];

        z_i = (z_i == 0 ? M - 1 : z_i - 1); // Iterate state pointer in reverse

        *out_ptr++ = lerp(*in_ptr++, wetSig, mix); // Mix
    }
}