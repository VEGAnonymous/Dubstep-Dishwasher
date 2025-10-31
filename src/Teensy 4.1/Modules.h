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
        virtual float LCCDE(float x) = 0; // LCCDE to implement; can also call function pointer (e.g., selectable filter order)

    public:
        void setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); }; // [0.0, 1.0]
        inline void setParam(ParamID param, float value) override {
            switch (param) {
                case MIX: setMix(value); break;
                // Subclasses can call IIR_Filter::setParam(param, value)
            }
        }
        
        float processSample(float x) { return LCCDE(x); }
        void process(const float* in, float* out, size_t n) override {
            for (size_t i = 0; i < n; ++i) {
                out[i] = dryWetMix(in[i], LCCDE(in[i]), mix);
            }
        }
        
};

class Biquad : public IIR_Filter { // Generic SOS form, Direct Form II-Transpose
    protected:
        enum Params : ParamID { CUTOFF = 1, Q, GAIN };

        float cutoff = 1000.0f, q = 0.707f, gainDB = 0.0f;

        // Biquad coefficients
        float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;

        // Filter state
        float z1 = 0.0f, z2 = 0.0f;

        float LCCDE(float x) override {
            // w[n] = x[n] - a1z1 - a2z2
            // y[n] = b0w[n] + b1z1 + b2z2
            // z2 = z1, z1 = w[n]
            float w = x - (a1 * z1) - (a2 * z2);
            float y = (b0 * w) + (b1 * z1) + (b2 * z2);
            z2 = z1; z1 = w;
            return y;
        }

        virtual void updateCoeffs() = 0;

    public:
        void setCutoff(float cutoff) { this->cutoff = std::clamp(cutoff, 20.0f, 20000.0f); updateCoeffs(); } // Hz, [20.0, 20000.0]
        void setQ(float q) { this->q = std::clamp(q, 0.025f, 40.0f); updateCoeffs(); } // [0.025, 40.0]
        virtual void setGain(float gainDB) { this->gainDB = std::clamp(gainDB, -24.0f, 24.0f); updateCoeffs(); } // dB, [-24.0, 24.0]
        
        inline void setParam(ParamID param, float value) override {
            switch (param) {
                case CUTOFF: setCutoff(value); break;
                case Q: setQ(value); break;
                case GAIN: setGain(value); break;
                default: IIR_Filter::setParam(param, value);
            }
        }
};

class Spectral_Effect : public Effect {
    protected:
        enum Params : ParamID { MIX, FFT_SIZE };

        float mix; size_t fftSize;

        STFT stft;
        std::vector<float> mag, phs; // Temp buffers
        size_t hopCounter = 0;

        DelayLine latencyComp;

        virtual void processSpectrum(STFT::FFTFrame& frame) = 0; // Subclasses must implement

    public:
        Spectral_Effect(float mix = 1.0f, size_t fftSize = 512, size_t hopFactor = 4) 
        : stft(fftSize, hopFactor, ((FFT_MAX_SIZE / (float)hopFactor) + 1.0f) / SAMPLE_RATE), 
          latencyComp(1.0f, ((FFT_MAX_SIZE + 1.0f) * 1000.0f) / SAMPLE_RATE) { 
            setMix(mix); setFFTSize(fftSize);
            // Set the STFT frame process callback to processSpectrum()
            stft.setProcessCallback([this](STFT::FFTFrame& frame) { processSpectrum(frame);
    });
        
        }
        virtual ~Spectral_Effect() = default;

        void setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setFFTSize(size_t N) { // [128, FFT_MAX_SIZE], MUST BE POWER OF 2
            const size_t fftN = std::clamp(N, (size_t)128, (size_t)FFT_MAX_SIZE);
            fftSize = fftN;
            
            stft.setFFTSize(fftSize);
            mag.assign((fftSize / 2) + 1, 0.0f); phs.assign((fftSize / 2) + 1, 0.0f);
            hopCounter = 0;
            latencyComp.setDelaySamples((float)fftSize);
        }

        inline void setParam(ParamID param, float value) override {
            switch (param) {
                case MIX: setMix(value); break;
                case FFT_SIZE: setFFTSize((size_t)value); break;
            }
        }

        void process(const float* in, float* out, size_t n) override {
            for (size_t i = 0; i < n; ++i) {
                latencyComp.write(in[i]);
                float drySig = latencyComp.read();
                
                stft.forward(in[i]);
                float wetSig = stft.inverse();
                
                out[i] = dryWetMix(drySig, wetSig, mix);
            }
        }
};

#endif // MODULES