#ifndef FILTERS
#define FILTERS

#include "Modules.h"

#include <memory>
#include <numeric>
#include <string>
#include <vector>

/* FILTERS */

/* FIR */

class FIR_Filter : public Effect {
    private:
        enum Params : ParamID { MIX };

        float mix;
        std::vector<float> h; // Kernel
        std::vector<float> z; // Circular double buffer (filter state)
        size_t z_i = 0; // State pointer
        
    public:
        FIR_Filter(float mix, std::vector<float> h) : h(move(h)), z(this->h.size() * 2, 0.0f), z_i(0) { setMix(mix); }

        void setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
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

/* 1st-order IIR */

class OnePole : public IIR_Filter { // One-pole LPF
    private:
        enum Params : ParamID { CUTOFF = 1, COEFF };

        float b0, a1, y = 0.0f;
        float cutoff;

    public:
        OnePole(float mix = 1.0f, float cutoff = SAMPLE_RATE / 2.0f) { IIR_Filter::mix = mix; setCutoff(cutoff); }

        void setCoeff(float a) {
            b0 = 1.0f - a; a1 = a;
            cutoff = -(SAMPLE_RATE * log(a)) / (2 * M_PI);
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

        const size_t maxDelaySamples;
        const bool useDFII;
        const bool usePSRAM;

        float cutoff, g;
        size_t N;
        bool invert;

        std::unique_ptr<DelayLineVector> bufferXv, bufferYv; // DFI vector
        std::unique_ptr<DelayLineVector> bufferv; // DFII vector
        std::unique_ptr<DelayLine> bufferX, bufferY; // DFI PSRAM
        std::unique_ptr<DelayLine> buffer; // DFII PSRAM

        float g_s, g_sq; // Cached

        void updateSign() {
            auto s = invert ? -1 : 1; // Invert sign as needed
            g_s = s * g;
        }
    public:
        APF(float mix = 1.0f, float cutoff = 1000.0f, float q = 0.8f, bool invert = false, 
            float maxDelayTime = 1.0f, bool useDFII = true, bool usePSRAM = true)
            : maxDelaySamples((size_t)(maxDelayTime * SAMPLE_RATE / 1000.0f)), useDFII(useDFII), usePSRAM(usePSRAM) {

            // Allocate buffers
            if (useDFII) {
                if (usePSRAM) buffer = std::make_unique<DelayLine>(500.0f / cutoff, maxDelayTime);
                else bufferv = std::make_unique<DelayLineVector>(500.0f / cutoff, maxDelayTime);
            } else {
                if (usePSRAM) { bufferX = std::make_unique<DelayLine>(500.0f / cutoff, maxDelayTime); bufferY = std::make_unique<DelayLine>(500.0f / cutoff, maxDelayTime); }
                else { bufferXv = std::make_unique<DelayLineVector>(500.0f / cutoff, maxDelayTime); bufferYv = std::make_unique<DelayLineVector>(500.0f / cutoff, maxDelayTime); }
            }

            IIR_Filter::mix = mix; setCutoff(cutoff); setQ(q); setInvert(invert);
        }

        float readTap(float offset) { // Tap the output delay line
            if (useDFII) return 0.0f;
            if (usePSRAM) return bufferY->read(offset);
            return bufferYv->read(offset);
        } 
        void setInvert(bool invert) { this->invert = invert; updateSign(); }
        void setDelay(size_t N) { 
            this->N = std::clamp(N, (size_t)1, maxDelaySamples); 
            cutoff = SAMPLE_RATE / (2.0f * (float)N);
            if (useDFII) { if (usePSRAM) buffer->setDelaySamples(N); else bufferv->setDelaySamples(N);
            } else { if (usePSRAM) { bufferX->setDelaySamples(N); bufferY->setDelaySamples(N); } else { bufferXv->setDelaySamples(N); bufferYv->setDelaySamples(N); } }
        }
        void setCutoff(float cutoff) { // Hz
            N = std::clamp(SAMPLE_RATE / (2.0f * cutoff), 1.0f, (float)maxDelaySamples); // Cutoff translates to delay N
            this->cutoff = cutoff;
            if (useDFII) { if (usePSRAM) buffer->setDelaySamples(N); else bufferv->setDelaySamples(N);
            } else { if (usePSRAM) { bufferX->setDelaySamples(N); bufferY->setDelaySamples(N); } else { bufferXv->setDelaySamples(N); bufferYv->setDelaySamples(N); } }
        }
        void setQ(float q) { // Q translates to coefficient g
            g = std::clamp(1.0f - (1.0f / q), -0.999f, 0.999f);
            g_sq = 1.0f - g*g;
            updateSign();
        } 
        inline void setParam(ParamID param, float value) override { 
            switch (param) {
                case CUTOFF: setCutoff(value); break;
                case Q: setQ(value); break;
                default: IIR_Filter::setParam(param, value);
            }
        }

        inline float LCCDE(float x) override {
            if (!useDFII) { // Direct Form I
                // y[n] = -gy[n-N] + gx[n] + x[n-N]
                float y = 0.0f;
                if (usePSRAM) { y = (-g_s * bufferY->read()) + (g_s * x) + bufferX->read(); bufferX->write(x); bufferY->write(y); }
                else { y = (-g_s * bufferYv->read()) + (g_s * x) + bufferXv->read(); bufferXv->write(x); bufferYv->write(y); }
                return y;
            } else { // Direct Form II
                // v[n] = (1-g^2)x[n] - gw[n-N]
                // y[n] = gx[n] + w[n-N]
                float v_D = usePSRAM ? buffer->read() : bufferv->read();
                float y = (g_s * x) + v_D;
                float v = (g_sq * x) - (g_s * v_D);
                if (usePSRAM) buffer->write(v); else bufferv->write(v);
                return y;
            }
        }
};

/* Biquads */
// https://www.w3.org/TR/audio-eq-cookbook/ <-- Godsend of DSP

class LPF_Biquad : public Biquad {
    private:
        void updateCoeffs() override {
            const float w0 = 2.0f * static_cast<float>(M_PI) * (cutoff / SAMPLE_RATE);
            const float cos_w0 = arm_cos_f32(w0), sin_w0 = arm_sin_f32(w0);
            const float a = sin_w0 / (2.0f * q);

            const float a0 = 1.0f + a;
            b0 = ((1.0f - cos_w0) / 2.0f) / a0; b1 = (1.0f - cos_w0) / a0; b2 = b0;
            a1 = (-2.0f * cos_w0) / a0; a2 = (1.0f - a) / a0;
        }
    public:
        LPF_Biquad(float cutoff = 1000.0f, float q = 0.707f, float gainDB = 0.0f) {
            this->cutoff = cutoff; this->q = q; this->gainDB = gainDB;
            updateCoeffs(); }
};

class HPF_Biquad : public Biquad {
    private:
        void updateCoeffs() override {
            const float w0 = 2.0f * static_cast<float>(M_PI) * (cutoff / SAMPLE_RATE);
            const float cos_w0 = arm_cos_f32(w0), sin_w0 = arm_sin_f32(w0);
            const float a = sin_w0 / (2.0f * q);

            const float a0 = 1.0f + a;
            b0 = ((1.0f + cos_w0) / 2.0f) / a0; b1 = -(1.0f + cos_w0) / a0; b2 = b0;
            a1 = (-2.0f * cos_w0) / a0; a2 = (1.0f - a) / a0;
        }
    public:
        HPF_Biquad(float cutoff = 1000.0f, float q = 0.707f, float gainDB = 0.0f) {
            this->cutoff = cutoff; this->q = q; this->gainDB = gainDB;
            updateCoeffs(); }
};

class BPF_Biquad : public Biquad {
    private:
        void updateCoeffs() override {
            const float w0 = 2.0f * static_cast<float>(M_PI) * (cutoff / SAMPLE_RATE);
            const float cos_w0 = cosf(w0), sin_w0 = sinf(w0);
            const float a = sin_w0 / (2.0f * q);

            const float a0 = 1.0f + a;
            b0 = (q * a) / a0; b1 = 0.0f; b2 = -b0;
            a1 = (-2.0f * cos_w0) / a0; a2 = (1.0f - a) / a0;
        }
    public:
        BPF_Biquad(float cutoff = 1000.0f, float q = 0.707f, float gainDB = 0.0f) {
            this->cutoff = cutoff; this->q = q; this->gainDB = gainDB;
            updateCoeffs(); }
};

class LowShelf_Biquad : public Biquad {
    private:
        void updateCoeffs() override {
            const float w0 = 2.0f * static_cast<float>(M_PI) * (cutoff / SAMPLE_RATE);
            const float cos_w0 = arm_cos_f32(w0), sin_w0 = arm_sin_f32(w0);
            const float A = exp10f(gainDB / 40.0f);
            float sqrt_A; arm_sqrt_f32(A, &sqrt_A);
            const float a = sin_w0 / (2.0f * q);

            const float a0 = (A + 1.0f) + ((A - 1.0f) * cos_w0) + (2.0f * sqrt_A * a);
            b0 = (A * ((A + 1.0f) - ((A - 1.0f) * cos_w0) + (2.0f * sqrt_A * a))) / a0;
            b1 = ((2.0f * A) * ((A - 1.0f) - ((A + 1.0f) * cos_w0))) / a0;
            b2 = (A * ((A + 1.0f) - ((A - 1.0f) * cos_w0) - (2.0f * sqrt_A * a))) / a0; 
            a1 = (-2.0f * ((A - 1.0f) + ((A + 1.0f) * cos_w0))) / a0; 
            a2 = ((A + 1.0f) + ((A - 1.0f) * cos_w0) - (2.0f * sqrt_A * a)) / a0;
        }
    public:
        void setGain(float gainDB) override { // dB, [-24.0, 24.0]
            setBypass(fabs(gainDB) < 1e-3f); // Bypass if gain is close or at 0.0dB
            this->gainDB = std::clamp(gainDB, -24.0f, 24.0f); 
            if (!isBypassed()) updateCoeffs();
        }

