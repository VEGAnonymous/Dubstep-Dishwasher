#pragma once

#include "Teensy/Modules/Effect.h"
#include "Teensy/Utilities/DelayLine.h"

class Gate : public Effect {
    private:
        enum Params : ParamID { MIX, THRESHOLD, ATTACK_TIME, RELEASE_TIME, HOLD_TIME, INVERT };

        float mix, threshold, attackTime, releaseTime, holdTime; bool invert;
        float attackCoeff, releaseCoeff; 

        DelayLine inBuffer;
        static constexpr float L = 50.0f, L_samples = L * SAMPLE_RATE / 1000.0f; // RMS window size
        float rms = 1e-6f, gainSmoothed = 0.0f;
        size_t holdSamples, holdCounter = 0;

    public:
        Gate(float mix = 1.0f, float threshold = -18.0f, float attack = 25.0f, float release = 25.0f, float hold = 50.0f, bool invert = false);

        void setMix(float mix); // [0.0, 1.0]
        void setThreshold(float threshold); // dB, [-100.0, 0.0]
        void setAttackTime(float attackTime); // ms, [0.01, 250.0]
        void setReleaseTime(float releaseTime); // ms, [0.01, 1500.0]
        void setHoldTime(float holdTime); // ms, [1.0, 1500.0]
        void setInvert(bool invert);
        void setParam(ParamID param, float value) override;
        float getParam(ParamID param) const override;

        void process(const float* in, float* out, size_t n) override;
};