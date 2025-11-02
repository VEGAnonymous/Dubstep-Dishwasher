#ifndef EFFECTS
#define EFFECTS

#include "Utilities.h"
#include "Modules.h"
#include "Generators.h"
#include "Filters.h"

#include <cmath>
#include <string>
#include <vector>

/* EFFECTS */

class Modulation : public Effect {
    private:
        enum Params : ParamID { MIX, MODE, MODULATOR, FREQ, DEPTH };

        float mix, freq, depth;
        ModulationEffectMode mode; 
        Wavetable modulator;

    public:
        Modulation(float mix = 1.0f, ModulationEffectMode mode = ModulationEffectMode::AM, WavetableType modulatorType = WavetableType::SINE, 
                   float freq = 5.0f, float depth = 0.5f) : modulator(freq, modulatorType) {
            setMix(mix); setMode(mode); setModulator(modulatorType); setFreq(freq); setDepth(depth);
        }

        void setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setMode(ModulationEffectMode mode) { this->mode = mode; }
        void setModulator(WavetableType modulatorType) { modulator.setTable(modulatorType); }
        void setFreq(float freq) { this->freq = std::clamp(freq, 1.0f, 2000.0f); modulator.setFreq(this->freq); } // [1.0, 2000.0]
        void setDepth(float depth) { this->depth = std::clamp(depth, 0.0f, 1.0f); } // [0.0, 1.0]
        inline void setParam(ParamID param, float value) override {
            switch (param) {
                case MIX: setMix(value); break;
                case MODE: setMode(static_cast<ModulationEffectMode>(value)); break;
                case MODULATOR: setModulator(static_cast<WavetableType>(value)); break;
                case FREQ: setFreq(value); break;
                case DEPTH: setDepth(value); break;
            }
        }

        void process(const float* in, float* out, size_t n) override {
            for (size_t i = 0; i < n; ++i) {
                const float modNext = modulator.next();
                switch (mode) {
                    case ModulationEffectMode::AM: out[i] = lerp(in[i], in[i] * (1.0f + (modNext * depth)) * 0.5f, mix); break;
                    case ModulationEffectMode::RM: out[i] = lerp(in[i], in[i] * modNext, mix); break;
                    default: out[i] = in[i];
                }
            }
        };
};

class Wah : public Effect {
    private:
        enum Params : ParamID { MIX, MIN_FREQ, MAX_FREQ, Q };

        float mix, minFreq, maxFreq, q;

        const float L = 50.0f, L_samples = L * SAMPLE_RATE / 1000.0f; // RMS window size
        float rms = 1e-6f; DelayLine inBuffer;
        BPF_Biquad bpf;

    public:
        Wah(float mix = 1.0f, float minFreq = 350.0f, float maxFreq = 2500.0f, float q = 1.6f) 
        : inBuffer(L, L + 1.0f), bpf(1000.0f, 1.6f, 0.0f) { 
            setMix(mix); setMinFreq(minFreq); setMaxFreq(maxFreq); setQ(q);
        }

        void setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setMinFreq(float minFreq) { this->minFreq = std::clamp(minFreq, 20.0f, 1000.0f); } // Hz, [20.0, 1000.0]
        void setMaxFreq(float maxFreq) { this->maxFreq = std::clamp(maxFreq, 1000.0f, 8000.0f); } // Hz, [1000.0, 8000.0]
        void setQ(float q) { this->q = std::clamp(q, 0.3f, 6.0f); bpf.setQ(this->q); } // [0.3, 6.0]
        inline void setParam(ParamID param, float value) override { 
            switch (param) {
                case MIX: setMix(value); break;
                case MIN_FREQ: setMinFreq(value); break;
                case MAX_FREQ: setMaxFreq(value); break;
                case Q: setQ(value); break;
            }
        }

        void process(const float* in, float* out, size_t n) override {
            for (size_t i = 0; i < n; ++i) {
                const float x_i = in[i]; const float x_L = inBuffer.read();
                inBuffer.write(x_i);

                // Envelope follower
                // Compute RMS recursively
                rms = sqrtf((rms * rms) + (((x_i * x_i) - (x_L * x_L)) / L_samples));
                float rmsDB = ampDB(std::max(rms, 1e-6f));

                // Map envelope to BPF cutoff
                float envNorm = std::clamp((rmsDB + 60.0f) / 60.0f, 0.0f, 1.0f); // 60->0 dB
                float cutoff = lerp(minFreq, maxFreq, powf(envNorm, 1.5f));
                bpf.setCutoff(cutoff);

                // Process BPF
                float wetSig = bpf.processSample(x_i);
                out[i] = dryWetMix(x_i, wetSig, mix); // Mix
            }
        }
};

class Distortion : public Effect {
    private:
        enum Params : ParamID { MIX, MODE, DRIVE, ENABLE_AAF };

        DistortionMode mode;
        float mix, drive;
        bool enableAAF;

        float (Distortion::*algorithm)(float, float) = nullptr; // Function pointer for distortion algorithm to use
        FIR_Filter antiAlias;

        // Distortion algorithms
        // https://www.desmos.com/calculator/qrqipgp7r4
        float tube(float in, float drive) {
            float x = in * (4.0f + (drive * 6.0f)); // d -> [4, 10]
            return atan(x) * (2.0f / M_PI);
        }
        float softClip(float in, float drive) { 
            float d = (1.3f + (drive * 3.7f)); // d -> [1.3, 5]
            float x;
            if (in < (-1.0f/d)) { x = -2.0f/3.0f; }
            else if (in > (1.0f/d)) { x = 2.0f/3.0f; }
            else { x = in * d; x = x - ((x * x * x) / 3.0f); } // x - (x^3)/3
            return x * 1.5f;
        } 
        float hardClip(float in, float drive) {
            float x = in * (1.0f + (drive * 4.0f)); // d -> [1, 5]
            if (x > 1.0f) { x = 1.0f; }
            else if (x < -1.0f) { x = -1.0f; }
            return x;
        }
        float diode(float in, float drive) {
            float x = in * (2.0f + (drive * 8.0f)); // d -> [2, 10]
            if (x < 0) { x = expf(x) - 1.0f; } // Shockley diode equation, B=1
            else if (x > 0) { x = 1.0 - expf(-x); }
            else { x = 0; }
            return x;
        }
        float bitCrush(float in, float drive) {
            int bitDepth = (int)(2 + ((1.0f - drive) * (1.0f - drive) * 14.0f)); // 16-bit to 2-bit depth
            float levels = (float)(1 << bitDepth); // 2^bits discrete levels
            return round(in * levels) / levels; // Quantize
        }
        float rectify(float in, float drive) {
            float x = in * (1.0f + (drive * 4.0f)); // d -> [1, 5]
            x = abs(x); // Full-wave rectify
            return (x > 1) ? 1.0f : x;
        }
        float saturate(float in, float drive) {
            float x = in * (2.0f + (drive * 8.0f)); // d -> [2, 10]
            return tanhf(x);
        }

    public:
        Distortion(float mix = 1.0f, DistortionMode mode = DistortionMode::TUBE, float drive = 0.25f, bool enableAAF = false) 
        : antiAlias(1.0f, std::vector<float>(std::begin(AAF), std::end(AAF))) { setMix(mix); setMode(mode); setDrive(drive); setAAF(enableAAF); }

