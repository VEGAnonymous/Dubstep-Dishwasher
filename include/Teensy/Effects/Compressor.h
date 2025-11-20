#pragma once

#include "Teensy/Modules/Effect.h"
#include "Teensy/Utilities/DelayLine.h"

class Compressor : public Effect {
    private:
        enum Params : ParamID { MIX, THRESHOLD, RATIO, KNEE, ATTACK_TIME, RELEASE_TIME, MAKEUP_GAIN, AUTO_MAKEUP };

        float mix, threshold, ratio, knee, attackTime, releaseTime, makeupGain; bool autoMakeup;
        float attackCoeff, releaseCoeff, makeupCoeff;

        DelayLine inBuffer; 
        static constexpr float L = 50.0f, L_samples = L * SAMPLE_RATE / 1000.0f; // RMS window size
        float rms = 1e-6f, gainSmoothed = 0.0f;
        float reductionSmoothed = 0.0f, autoMakeupGain = 0.0f;

        float computeReduction(float rmsDB);

    public:
        Compressor(float mix = 1.0f, float threshold = -18.0f, float ratio = 4.0f, float knee = 10.0f, 
                   float attack = 100.0f, float release = 100.0f, float makeupGain = 0.0f, bool autoMakeup = true);

        void setMix(float mix); // [0.0, 1.0]
        void setThreshold(float threshold); // dB, [-100.0, 0.0]
        void setRatio(float ratio); // [1.0, 100.0]
        void setKnee(float knee); // dB, [0.0, 40.0]
        void setAttackTime(float attackTime); // ms, [0.01, 250.0]
        void setReleaseTime(float releaseTime); // ms, [10.0, 2500.0]
        void setMakeupGain(float makeupGain); // dB, [-72.0, 36.0]
        void setAutoMakeup(bool autoMakeup);
        void setParam(ParamID param, float value) override;
        float getParam(ParamID param) const override;

        void process(const float* in, float* out, size_t n) override;
};