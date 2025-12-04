#pragma once

#include "Teensy/Utilities/STFT.h"
#include "Handler.h"

class Log_Mel {
    private:
        static constexpr size_t fftSize = 512, hopFactor = 4;

        STFT stft;
        std::vector<float> powerSpec;
        float melEnergies[NUM_MELS];
        std::function<void(const float*, size_t)> melCallback; // (melEnergies, numMels)

        MelFrame melFrame;
        bool melFrameReady = false;
        uint32_t melFrameCounter = 0;

        void processSpectrum(STFT::FFTFrame& frame);

    public:
        Log_Mel();
        virtual ~Log_Mel() = default;

        bool isFrameReady() const;
        void clearReady();
        MelFrame getFrame() const;
        
        void processBlock(const float* in, size_t n);
};