        void setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setMode(DistortionMode mode) {
            this->mode = mode;
            switch (mode) {
                case DistortionMode::TUBE: algorithm = &Distortion::tube; break;
                case DistortionMode::SOFT_CLIP: algorithm = &Distortion::softClip; break;
                case DistortionMode::HARD_CLIP: algorithm = &Distortion::hardClip; break;
                case DistortionMode::DIODE: algorithm = &Distortion::diode; break;
                case DistortionMode::BITCRUSH: algorithm = &Distortion::bitCrush; break;
                case DistortionMode::RECTIFY: algorithm = &Distortion::rectify; break;
                case DistortionMode::SATURATE: algorithm = &Distortion::saturate; break;
                default: algorithm = &Distortion::hardClip;
            }
        }
        void setDrive(float drive) { this->drive = std::clamp(drive, 0.0f, 1.0f); } // [0.0, 1.0]
        void setAAF(bool enableAAF) { this->enableAAF = enableAAF; } 
        inline void setParam(ParamID param, float value) override {
            switch (param) {
                case MIX: setMix(value); break;
                case MODE: setMode(static_cast<DistortionMode>(value)); break;
                case DRIVE: setDrive(value); break;
                case ENABLE_AAF: setAAF(value > 0.5f); break;
            }
        }

        void process(const float* in, float* out, size_t n) override {
            const float* in_ptr = in;
            float* out_ptr = out;

            float wetSig;
            for (size_t i = 0; i < n; ++i) {
                wetSig = (this->*algorithm)(*in_ptr, drive) * dbAmp(-0.3f); // Apply non-linearity
                if (enableAAF) antiAlias.process(&wetSig, &wetSig, 1); // Optional AAF (~0.8s processing time)
                *out_ptr++ = dryWetMix(*in_ptr++, wetSig, mix); // Mix
            }
        }
};

class Delay : public Effect {
    private:
        enum Params : ParamID { MIX, DELAY_TIME, FEEDBACK };

        const float maxDelayTime = 500.0f; // ms
        float mix, feedback;
        DelayLine delayLine;

    public:
        Delay(float mix = 0.3f, float delayTime = 200.0f, float feedback = 0.4f) :
        delayLine(delayTime, maxDelayTime) { setMix(mix); setFeedback(feedback); }

        void setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setDelayTime(float delayTime) { delayLine.setDelayTime(std::clamp(delayTime, 1.0f, maxDelayTime)); } // ms, [1.0, 500.0]
        void setFeedback(float feedback) { this->feedback = std::clamp(feedback, -0.95f, 0.95f); } // [-0.95, 0.95]
        inline void setParam(ParamID param, float value) override {
            switch (param) {
                case MIX: setMix(value); break;
                case DELAY_TIME: setDelayTime(value); break;
                case FEEDBACK: setFeedback(value); break;
            }
        }

        void process(const float* in, float* out, size_t n) override {
            const float* in_ptr = in;
            float* out_ptr = out;

            float delaySig;
            for (size_t i = 0; i < n; ++i) {
                delaySig = delayLine.read(); // Read from delay line
                delayLine.write(*in_ptr + (delaySig * feedback)); // Feedback and write new sample
                *out_ptr++ = dryWetMix(*in_ptr++, delaySig, mix); // Mix
            }
        }
};

class Flanger : public Effect {
    private:
        enum Params : ParamID { MIX, RATE, DEPTH, FEEDBACK };

        DelayLine delayLine;
        Wavetable LFO;
        float mix, rate, depth, feedback;
        float wetSig = 0;

    public:
        Flanger(float mix = 1.0f, float rate = 0.08f, float depth = 1.0f, float feedback = 0.5f) 
        : delayLine(15, 30), LFO(rate, WavetableType::SINE) { setMix(mix); setRate(rate); setDepth(depth); setFeedback(feedback); }

        void setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setRate(float rate) { this->rate = std::clamp(rate, 0.0f, 20.0f); LFO.setFreq(rate); } // Hz, [0.0, 20.0]
        void setDepth(float depth) { this->depth = std::clamp(depth, 0.0f, 1.0f); } // [0.0, 1.0]
        void setFeedback(float feedback) { this->feedback = std::clamp(feedback, -0.95f, 0.95f); } // [-0.95, 0.95]
        inline void setParam(ParamID param, float value) override {
            switch (param) {
                case MIX: setMix(value); break;
                case RATE: setRate(value); break;
                case DEPTH: setDepth(value); break;
                case FEEDBACK: setFeedback(value); break;
            }
        }

        void process(const float* in, float* out, size_t n) override {
            const float* in_ptr = in;
            float* out_ptr = out;

            for (size_t i = 0; i < n; ++i) {
                delayLine.setDelayTime(15 + (LFO.next() * 10.0f * depth)); // Modulate delay with LFO, 5-25 ms
                delayLine.write(*in_ptr + (feedback * wetSig));
                
                wetSig = delayLine.read();
                *out_ptr++ = dryWetMix(*in_ptr++, wetSig, mix); // Mix
            }
        }
};

class Phaser : public Effect {
    private:
        enum Params : ParamID { MIX, RATE, CENTER_FREQ, SPREAD, DEPTH, FEEDBACK };

        const uint8_t order = 6;
        const float q = 0.8f;

        float mix, rate, centerFreq, spread, depth, feedback;

        std::vector<APF> apfSections; // APF bank
        std::vector<float> baseFreqs; // Store APF base freqs
        Wavetable LFO;
        float wetSig = 0.0f, stageSig = 0.0f;

    public:
        void setupStages() { // Geometrically separate APF base freqs
            const float R = 1.0f + (spread * 16.0f); // Ratio from max/min base freqs, [1.6, 16.0]
            const float r = pow(R, 1.0f/(order-1.0f));
            baseFreqs.resize(order);
            for (size_t i = 0; i < order; ++i) {
                float freq_i = centerFreq * pow(r, i-((order-1.0f)/2.0f));
                baseFreqs[i] = freq_i;
                apfSections[i].setCutoff(freq_i);
            }
        }
        Phaser(float mix = 1.0f, float rate = 0.08f, float centerFreq = 600.0f, float spread = 1.0f, float depth = 0.5f, float feedback = 0.8f)
        : LFO(rate, WavetableType::SINE) {
            for (size_t i = 0; i < order; ++i) { apfSections.push_back(APF(1.0f, centerFreq, q, false, 20.0f, true)); }
            setupStages();
            setMix(mix); setRate(rate); setCenterFreq(centerFreq); setSpread(spread), setDepth(depth); setFeedback(feedback);
        }

        void setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setRate(float rate) { this->rate = std::clamp(rate, 0.0f, 20.0f); LFO.setFreq(rate); } // Hz, [0.0, 20.0]
        void setCenterFreq(float centerFreq) { this->centerFreq = std::clamp(centerFreq, 50.0f, 8000.0f); setupStages(); } // Hz, // [50.0, 8000.0]
        void setSpread(float spread) { this->spread = std::clamp(spread, 0.1f, 1.0f); setupStages(); } // [0.1, 1.0]
        void setDepth(float depth) { this->depth = std::clamp(depth, 0.0f, 1.0f); } // [0.0, 1.0]
        void setFeedback(float feedback) { this->feedback = std::clamp(feedback, -0.95f, 0.95f); } // [-0.95, 0.95]
        inline void setParam(ParamID param, float value) override { // [0.0, 1.0]
            switch (param) {
                case MIX: setMix(value); break;
                case RATE: setRate(value); break;
                case CENTER_FREQ: setCenterFreq(value); break;
                case SPREAD: setSpread(value); break;
                case DEPTH: setDepth(value); break;
                case FEEDBACK: setFeedback(value); break;
            }
        }

