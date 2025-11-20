#pragma once

#include "Teensy/Modules/Effect.h"

class Gain : public Effect {
    private:
        enum Params : ParamID { GAIN, CLIP };

        float gainDB; bool clip;
        float gainFactor;

    public:
        Gain(float gainDB = 0.0f);

        void setGain(float gainDB);
        void setClip(bool clip);
        void setParam(ParamID param, float value) override;
        float getParam(ParamID param) const override;

        void process(const float* in, float* out, size_t n) override;
};