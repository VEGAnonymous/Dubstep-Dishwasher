#include "Teensy/Effects/Modulation.h"
#include "Teensy/Utilities/Utilities.h"

/* PRIVATE  */

/*

enum Params : ParamID { MIX, MODE, MODULATOR, FREQ, DEPTH, BIAS, RECTIFY };

float mix, freq, depth, bias, rectify;
ModulationEffectMode mode; WavetableType modulatorType;
Wavetable modulator;

*/

/* PUBLIC */

Modulation::Modulation(float mix, ModulationEffectMode mode, WavetableType modulatorType, 
                       float freq, float depth, float bias, float rectify) : modulator(freq, modulatorType) {
    setMix(mix); setMode(mode); setModulator(modulatorType); setFreq(freq); setDepth(depth); setBias(bias); setRectify(rectify);
}

void Modulation::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void Modulation::setMode(ModulationEffectMode mode) { this->mode = mode; }
void Modulation::setModulator(WavetableType modulatorType) { 
    this->modulatorType = modulatorType;
    modulator.setTable(modulatorType); 
}
void Modulation::setFreq(float freq) { this->freq = std::clamp(freq, 1.0f, 2000.0f); modulator.setFreq(this->freq); } // [1.0, 2000.0]
void Modulation::setDepth(float depth) { this->depth = std::clamp(depth, 0.0f, 1.0f); } // [0.0, 1.0]
void Modulation::setBias(float bias) { this->bias = std::clamp(bias, 0.0f, 1.0f); } // [0.0, 1.0]
void Modulation::setRectify(float rectify) { this->rectify = std::clamp(rectify, -1.0f, 1.0f); } // [-1.0, 1.0]
void Modulation::setParam(ParamID param, float value) {
    switch (param) {
        case MIX: setMix(value); break;
        case MODE: setMode(static_cast<ModulationEffectMode>(value)); break;
        case MODULATOR: setModulator(static_cast<WavetableType>(value)); break;
        case FREQ: setFreq(value); break;
        case DEPTH: setDepth(value); break;
        case BIAS: setBias(value); break;
        case RECTIFY: setRectify(value); break;
    }
}
float Modulation::getParam(ParamID param) const {
    switch (param) {
        case MIX: return mix;
        case MODE: return (float)mode;
        case MODULATOR: return (float)modulatorType;
        case FREQ: return freq;
        case DEPTH: return depth;
        case BIAS: return bias;
        case RECTIFY: return rectify;
        default: return 0.0f;
    }
}

void Modulation::process(const float* in, float* out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        float modNext = modulator.next();
        if (rectify > 0.0f) modNext = lerp(modNext, fabs(modNext), rectify);
        else if (rectify < 0.0f) modNext = lerp(modNext, -fabs(modNext), -rectify);

        modNext += bias;
        modNext = std::clamp(modNext, -1.0f, 1.0f);

        switch (mode) {
            case ModulationEffectMode::AM: out[i] = lerp(in[i], in[i] * (1.0f + (modNext * depth)) * 0.5f, mix); break;
            case ModulationEffectMode::RM: out[i] = lerp(in[i], in[i] * modNext, mix); break;
            default: out[i] = in[i];
        }
    }
};