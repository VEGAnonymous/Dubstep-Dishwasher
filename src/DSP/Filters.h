#ifndef FILTERS
#define FILTERS

#include "Modules.h"

#include <numeric>
#include <string>
#include <vector>

/* FILTERS */

class FIR_Filter : public Effect {
    private:
        float mix;
        vector<float> h; // Kernel
        vector<float> z; // Circular double buffer (filter state)
        size_t z_i = 0; // State pointer
    public:
        FIR_Filter(float mix, vector<float> h) : h(move(h)), z(this->h.size() * 2, 0.0f), z_i(0) { setMix(mix); }

        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); };
        inline void setParam(const string& name, float value) override { 
            if (name == "Mix") { setMix(value); }
        }

        void process(const float* in, float* out, size_t n) override { // Filter via convolution
            // Stolen from https://ccrma.stanford.edu/~jatin/Notebooks/FIRBenchmarks.html
            const float* in_ptr = in;
            float* out_ptr = out;

            const size_t M = h.size();
            for (size_t i = 0; i < n; ++i) {
                z[z_i] = *in_ptr; z[z_i + M] = *in_ptr; // Write input in double-buffered state

                // Compute inner product over kernel and double-buffer state
                float wetSig = inner_product(z.begin() + z_i, z.begin() + z_i + M, h.begin(), 0.0f);

                z_i = (z_i == 0 ? h.size() - 1 : z_i - 1); // Iterate state pointer in reverse

                *out_ptr++ = ((1.0f - mix) * *in_ptr++) + (mix * wetSig); // Mix
            }
        }
};

class OnePole : public IIR_Filter { // One pole
    private:
        float b0, a1, y = 0.0f;
        float cutoff;
    public:
        OnePole(float mix, float cutoff) { IIR_Filter::mix = mix; setCutoff(cutoff); }

        void setCoeff(float a) {
            b0 = 1.0f - a; a1 = a;
            cutoff = -((float)SAMPLE_RATE * log(a)) / (2 * M_PI);
        }
        void setCutoff(float cutoff) {
            this->cutoff = cutoff;
            float x = exp((-2.0f * M_PI * cutoff) / SAMPLE_RATE);
            b0 = 1.0f - x;
            a1 = x;
        }
        void setParam(const string& name, float value) override { 
            if (name == "Cutoff") { setCutoff(value); }
            else if (name == "Coefficient") { setCoeff(value); } 
            else { IIR_Filter::setParam(name, value); }
        }

        float LCCDE(float x) override { 
            // y[n] = (1-a)x[n] - ay[n-1]
            y = b0 * x + a1 * y;
            return y;
        }
};

class APF : public IIR_Filter { // 1st order
    private:
        const int maxDelaySamples;
        float cutoff, g;
        float N;
        bool invert;

        DelayLine bufferX, bufferY;
    public:
        APF(float mix, float cutoff, float q, bool invert, float maxDelayTime)
            : maxDelaySamples(maxDelayTime * SAMPLE_RATE / 1000.0f),
            bufferX((SAMPLE_RATE / (2.0f * cutoff)) * 1000.0f / SAMPLE_RATE, maxDelayTime), 
            bufferY((SAMPLE_RATE / (2.0f * cutoff)) * 1000.0f / SAMPLE_RATE, maxDelayTime) { 
            IIR_Filter::mix = mix; setCutoff(cutoff); setQ(q); setInvert(invert); }

        float readTap(float offset) { return bufferY.read(offset); } // Tap the output delay line

        void setInvert(bool invert) { this->invert = invert; }
        void setDelay(float N) { 
            this->N = N; 
            cutoff = SAMPLE_RATE / (2.0f * (float)N);
            bufferX.setDelaySamples(N); bufferY.setDelaySamples(N);
        }
        void setCutoff(float cutoff) {
            N = clamp(SAMPLE_RATE / (2.0f * cutoff), 1.0f, (float)maxDelaySamples); // Cutoff translates to delay N
            this->cutoff = cutoff;
            bufferX.setDelaySamples(N); bufferY.setDelaySamples(N);
        }
        void setQ(float q) { g = clamp(1.0f - (1.0f / q), -0.999f, 0.999f); } // Q translates to coefficient g
        inline void setParam(const string& name, float value) override { 
            if (name == "Cutoff") { setCutoff(value); }
            else if (name == "Q") { setQ(value); }
            else { IIR_Filter::setParam(name, value); }
        }

        float LCCDE(float x) override {
            // y[n] = -gy[n-N] + gx[n] + x[n-N]
            auto s = invert ? -1 : 1; // Invert sign as needed
            float y = (s * -g * bufferY.read()) + (s * g * x) + bufferX.read();
            bufferX.write(x); bufferY.write(y);
            return y;
        }
};

#endif // FILTERS