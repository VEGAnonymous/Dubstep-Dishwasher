#pragma once

#include "Teensy/Modules/Effect.h"
#include "Teensy/Utilities/DelayLine.h"
#include "Teensy/Utilities/STFT.h"

class Phase_Vocoder : public Effect { // Classes which process FFT frames in the frequency domain 
    protected:
        enum Params : ParamID { MIX, FFT_SIZE };

        float mix; size_t fftSize;

        STFT stft;
        DelayLine latencyComp;

        virtual void processSpectrum(STFT::FFTFrame& frame) = 0; // Subclasses must implement

    public:
        Phase_Vocoder(float mix = 1.0f, size_t fftSize = 512, size_t hopFactor = 4);
        virtual ~Phase_Vocoder() = default;

        void setMix(float mix); // [0.0, 1.0]
        void setFFTSize(size_t N); // [128, FFT_MAX_SIZE], MUST BE POWER OF 2 (please? I'm asking nicely)

        void setParam(ParamID param, float value) override;
        float getParam(ParamID param) const override;
        void process(const float* in, float* out, size_t n) override;
};