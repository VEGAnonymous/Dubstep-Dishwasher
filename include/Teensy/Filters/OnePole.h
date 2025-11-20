#pragma once

#include "Teensy/Modules/IIR_Filter.h"
#include "Teensy/Defines.h"

class OnePole : public IIR_Filter { // One-pole LPF
    private:
        enum Params : ParamID { CUTOFF = 1, COEFF };

        float b0, a1, y = 0.0f;
        float cutoff;

    public:
        OnePole(float mix = 1.0f, float cutoff = SAMPLE_RATE / 2.0f);

        void setCoeff(float a);
        void setCutoff(float cutoff); // Hz
        void setParam(ParamID param, float value) override;
        float getParam(ParamID param) const override;

        float LCCDE(float x) override;
};