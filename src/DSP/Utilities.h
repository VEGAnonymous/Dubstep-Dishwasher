#ifndef UTILITIES
#define UTILITIES

#include "Defines.h"
#include <kissfft/kiss_fft.h>
#include <kissfft/kiss_fftr.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

using namespace std;

/* FUNCTIONS */

float dbAmp(float dB) { return pow(10.0f, dB / 20.0f); }
float ampDB(float amp) { return 20.0f * log10(amp + 1e-12); }

float uniform() { return ((float)rand() / RAND_MAX) * 2.0f - 1.0f; } // Random float between [-1, 1]

void makeEnvelope(vector<float>& env, size_t N, envelopeType type) {
    // https://www.desmos.com/calculator/j7vhnwaylq
    function<float(size_t)> func;

    switch (type) {
        case HANN:
            func = [N](size_t n) {
                return 0.5f * (1.0f - cosf((2.0f * static_cast<float>(M_PI) * n) / (N - 1)));
            }; break;
        case HAMMING:
            func = [N](size_t n) {
                return 0.54f - (0.46f * cosf((2.0f * static_cast<float>(M_PI) * n) / (N - 1)));
            }; break;
        case SINE:
            func = [N](size_t n) {
                return sinf((static_cast<float>(M_PI) * n) / N);
            }; break;
        case TRI:
            func = [N](size_t n) {
                return 1.0f - abs((n - (N / 2.0f)) / (N / 2.0f));
            }; break;
        case PERC:
            func = [N](size_t n) {
                const float attack = 0.03f;
                const float decay = 6.0f;

                float attackSamples = N * attack;
                if (attackSamples < 1) attackSamples = 1;

                if (n < attackSamples) { return n / (float)attackSamples; } // Linear attack
                else { // Exponential decay  
                    float t = (n - attackSamples) / (float)(N - attackSamples);
                    return expf(-decay * t);
                }
            }; break;
        case SMOOTH_RECT:
            func = [N](size_t n) {
                const float smooth = 0.05f;
                float edge = N * smooth;
                if (edge < 1) edge = 1;

                if (n < edge) { return 0.5f * (1.0f - cosf(static_cast<float>(M_PI) * n / edge)); } // Fade in
                else if (n >= (N - edge)) { return 0.5f * (1.0f - cosf(static_cast<float>(M_PI) * (N - n) / edge)); } // Fade out
                else { return 1.0f; }
            }; break;
        default: func = [](size_t n){ return 1.0f; };
    }

    env.resize(N);
    for (size_t n = 0; n < N; ++n) {
        env[n] = func(n); // Hann window
    }
}

template <typename T>
inline float lerp(const T& buffer, float index, size_t size) { // Linearly interpolate buffer indices
    if (index < 0) index += size;
    size_t i0 = (static_cast<size_t>(floor(index))) % size;
    size_t i1 = (i0 + 1) % size;
    float frac = index - floor(index);
    return buffer[i0] + (frac * (buffer[i1] - buffer[i0]));
}

inline float lerp(float a, float b, float t) { return a + (t * (b - a)); } // Linearly interpolate scalars

inline void overlapAdd(vector<float>& target, const vector<float>& frame, const vector<float>& window, size_t startPos = 0) {
    for (size_t j = 0; j < target.size(); ++j) {
        size_t pos = (startPos + j) % target.size();
        target[pos] += frame[j] * window[j];
    }
}

/* CLASSES */

class DelayLine { // Implements z^-N
    private:
        float delaySamples; 
        int writeIndex;
        vector<float> buffer;

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
            return lerp(buffer, readIndex, buffer.size());
        }

        inline void write(float in) { 
            buffer[writeIndex] = in;
            if (++writeIndex >= buffer.size()) writeIndex = 0;
        }
};

class FFT {
    private:
        size_t fftSize;
        kiss_fftr_cfg cfgF = nullptr, cfgI = nullptr;
        vector<kiss_fft_cpx> fftOut; // Complex output buffer

        void allocateFFT() {
            if(cfgF) kiss_fft_free(cfgF); if(cfgI) kiss_fft_free(cfgI);
            cfgF = kiss_fftr_alloc(static_cast<int>(fftSize), 0, nullptr, nullptr); // Forward
            cfgI = kiss_fftr_alloc(static_cast<int>(fftSize), 1, nullptr, nullptr); // Inverse
            
            fftOut.resize(fftSize/2 + 1);
        }
    public:
        FFT(size_t fftSize = 1024) { setFFTSize(fftSize); }
        ~FFT() { if(cfgF) kiss_fft_free(cfgF); if(cfgI) kiss_fft_free(cfgI); }
        FFT(const FFT&) = delete;
        FFT& operator=(const FFT&) = delete;

        void setFFTSize(size_t N) { // !!! Must be a power of 2 !!!
            fftSize = N;
            allocateFFT();
        }

        // Process real input buffer (fftSize) into output magnitudes and phase (fftSize/2 + 1)
        void forward(const float* in, float* mag, float* phs) {
            kiss_fftr(cfgF, in, fftOut.data());
            for(size_t k = 0; k < (fftSize/2 + 1); ++k) {
                mag[k] = sqrt((fftOut[k].r * fftOut[k].r) + (fftOut[k].i * fftOut[k].i)); // |H[k]| = sqrt(Re[k]^2 + Im[k]^2)
                phs[k] = atan2(fftOut[k].i, fftOut[k].r); // arg(H[k])
            }
        }

