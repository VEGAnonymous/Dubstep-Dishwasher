#pragma once

#include "Teensy/Modules/Effect.h"
#include "Teensy/Generators/Random.h"
#include "Teensy/Utilities/DelayLine.h"

class Chorus : public Effect {
    private:
        enum Params : ParamID { MIX, RATE, DEPTH, DELAY_TIME, FEEDBACK };

        float mix, rate, depth, delayTime, feedback;
        static constexpr uint8_t voiceCount = 4; // 1-5
        float sqrt_vc;

        struct voice {
            float mix, depth, baseDelay;
            Random mod;
            DelayLine delayLine;

            voice(float mix, float depth, float baseDelay, float rate) 
            : mix(mix), depth(depth), baseDelay(baseDelay), mod(rate, RandomMode::PERLIN), delayLine(baseDelay, 51.0f) {}
        };
        std::vector<voice> voices;

    public:
        Chorus(float mix = 1.0f, float rate = 0.08f, float depth = 25.0f, float delayTime = 5.0f, float feedback = 0.1f);

        void setMix(float mix); // [0.0, 1.0]
        void setRate(float rate); // Hz, [0.0, 20.0]
        void setDepth(float depth); // ms, [0.0, 25.0]
        void setDelayTime(float delayTime); // ms, [0.0, 20.0]
        void setFeedback(float feedback); // [-0.95, 0.95]
        void setParam(ParamID param, float value) override;
        float getParam(ParamID param) const override;

        void process(const float* in, float* out, size_t n) override;
};