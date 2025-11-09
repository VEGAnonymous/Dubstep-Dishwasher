#pragma once

#include "Teensy/Defines.h"
#include "Teensy/Utilities/FFT.h"

#include <deque>
#include <functional>
#include <stddef.h>
#include <vector>

class STFT {
    public:
        struct FFTFrame { // Stores a frame of complex FFT bins
            std::vector<fft_cpx> bins;
            FFTFrame(size_t numBins) : bins(numBins) {}
        };

    private:
        const float bufDur;

        FFT fft;
        size_t fftSize, numBins, hopSize, hopFactor = 4;
        
        // Time-domain
        std::vector<float> inBuf, outBuf;
        size_t inPos = 0, outPos = 0, hopCounter = 0;

        // Spectral-domain
        std::vector<FFTFrame> spectrogram; // Store FFT frames
        std::vector<float> forwardFrame, inverseFrame;
        size_t spectPos = 0, spectSize;
        std::deque<size_t> processingQueue; // Queue frame indices ready for IFFT
        std::function<void(FFTFrame&)> processCallback; // Function to process frames (for phase vocoding)

    public:
        STFT(size_t fftSize, size_t hopFactor, float bufDur = 0.25f);

        // Exposing this shit for external use
        size_t getSpectSize() const;
        size_t getFFTSize() const;
        size_t getHopSize() const;
        size_t getNumBins() const;
        FFT& getFFT();
        FFTFrame& getFrame();

        void setFFTSize(size_t N); // [128, FFT_MAX_SIZE], MUST BE POWER OF 2 (but I can't make you)
        void setHopSize(size_t hopFactor); // [2, 8]
        void setProcessCallback(std::function<void(FFTFrame&)> callback);

        void forward(float input);

        float inverse();

        FFTFrame interpolateFrame(float framePos);
};