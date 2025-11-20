#include "Teensy/Effects/Distortion.h"
#include "Teensy/Utilities/Utilities.h"
#include "Teensy/LUTs.h"

/* PRIVATE */

/*

enum Params : ParamID { MIX, MODE, DRIVE };

DistortionMode mode;
float mix, drive;

float (Distortion::*algorithm)(float, float) = nullptr; // Function pointer for distortion algorithm to use

*/

// Distortion algorithms
// https://www.desmos.com/calculator/qrqipgp7r4
float Distortion::tube(float in, float drive) {
    float x = in * (4.0f + (drive * 6.0f)); // d -> [4, 10]
    return atan(x) * (2.0f / M_PI);
}
float Distortion::softClip(float in, float drive) { 
    float d = (1.3f + (drive * 3.7f)); // d -> [1.3, 5]
    float x;
    if (in < (-1.0f/d)) { x = -2.0f/3.0f; }
    else if (in > (1.0f/d)) { x = 2.0f/3.0f; }
    else { x = in * d; x = x - ((x * x * x) / 3.0f); } // x - (x^3)/3
    return x * 1.5f;
} 
float Distortion::hardClip(float in, float drive) {
    float x = in * (1.0f + (drive * 4.0f)); // d -> [1, 5]
    if (x > 1.0f) { x = 1.0f; }
    else if (x < -1.0f) { x = -1.0f; }
    return x;
}
float Distortion::diode(float in, float drive) {
    float x = in * (2.0f + (drive * 8.0f)); // d -> [2, 10]
    if (x < 0) { x = expf(x) - 1.0f; } // Shockley diode equation, B=1
    else if (x > 0) { x = 1.0 - expf(-x); }
    else { x = 0; }
    return x;
}
float Distortion::bitCrush(float in, float drive) {
    int bitDepth = (int)(2 + ((1.0f - drive) * (1.0f - drive) * 14.0f)); // 16-bit to 2-bit depth
    float levels = (float)(1 << bitDepth); // 2^bits discrete levels
    return round(in * levels) / levels; // Quantize
}
float Distortion::rectify(float in, float drive) {
    float x = in * (1.0f + (drive * 4.0f)); // d -> [1, 5]
    x = abs(x); // Full-wave rectify
    return (x > 1) ? 1.0f : x;
}
float Distortion::saturate(float in, float drive) {
    float x = in * (2.0f + (drive * 8.0f)); // d -> [2, 10]
    return tanhf(x);
}

/* PUBLIC */

Distortion::Distortion(float mix, DistortionMode mode, float drive) { setMix(mix); setMode(mode); setDrive(drive); }

void Distortion::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void Distortion::setMode(DistortionMode mode) {
    this->mode = mode;
    switch (mode) {
        case DistortionMode::TUBE: algorithm = &Distortion::tube; break;
        case DistortionMode::SOFT_CLIP: algorithm = &Distortion::softClip; break;
        case DistortionMode::HARD_CLIP: algorithm = &Distortion::hardClip; break;
        case DistortionMode::DIODE: algorithm = &Distortion::diode; break;
        case DistortionMode::BITCRUSH: algorithm = &Distortion::bitCrush; break;
        case DistortionMode::RECTIFY: algorithm = &Distortion::rectify; break;
        case DistortionMode::SATURATE: algorithm = &Distortion::saturate; break;
        default: algorithm = &Distortion::hardClip;
    }
}
void Distortion::setDrive(float drive) { this->drive = std::clamp(drive, 0.0f, 1.0f); } // [0.0, 1.0]
void Distortion::setParam(ParamID param, float value) {
    switch (param) {
        case MIX: setMix(value); break;
        case MODE: setMode(static_cast<DistortionMode>(value)); break;
        case DRIVE: setDrive(value); break;
   
    }
}
float Distortion::getParam(ParamID param) const {
    switch (param) {
        case MIX: return mix;
        case MODE: return (float)mode;
        case DRIVE: return drive;
        default: return 0.0f;
    }
}

void Distortion::process(const float* in, float* out, size_t n) {
    const float* in_ptr = in;
    float* out_ptr = out;

    float wetSig;
    for (size_t i = 0; i < n; ++i) {
        wetSig = (this->*algorithm)(*in_ptr, drive) * 0.966051f; // Apply non-linearity (-0.3dB)
        *out_ptr++ = dryWetMix(*in_ptr++, wetSig, mix); // Mix
    }
}