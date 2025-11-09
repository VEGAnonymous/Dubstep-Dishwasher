#include <Teensy/Utilities/FFT.h>

#include <algorithm>

/* PRIVATE */

/*

size_t fftSize;
arm_rfft_fast_instance_f32 rfft;
std::vector<float32_t> fftBuffer; // Output buffer, real interleaved CMSIS form

*/

void FFT::allocateFFT() {
    fftSize = std::clamp(fftSize, (size_t)128, (size_t)FFT_MAX_SIZE);
    arm_rfft_fast_init_f32(&rfft, static_cast<uint16_t>(fftSize));
    fftBuffer.resize(fftSize);
}

// Convert CMSIS interleaved -> fft_cpx bins
void FFT::deinterleave(const float32_t* interleaved, fft_cpx* outBins) const {
    const size_t N = fftSize;
    const size_t K = N / 2; // Highest bin index
    
    outBins[0].r = interleaved[0]; outBins[0].i = 0.0f; // Bin 0
    outBins[K].r = interleaved[1]; outBins[K].i = 0.0f; // Bin N/2
    for (size_t k = 1; k < K; ++k) { outBins[k].r = interleaved[2 * k]; outBins[k].i = interleaved[2 * k + 1]; } // Bins 1..K-1
}
// Convert fft_cpx bins -> CMSIS interleaved
void FFT::interleave(const fft_cpx* inBins, float32_t* interleaved) const {
    const size_t N = fftSize;
    const size_t K = N / 2; // Highest bin index
    
    interleaved[0] = inBins[0].r; // Bin 0
    interleaved[1] = inBins[K].r; // Bin N/2
    for (size_t k = 1; k < K; ++k) { interleaved[2 * k] = inBins[k].r; interleaved[2 * k + 1] = inBins[k].i; } // Bins 1..K-1
}

/* PUBLIC */

FFT::FFT(size_t N) : fftSize(N) { allocateFFT(); }

void FFT::setFFTSize(size_t fftSize) { this->fftSize = fftSize; allocateFFT(); } // Doesn't enforce limitations - dependencies should
size_t FFT::getFFTSize() const { return fftSize; }
size_t FFT::getNumBins() const { return (fftSize / 2) + 1; }

void FFT::forward(const float32_t* in, fft_cpx* outBins) {
    std::copy(in, in + fftSize, fftBuffer.begin()); // Copy to prevent in-place modification
    arm_rfft_fast_f32(&rfft, fftBuffer.data(), fftBuffer.data(), 0);
    deinterleave(fftBuffer.data(), outBins);
}

void FFT::inverse(const fft_cpx* inBins, float32_t* out) {
    interleave(inBins, fftBuffer.data());
    arm_rfft_fast_f32(&rfft, fftBuffer.data(), out, 1);
}