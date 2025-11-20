#pragma once

#include "Teensy/Modules/Effect.h"
#include "Teensy/Generators/Wavetable.h"
#include "Teensy/Utilities/DelayLine.h"

class Flanger : public Effect {
    private:
        enum Params : ParamID { MIX, RATE, DEPTH, FEEDBACK };

        DelayLine delayLine;
        Wavetable LFO;
        float mix, rate, depth, feedback;
        float wetSig = 0;

    public:
        Flanger(float mix = 1.0f, float rate = 0.08f, float depth = 1.0f, float feedback = 0.5f);

        void setMix(float mix); // [0.0, 1.0]
        void setRate(float rate); // Hz, [0.0, 20.0]
        void setDepth(float depth); // [0.0, 1.0]
        void setFeedback(float feedback); // [-0.95, 0.95]
        void setParam(ParamID param, float value) override;
        float getParam(ParamID param) const override;

        void process(const float* in, float* out, size_t n) override;
};