        void process(const float* in, float* out, size_t n) override {
            float inSig, mod;
            for (size_t i = 0; i < n; ++i) {
                mod = exp2f(depth * LFO.next());
                inSig = in[i] + (wetSig * feedback);
                
                float x = inSig, y = 0.0f;
                for (size_t j = 0; j < apfSections.size(); ++j) { // Process APFs serially, output to wetSig
                    apfSections[j].setCutoff(baseFreqs[j] * mod); // Modulate each base freq with LFO
                    y = apfSections[j].processSample(x);
                    x = y;
                } wetSig = y;

                out[i] = dryWetMix(in[i], wetSig, mix); // Mix
            }
        }
};

class Chorus : public Effect {
    private:
        enum Params : ParamID { MIX, RATE, DEPTH, DELAY_TIME, FEEDBACK };

        float mix, rate, depth, delayTime, feedback;
        const uint8_t voiceCount = 4; // 1-5
        struct voice {
            float mix, depth, baseDelay;
            Random mod;
        };

        std::vector<voice> voices;
        DelayLine delayLine;
    public:
        Chorus(float mix = 1.0f, float rate = 0.08f, float depth = 25.0f, float delayTime = 5.0f, float feedback = 0.1f)
        : delayLine(delayTime, 51.0f) {
            for (size_t i = 0; i < voiceCount; ++i) voices.push_back({1.0f / (float)voiceCount, depth, delayTime, Random(rate, RandomMode::PERLIN)});
            setMix(mix); setRate(rate); setDepth(depth); setDelayTime(delayTime); setFeedback(feedback);
        }

        void setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setRate(float rate) { // Hz, [0.0, 20.0]
            this->rate = std::clamp(rate, 0.0f, 20.0f); 
            for (voice& vc : voices) vc.mod.setFreq(rate);
        }
        void setDepth(float depth) { // ms, [0.0, 25.0]
            this->depth = std::clamp(depth, 0.0f, 25.0f);
            for (voice& vc : voices) vc.depth = depth; 
        }
        void setDelayTime(float delayTime) { // ms, [0.0, 20.0]
            this->delayTime = std::clamp(delayTime, 0.0f, 20.0f); 
            for (size_t i = 0; i < voiceCount; ++i) { 
                float detune = ((float)i / voiceCount - 0.5f) * 2.0f;
                voices[i].baseDelay = delayTime * (1.0f + 0.3f * detune);
            };
        }
        void setFeedback(float feedback) { this->feedback = std::clamp(feedback, -0.95f, 0.95f); } // [-0.95, 0.95]
        inline void setParam(ParamID param, float value) override {
            switch (param) {
                case MIX: setMix(value); break;
                case RATE: setRate(value); break;
                case DEPTH: setDepth(value); break;
                case DELAY_TIME: setDelayTime(value); break;
                case FEEDBACK: setFeedback(value); break;
            }
        }

        void process(const float* in, float* out, size_t n) override {
            float sqrt_vc; arm_sqrt_f32((float)voiceCount, &sqrt_vc);
            const float *in_ptr = in;
            float *out_ptr = out;
            for (size_t i = 0; i < n; i++) {
                float wetSig = 0.0f;
                for (voice& vc : voices) {
                    float mod = vc.baseDelay + (vc.depth * vc.mod.next());
                    wetSig += delayLine.read(mod * SAMPLE_RATE / 1000.0f) * vc.mix;
                }
                delayLine.write(*in_ptr + (feedback * wetSig));

                wetSig *= sqrt_vc * 1.2f;
                *out_ptr++ = dryWetMix(*in_ptr++, wetSig, mix); // Mix
            }
        }
};

class Reverb : public Effect { 
    // Datarro reverb algorithm 
    // https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf
    private:
        enum Params : ParamID { MIX, PREDELAY_TIME, DECAY_TIME, MOD_RATE, MOD_DEPTH };

        float mix, predelayTime, decayTime, decayGainL, decayGainR, modRate, modDepth;
        std::vector<APF> diffusers; // 8
        std::vector<OnePole> filters; // 3
        std::vector<DelayLine> delayLines; // 5
        Wavetable LFO;
        float tankInSig, nodeSig, tankSig1 = 0, tankSig2 = 0;

    public:
        Reverb(float mix = 0.2f, float predelayTime = 0.0f, float decayTime = 3000.0f, float modRate = 0.5f, float modDepth = 0.2f) 
        : LFO(modRate, WavetableType::SINE) {
            const float inputDiffuse[2] = {0.750f, 0.625f};
            const float decayDiffuse[2] = {0.70f, 0.50f};
            const size_t apfDelays[8] = {142, 107, 379, 277, 672, 908, 1800, 2656};
            const float delays[5] = {predelayTime, 100.97f, 84.35f, 95.62f, 71.72f};
            const float bandwidth = 0.9995f, damping = 0.0005f;

            // Diffusers
            size_t apf_i = 0;
            for (float diff : inputDiffuse) {
                for (size_t i = 0; i < 2; ++i) { diffusers.push_back(APF(1.0f, 0.0f, 0.0f, false, 10.0f, true)); 
                diffusers.back().setDelay(apfDelays[apf_i++]); diffusers.back().setQ(1.0f / (1.0f - diff)); };
            }
            for (float diff : decayDiffuse) {
                for (size_t i = 0; i < 2; ++i) { diffusers.push_back(APF(1.0f, 0.0f, 0.0f, false, 61.0f, true));
                diffusers.back().setDelay(apfDelays[apf_i++]); diffusers.back().setQ(1.0f / (1.0f - diff)); };
            } diffusers[4].setInvert(true); diffusers[5].setInvert(true);

            // Filters
            filters.push_back(OnePole(1.0f, bandwidth * SAMPLE_RATE / 2.0f));
            for (size_t i = 0; i < 2; ++i) { 
                filters.push_back(OnePole(1.0f, 2000.0f + (damping * 10000.0f)));
            }

            // Delays
            for (float delay : delays) { delayLines.push_back(DelayLine(delay, 102.0f)); }

            setMix(mix); setPredelayTime(predelayTime); setDecayTime(decayTime); setModRate(modRate); setModDepth(modDepth);
        }

