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
        EffectID id;

    public:
        virtual ~Effect() = default;
        
        void setBypass(bool state) { bypass = state; }
        bool isBypassed() const { return bypass; }
        void setID(EffectID id) { this->id = id; }
        EffectID getID() const { return id; }

        inline virtual void setParam(ParamID param, float value) = 0; // Allows setting subclass parameters from an Effect pointer

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
        enum Params : ParamID { MIX };
        float mix;
        virtual float LCCDE(float in) = 0; // LCCDE to implement; can also call function pointer (e.g., selectable filter order)

    public:
        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); }; // [0.0, 1.0]
        inline void setParam(ParamID param, float value) override {
            switch (param) {
                case MIX: setMix(value); break;
                // Subclasses can call IIR_Filter::setParam(param, value)
            }
        }

        void process(const float* in, float* out, size_t n) override {
            for (size_t i = 0; i < n; ++i) {
                out[i] = dryWetMix(in[i], LCCDE(in[i]), mix);
            }
        }
};

class Spectral_Effect : public Effect {
    protected:
        enum Params : ParamID { MIX, FFT_SIZE };

        float mix;
        size_t fftSize, hopSize;

        STFT stft;
        vector<float> mag, phs; // Temp buffers
        size_t hopCounter = 0;

        DelayLine latencyComp;

        virtual void processSpectrum(float* mag, float* phs, size_t numBins) = 0; // Subclasses must implement

    public:
        Spectral_Effect(float mix = 1.0f, size_t fftSize = 1024) : stft(fftSize, 2), latencyComp(1.0f, (8192.0f * 1000.0f) / (float)SAMPLE_RATE) { 
            setMix(mix); setFFTSize(fftSize); }
        virtual ~Spectral_Effect() = default;

        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setFFTSize(size_t N) { // [256, 8192], MUST BE POWER OF 2
            const size_t fftN = clamp(N, (size_t)256, (size_t)8192);
            fftSize = fftN; hopSize = fftN / 4;
            
            stft.setFFTSize(fftSize);
            mag.assign((fftSize / 2) + 1, 0.0f); phs.assign((fftSize / 2) + 1, 0.0f);
            hopCounter = 0;
            latencyComp.setDelaySamples((float)hopSize + ((float)fftSize / 2.0f));
        }

        inline void setParam(ParamID param, float value) override {
            switch (param) {
                case MIX: setMix(value); break;
                case FFT_SIZE: setFFTSize((size_t)value); break;
            }
        }

        void process(const float* in, float* out, size_t n) override {
            const size_t hopN = hopSize;
            for (size_t i = 0; i < n; ++i) {
                latencyComp.write(in[i]);
                float drySig = latencyComp.read();

                stft.forward(in[i]); // Forward FFT

                ++hopCounter;
                /* SPECTRAL PROCESSING */
                if (hopCounter >= hopN) { // Every hopN samples
                    hopCounter = 0;

                    // Get latest FFT frame
                    STFT::FFTFrame& frame = stft.getFrame();
                    size_t numBins = frame.mag.size();

                    // Read to temp buffers
                    copy(frame.mag.begin(), frame.mag.end(), mag.begin());
                    copy(frame.phase.begin(), frame.phase.end(), phs.begin());

                    // Process spectrum
                    processSpectrum(mag.data(), phs.data(), numBins);

                    // Overwrite frame with processed data
                    copy(mag.begin(), mag.end(), frame.mag.begin());
                    copy(phs.begin(), phs.end(), frame.phase.begin());
                }

                float wetSig = stft.inverse(); // IFFT
                out[i] = dryWetMix(drySig, wetSig, mix); // Mix
            }
        }
};

#endif // MODULES