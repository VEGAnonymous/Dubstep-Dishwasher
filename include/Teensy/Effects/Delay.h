#pragma once

#include "Teensy/Modules/Effect.h"
#include "Teensy/Utilities/DelayLine.h"

class Delay : public Effect {
    private:
        enum Params : ParamID { MIX, DELAY_TIME, FEEDBACK };

        static constexpr float maxDelayTime = 500.0f; // ms
        float mix, delayTime, feedback;
        DelayLine delayLine;

    public:
        Delay(float mix = 0.3f, float delayTime = 200.0f, float feedback = 0.4f);

        void setMix(float mix); // [0.0, 1.0]
        void setDelayTime(float delayTime); // ms, [1.0, 500.0]
        void setFeedback(float feedback); // [-0.95, 0.95]
        void setParam(ParamID param, float value) override;
        float getParam(ParamID param) const override;

        void process(const float* in, float* out, size_t n) override;
};