        void setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setPredelayTime(float predelayTime) { // ms, [0.0, 100.0]
            this->predelayTime = std::clamp(predelayTime, 0.0f, 100.0f); 
            delayLines.front().setDelayTime(this->predelayTime); 
        } 
        void setDecayTime(float decayTime) { // ms, [100.0, 10000.0]
            this->decayTime = std::clamp(decayTime, 100.0f, 10000.0f);
            float RT60 = this->decayTime / 1000.0f;
            decayGainL = powf(0.001f, 4648.0f / (RT60 * SAMPLE_RATE));
            decayGainR = powf(0.001f, 4924.0f / (RT60 * SAMPLE_RATE));
        }
        void setModRate(float modRate) { this->modRate = std::clamp(modRate, 0.05f, 5.0f); LFO.setFreq(this->modRate); } // Hz, [0.05, 5.0]
        void setModDepth(float modDepth) { this->modDepth = std::clamp(modDepth, 0.0f, 1.0f); } // [0.0, 1.0]
        inline void setParam(ParamID param, float value) override {
            switch (param) {
                case MIX: setMix(value); break;
                case PREDELAY_TIME: setPredelayTime(value); break;
                case DECAY_TIME: setDecayTime(value); break;
                case MOD_RATE: setModRate(value); break;
                case MOD_DEPTH: setModDepth(value); break;
            }
        }

        void process(const float* in, float* out, size_t n) override {
            for (size_t i = 0; i < n; ++i) {
                nodeSig = delayLines.front().read(); delayLines.front().write(in[i]); // Predelay
                tankInSig = filters.front().processSample(nodeSig); // Input-bandwidth filter

                float x = tankInSig, y = 0.0f;
                for (size_t apf_i = 0; apf_i < 4; ++apf_i) { // Input diffusion
                    y = diffusers[apf_i].processSample(x);
                    x = y;
                }

                // Tank left
                tankSig1 = tankInSig + tankSig2;

                float tankMod = LFO.next() * modDepth * 16.0f; // EXCURSION = 16 samples
                diffusers[4].setDelay(672.0f + tankMod);
                tankSig1 = diffusers[4].processSample(tankSig1); // Decay diffusion 1L

                nodeSig = delayLines[1].read(); delayLines[1].write(tankSig1); tankSig1 = nodeSig;
                tankSig1 = filters[1].processSample(tankSig1); // Damping L

                tankSig1 *= decayGainL; // Decay L
                tankSig1 = diffusers[6].processSample(tankSig1); // Decay diffusion 2L
                nodeSig = delayLines[2].read(); delayLines[2].write(tankSig1); tankSig1 = nodeSig; // END

                // Tank right
                tankSig2 = tankInSig + tankSig1;

                diffusers[5].setDelay(908.0f + tankMod);
                tankSig2 = diffusers[5].processSample(tankSig2); // Decay diffusion 1L

                nodeSig = delayLines[3].read(); delayLines[3].write(tankSig2); tankSig2 = nodeSig;
                tankSig2 = filters[2].processSample(tankSig2); // Damping L

                tankSig2 *= decayGainR; // Decay R
                tankSig2 = diffusers[7].processSample(tankSig2); // Decay diffusion 2L
                nodeSig = delayLines[4].read(); delayLines[4].write(tankSig2); tankSig2 = nodeSig; // END

                // Mixdown output taps
                const float tapGain = 0.6f;
                float accumulatorL = (
                    delayLines[3].read(266.0f) +
                    delayLines[3].read(2974.0f) +
                    delayLines[4].read(1996.0f)
                ) - (
                    diffusers[7].readTap(1913.0f) +
                    delayLines[1].read(1990.0f) +
                    diffusers[6].readTap(187.0f) +
                    delayLines[2].read(1066.0f)
                ); accumulatorL *= tapGain;

                float accumulatorR = (
                    delayLines[1].read(353.0f) + 
                    delayLines[1].read(3627.0f) +
                    delayLines[2].read(2673.0f) 
                ) - (
                    diffusers[6].readTap(1228.0f) +
                    delayLines[3].read(2111.0f) +
                    diffusers[7].readTap(335.0f) +
                    delayLines[4].read(121.0f)
                ); accumulatorR *= tapGain;

                float wetSig = (accumulatorL + accumulatorR) * 0.5f;
                out[i] = dryWetMix(in[i], wetSig, mix); // Total mix
            }
        }
};

class Compressor : public Effect {
    private:
        enum Params : ParamID { MIX, THRESHOLD, RATIO, KNEE, ATTACK_TIME, RELEASE_TIME, MAKEUP_GAIN, AUTO_MAKEUP };

        float mix, threshold, ratio, knee, makeupGain; bool autoMakeup;
        const float L = 50.0f, L_samples = L * SAMPLE_RATE / 1000.0f; // RMS window size

        float rms = 1e-6f; DelayLine inBuffer;  
        float gainSmoothed = 0.0f;
        float attackCoeff, releaseCoeff, makeupCoeff;
        float reductionSmoothed = 0.0f; float autoMakeupGain = 0.0f;

        float computeReduction(float rmsDB) {
            // https://www.desmos.com/calculator/wkmkrmn9le
            float gDB = 0.0f;
            if (rmsDB < (threshold - (knee / 2.0f))) { gDB = rmsDB; } // Below threshold, linear
            else if (rmsDB > (threshold + (knee / 2.0f))) { gDB = threshold + ((rmsDB - threshold) / ratio); } // Above threshold, attenuate
            else { gDB = rmsDB + ((((1.0f / ratio) - 1.0f) * pow(rmsDB - threshold + (knee / 2.0f), 2.0f)) / (2.0f * knee)); } // Soft knee
            
            return gDB - rmsDB;
        }

    public:
        Compressor(float mix = 1.0f, float threshold = -18.0f, float ratio = 4.0f, float knee = 10.0f, 
                   float attack = 100.0f, float release = 100.0f, float makeupGain = 0.0f, bool autoMakeup = true) 
            : inBuffer(1.0f, L + 1.0f) {
            setMix(mix); setThreshold(threshold); setRatio(ratio); setKnee(knee); 
            setAttackTime(attack); setReleaseTime(release); setMakeupGain(makeupGain); setAutoMakeup(autoMakeup);
            inBuffer.setDelayTime(L);
            makeupCoeff = exp(-2.2f / (100.0f * SAMPLE_RATE / 1000.0f)); // Auto-makeup smoothing factor
        }

        void setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setThreshold(float threshold) { this->threshold = std::clamp(threshold, -200.0f, 0.0f); } // dB, [-200.0, 0.0]
        void setRatio(float ratio) { this->ratio = std::clamp(ratio, 1.0f, 100.0f); } // [1.0, 100.0]
        void setKnee(float knee) { this->knee = std::clamp(knee, 0.0f, 40.0f); } // dB, [0.0, 40.0]
        void setAttackTime(float attackTime) { attackCoeff = exp(-2.2f / (std::clamp(attackTime, 0.01f, 250.0f) * SAMPLE_RATE / 1000.0f)); } // ms, [0.01, 250.0]
        void setReleaseTime(float releaseTime) { releaseCoeff = exp(-2.2f / (std::clamp(releaseTime, 10.0f, 2500.0f) * SAMPLE_RATE / 1000.0f)); } // ms, [10.0, 2500.0]
        void setMakeupGain(float makeupGain) { this->makeupGain = std::clamp(makeupGain, -72.0f, 36.0f); } // dB, [-72.0, 36.0]
        void setAutoMakeup(bool autoMakeup) { 
            this->autoMakeup = autoMakeup;
            if (autoMakeup) { reductionSmoothed = 0.0f; autoMakeupGain = 0.0f; }
        }
        inline void setParam(ParamID param, float value) override {
            switch (param) {
                case MIX: setMix(value); break;
                case THRESHOLD: setThreshold(value); break;
                case RATIO: setRatio(value); break;
                case KNEE: setKnee(value); break;
                case ATTACK_TIME: setAttackTime(value); break;
                case RELEASE_TIME: setReleaseTime(value); break;
                case MAKEUP_GAIN: setMakeupGain(value); break;
                case AUTO_MAKEUP: setAutoMakeup(value > 0.5f); break;
            }
        }