        // Do the opposite (reconstruct)
        void inverse(const float* mag, const float* phs, float* out) {
            fftOut[0].r = 0.0f; fftOut[0].i = 0.0f; // Zero DC
            for (size_t k = 0; k < (fftSize/2 + 1); ++k) { // Convert polar to rectangular
                fftOut[k].r = mag[k] * cos(phs[k]);
                fftOut[k].i = mag[k] * sin(phs[k]);
            }

            kiss_fftri(cfgI, fftOut.data(), out);
            for (size_t n = 0; n < fftSize; ++n) out[n] /= (float)fftSize; // Normalize
        }
};

class STFT {
    public:
        struct FFTFrame {
            vector<float> mag;
            vector<float> phase;
            FFTFrame(size_t bins) : mag(bins, 0.0f), phase(bins, 0.0f) {}
        };
    private:
        const float bufDur = 3.0f;

        FFT fft;
        size_t fftSize, numBins, hopSize, hopFactor = 4;
        
        // Time-domain
        vector<float> inBuf, outBuf, window;
        size_t inPos = 0, outPos = 0, hopCounter = 0;
        bool frameReady = false;

        // Spectral-domain
        vector<FFTFrame> spectrogram; // Store FFT frames
        size_t spectPos = 0, spectSize;
    public:
        STFT(size_t fftSize, size_t hopFactor) : fft(fftSize) { setHopSize(hopFactor); setFFTSize(fftSize); }

        // Exposing this shit for external use
        size_t getSpectSize() const { return spectSize; }
        size_t getHopSize() const { return hopSize; }
        size_t getFFTSize() const { return fftSize; }
        size_t getNumBins() const { return numBins; }
        FFT& getFFT() { return fft; }
        const vector<float>& getWindow() const { return window; }
        FFTFrame& getFrame() { // Get most recent FFT frame
            size_t index = (spectPos == 0) ? (spectSize - 1) : (spectPos - 1);
            return spectrogram[index];
        }

        void setHopSize(size_t hopFactor) { hopSize = fftSize / hopFactor; }
        void setFFTSize(size_t N) { 
            fftSize = N; numBins = (fftSize / 2) + 1; hopSize = fftSize / hopFactor;
            inPos = 0; outPos = 0; hopCounter = 0; frameReady = false;
            inBuf.resize(fftSize, 0.0f); outBuf.assign(fftSize, 0.0f);

            // Init analysis window / FFT
            makeEnvelope(window, fftSize, HANN);
            fft.setFFTSize(fftSize);

            // Init spectrogram
            spectrogram.clear();
            spectSize = (size_t)(bufDur * ((float)SAMPLE_RATE / (float)hopSize));
            for (size_t i = 0; i < spectSize; ++i) { spectrogram.emplace_back(numBins); }
            spectPos = 0;
        }

        void forward(float input) { // Store FFT frames in spectrogram
            const size_t fftN = fftSize, hopN = hopSize;
            inBuf[inPos] = input; // Write input to buffer
            ++inPos; if (inPos >= fftSize) inPos = 0;
            ++hopCounter;
            
            /* GENERATE FFT FRAMES */
            if (hopCounter >= hopSize) { // Every hopN samples
                hopCounter = 0;
                
                // Extract full FFT frame from circular buffer
                vector<float> frame(fftN);
                size_t readPos = inPos; // Start from oldest sample
                for (size_t j = 0; j < fftN; ++j) {
                    frame[j] = inBuf[readPos] * window[j];
                    ++readPos; if (readPos >= fftN) readPos = 0;
                }
                
                // Forward FFT
                vector<float> mag(numBins), phs(numBins);
                fft.forward(frame.data(), mag.data(), phs.data());

                // Store in spectrogram buffer
                FFTFrame& currentFrame = spectrogram[spectPos];
                for (size_t k = 0; k < numBins; ++k) {
                    currentFrame.mag[k] = mag[k];
                    currentFrame.phase[k] = phs[k];
                }
                
                ++spectPos; if (spectPos >= spectSize) spectPos = 0;
                frameReady = true;
            }
        }

        float inverse() { // Reconstruct signal from spectrogram (ideally sync this with forward() to match inPos/outPos)
            float out = outBuf[outPos];
            outBuf[outPos] = 0.0f;

            /* RECONSTRUCT FFT FRAMES */
            if (frameReady) { // Every hopN samples
                const FFTFrame& frameSpec = getFrame();
                // IFFT
                vector<float> frame(fftSize);
                fft.inverse(frameSpec.mag.data(), frameSpec.phase.data(), frame.data());
                // Window and OLA
                overlapAdd(outBuf, frame, window, outPos);        
                frameReady = false;
            }

            ++outPos; if (outPos >= fftSize) outPos = 0;
            return out;
        }

        FFTFrame interpolateFrame(float framePos) { // Take float index to spectrogram buffer and return lerped frame mag/phs
            if (spectrogram.empty()) { return FFTFrame(numBins); }

            int frameIndex = (int)floor(framePos);
            float frameFrac = framePos - floor(framePos);

            size_t frame0 = frameIndex % spectSize;
            size_t frame1 = (frame0 + 1) % spectSize;

            FFTFrame interpFrame(numBins);
            for (size_t k = 0; k < numBins; ++k) {
                // Lerp magnitudes
                interpFrame.mag[k] = lerp(spectrogram[frame0].mag[k], spectrogram[frame1].mag[k], frameFrac);

                // Lerp phases
                float phase0 = spectrogram[frame0].phase[k];
                float phase1 = spectrogram[frame1].phase[k];
                float phaseDiff = fmod((phase1 - phase0) + M_PI, 2.0f * M_PI); // PHASE UNWRAPPING!
                if (phaseDiff < 0) phaseDiff += 2.0f * M_PI;
                phaseDiff -= M_PI;
                
                interpFrame.phase[k] = phase0 + (frameFrac * phaseDiff);
            }
            return interpFrame;
        }
};

#endif // UTILITIES