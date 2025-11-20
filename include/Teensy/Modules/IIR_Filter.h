#pragma once

#include "Teensy/Modules/Effect.h"
#include "Teensy/Utilities/Utilities.h"

#include <algorithm>

class IIR_Filter : public Effect {
    protected:
        enum Params : ParamID { MIX };
        float mix;
        virtual float LCCDE(float x) = 0; // LCCDE to implement; can also call function pointer (e.g., selectable filter order)

    public:
        void setMix(float mix); // [0.0, 1.0]
        void setParam(ParamID param, float value) override;
        float getParam(ParamID param) const override;
        float processSample(float x);
        void process(const float* in, float* out, size_t n) override;
};