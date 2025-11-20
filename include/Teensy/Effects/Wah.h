#pragma once

#include "Teensy/Modules/Effect.h"
#include "Teensy/Utilities/DelayLine.h"
#include "Teensy/Filters/Biquad/BPF_Biquad.h"

class Wah : public Effect {
    private:
        enum Params : ParamID { MIX, MIN_FREQ, MAX_FREQ, Q };

        float mix, minFreq, maxFreq, q;

        static constexpr float L = 50.0f, L_samples = L * SAMPLE_RATE / 1000.0f; // RMS window size
        float rms = 1e-6f; DelayLine inBuffer;
        BPF_Biquad bpf;

    public:
        Wah(float mix = 1.0f, float minFreq = 350.0f, float maxFreq = 2500.0f, float q = 1.6f);

        void setMix(float mix); // [0.0, 1.0]
        void setMinFreq(float minFreq); // Hz, [20.0, 1000.0]
        void setMaxFreq(float maxFreq); // Hz, [1000.0, 8000.0]
        void setQ(float q); // [0.3, 6.0]
        void setParam(ParamID param, float value) override;
        float getParam(ParamID param) const override;

        void process(const float* in, float* out, size_t n) override;
};