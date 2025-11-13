#include "Teensy/Filters/Biquad/Peak_Biquad.h"

/* PRIVATE */

void Peak_Biquad::updateCoeffs() {
    const float w0 = 2.0f * static_cast<float>(M_PI) * (cutoff / SAMPLE_RATE);
    const float cos_w0 = arm_cos_f32(w0), sin_w0 = arm_sin_f32(w0);
    const float A = exp10f(gainDB / 40.0f);
    const float a = sin_w0 / (2.0f * q);

    const float a0 = 1.0f + (a / A);
    b0 = (1.0f + (a * A)) / a0; b1 = (-2.0f * cos_w0) / a0; b2 = (1.0f - (a * A)) / a0; 
    a1 = b1; a2 = (1.0f - (a / A)) / a0;
}

/* PUBLIC */

void Peak_Biquad::setGain(float gainDB) { // dB, [-24.0, 24.0]
    setBypass(fabs(gainDB) < 1e-3f); // Bypass if gain is close or at 0.0dB
    this->gainDB = std::clamp(gainDB, -24.0f, 24.0f); 
    if (!isBypassed()) updateCoeffs();
}

Peak_Biquad::Peak_Biquad(float cutoff, float q, float gainDB) {
    this->cutoff = cutoff; this->q = q; this->gainDB = gainDB;
    updateCoeffs(); }