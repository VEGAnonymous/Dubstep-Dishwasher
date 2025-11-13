#pragma once

#include "Teensy/Modules/Effect.h"
#include "Teensy/Generators/Wavetable.h"
#include "Teensy/Filters/APF.h"

class Phaser : public Effect {
    private:
        enum Params : ParamID { MIX, RATE, CENTER_FREQ, SPREAD, DEPTH, FEEDBACK };

        static constexpr uint8_t order = 6; // Number of APFs
        static constexpr float q = 0.8f;

        float mix, rate, centerFreq, spread, depth, feedback;

        std::vector<APF> apfSections; // APF bank
        std::vector<float> baseFreqs; // Store APF base freqs
        Wavetable LFO;
        float wetSig = 0.0f;

    public:
        void setupStages();
        Phaser(float mix = 1.0f, float rate = 0.08f, float centerFreq = 600.0f, float spread = 1.0f, float depth = 0.5f, float feedback = 0.8f);

        void setMix(float mix); // [0.0, 1.0]
        void setRate(float rate); // Hz, [0.0, 20.0]
        void setCenterFreq(float centerFreq); // Hz, // [50.0, 8000.0]
        void setSpread(float spread); // [0.1, 1.0]
        void setDepth(float depth); // [0.0, 1.0]
        void setFeedback(float feedback); // [-0.95, 0.95]
        void setParam(ParamID param, float value) override;

        void process(const float* in, float* out, size_t n) override;
};