        void process(const float* in, float* out, size_t n) override {
            for (size_t i = 0; i < n; ++i) {
                // Compute RMS recursively
                const float x_i = in[i], x_L = inBuffer.read();
                inBuffer.write(x_i); 
                arm_sqrt_f32((rms * rms) + (((x_i * x_i) - (x_L * x_L)) / L_samples), &rms);
                float rmsDB = ampDB(std::max(rms, 1e-6f));

                // Gain computation
                float gDB = computeReduction(rmsDB);

                // Gain smoothing (one-pole IIR LPF)
                if (gDB <= gainSmoothed) { gainSmoothed = (attackCoeff * gainSmoothed) + ((1.0f - attackCoeff) * gDB); } // Attack
                else { gainSmoothed = (releaseCoeff * gainSmoothed) + ((1.0f - releaseCoeff) * gDB); } // Release

                // Makeup gain
                if (autoMakeup) { 
                    reductionSmoothed = (makeupCoeff * reductionSmoothed) + ((1.0f - makeupCoeff) * gDB); // Makeup smoothing
                    autoMakeupGain = -reductionSmoothed;
                }
                float makeupDB = autoMakeup ? autoMakeupGain : makeupGain;
                float g = dbAmp(std::clamp(gainSmoothed + makeupDB, -60.0f, 20.0f));
                float y_i = x_i * g;

                // Hard clip just in case (e.g., limiter use case)
                if (y_i > 1.0f) { y_i = 1.0f; }
                else if (y_i < -1.0f) { y_i = -1.0f; }

                out[i] = dryWetMix(x_L, y_i, mix); // Mix
            }
        }
};

class Equalizer : public Effect {
    private:
        enum Params : ParamID { MIX, BAND1_TYPE, BAND1_CUTOFF, BAND1_Q, BAND1_GAIN, BAND2_TYPE, BAND2_CUTOFF, BAND2_Q, BAND2_GAIN };

        std::array<std::unique_ptr<Biquad>, 2> bands;

        BiquadType band1Type, band2Type;
        float mix, band1Cutoff, band1Q, band1Gain, band2Cutoff, band2Q, band2Gain;

        std::unique_ptr<Biquad> createBiquad(BiquadType type, float cutoff, float q, float gainDB) {
            switch (type) {
                case BiquadType::LOW_PASS: return std::make_unique<LPF_Biquad>(cutoff, q, gainDB);
                case BiquadType::HIGH_PASS: return std::make_unique<HPF_Biquad>(cutoff, q, gainDB);
                case BiquadType::LOW_SHELF: return std::make_unique<LowShelf_Biquad>(cutoff, q, gainDB);
                case BiquadType::HIGH_SHELF: return std::make_unique<HighShelf_Biquad>(cutoff, q, gainDB);
                case BiquadType::PEAK: return std::make_unique<Peak_Biquad>(cutoff, q, gainDB);
                case BiquadType::NOTCH: return std::make_unique<Notch_Biquad>(cutoff, q, gainDB);
                default: return std::make_unique<Peak_Biquad>(cutoff, q, gainDB);
                }
        }

    public:
        Equalizer(float mix = 1.0f,
                  BiquadType band1Type = BiquadType::LOW_SHELF,  float band1Cutoff = 200.0f,  float band1Q = 0.707f, float band1Gain = 0.0f,
                  BiquadType band2Type = BiquadType::HIGH_SHELF, float band2Cutoff = 2000.0f, float band2Q = 0.707f, float band2Gain = 0.0f) {
            bands[0] = createBiquad(band1Type, band1Cutoff, band1Q, band1Gain);
            bands[1] = createBiquad(band2Type, band2Cutoff, band2Q, band2Gain);

            setMix(mix);
            setBand1Type(band1Type); setBand1Cutoff(band1Cutoff); setBand1Q(band1Q); setBand1Gain(band1Gain);
            setBand2Type(band2Type); setBand2Cutoff(band2Cutoff); setBand2Q(band2Q); setBand2Gain(band2Gain);
        }

        void setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setBand1Type(BiquadType type) {
            if (type == band1Type) return;
            band1Type = type;
            bands[0] = createBiquad(band1Type, band1Cutoff, band1Q, band1Gain);
        }
        void setBand1Cutoff(float cutoff) { band1Cutoff = std::clamp(cutoff, 20.0f, 20000.0f); bands[0]->setCutoff(band1Cutoff); } // Hz, [20.0, 20000.0]
        void setBand1Q(float q) { band1Q = std::clamp(q, 0.02f, 40.0f); bands[0]->setQ(band1Q); } // [0.02, 40.0]
        void setBand1Gain(float gainDB) { band1Gain = std::clamp(gainDB, -24.0f, 24.0f); bands[0]->setGain(band1Gain); } // dB, [-24.0, 24.0]
        void setBand2Type(BiquadType type) {
            if (type == band2Type) return; 
            band2Type = type;
            bands[1] = createBiquad(band2Type, band2Cutoff, band2Q, band2Gain);
        }
        void setBand2Cutoff(float cutoff) { band2Cutoff = std::clamp(cutoff, 20.0f, 20000.0f); bands[1]->setCutoff(band2Cutoff); } // Hz, [20.0, 20000.0]
        void setBand2Q(float q) { band2Q = std::clamp(q, 0.02f, 40.0f); bands[1]->setQ(band2Q); } // [0.02, 40.0]
        void setBand2Gain(float gainDB) { band2Gain = std::clamp(gainDB, -24.0f, 24.0f); bands[1]->setGain(band2Gain); } // dB, [-24.0, 24.0]
        inline void setParam(ParamID param, float value) override { 
            switch (param) {
                case MIX: setMix(value); break;
                case BAND1_TYPE: setBand1Type((BiquadType)value); break; 
                case BAND1_CUTOFF: setBand1Cutoff(value); break;
                case BAND1_Q: setBand1Q(value); break;
                case BAND1_GAIN: setBand1Gain(value); break;
                case BAND2_TYPE: setBand2Type((BiquadType)value); break;
                case BAND2_CUTOFF: setBand2Cutoff(value); break;
                case BAND2_Q: setBand2Q(value); break;
                case BAND2_GAIN: setBand2Gain(value); break;
            }
        }

        void process(const float* in, float* out, size_t n) override {
            for (size_t i = 0; i < n; ++i) {
                float y = in[i];
                for (auto& band : bands) if (!band->isBypassed()) y = band->processSample(y);
                out[i] = dryWetMix(in[i], y, mix);
            }
        }
};

class Granulator : public Effect {
    // HACK: Works, but produces clicks with larger positionRand since grains may end up reading from indices overwritten by write head
    // Non-clicking usage: No reversed grains + any positionRand, OR reversed grains + no/small positionRand
    private:
        enum Params : ParamID { MIX, POSITION, POSITION_RAND, RATE, RATE_RAND, LENGTH, LENGTH_RAND, 
                                LEVEL, LEVEL_RAND, REVERSE_CHANCE, ENVELOPE_TYPE };

