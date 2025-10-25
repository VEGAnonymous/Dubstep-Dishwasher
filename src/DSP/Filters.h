#ifndef FILTERS
#define FILTERS

#include "Modules.h"

#include <memory>
#include <numeric>
#include <string>
#include <vector>

/* FILTERS */

class FIR_Filter : public Effect {
    private:
        enum Params : ParamID { MIX };

        float mix;
        vector<float> h; // Kernel
        vector<float> z; // Circular double buffer (filter state)
        size_t z_i = 0; // State pointer
        
    public:
        FIR_Filter(float mix, vector<float> h) : h(move(h)), z(this->h.size() * 2, 0.0f), z_i(0) { setMix(mix); }

        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        inline void setParam(ParamID param, float value) override { 
            switch (param) {
                case MIX: setMix(value); break;
            }
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
        enum Params : ParamID { CUTOFF = 1, COEFF };

        float b0, a1, y = 0.0f;
        float cutoff;

    public:
        OnePole(float mix, float cutoff) { IIR_Filter::mix = mix; setCutoff(cutoff); }

        void setCoeff(float a) {
            b0 = 1.0f - a; a1 = a;
            cutoff = -((float)SAMPLE_RATE * log(a)) / (2 * M_PI);
        }
        void setCutoff(float cutoff) { // Hz
            this->cutoff = cutoff;
            float x = exp((-2.0f * M_PI * cutoff) / SAMPLE_RATE);
            b0 = 1.0f - x;
            a1 = x;
        }
        void setParam(ParamID param, float value) override { 
            switch (param) {
                case CUTOFF: setCutoff(value); break;
                case COEFF: setCoeff(value); break;
                default: IIR_Filter::setParam(param, value);
            }
        }

        float LCCDE(float x) override { 
            // y[n] = (1-a)x[n] - ay[n-1]
            y = b0 * x + a1 * y;
            return y;
        }
};

class APF : public IIR_Filter { // 1st order, Direct Form I/II
    private:
        enum Params : ParamID { CUTOFF = 1, Q };

        const int maxDelaySamples;
        const bool useDFII;

        float cutoff, g;
        float N;
        bool invert;

        unique_ptr<DelayLine> bufferX, bufferY; // DFI
        unique_ptr<DelayLine> buffer; // DFII

    public:
        APF(float mix, float cutoff, float q, bool invert, float maxDelayTime, bool useDFII = false)
            : maxDelaySamples(maxDelayTime * SAMPLE_RATE / 1000.0f), useDFII(useDFII) {

            // Allocate buffers
            if (useDFII) {
                buffer = make_unique<DelayLine>(500.0f / cutoff, maxDelayTime);
            } else {
                bufferX = make_unique<DelayLine>(500.0f / cutoff, maxDelayTime);
                bufferY = make_unique<DelayLine>(500.0f / cutoff, maxDelayTime);
            }

            IIR_Filter::mix = mix; setCutoff(cutoff); setQ(q); setInvert(invert);
        }

        float readTap(float offset) { // Tap the output delay line
            if (useDFII) return 0.0f;
            return bufferY->read(offset);
        } 
        void setInvert(bool invert) { this->invert = invert; }
        void setDelay(float N) { 
            this->N = N; 
            cutoff = SAMPLE_RATE / (2.0f * (float)N);
            if (useDFII) { buffer->setDelaySamples(N);
            } else { bufferX->setDelaySamples(N); bufferY->setDelaySamples(N); }
        }
        void setCutoff(float cutoff) { // Hz
            N = clamp(SAMPLE_RATE / (2.0f * cutoff), 1.0f, (float)maxDelaySamples); // Cutoff translates to delay N
            this->cutoff = cutoff;
            if (useDFII) { buffer->setDelaySamples(N);
            } else { bufferX->setDelaySamples(N); bufferY->setDelaySamples(N); }
        }
        void setQ(float q) { g = clamp(1.0f - (1.0f / q), -0.999f, 0.999f); } // Q translates to coefficient g
        inline void setParam(ParamID param, float value) override { 
            switch (param) {
                case CUTOFF: setCutoff(value); break;
                case Q: setQ(value); break;
                default: IIR_Filter::setParam(param, value);
            }
        }

        float LCCDE(float x) override {
            auto s = invert ? -1 : 1; // Invert sign as needed
            if (!useDFII) { // Direct Form I
                // y[n] = -gy[n-N] + gx[n] + x[n-N]
                float y = (s * -g * bufferY->read()) + (s * g * x) + bufferX->read();
                bufferX->write(x); bufferY->write(y);
                return y;
            } else { // Direct Form II
                // v[n] = (1-g^2)x[n] - gw[n-N]
                // y[n] = gx[n] + w[n-N]
                float v_D = buffer->read();
                float y = (s * g * x) + v_D;
                float v = ((1 - g*g) * x) - (s * g * v_D);
                buffer->write(v);
                return y;
            }
        }
};

#endif // FILTERS