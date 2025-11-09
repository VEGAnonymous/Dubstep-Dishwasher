#include "Teensy/Filters/Biquad/HPF_Biquad.h"

/* PRIVATE */

void HPF_Biquad::updateCoeffs() {
    const float w0 = 2.0f * static_cast<float>(M_PI) * (cutoff / SAMPLE_RATE);
    const float cos_w0 = arm_cos_f32(w0), sin_w0 = arm_sin_f32(w0);
    const float a = sin_w0 / (2.0f * q);

    const float a0 = 1.0f + a;
    b0 = ((1.0f + cos_w0) / 2.0f) / a0; b1 = -(1.0f + cos_w0) / a0; b2 = b0;
    a1 = (-2.0f * cos_w0) / a0; a2 = (1.0f - a) / a0;
}

/* PUBLIC */

HPF_Biquad::HPF_Biquad(float cutoff, float q, float gainDB) {
    this->cutoff = cutoff; this->q = q; this->gainDB = gainDB;
    updateCoeffs(); }