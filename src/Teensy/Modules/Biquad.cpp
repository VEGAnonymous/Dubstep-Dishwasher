#include "Teensy/Modules/Biquad.h"

/* PROTECTED */

/*

enum Params : ParamID { CUTOFF = 1, Q, GAIN };

float cutoff = 1000.0f, q = 0.707f, gainDB = 0.0f;

// Biquad coefficients
float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
float a1 = 0.0f, a2 = 0.0f;

// Filter state
float z1 = 0.0f, z2 = 0.0f; 

*/

float Biquad::LCCDE(float x) {
    // w[n] = x[n] - a1z1 - a2z2
    // y[n] = b0w[n] + b1z1 + b2z2
    // z2 = z1, z1 = w[n]
    float w = x - (a1 * z1) - (a2 * z2);
    float y = (b0 * w) + (b1 * z1) + (b2 * z2);
    z2 = z1; z1 = w;
    return y;
}

/* PUBLIC */

void Biquad::setCutoff(float cutoff) { this->cutoff = std::clamp(cutoff, 20.0f, 20000.0f); updateCoeffs(); } // Hz, [20.0, 20000.0]
void Biquad::setQ(float q) { this->q = std::clamp(q, 0.025f, 40.0f); updateCoeffs(); } // [0.025, 40.0]
void Biquad::setGain(float gainDB) { this->gainDB = std::clamp(gainDB, -24.0f, 24.0f); updateCoeffs(); } // dB, [-24.0, 24.0]
void Biquad::setParam(ParamID param, float value) {
    switch (param) {
        case Biquad::Params::CUTOFF: setCutoff(value); break;
        case Biquad::Params::Q: setQ(value); break;
        case Biquad::Params::GAIN: setGain(value); break;
        default: IIR_Filter::setParam(param, value);
    }
}
float Biquad::getParam(ParamID param) const {
    switch (param) {
        case Biquad::Params::CUTOFF: return cutoff;
        case Biquad::Params::Q: return q;
        case Biquad::Params::GAIN: return gainDB;
        default: return IIR_Filter::getParam(param);
    }
}