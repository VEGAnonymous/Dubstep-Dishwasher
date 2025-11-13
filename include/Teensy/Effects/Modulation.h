#pragma once

#include "Teensy/Modules/Effect.h"
#include "Teensy/Generators/Wavetable.h"

class Modulation : public Effect {
    private:
        enum Params : ParamID { MIX, MODE, MODULATOR, FREQ, DEPTH, BIAS, RECTIFY };

        float mix, freq, depth, bias, rectify;
        ModulationEffectMode mode; 
        Wavetable modulator;

    public:
        Modulation(float mix = 1.0f, ModulationEffectMode mode = ModulationEffectMode::AM, WavetableType modulatorType = WavetableType::SINE, 
                   float freq = 5.0f, float depth = 0.5f, float bias = 0.0f, float rectify = 0.0f);

        void setMix(float mix); // [0.0, 1.0]
        void setMode(ModulationEffectMode mode);
        void setModulator(WavetableType modulatorType);
        void setFreq(float freq); // [1.0, 2000.0]
        void setDepth(float depth); // [0.0, 1.0]
        void setBias(float bias); // [0.0, 1.0]
        void setRectify(float rectify); // [-1.0, 1.0]
        void setParam(ParamID param, float value) override;

        void process(const float* in, float* out, size_t n) override;
};