#include "Teensy/Filters/OnePole.h"

/* PRIVATE */

/*

enum Params : ParamID { CUTOFF = 1, COEFF };

float b0, a1, y = 0.0f;
float cutoff;

*/

/* PUBLIC */

OnePole::OnePole(float mix, float cutoff) { IIR_Filter::mix = mix; setCutoff(cutoff); }

void OnePole::setCoeff(float a) {
    b0 = 1.0f - a; a1 = a;
    cutoff = -(SAMPLE_RATE * log(a)) / (2 * M_PI);
}
void OnePole::setCutoff(float cutoff) { // Hz
    this->cutoff = cutoff;
    float x = exp((-2.0f * M_PI * cutoff) / SAMPLE_RATE);
    b0 = 1.0f - x;
    a1 = x;
}
void OnePole::setParam(ParamID param, float value) { 
    switch (param) {
        case CUTOFF: setCutoff(value); break;
        case COEFF: setCoeff(value); break;
        default: IIR_Filter::setParam(param, value);
    }
}
float OnePole::getParam(ParamID param) const { 
    switch (param) {
        case CUTOFF: return cutoff;
        case COEFF: return a1;
        default: return IIR_Filter::getParam(param);
    }
}

float OnePole::LCCDE(float x) { 
    // y[n] = (1-a)x[n] - ay[n-1]
    y = b0 * x + a1 * y;
    return y;
}