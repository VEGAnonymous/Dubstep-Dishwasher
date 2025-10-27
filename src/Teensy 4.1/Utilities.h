#ifndef UTILITIES
#define UTILITIES

#include "Defines.h"
#include <kiss_fft.h>
#include <kiss_fftr.h>

#include <algorithm>
#include <cmath>
#include <deque>
#include <functional>
#include <vector>

/* UTILITIES */

/* FUNCTIONS */

float dbAmp(float dB) { return pow(10.0f, dB / 20.0f); }
float ampDB(float amp) { return 20.0f * log10(amp + 1e-12); }

float uniform() { return ((float)rand() / RAND_MAX) * 2.0f - 1.0f; } // Random float between [-1, 1]

float getEnvelopeValue(float t, size_t N, envelopeType type) {
    // https://www.desmos.com/calculator/j7vhnwaylq
    switch (type) {
        case HANN: return 0.5f * (1.0f - cosf(2.0f * static_cast<float>(M_PI) * t));
        case HAMMING: return 0.54f - (0.46f * cosf(2.0f * static_cast<float>(M_PI) * t));
        case SINE: return sinf(static_cast<float>(M_PI) * t);
        case TRI: return 1.0f - fabsf(2.0f * t - 1.0f);
        case PERC: {
            const float attack = 0.03f;
            const float decay = 6.0f;
            if (t < attack) { return t / attack; // Linear attack
            } else { // Exponential decay
                float t_decay = (t - attack) / (1.0f - attack); 
                return expf(-decay * t_decay);
            }
        }
        case SMOOTH_RECT: {
            const float smooth = 0.05f;
            if (t < smooth) { // Fade in
                return 0.5f * (1.0f - cosf(static_cast<float>(M_PI) * t / smooth)); 
            } else if (t >= (1.0f - smooth)) { // Fade out
                return 0.5f * (1.0f - cosf(static_cast<float>(M_PI) * (1.0f - t) / smooth)); 
            } else { return 1.0f; }
        }
        default: return 1.0f;
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

inline float dryWetMix(float dry, float wet, float mix, bool lin = true) {
    if (mix == 1.0f) return wet;
    else if (mix == 0.0f) return dry;
    else if (lin) return lerp(dry, wet, mix); // Linear mix
    else return (dry * cosf(mix * M_PI_2)) + (wet * sinf(mix * M_PI_2)); // Equal power crossfade
}

inline void overlapAdd(std::vector<float>& target, const std::vector<float>& frame, envelopeType type, size_t startPos = 0) {
    const size_t N = frame.size();
    for (size_t i = 0; i < N; ++i) {
        size_t pos = (startPos + i) % target.size();
        target[pos] += frame[i] * getEnvelopeValue((float)i / N, N, type);
    }
}

/* CLASSES */

class DelayLine { // Implements z^-N
    private:
        float delaySamples; 
        size_t writeIndex;
        // std::vector<float> buffer;
        float* buffer; size_t size;
    public:
        DelayLine(float delayTime, float maxDelayTime) : writeIndex((size_t)0) {
            size = (size_t)((maxDelayTime * SAMPLE_RATE) / 1000.0f) + 1;
            buffer = (float*)extmem_malloc(size * sizeof(float));
            if (!buffer) while (1) { }
            memset(buffer, 0, size * sizeof(float));

            setDelayTime(delayTime);
        }

        void setDelayTime(float delayTime) { delaySamples = (delayTime * SAMPLE_RATE) / 1000.0f; } // ms
        void setDelaySamples(float delaySamples) { this->delaySamples = delaySamples; }
        int getSize() const { return size; }

        inline float read(float offset = -1.0f) {
            float readOffset = (offset >= 0.0f) ? offset : delaySamples;
            float readIndex = (float)writeIndex - readOffset;
            if (readIndex < 0) readIndex += size;
            if (readOffset == floor(readOffset)) return buffer[(int)readIndex % size];
            return lerp(buffer, readIndex, size);
        }

        inline void write(float in) { 
            buffer[writeIndex] = in;
            if (++writeIndex == size) writeIndex = 0;
        }
};

/* Vector version placeholder for testing
class DelayLine { // Implements z^-N
    private:
        float delaySamples; 
        size_t writeIndex;
        std::vector<float> buffer;
    public:
        DelayLine(float delayTime, float maxDelayTime) : writeIndex((size_t)0) {
            buffer.resize(((maxDelayTime * SAMPLE_RATE) / 1000.0f) + 1, 0.0f);
            setDelayTime(delayTime);
        }

        void setDelayTime(float delayTime) { delaySamples = (delayTime * SAMPLE_RATE) / 1000.0f; } // ms
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
}; */

class FFT {
    private:
        size_t fftSize;
        kiss_fftr_cfg cfgF = nullptr, cfgI = nullptr;
        std::vector<kiss_fft_cpx> fftOut; // Complex output buffer

        void allocateFFT() {
            if(cfgF) { kiss_fft_free(cfgF); } if(cfgI) { kiss_fft_free(cfgI); }
            cfgF = kiss_fftr_alloc(static_cast<int>(fftSize), 0, nullptr, nullptr); // Forward
            cfgI = kiss_fftr_alloc(static_cast<int>(fftSize), 1, nullptr, nullptr); // Inverse
            
            fftOut.resize(fftSize/2 + 1);
        }
    public:
        FFT(size_t fftSize = 512) { setFFTSize(fftSize); }
        ~FFT() { if(cfgF) kiss_fft_free(cfgF); if(cfgI) kiss_fft_free(cfgI); }
        FFT(const FFT&) = delete;
        FFT& operator=(const FFT&) = delete;

        void setFFTSize(size_t N) { // [128, FFT_MAX_SIZE], MUST BE POWER OF 2
            fftSize = std::clamp(N, (size_t)128, (size_t)FFT_MAX_SIZE);
            allocateFFT();
        }

        void forward(const float* in, kiss_fft_cpx* out) {
            kiss_fftr(cfgF, in, out);
        }

        void inverse(kiss_fft_cpx* in, float* out) {
            for(size_t k = 0; k < (fftSize/2 + 1); ++k) fftOut[k] = in[k];
            kiss_fftri(cfgI, fftOut.data(), out);
            for (size_t n = 0; n < fftSize; ++n) out[n] /= (float)fftSize;
        }
};

class STFT {
    public:
        struct FFTFrame {
            std::vector<kiss_fft_cpx> bins;
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
        std::function<void(FFTFrame&)> processCallback; // Function to process frames

    public:
        STFT(size_t fftSize, size_t hopFactor, float bufDur = 0.25f) : bufDur(bufDur), fft(fftSize), hopFactor(hopFactor) { setFFTSize(fftSize); }

        // Exposing this shit for external use
        size_t getSpectSize() const { return spectSize; }
        size_t getFFTSize() const { return fftSize; }
                size_t getHopSize() const { return hopSize; }
        size_t getNumBins() const { return numBins; }
        FFT& getFFT() { return fft; }
        FFTFrame& getFrame() { // Get most recent FFT frame
            size_t index = (spectPos == 0) ? (spectSize - 1) : (spectPos - 1);
            return spectrogram[index];
        }

        void setFFTSize(size_t N) { // [128, FFT_MAX_SIZE], MUST BE POWER OF 2
            fftSize = std::clamp(N, (size_t)128, (size_t)FFT_MAX_SIZE); numBins = (fftSize / 2) + 1; 
            hopSize = fftSize / hopFactor;
            
            inPos = 0; outPos = 0; hopCounter = 0;
            inBuf.resize(fftSize, 0.0f); outBuf.assign(fftSize, 0.0f);

            fft.setFFTSize(fftSize);

            forwardFrame.resize(fftSize, 0.0f); inverseFrame.resize(fftSize, 0.0f);

            // Init spectrogram
            spectrogram.clear();
            spectSize = (size_t)(bufDur * ((float)SAMPLE_RATE / (float)hopSize));
            for (size_t i = 0; i < spectSize; ++i) { spectrogram.emplace_back(numBins); }
            spectPos = 0;
        }
        void setHopSize(size_t hopFactor) { // [2, 8]
            this->hopFactor = std::clamp(hopFactor, (size_t)2, (size_t)8); 
            setFFTSize(fftSize);
        }
        void setProcessCallback(std::function<void(FFTFrame&)> callback) { processCallback = callback; }

        void forward(float input) { // Store FFT frames in spectrogram
            const size_t fftN = fftSize, hopN = hopSize;
            inBuf[inPos] = input; // Write input to buffer
            ++inPos; if (inPos >= fftSize) inPos = 0;
            ++hopCounter;
            
            /* GENERATE FFT FRAMES */
            if (hopCounter >= hopN) { // Every hopN samples
                hopCounter = 0;
                
                // Extract full FFT frame from circular buffer
                size_t readPos = inPos; // Start from oldest sample
                for (size_t j = 0; j < fftN; ++j) {
                    forwardFrame[j] = inBuf[readPos] * getEnvelopeValue((float)j / fftN, fftN, HANN);
                    ++readPos; if (readPos >= fftN) readPos = 0;
                }
                
                // Forward FFT, store in spectrogram buffer
                fft.forward(forwardFrame.data(), spectrogram[spectPos].bins.data());

                // Process frame if applicable
                if (processCallback) processCallback(spectrogram[spectPos]);
                // Enqueue frame for IFFT 
                processingQueue.push_back(spectPos); 
                
                ++spectPos; if (spectPos >= spectSize) spectPos = 0;
            }
        }

        float inverse() { // Reconstruct signal from spectrogram (ideally sync this with forward() to match inPos/outPos)
            float out = outBuf[outPos];
            outBuf[outPos] = 0.0f;

            /* RECONSTRUCT FFT FRAMES */
            if (!processingQueue.empty()) { // Process queued frames
                size_t framePos = processingQueue.front();
                
                fft.inverse(spectrogram[framePos].bins.data(), inverseFrame.data());
                overlapAdd(outBuf, inverseFrame, HANN, outPos);
                
                processingQueue.pop_front();
            }

            ++outPos; if (outPos >= fftSize) outPos = 0;
            return out;
        }

        FFTFrame interpolateFrame(float framePos) { 
            if (spectrogram.empty()) { return FFTFrame(numBins); }
            
            int frameIndex = (int)floor(framePos);
            float frameFrac = framePos - floor(framePos);
            size_t frame0 = frameIndex % spectSize;
            size_t frame1 = (frame0 + 1) % spectSize;
            
            FFTFrame interpFrame(numBins);
            for (size_t k = 0; k < numBins; ++k) {
                // Lerp real and imaginary components
                interpFrame.bins[k].r = lerp(spectrogram[frame0].bins[k].r, spectrogram[frame1].bins[k].r, frameFrac);
                interpFrame.bins[k].i = lerp(spectrogram[frame0].bins[k].i, spectrogram[frame1].bins[k].i, frameFrac);
            }
            return interpFrame;
        }
};

#endif // UTILITIES