        LowShelf_Biquad(float cutoff = 1000.0f, float q = 0.707f, float gainDB = 0.0f) {
            this->cutoff = cutoff; this->q = q; this->gainDB = gainDB;
            updateCoeffs(); }
};

class HighShelf_Biquad : public Biquad {
    private:
        void updateCoeffs() override {
            const float w0 = 2.0f * static_cast<float>(M_PI) * (cutoff / SAMPLE_RATE);
            const float cos_w0 = arm_cos_f32(w0), sin_w0 = arm_sin_f32(w0);
            const float A = exp10f(gainDB / 40.0f);
            float sqrt_A; arm_sqrt_f32(A, &sqrt_A);
            const float a = sin_w0 / (2.0f * q);

            const float a0 = (A + 1.0f) - ((A - 1.0f) * cos_w0) + (2.0f * sqrt_A * a);
            b0 = (A * ((A + 1.0f) + ((A - 1.0f) * cos_w0) + (2.0f * sqrt_A * a))) / a0;
            b1 = ((-2.0f * A) * ((A - 1.0f) + ((A + 1.0f) * cos_w0))) / a0;
            b2 = (A * ((A + 1.0f) + ((A - 1.0f) * cos_w0) - (2.0f * sqrt_A * a))) / a0; 
            a1 = (2.0f * ((A - 1.0f) - ((A + 1.0f) * cos_w0))) / a0; 
            a2 = ((A + 1.0f) - ((A - 1.0f) * cos_w0) - (2.0f * sqrt_A * a)) / a0;
        }
    public:
        void setGain(float gainDB) { // dB, [-24.0, 24.0]
            setBypass(fabs(gainDB) < 1e-3f); // Bypass if gain is close or at 0.0dB
            this->gainDB = std::clamp(gainDB, -24.0f, 24.0f); 
            if (!isBypassed()) updateCoeffs();
        }

