#pragma once

#include "Teensy/Utilities/STFT.h"

class Log_Mel {
    private:
        static constexpr size_t fftSize = 512, hopFactor = 4;

        STFT stft;
        std::vector<float> powerSpec;
        float melEnergies[NUM_MELS];
        std::function<void(const float*, size_t)> melCallback; // (melEnergies, numMels)

        void processSpectrum(STFT::FFTFrame& frame);

    public:
        Log_Mel();
        virtual ~Log_Mel() = default;

        void setMelCallback(std::function<void(const float*, size_t)> callback);
        void processBlock(const float* in, size_t n);
};