        const size_t bufSize = 1 * (size_t)SAMPLE_RATE;
        const int maxGrains = 32;
        
        float mix, position, rate, length, level, reverseChance;
        float positionRand, rateRand, lengthRand, levelRand;
        EnvelopeType envType;
        
        std::vector<float> inBuf; size_t writePos = 0;
        float grainCounter = 0.0f;
        
        struct Grain {
            bool active = false;
            uint32_t startPos;
            uint32_t playhead;
            uint32_t length;
            float level;
            bool reverse;
        };
        
        std::vector<Grain> grains; // Grain pool

        void spawnGrain() {
            // Find free grain
            Grain* freeGrain = nullptr;
            for (auto& grain : grains) if (!grain.active) { freeGrain = &grain; break; }
            // If none, steal oldest grain
            if (freeGrain == nullptr) {
                for (auto& grain : grains) if (grain.active) { freeGrain = &grain; break; }
            }
            
            // Calculate grain parameters
            float positionFactor = std::clamp(position + (uniform() * positionRand), 0.0f, 1.0f);
            int grainStartPos = (int)writePos - (int)(positionFactor * (bufSize - 1));
            if (grainStartPos < 0) grainStartPos += bufSize;
            
            float grainLength = std::clamp(length * (1.0f + (uniform() * lengthRand)), 5.0f, 1000.0f);
            int grainLengthSamples = std::min((int)(grainLength * SAMPLE_RATE / 1000.0f), (int)bufSize - 1);

            float grainLevel = std::clamp(level + (uniform() * 0.25f * levelRand), 0.0f, 1.0f) * 0.5f;
            
            bool reversed = ((uniform() + 1.0f) / 2.0f) <= reverseChance;
            
            // Init
            freeGrain->active = true;
            freeGrain->startPos = grainStartPos;
            freeGrain->playhead = 0;
            freeGrain->length = grainLengthSamples;
            freeGrain->level = grainLevel;
            freeGrain->reverse = reversed;
        }

        float processGrain(Grain& grain) {
            // Calculate read position in input buffer
            int offset = grain.reverse ? ((grain.length - 1) - grain.playhead) : grain.playhead;
            int readPos = (grain.startPos + offset) % bufSize;
            
            float envelopeValue = getEnvelopeValue((float)grain.playhead / (float)grain.length, envType);
            
            ++grain.playhead;
            if (grain.playhead >= grain.length) grain.active = false; // Free if done
            
            return inBuf[readPos] * envelopeValue * grain.level;
        }

    public:
        Granulator(float mix = 1.0f, float position = 0.5f, float positionRand = 0.5f, float time = 50.0f, float timeRand = 0.0f, 
                   float length = 200.0f, float lengthRand = 0.0f, float level = 0.8f, float levelRand = 0.0f, 
                   float reverseChance = 0.0f, EnvelopeType envType = EnvelopeType::HANN) {
            setMix(mix); setPosition(position); setPositionRand(positionRand); setRate(time); setRateRand(timeRand); setLength(length); 
            setLengthRand(lengthRand); setLevel(level); setLevelRand(levelRand); setReverseChance(reverseChance); setEnvelopeType(envType);
            grains.resize(maxGrains);
            inBuf.resize(bufSize, 0.0f);
        }

        void setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setPosition(float position) { this->position = std::clamp(position, 0.0f, 1.0f); } // [0.0, 1.0]
        void setPositionRand(float positionRand) { this->positionRand = std::clamp(positionRand, 0.0f, 1.0f); } // [0.0, 1.0]
        void setRate(float rate) { this->rate = std::clamp(rate, 1.0f, 500.0f); } // ms, [1.0, 500.0]
        void setRateRand(float rateRand) { this->rateRand = std::clamp(rateRand, 0.0f, 1.0f); } // [0.0, 1.0]
        void setLength(float length) { this->length = std::clamp(length, 5.0f, 500.0f); } // ms, [5.0, 500.0]
        void setLengthRand(float lengthRand) { this->lengthRand = std::clamp(lengthRand, 0.0f, 1.0f); } // [0.0, 1.0]
        void setReverseChance(float reverseChance) { this->reverseChance = std::clamp(reverseChance, 0.0f, 1.0f); } // [0.0, 1.0]
        void setLevel(float level) { this->level = std::clamp(level, 0.0f, 1.0f); } // [0.0, 1.0]
        void setLevelRand(float levelRand) { this->levelRand = std::clamp(levelRand, 0.0f, 1.0f); } // [0.0, 1.0]
        void setEnvelopeType(EnvelopeType envType) { this->envType = envType; }
        inline void setParam(ParamID param, float value) override {
            switch (param) {
                case MIX: setMix(value); break;
                case POSITION: setPosition(value); break;
                case POSITION_RAND: setPositionRand(value); break;
                case RATE: setRate(value); break;
                case RATE_RAND: setRateRand(value); break;
                case LENGTH: setLength(value); break;
                case LENGTH_RAND: setLengthRand(value); break;
                case LEVEL: setLevel(value); break;
                case LEVEL_RAND: setLevelRand(value); break;
                case REVERSE_CHANCE: setReverseChance(value); break;
                case ENVELOPE_TYPE: setEnvelopeType(static_cast<EnvelopeType>(value)); break;
            }
        }

    void process(const float* in, float* out, size_t n) override {
        for (size_t i = 0; i < n; ++i) {
            inBuf[writePos] = in[i]; // Write input to circular buffer
            ++grainCounter;
            
            float timeSamples = (rate + (rate * rateRand * uniform())) * SAMPLE_RATE / 1000.0f;
            if (grainCounter >= timeSamples) { // Time to spawn a grain!
                spawnGrain();
                grainCounter = 0.0f;
            }
            
            // Process active grains and accumulate output
            float wetSig = 0.0f;
            for (auto& grain : grains) if (grain.active) { wetSig += processGrain(grain); }
            out[i] = dryWetMix(in[i], wetSig, mix); // Mix

            ++writePos;
            if (writePos >= bufSize) writePos = 0;
        }
    }
};

class Freezer : public Effect {
    private:
        enum Params : ParamID { MIX, RATE, SPECTRAL_MODE, FFT_SIZE, HOP_SIZE, LOOP_START, LOOP_END };

        const size_t bufSize = (size_t)2 * (size_t)SAMPLE_RATE;
        const float smooth = 0.005f;
        
        float mix, rate; bool spectralMode;
        float loopStart, loopEnd;
        
        std::vector<float> inBuf; size_t writePos = 0; float readPos = 0.0f;
        
        // Spectral resythesis
        size_t fftSize, hopFactor;
        std::unique_ptr<STFT> stft;
        std::vector<float> spectBuf, spectFrame;
        size_t spectPos = 0, spectHopCounter = 0;

        void allocateSTFT() {
            stft = std::make_unique<STFT>(fftSize, hopFactor, 0.5f);
            spectBuf.assign(fftSize, 0.0f); spectFrame.assign(fftSize, 0.0f);
            spectPos = 0; spectHopCounter = 0;
        }
        void freeSTFT() {
            stft.reset(); 
            spectBuf.clear(); spectFrame.clear();
        }