        HighShelf_Biquad(float cutoff = 1000.0f, float q = 0.707f, float gainDB = 0.0f) {
            this->cutoff = cutoff; this->q = q; this->gainDB = gainDB;
            updateCoeffs(); }
};

class Peak_Biquad : public Biquad {
    private:
        void updateCoeffs() override {
            const float w0 = 2.0f * static_cast<float>(M_PI) * (cutoff / SAMPLE_RATE);
            const float cos_w0 = arm_cos_f32(w0), sin_w0 = arm_sin_f32(w0);
            const float A = exp10f(gainDB / 40.0f);
            const float a = sin_w0 / (2.0f * q);

            const float a0 = 1.0f + (a / A);
            b0 = (1.0f + (a * A)) / a0; b1 = (-2.0f * cos_w0) / a0; b2 = (1.0f - (a * A)) / a0; 
            a1 = b1; a2 = (1.0f - (a / A)) / a0;
        }
    public:
        void setGain(float gainDB) { // dB, [-24.0, 24.0]
            setBypass(fabs(gainDB) < 1e-3f); // Bypass if gain is close or at 0.0dB
            this->gainDB = std::clamp(gainDB, -24.0f, 24.0f); 
            if (!isBypassed()) updateCoeffs();
        }

        Peak_Biquad(float cutoff = 1000.0f, float q = 0.707f, float gainDB = 0.0f) {
            this->cutoff = cutoff; this->q = q; this->gainDB = gainDB;
            updateCoeffs(); }
};

class Notch_Biquad : public Biquad {
    private:
        void updateCoeffs() override {
            const float w0 = 2.0f * static_cast<float>(M_PI) * (cutoff / SAMPLE_RATE);
            const float cos_w0 = arm_cos_f32(w0), sin_w0 = arm_sin_f32(w0);
            const float a = sin_w0 / (2.0f * q);

            const float a0 = 1.0f + a;
            b0 = 1.0f / a0; b1 = (-2.0f * cos_w0) / a0; b2 = 1.0f / a0; 
            a1 = b1; a2 = (1.0f - a) / a0;
        }
    public:
        Notch_Biquad(float cutoff = 1000.0f, float q = 0.707f, float gainDB = 0.0f) { 
            this->cutoff = cutoff; this->q = q; this->gainDB = gainDB;
            updateCoeffs(); }
};

#endif // FILTERS