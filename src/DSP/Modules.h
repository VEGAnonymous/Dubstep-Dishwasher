#ifndef MODULES
#define MODULES

#include "Utilities.h"

#include <algorithm>
#include <cstddef>
#include <string>

/* MODULES (ABCs) */

class Effect {
    protected:
        bool bypass = false;
    public:
        void setBypass(bool state) { bypass = state; }
        bool isBypassed() const { return bypass; }

        inline virtual void setParam(const string& name, float value) {} // Allows setting subclass parameters from an Effect pointer

        virtual void process(const float* in, float* out, size_t n) = 0; // Process sample block, implemented per effect
};

class Generator {
    public:
        virtual float next() = 0; // Generate next sample
        virtual void generate(float* out, size_t n) { // Generate block of samples
            for (size_t i = 0; i < n; ++i) {
                out[i] = next();
        }
    }
};

class IIR_Filter : public Effect {
    protected:
        float mix;
        virtual float LCCDE(float in) = 0; // LCCDE to implement; can also call function pointer (e.g., selectable filter order)
    public:
        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); };
        inline void setParam(const string& name, float value) override {
            if (name == "Mix") { setMix(value); }
            // Subclasses can call IIR_Filter::setParam(name, value)
        }

        void process(const float* in, float* out, size_t n) override {
            for (size_t i = 0; i < n; ++i) {
                out[i] = (mix * LCCDE(in[i])) + ((1.0f - mix) * in[i]);
            }
        }
};

class Spectral_Effect : public Effect {
    protected:
        float mix;
        size_t fftSize, hopSize;

        vector<float> inBuf, outBuf, window;
        size_t writePos = 0, frameCount = 0;

        FFT fft;
        vector<float> mag, phs;
        
        void makeWindow(size_t N) {
            window.resize(N);
            for (size_t n = 0; n < N; ++n) {
                window[n] = 0.5f * (1.0f - cos((2.0f * M_PI * n) / (N - 1))); // Hann window
            }
        }
        
        virtual void processSpectrum(float* mag, float* phs, size_t numBins) = 0; // Process FFT frame, implemented per effect
    
    public:
        Spectral_Effect(float mix, int fftSize) : fft(fftSize) {
            setMix(mix); setFFTSize(fftSize); }
        virtual ~Spectral_Effect() = default;
        
        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); }
        void setFFTSize(int N) { // MUST be power of 2!
            fftSize = N; hopSize = fftSize / 2; // 50% overlap
            
            inBuf.resize(fftSize, 0.0f); outBuf.resize(fftSize, 0.0f);
            mag.resize(fftSize / 2 + 1, 0.0f); phs.resize(fftSize / 2 + 1, 0.0f);
            
            makeWindow(fftSize);
            fft.setFFTSize(fftSize);
            
            writePos = 0; frameCount = 0;
        }
        inline void setParam(const string& name, float value) override {
            if (name == "Mix") { setMix(value); }
            if (name == "FFT Size") { setFFTSize(static_cast<int>(value)); }
        }
        
        void process(const float* in, float* out, size_t n) override {
            const size_t fftN = fftSize, hopN = hopSize; 
            for (size_t i = 0; i < n; ++i) {
                inBuf[writePos] = in[i]; // Write input to circular buffer
                
                // Output from overlap-add buffer
                float wetSig = outBuf[writePos];
                outBuf[writePos] = 0.0f;
                out[i] = ((1.0f - mix) * in[i]) + (mix * wetSig); // Mix
                writePos++; frameCount++;
                
                /* PROCESS FFT FRAMES */
                if (frameCount >= hopN) { // Every hopN samples
                    frameCount = 0;

                    // Extract full FFT frame from circular buffer
                    vector<float> frame(fftN);
                    size_t readPos = writePos; // Read oldest data first
                    for (size_t j = 0; j < fftN; ++j) {
                        frame[j] = inBuf[readPos] * window[j];
                        readPos++; if (readPos >= fftN) readPos = 0;
                    }
                    
                    // Forward FFT
                    fft.forward(frame.data(), mag.data(), phs.data());   

                    // Spectrally process the frame
                    processSpectrum(mag.data(), phs.data(), fftSize / 2 + 1);

                    // Inverse FFT
                    fft.inverse(mag.data(), phs.data(), frame.data()); 
                    
                    // Overlap-add
                    size_t olaPos = writePos; // hopSize behind read
                    for (size_t j = 0; j < fftN; ++j) {
                        outBuf[olaPos] += frame[j] * window[j];
                        olaPos++; if (olaPos >= fftN) olaPos = 0;
                    }
                } 

                if (writePos >= fftN) writePos = 0;
            }
        }
};

#endif // MODULES