    public:
        Freezer(float mix = 1.0f, float rate = 1.0f, bool spectralMode = true, size_t fftSize = 1024, size_t hopFactor = 4, 
                float loopStart = 0.0f, float loopEnd = 1.0f) : fftSize(fftSize), hopFactor(hopFactor) {
            setMix(mix); setRate(rate); setFFTSize(fftSize); setHopSize(hopFactor); 
            setSpectralMode(spectralMode); setLoopRegion(loopStart, loopEnd); 
            inBuf.resize(bufSize, 0.0f);
        }
        
        void setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setRate(float rate) { this->rate = std::clamp(rate, -4.0f, 4.0f); } // [-4.0, 4.0]
        void setSpectralMode(bool mode) { 
            if (mode == spectralMode) return;
            spectralMode = mode;

            if (spectralMode) allocateSTFT();
            else freeSTFT();
        }
        void setFFTSize(size_t N) { // [128, FFT_MAX_SIZE], MUST BE POWER OF 2
            const size_t fftN = std::clamp(N, (size_t)128, (size_t)FFT_MAX_SIZE);
            fftSize = fftN;
            if (stft) {
                stft->setFFTSize(fftN); 
                spectBuf.assign(fftN, 0.0f); spectFrame.resize(fftN);
                spectPos = 0; spectHopCounter = 0;
            }

        }
        void setHopSize(size_t hopFactor) { // [2, 8]
            this->hopFactor = std::clamp(hopFactor, (size_t)2, (size_t)8);
            if (stft) stft->setHopSize(hopFactor);
        } 
        void setLoopRegion(float start, float end) { // [0.0, 1.0] for both
            loopStart = std::clamp(start, 0.0f, 1.0f);
            loopEnd = std::clamp(end, 0.0f, 1.0f);
            if (loopStart >= loopEnd) { // start < end
                loopEnd = loopStart + 0.01f;
                if (loopEnd > 1.0f) { loopEnd = 1.0f; loopStart = 0.99f; }
            }
            readPos = loopStart * (float)bufSize;
        }
        inline void setParam(ParamID param, float value) override {
            switch (param) {
                case MIX: setMix(value); break;
                case RATE: setRate(value); break;
                case SPECTRAL_MODE: setSpectralMode(value > 0.5f); break;
                case FFT_SIZE: setFFTSize((size_t)value); break;
                case HOP_SIZE: setHopSize((size_t)value); break;
                case LOOP_START: setLoopRegion(value, loopEnd); break;
                case LOOP_END: setLoopRegion(loopStart, value); break;
            }
        }
        
        void process(const float* in, float* out, size_t n) override {
            const size_t bufN = bufSize,
                         fftN = stft ? stft->getFFTSize() : 0,
                         hopN = stft ? stft->getHopSize() : 0;

            // Compute loop boundaries
            float loopStartSamples = loopStart * (float)bufN;
            float loopEndSamples = loopEnd * (float)bufN;
            float loopLength = loopEndSamples - loopStartSamples;
            
            for (size_t i = 0; i < n; ++i) {
                inBuf[writePos] = in[i]; // Write input to circular buffer
                ++writePos; if (writePos >= bufN) writePos = 0;
                
                float wetSig = 0.0f;
                if (spectralMode && stft) { // Spectral resynthesis mode

                    stft->forward(in[i]); // Forward FFT

                    // Read from OLA buffer
                    wetSig = spectBuf[spectPos];
                    spectBuf[spectPos] = 0.0f;
                    ++spectHopCounter;

                    /* SYNTHESIZE FFT FRAMES */
                    if (spectHopCounter >= hopN) { // Every hopN samples
                        spectHopCounter = 0;
                        if (stft->getSpectSize() > 0) {
                            // Map readPos to FFT frame index
                            float framePos = fmod(readPos / (float)hopN, (float)stft->getSpectSize());
                            if (framePos < 0) framePos += stft->getSpectSize();
                            // Interpolate spectral frame
                            STFT::FFTFrame interpFrame = stft->interpolateFrame(framePos);
                            // IFFT
                            stft->getFFT().inverse(interpFrame.bins.data(), spectFrame.data());
                            // Window and OLA
                            overlapAdd(spectBuf, spectFrame, EnvelopeType::HANN, spectPos);
                        }
                    }
                    
                    ++spectPos; if (spectPos >= fftN) spectPos = 0;
                }

                // Advance and wrap read pos
                readPos += rate;
                if (readPos < loopStartSamples) { readPos += loopLength; }
                else if (readPos >= loopEndSamples) { readPos -= loopLength; }
                float loopPos = (readPos - loopStartSamples) / loopLength; // Map to loop pos

                if (!spectralMode) wetSig = lerp(inBuf, readPos, bufN); // Time domain mode

                float crossfade = 1.0f;
                if (rate != 0.0f) {
                    const float fadeLen = smooth;
                    if (loopPos < fadeLen) { // Fade in
                        float t = loopPos / fadeLen;
                        crossfade = 0.5f * (1.0f - arm_cos_f32(M_PI * t)); 
                    } else if (loopPos > 1.0f - fadeLen) { // Fade out
                        float t = (loopPos - (1.0f - fadeLen)) / fadeLen;
                        crossfade = 0.5f * (1.0f + arm_cos_f32(M_PI * t));
                    }
                }
                wetSig *= crossfade;
            
                out[i] = dryWetMix(in[i], wetSig, mix); // Mix
            }
        }
};

/* SPECTRAL */

class SpectralGate : public Spectral_Effect {
    private:
        enum Params : ParamID { THRESHOLD = 2, TILT };

        float threshold, tilt;
        
    public:
        SpectralGate(float mix = 1.0f, float thresholdDB = -10.0f, float tilt = 0.5f, 
            size_t fftSize = 512, size_t hopFactor = 4) : Spectral_Effect(mix, fftSize, hopFactor)
            { setThreshold(thresholdDB); setTilt(tilt); }
        
        void setThreshold(float thresholdDB) { threshold = dbAmp(std::clamp(thresholdDB, -100.0f, 0.0f)); } // dB, [-100.0, 0.0]
        void setTilt(float tilt) { this->tilt = std::clamp(tilt, -1.0f, 1.0f) * 2.0f; } // [-1.0, 1.0]
        inline void setParam(ParamID param, float value) override {
            switch (param) {
                case THRESHOLD: setThreshold(value); break;
                case TILT: setTilt(value); break;
                default: Spectral_Effect::setParam(param, value);
            }
        }

    protected:
        void processSpectrum(STFT::FFTFrame& frame) override {
            const size_t numBins = frame.bins.size();
            const float binTilt = tilt / (float)(numBins - 1);
            const float threshold_sq = threshold * threshold;
            
            for (size_t k = 0; k < numBins; ++k) {
                // + tilt gates lows more, - tilt gates highs more
                float weight = 1.0f - ((k * binTilt) - (tilt * 0.5f));
                if (weight < 0.0f) weight = 0.0f;
                
                // Compare squared magnitudes
                float mag_sq = (frame.bins[k].r * frame.bins[k].r) + (frame.bins[k].i * frame.bins[k].i);
                float weight_sq = weight * weight;
                
                if ((mag_sq * weight_sq) < threshold_sq) { // Gate bins under threshold
                    frame.bins[k].r = 0.0f; frame.bins[k].i = 0.0f; }
            }
        }
};

