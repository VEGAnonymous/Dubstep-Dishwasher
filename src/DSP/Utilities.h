#ifndef UTILITIES
#define UTILITIES

#include "Defines.h"
#include <kissfft/kiss_fft.h>
#include <kissfft/kiss_fftr.h>

#include <algorithm>
#include <cmath>
#include <vector>

using namespace std;

/* FUNCTIONS */

float dbAmp(float dB) { return powf(10.0f, dB / 20.0f); }

/* CLASSES */

class DelayLine { // Implements z^-N
    private:
        float delaySamples; 
        int writeIndex;
        vector<float> buffer;

        inline float interpolate(float readIndex) { // Linearly interpolate (fractional delay)
            int size = buffer.size();
            if (readIndex < 0) readIndex += size;

            int i0 = ((int)floor(readIndex)) % size;
            int i1 = (i0 + 1) % size;
            float frac = readIndex - floor(readIndex);

            return buffer[i0] + (frac * (buffer[i1] - buffer[i0]));
        }

    public:
        DelayLine(float delayTime, float maxDelayTime) : writeIndex(0) {
            int maxDelaySamples = (int)((maxDelayTime * SAMPLE_RATE) / 1000.0f);
            buffer.assign(maxDelaySamples + 1, 0.0f);
            setDelayTime(delayTime);
        }

        void setDelayTime(float delayTime) { delaySamples = (delayTime * SAMPLE_RATE) / 1000.0f; }
        void setDelaySamples(float delaySamples) { this->delaySamples = delaySamples; }
        int getSize() const { return buffer.size(); }

        inline float read(float offset = -1.0f) {
            float readOffset = (offset >= 0.0f) ? offset : delaySamples;
            float readIndex = (float)writeIndex - readOffset;
            if (readIndex < 0) readIndex += buffer.size();
            if (readOffset == floor(readOffset)) return buffer[(int)readIndex % buffer.size()];
            return interpolate(readIndex);
        }

        inline void write(float in) { 
            buffer[writeIndex] = in;
            if (++writeIndex >= buffer.size()) writeIndex = 0;
        }
};

class FFT {
    private:
        int N; // FFT size
        kiss_fftr_cfg cfgF = nullptr, cfgI = nullptr;
        std::vector<kiss_fft_cpx> fftOut; // Complex output buffer

        void allocateFFT() {
            if(cfgF) kiss_fft_free(cfgF); if(cfgI) kiss_fft_free(cfgI);
            cfgF = kiss_fftr_alloc(static_cast<int>(N), 0, nullptr, nullptr); // Forward
            cfgI = kiss_fftr_alloc(static_cast<int>(N), 1, nullptr, nullptr); // Inverse
            
            fftOut.resize(N/2 + 1);
        }
    public:
        FFT(int fftSize = 1024) : N(fftSize) { allocateFFT(); }
        ~FFT() { if(cfgF) kiss_fft_free(cfgF); if(cfgI) kiss_fft_free(cfgI); }
        FFT(const FFT&) = delete;
        FFT& operator=(const FFT&) = delete;

        void setFFTSize(int fftSize) { // !!! Must be a power of 2 !!!
            if (fftSize == N) return;
            N = fftSize; 
            allocateFFT();
        }

        // Process real input buffer (N) into output magnitudes and phase (N/2 + 1)
        void forward(const float* in, float* mag, float* phs) {
            kiss_fftr(cfgF, in, fftOut.data());
            for(size_t k = 0; k < (N/2 + 1); ++k) {
                mag[k] = sqrt((fftOut[k].r * fftOut[k].r) + (fftOut[k].i * fftOut[k].i)); // |H[k]| = sqrt(Re[k]^2 + Im[k]^2)
                phs[k] = atan2(fftOut[k].i, fftOut[k].r); // arg(H[k])
            }
        }

        // Do the opposite (reconstruct)
        void inverse(const float* mag, const float* phs, float* out) {
            fftOut[0].r = 0.0f; fftOut[0].i = 0.0f; // Zero DC
            for (size_t k = 0; k < (N/2 + 1); ++k) { // Convert polar to rectangular
                fftOut[k].r = mag[k] * cos(phs[k]);
                fftOut[k].i = mag[k] * sin(phs[k]);
            }

            kiss_fftri(cfgI, fftOut.data(), out);
            for (size_t n = 0; n < N; ++n) out[n] /= (float)N; // Normalize
        }
};

#endif // UTILITIES