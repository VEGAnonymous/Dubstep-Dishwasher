#pragma once

#include "Teensy/Filters/Biquad/LPF_Biquad.h"
#include "Teensy/Filters/Biquad/HPF_Biquad.h"
#include "Teensy/Filters/Biquad/LowShelf_Biquad.h"
#include "Teensy/Filters/Biquad/HighShelf_Biquad.h"
#include "Teensy/Filters/Biquad/Peak_Biquad.h"
#include "Teensy/Filters/Biquad/Notch_Biquad.h"

#include <array>
#include <memory>

class Equalizer : public Effect {
    private:
        enum Params : ParamID { MIX, BAND1_TYPE, BAND1_CUTOFF, BAND1_Q, BAND1_GAIN, BAND2_TYPE, BAND2_CUTOFF, BAND2_Q, BAND2_GAIN };

        std::array<std::unique_ptr<Biquad>, 2> bands;

        BiquadType band1Type, band2Type;
        float mix, band1Cutoff, band1Q, band1Gain, band2Cutoff, band2Q, band2Gain;

        std::unique_ptr<Biquad> createBiquad(BiquadType type, float cutoff, float q, float gainDB);

    public:
        Equalizer(float mix = 1.0f,
                  BiquadType band1Type = BiquadType::LOW_SHELF,  float band1Cutoff = 200.0f,  float band1Q = 0.707f, float band1Gain = 0.0f,
                  BiquadType band2Type = BiquadType::HIGH_SHELF, float band2Cutoff = 2000.0f, float band2Q = 0.707f, float band2Gain = 0.0f);

        void setMix(float mix); // [0.0, 1.0]
        void setBand1Type(BiquadType type);
        void setBand1Cutoff(float cutoff); // Hz, [20.0, 20000.0]
        void setBand1Q(float q); // [0.02, 40.0]
        void setBand1Gain(float gainDB); // dB, [-24.0, 24.0]
        void setBand2Type(BiquadType type);
        void setBand2Cutoff(float cutoff); // Hz, [20.0, 20000.0]
        void setBand2Q(float q); // [0.02, 40.0]
        void setBand2Gain(float gainDB); // dB, [-24.0, 24.0]
        void setParam(ParamID param, float value) override;
        float getParam(ParamID param) const override;

        void process(const float* in, float* out, size_t n) override;
};