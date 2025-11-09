#include "Teensy/Filters/Biquad/BPF_Biquad.h"

/* PRIVATE */

/*

bool flatGain; 

*/

void BPF_Biquad::updateCoeffs() {
    const float w0 = 2.0f * static_cast<float>(M_PI) * (cutoff / SAMPLE_RATE);
    const float cos_w0 = cosf(w0), sin_w0 = sinf(w0);
    const float a = sin_w0 / (2.0f * q);

    const float a0 = 1.0f + a;
    b0 = flatGain ? (a / a0) : ((q * a) / a0); b1 = 0.0f; b2 = -b0;
    a1 = (-2.0f * cos_w0) / a0; a2 = (1.0f - a) / a0;
}

/* PUBLIC */

BPF_Biquad::BPF_Biquad(float cutoff, float q, float gainDB, bool flatGain) {
    this->cutoff = cutoff; this->q = q; this->gainDB = gainDB; this->flatGain = flatGain;
    updateCoeffs(); }
