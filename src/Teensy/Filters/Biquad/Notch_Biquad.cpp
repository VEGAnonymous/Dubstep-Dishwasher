#include "Teensy/Filters/Biquad/Notch_Biquad.h"

/* PRIVATE */

void Notch_Biquad::updateCoeffs() {
    const float w0 = 2.0f * static_cast<float>(M_PI) * (cutoff / SAMPLE_RATE);
    const float cos_w0 = arm_cos_f32(w0), sin_w0 = arm_sin_f32(w0);
    const float a = sin_w0 / (2.0f * q);

    const float a0 = 1.0f + a;
    b0 = 1.0f / a0; b1 = (-2.0f * cos_w0) / a0; b2 = 1.0f / a0; 
    a1 = b1; a2 = (1.0f - a) / a0;
}

/* PUBLIC */

Notch_Biquad::Notch_Biquad(float cutoff, float q, float gainDB) { 
    this->cutoff = cutoff; this->q = q; this->gainDB = gainDB;
    updateCoeffs(); }