class FormantShifter : public Spectral_Effect {
    // Alexander Panos' GOATed formant shifter device, faithfully ported from Max/MSP (Gen) to C++!
    // https://alexanderpanos.com/software
    private:
        enum Params : ParamID { FORMANT_SHIFT = 2, ENVELOPE_WIDTH };

        float formantShift; size_t envelopeWidth;
        size_t width; // internal value after env_compensation
        
        std::vector<float> buf;
        std::vector<float> interp; // interpolated peaks
        std::vector<std::pair<float, size_t>> pk_info; // ch.0 = value of peak, ch.1 = index of peak in "buf"
        std::vector<float> formants;
        
    public:
        FormantShifter(float mix = 1.0f, float formantShift = 0.0f, size_t envelopeWidth = 16, size_t fftSize = 1024, size_t hopFactor = 4) 
        : Spectral_Effect(mix, fftSize, hopFactor) { 
            setFormantShift(formantShift); setEnvelopeWidth(envelopeWidth);
            
            // Allocate buffers
            const size_t numBins = (fftSize / 2) + 1;
            buf.resize(numBins); interp.resize(numBins); formants.resize(numBins);
        }
        
        void setFormantShift(float formantShift) { this->formantShift = std::clamp(formantShift, -12.0f, 12.0f); } // semitones, [-12.0, 12.0]
        void setEnvelopeWidth(size_t envelopeWidth) { 
            this->envelopeWidth = std::clamp(envelopeWidth, (size_t)2, stft.getNumBins() / (size_t)4); 
            // env_compensation
            const size_t srComp = 1; // hardcoded for SAMPLE_RATE = 44.1 kHz
            float scaled = 0.0f;
            if (formantShift > 0) { 
                scaled = scale((float)std::clamp(formantShift, 0.0f, 12.0f), 
                0.0f, 12.0f, 10.0f, 5.0f, 2.5f); 
            } else { 
                scaled = scale((float)std::clamp(formantShift, -12.0f, 0.0f), 
                -12.0f, 0.0f, 16.0f, 10.0f, 1.0f);    
            }
            width = (size_t)scaled + (envelopeWidth - (size_t)8);
            width += srComp;
            width = std::clamp(width, (size_t)2, (size_t)32);
        }
        inline void setParam(ParamID param, float value) override {
            switch (param) {
                case FORMANT_SHIFT: setFormantShift(value); break;
                case ENVELOPE_WIDTH: setEnvelopeWidth((size_t)value); break;
                default: Spectral_Effect::setParam(param, value);
            }
        }
        
    protected:
        void processSpectrum(STFT::FFTFrame& frame) override {
            if (formantShift == 0.0f) return;

            const size_t len = stft.getNumBins(); // dim(buf)
            const float shift = exp2f(formantShift / 12.0f);
            const float epsilon = 1e-6f;
            
            const float cutoff = 8000.0f; // band limit to save compute
            const size_t maxBin = std::min(len, static_cast<size_t>((cutoff / (SAMPLE_RATE * 0.5f)) * (float)(len - 1)));
            const size_t num_regions = (maxBin + width - 1) / width; // number of equidistant regions within the buffer to find peaks

            // store magnitudes in "buf"
            for (size_t k = 0; k < maxBin; ++k) {
                float re = frame.bins[k].r; float im = frame.bins[k].i;
                arm_sqrt_f32(re * re + im * im, &buf[k]);
            }

            /*————— INITIALIZE BUFFERS —————*/
            // std::fill(interp.begin(), interp.end(), 0.0f); // clear
            
            pk_info.clear();
            pk_info.reserve(num_regions + 2);
            pk_info.push_back({buf[0], 0}); // set first element in pk_info to the first element in "buf" buffer
            
            /*————— PEAK DETECTION MAIN LOOP —————*/
            size_t local_len = width;
            for (size_t j = 0; j < num_regions; ++j) {
                float loc_max = 0.0f; float loc_sum = 0.0f; size_t max_idx = 0;
                
                // calculate mean of local region
                size_t start = (local_len > width) ? (local_len - width) : 0;
                size_t end = std::min(start + width, len);
                for (size_t i = start; i < end; ++i) {
                    const float v = buf[i];
                    loc_sum += v;
                    if (v > loc_max) { loc_max = v; max_idx = i; } // local maxima
                }
                const float loc_avg = loc_sum / ((end > start) ? (float)(end - start) : 1.0f);

                // use absolute maximum if no local maxima
                size_t idx = max_idx;
                float val = loc_max;
                for (size_t i = start; i < end; ++i) {
                    const float v = buf[i];
                    if (v > loc_avg && v >= val) { idx = i; val = v; }
                }
                pk_info.push_back({val, idx});
                
                local_len += width;
            }
            
            pk_info.push_back({buf[len - 1], len - 1}); // set last element in pk_info to the last element in "buf"
            
            /*————— PEAK INTERPOLATION MAIN LOOP —————*/
            for (size_t j = 1; j < pk_info.size(); ++j) { // start loop at j=1; 0th index was set to the first element in "buf"
                size_t start_pos = pk_info[j - 1].second;
                size_t distance = pk_info[j].second;
                float previous_pk = pk_info[j - 1].first;
                float current_pk = pk_info[j].first; // the literal value of the peak

                if (distance <= start_pos) continue;
                for (size_t i = start_pos; i < distance; ++i) {
                    float val = scale((float)i, (float)start_pos, (float)distance, previous_pk, current_pk);
                    interp[i] = val;
                }
            }
            interp[len - 1] = pk_info.back().first;
            
            /*————— FORMANT SHIFT ROUTINE —————*/
            const float maxIdx = (float)(len - 1);
            const float inv_shift = 1.0f / shift;
            for (size_t i = 0; i < len; ++i) {
                float sourceIdx = (float)i * inv_shift;
                
                // poke(formants, valf, i, boundmode="clip")
                if (sourceIdx <= 0.0f) { formants[i] = interp[0];
                } else if (sourceIdx > maxIdx) { formants[i] = interp[len - 1];
                } else { formants[i] = lerp(interp, sourceIdx, len); }
            }
            
            /*————— CONVOLUTION —————*/
            for (size_t i = 1; i < maxBin; ++i) {
                if (interp[i] <= epsilon) { formants[i] = 0.0f; continue; } // avoid divide-by-zero

                float det = buf[i] / (interp[i] + epsilon); // deconvolution to get spectral detail
                float spectrum = det * formants[i]; // convolution of spectral detail & shifted spectral envelope
                if (spectrum > 10.0f * buf[i]) spectrum = 10.0f * buf[i]; // clamp extreme gains
                formants[i] = spectrum;
                
                // apply to complex FFT bins (preserve phase)
                if (buf[i] > epsilon) {
                    const float scaleFactor = spectrum / buf[i];
                    frame.bins[i].r *= scaleFactor; frame.bins[i].i *= scaleFactor;
                } else { frame.bins[i].r = 0.0f; frame.bins[i].i = 0.0f; }
            }
        }
};

#endif // EFFECTS