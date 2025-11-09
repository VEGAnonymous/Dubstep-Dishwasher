#pragma once

#include "Teensy/Defines.h"

#include "arm_math.h"

#include <cmath>
#include <vector>

class FFT { // Optimized *real* FFT
    private:
        size_t fftSize;
        arm_rfft_fast_instance_f32 rfft;
        std::vector<float32_t> fftBuffer; // Output buffer, real interleaved CMSIS form

        void allocateFFT();

        // Convert CMSIS interleaved -> fft_cpx bins
        void deinterleave(const float32_t* interleaved, fft_cpx* outBins) const;
        // Convert fft_cpx bins -> CMSIS interleaved
        void interleave(const fft_cpx* inBins, float32_t* interleaved) const;

    public:
        FFT(size_t N = 512);
        // Memory management for safety and shit
        ~FFT() = default;
        FFT(const FFT&) = delete;
        FFT& operator=(const FFT&) = delete;

        void setFFTSize(size_t fftSize); // Doesn't enforce limitations - dependencies should
        size_t getFFTSize() const;
        size_t getNumBins() const;

        void forward(const float32_t* in, fft_cpx* outBins);

        void inverse(const fft_cpx* inBins, float32_t* out);
};