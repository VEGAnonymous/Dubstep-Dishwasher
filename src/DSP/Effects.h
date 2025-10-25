#ifndef EFFECTS
#define EFFECTS

#include "Utilities.h"
#include "Modules.h"
#include "Generators.h"
#include "Filters.h"

#include <cmath>
#include <string>
#include <vector>
#include <iostream>

/* EFFECTS */

class Gain : public Effect { // Example, not for practical use
    private:
        enum Params : ParamID { GAIN };

        float gainFactor;

    public:
        Gain(float gainFactor = 1.0f) { setGain(gainFactor); }

        void setGain(float gainFactor) { this->gainFactor = gainFactor; }
        inline void setParam(ParamID param, float value) override {
            switch (param) {
                case GAIN: setGain(value); break;
            }
        }

        void process(const float* in, float* out, size_t n) override {
            const float* in_ptr = in;
            float* out_ptr = out;

            for (size_t i = 0; i < n; ++i) {
                *out_ptr++ = *in_ptr++ * gainFactor;
                // out[i] = in[i] * gainFactor; // Equivalent
            }
        };
};

class Distortion : public Effect {
    private:
        enum Params : ParamID { MIX, MODE, DRIVE, ENABLE_AAF };

        distortionMode mode;
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
            if (x < 0) { x = exp(x) - 1.0f; } // Shockley diode equation, B=1
            else if (x > 0) { x = 1.0 - exp(-x); }
            else { x = 0; }
            return x;
        }
        float bitCrush(float in, float drive) {
            int bitDepth = (int)(2 + ((1.0f - drive) * (1.0f - drive) * 22.0f)); // 24-bit to 2-bit depth
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
            return tanh(x);
        }

    public:
        Distortion(float mix = 1.0f, distortionMode mode = TUBE, float drive = 0.25f, bool enableAAF = false) 
        : antiAlias(1.0f, vector<float>(begin(AAF), end(AAF))) { setMix(mix); setMode(mode); setDrive(drive); setAAF(enableAAF); }

        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setMode(distortionMode mode) {
            this->mode = mode;
            switch (mode) {
                case TUBE: algorithm = &Distortion::tube; break;
                case SOFT_CLIP: algorithm = &Distortion::softClip; break;
                case HARD_CLIP: algorithm = &Distortion::hardClip; break;
                case DIODE: algorithm = &Distortion::diode; break;
                case BITCRUSH: algorithm = &Distortion::bitCrush; break;
                case RECTIFY: algorithm = &Distortion::rectify; break;
                case SATURATE: algorithm = &Distortion::saturate; break;
                default: algorithm = &Distortion::hardClip;
            }
        }
        void setDrive(float drive) { this->drive = clamp(drive, 0.0f, 1.0f); } // [0.0, 1.0]
        void setAAF(bool enableAAF) { this->enableAAF = enableAAF; } 
        inline void setParam(ParamID param, float value) override {
            switch (param) {
                case MIX: setMix(value); break;
                case MODE: setMode(static_cast<distortionMode>(value)); break;
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

        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setDelayTime(float delayTime) { delayLine.setDelayTime(clamp(delayTime, 1.0f, maxDelayTime)); } // ms, [1.0, 500.0]
        void setFeedback(float feedback) { this->feedback = clamp(feedback, -0.95f, 0.95f); } // [-0.95, 0.95]
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
        : delayLine(15, 30), LFO(rate, SINE_TABLE) { setMix(mix); setRate(rate); setDepth(depth); setFeedback(feedback); }

        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setRate(float rate) { this->rate = clamp(rate, 0.0f, 20.0f); LFO.setFreq(rate); } // Hz, [0.0, 20.0]
        void setDepth(float depth) { this->depth = clamp(depth, 0.0f, 1.0f); } // [0.0, 1.0]
        void setFeedback(float feedback) { this->feedback = clamp(feedback, -0.95f, 0.95f); } // [-0.95, 0.95]
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

        const uint8_t order = 8;
        const float q = 0.8f;

        float mix, rate, centerFreq, spread, depth, feedback;

        vector<APF> apfSections; // APF bank
        vector<float> baseFreqs; // Store APF base freqs
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
        : LFO(rate, SINE_TABLE) {
            for (size_t i = 0; i < order; ++i) { apfSections.push_back(APF(1.0f, centerFreq, q, false, 20.0f, true)); }
            setupStages();
            setMix(mix); setRate(rate); setCenterFreq(centerFreq); setSpread(spread), setDepth(depth); setFeedback(feedback);
        }

        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setRate(float rate) { this->rate = clamp(rate, 0.0f, 20.0f); LFO.setFreq(rate); } // Hz, [0.0, 20.0]
        void setCenterFreq(float centerFreq) { this->centerFreq = clamp(centerFreq, 50.0f, 8000.0f); setupStages(); } // Hz, // [50.0, 8000.0]
        void setSpread(float spread) { this->spread = clamp(spread, 0.1f, 1.0f); setupStages(); } // [0.1, 1.0]
        void setDepth(float depth) { this->depth = clamp(depth, 0.0f, 1.0f); } // [0.0, 1.0]
        void setFeedback(float feedback) { this->feedback = clamp(feedback, -0.95f, 0.95f); } // [-0.95, 0.95]
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
                mod = pow(2.0f, depth * LFO.next());
                inSig = in[i] + (wetSig * feedback);
                
                const float* in_ptr = &inSig;
                float* out_ptr = nullptr;
                for (size_t j = 0; j < apfSections.size(); ++j) { // Process APFs serially, output to wetSig
                    apfSections[j].setCutoff(baseFreqs[j] * mod); // Modulate each base freq with LFO
                    out_ptr = (j == apfSections.size() - 1) ? &wetSig : &stageSig;
                    apfSections[j].process(in_ptr, out_ptr, 1);
                    in_ptr = out_ptr;
                }

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

        vector<voice> voices;
        DelayLine delayLine;
    public:
        Chorus(float mix = 1.0f, float rate = 0.08f, float depth = 25.0f, float delayTime = 5.0f, float feedback = 0.1f)
        : delayLine(delayTime, 51.0f) {
            for (size_t i = 0; i < voiceCount; ++i) voices.push_back({1.0f / (float)voiceCount, depth, delayTime, Random(rate, PERLIN)});
            setMix(mix); setRate(rate); setDepth(depth); setDelayTime(delayTime); setFeedback(feedback);
        }

        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setRate(float rate) { // Hz, [0.0, 20.0]
            this->rate = clamp(rate, 0.0f, 20.0f); 
            for (voice& vc : voices) vc.mod.setFreq(rate);
        }
        void setDepth(float depth) { // ms, [0.0, 25.0]
            this->depth = clamp(depth, 0.0f, 25.0f);
            for (voice& vc : voices) vc.depth = depth; 
        }
        void setDelayTime(float delayTime) { // ms, [0.0, 20.0]
            this->delayTime = clamp(delayTime, 0.0f, 20.0f); 
            for (size_t i = 0; i < voiceCount; ++i) { 
                float detune = ((float)i / voiceCount - 0.5f) * 2.0f;
                voices[i].baseDelay = delayTime * (1.0f + 0.3f * detune);
            };
        }
        void setFeedback(float feedback) { this->feedback = clamp(feedback, -0.95f, 0.95f); } // [-0.95, 0.95]
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
            const float *in_ptr = in;
            float *out_ptr = out;
            for (size_t i = 0; i < n; i++) {
                float wetSig = 0.0f;
                for (voice& vc : voices) {
                    float mod = vc.baseDelay + (vc.depth * vc.mod.next());
                    wetSig += delayLine.read(mod * SAMPLE_RATE / 1000.0f) * vc.mix;
                }
                delayLine.write(*in_ptr + (feedback * wetSig));

                wetSig *= sqrtf((float)voiceCount) * 1.2f;
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
        vector<APF> diffusers; // 8
        vector<OnePole> filters; // 3
        vector<DelayLine> delayLines; // 5
        Wavetable LFO;
        float tankInSig, nodeSig, tankSig1 = 0, tankSig2 = 0;

    public:
        Reverb(float mix = 0.2f, float predelayTime = 0.0f, float decayTime = 3000.0f, float modRate = 0.5f, float modDepth = 0.2f) 
        : LFO(modRate, SINE_TABLE) {
            const float inputDiffuse[2] = {0.750f, 0.625f};
            const float decayDiffuse[2] = {0.70f, 0.50f};
            const float apfDelays[8] = {142, 107, 379, 277, 672, 908, 1800, 2656};
            const float delays[5] = {predelayTime, 100.97f, 84.35f, 95.62f, 71.72f};
            const float bandwidth = 0.9995f, damping = 0.0005f;

            // Diffusers
            size_t apf_i = 0;
            for (float diff : inputDiffuse) {
                for (size_t i = 0; i < 2; ++i) { diffusers.push_back(APF(1.0f, 0.0f, 0.0f, false, 10.0f, false)); 
                diffusers.back().setDelay(apfDelays[apf_i++]); diffusers.back().setQ(1.0f / (1.0f - diff)); };
            }
            for (float diff : decayDiffuse) {
                for (size_t i = 0; i < 2; ++i) { diffusers.push_back(APF(1.0f, 0.0f, 0.0f, false, 61.0f, false));
                diffusers.back().setDelay(apfDelays[apf_i++]); diffusers.back().setQ(1.0f / (1.0f - diff)); };
            } diffusers[4].setInvert(true); diffusers[5].setInvert(true);

            // Filters
            filters.push_back(OnePole(1.0f, bandwidth * SAMPLE_RATE / 2.0f));
            for (size_t i = 0; i < 2; ++i) { 
                filters.push_back(OnePole(1.0f, damping * 10000.0f));
            }

            // Delays
            for (float delay : delays) { delayLines.push_back(DelayLine(delay, 100.0f)); }

            setMix(mix); setPredelayTime(predelayTime); setDecayTime(decayTime); setModRate(modRate); setModDepth(modDepth);
        }

        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setPredelayTime(float predelayTime) { // ms, [0.0, 500.0]
            this->predelayTime = clamp(predelayTime, 0.0f, 500.0f); 
            delayLines.front().setDelayTime(this->predelayTime); 
        } 
        void setDecayTime(float decayTime) { // ms, [100.0, 10000.0]
            this->decayTime = clamp(decayTime, 100.0f, 10000.0f);
            float RT60 = this->decayTime / 1000.0f;
            decayGainL = powf(0.001f, 4648.0f / (RT60 * SAMPLE_RATE));
            decayGainR = powf(0.001f, 4924.0f / (RT60 * SAMPLE_RATE));
        }
        void setModRate(float modRate) { this->modRate = clamp(modRate, 0.05f, 5.0f); LFO.setFreq(this->modRate); } // Hz, [0.05, 5.0]
        void setModDepth(float modDepth) { this->modDepth = clamp(modDepth, 0.0f, 1.0f); } // [0.0, 1.0]
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
                filters.front().process(&nodeSig, &tankInSig, 1); // Input-bandwidth filter

                float* in_ptr = &tankInSig;
                float* out_ptr = nullptr;
                for (size_t apf_i = 0; apf_i < 4; ++apf_i) { // Input diffusion
                    out_ptr = (apf_i == 3) ? &tankInSig : &nodeSig; 
                    diffusers[apf_i].process(in_ptr, out_ptr, 1);
                    in_ptr = out_ptr;
                }

                // Tank left
                tankSig1 = tankInSig + tankSig2;

                float tankMod = LFO.next() * modDepth * 16.0f; // EXCURSION = 16 samples
                diffusers[4].setDelay(672.0f + tankMod);
                diffusers[4].process(&tankSig1, &nodeSig, 1); tankSig1 = nodeSig; // Decay diffusion 1L

                nodeSig = delayLines[1].read(); delayLines[1].write(tankSig1); tankSig1 = nodeSig;
                filters[1].process(&tankSig1, &nodeSig, 1); // Damping L

                tankSig1 = nodeSig * decayGainL; // Decay L
                diffusers[6].process(&tankSig1, &nodeSig, 1); tankSig1 = nodeSig; // Decay diffusion 2L
                nodeSig = delayLines[2].read(); delayLines[2].write(tankSig1); tankSig1 = nodeSig; // END

                // Tank right
                tankSig2 = tankInSig + tankSig1;

                diffusers[5].setDelay(908.0f + tankMod);
                diffusers[5].process(&tankSig2, &nodeSig, 1); tankSig2 = nodeSig; // Decay diffusion 1L

                nodeSig = delayLines[3].read(); delayLines[3].write(tankSig2); tankSig2 = nodeSig;
                filters[2].process(&tankSig2, &nodeSig, 1); // Damping L

                tankSig2 = nodeSig * decayGainR; // Decay R
                diffusers[7].process(&tankSig2, &nodeSig, 1); tankSig2 = nodeSig; // Decay diffusion 2L
                nodeSig = delayLines[4].read(); delayLines[4].write(tankSig2); tankSig2 = nodeSig; // END

                // Mixdown output taps
                const float tapGain = 0.6f;
                float accumulatorL = tapGain * delayLines[3].read(266.0f);
                accumulatorL += tapGain * delayLines[3].read(2974.0f);
                accumulatorL -= tapGain * diffusers[7].readTap(1913.0f);
                accumulatorL += tapGain * delayLines[4].read(1996.0f);
                accumulatorL -= tapGain * delayLines[1].read(1990.0f);
                accumulatorL -= tapGain * diffusers[6].readTap(187.0f);
                accumulatorL -= tapGain * delayLines[2].read(1066.0f);

                float accumulatorR = tapGain * delayLines[1].read(353.0f);
                accumulatorR += tapGain * delayLines[1].read(3627.0f);
                accumulatorR -= tapGain * diffusers[6].readTap(1228.0f);
                accumulatorR += tapGain * delayLines[2].read(2673.0f);
                accumulatorR -= tapGain * delayLines[3].read(2111.0f);
                accumulatorR -= tapGain * diffusers[7].readTap(335.0f);
                accumulatorR -= tapGain * delayLines[4].read(121.0f);

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
            float attack = 100.0f, float release = 100.0f, float makeupGain = 0.0f, bool autoMakeup = false) : inBuffer(1.0f, L + 1.0f) {
            setMix(mix); setThreshold(threshold); setRatio(ratio); setKnee(knee); 
            setAttackTime(attack); setReleaseTime(release); setMakeupGain(makeupGain); setAutoMakeup(autoMakeup);
            inBuffer.setDelayTime(L);
            makeupCoeff = exp(-2.2f / (100.0f * SAMPLE_RATE / 1000.0f)); // Auto-makeup smoothing factor
        }

        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setThreshold(float threshold) { this->threshold = clamp(threshold, -200.0f, 0.0f); } // dB, [-200.0, 0.0]
        void setRatio(float ratio) { this->ratio = clamp(ratio, 1.0f, 100.0f); } // [1.0, 100.0]
        void setKnee(float knee) { this->knee = clamp(knee, 0.0f, 40.0f); }// [0.0, 40.0]
        void setAttackTime(float attackTime) { attackCoeff = exp(-2.2f / (clamp(attackTime, 0.005f, 250.0f) * SAMPLE_RATE / 1000.0f)); } // ms, [0.005, 250.0]
        void setReleaseTime(float releaseTime) { releaseCoeff = exp(-2.2f / (clamp(releaseTime, 10.0f, 2500.0f) * SAMPLE_RATE / 1000.0f)); } // ms, [10.0, 2500.0]
        void setMakeupGain(float makeupGain) { this->makeupGain = clamp(makeupGain, -72.0f, 36.0f); } // dB, [-72.0, 36.0]
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
                rms = sqrt((rms * rms) + (((x_i * x_i) - (x_L * x_L)) / L_samples));
                float rmsDB = ampDB(max(rms, 1e-6f));

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
                float g = dbAmp(clamp(gainSmoothed + makeupDB, -60.0f, 20.0f));
                float y_i = x_i * g;

                // Hard clip just in case (e.g., limiter use case)
                if (y_i > 1.0f) { y_i = 1.0f; }
                else if (y_i < -1.0f) { y_i = -1.0f; }

                out[i] = dryWetMix(x_L, y_i, mix); // Mix
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
        envelopeType envType;
        
        vector<float> inBuf; size_t writePos = 0;
        float grainCounter = 0.0f;
        
        struct Grain {
            bool active = false;
            uint32_t startPos;
            uint32_t playhead;
            uint32_t length;
            float level;
            bool reverse;
        };
        
        vector<Grain> grains; // Grain pool

        void spawnGrain() {
            // Find free grain
            Grain* freeGrain = nullptr;
            for (auto& grain : grains) if (!grain.active) { freeGrain = &grain; break; }
            // If none, steal oldest grain
            if (freeGrain == nullptr) {
                for (auto& grain : grains) if (grain.active) { freeGrain = &grain; break; }
            }
            
            // Calculate grain parameters
            float positionFactor = clamp(position + (uniform() * positionRand), 0.0f, 1.0f);
            int grainStartPos = (int)writePos - (int)(positionFactor * (bufSize - 1));
            if (grainStartPos < 0) grainStartPos += bufSize;
            
            float grainLength = clamp(length * (1.0f + (uniform() * lengthRand)), 5.0f, 1000.0f);
            int grainLengthSamples = min((int)(grainLength * SAMPLE_RATE / 1000.0f), (int)bufSize - 1);

            float grainLevel = clamp(level + (uniform() * 0.25f * levelRand), 0.0f, 1.0f) * 0.5f;
            
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
            
            float envelopeValue = getEnvelopeValue((float)grain.playhead / (float)grain.length, grain.length, envType);
            
            ++grain.playhead;
            if (grain.playhead >= grain.length) grain.active = false; // Free if done
            
            return inBuf[readPos] * envelopeValue * grain.level;
        }

    public:
        Granulator(float mix = 1.0f, float position = 0.5f, float positionRand = 0.5f, float time = 50.0f, float timeRand = 0.0f, 
            float length = 200.0f, float lengthRand = 0.0f, float level = 0.8f, float levelRand = 0.0f, 
            float reverseChance = 0.0f, envelopeType envType = HANN) {
            setMix(mix); setPosition(position); setPositionRand(positionRand); setRate(time); setRateRand(timeRand); setLength(length); 
            setLengthRand(lengthRand); setLevel(level); setLevelRand(levelRand); setReverseChance(reverseChance); setEnvelopeType(envType);
            grains.resize(maxGrains);
            inBuf.resize(bufSize, 0.0f);
        }

        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setPosition(float position) { this->position = clamp(position, 0.0f, 1.0f); } // [0.0, 1.0]
        void setPositionRand(float positionRand) { this->positionRand = clamp(positionRand, 0.0f, 1.0f); } // [0.0, 1.0]
        void setRate(float rate) { this->rate = clamp(rate, 1.0f, 500.0f); } // ms, [1.0, 500.0]
        void setRateRand(float rateRand) { this->rateRand = clamp(rateRand, 0.0f, 1.0f); } // [0.0, 1.0]
        void setLength(float length) { this->length = clamp(length, 5.0f, 500.0f); } // ms, [5.0, 500.0]
        void setLengthRand(float lengthRand) { this->lengthRand = clamp(lengthRand, 0.0f, 1.0f); } // [0.0, 1.0]
        void setReverseChance(float reverseChance) { this->reverseChance = clamp(reverseChance, 0.0f, 1.0f); } // [0.0, 1.0]
        void setLevel(float level) { this->level = clamp(level, 0.0f, 1.0f); } // [0.0, 1.0]
        void setLevelRand(float levelRand) { this->levelRand = clamp(levelRand, 0.0f, 1.0f); } // [0.0, 1.0]
        void setEnvelopeType(envelopeType envType) { this->envType = envType; }
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
                case ENVELOPE_TYPE: setEnvelopeType(static_cast<envelopeType>(value)); break;
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
    // HACK: Works, except for negative rates for time-domain mode which produce silence
    private:
        enum Params : ParamID { MIX, RATE, SPECTRAL_MODE, FFT_SIZE, HOP_SIZE, LOOP_START, LOOP_END };

        const size_t bufSize = 3 * (size_t)SAMPLE_RATE; // 3 seconds
        const float smooth = 0.5f;
        
        float mix, rate; bool spectralMode;
        float loopStart, loopEnd;
        
        vector<float> inBuf; size_t writePos = 0; float readPos = 0.0f;
        
        // Spectral resythesis
        STFT stft;
        vector<float> spectBuf, spectFrame;
        size_t spectPos = 0, spectHopCounter = 0;

    public:
        Freezer(float mix = 1.0f, float rate = 1.0f, bool spectralMode = false, size_t fftSize = 1024, size_t hopFactor = 4, 
            float loopStart = 0.0f, float loopEnd = 1.0f) : stft(fftSize, hopFactor) {
            setMix(mix); setRate(rate); setSpectralMode(spectralMode);
            setFFTSize(fftSize); setHopSize(hopFactor); setLoopRegion(loopStart, loopEnd); 
            inBuf.resize(bufSize, 0.0f);
        }
        
        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
        void setRate(float rate) { this->rate = clamp(rate, -4.0f, 4.0f); } // [-4.0, 4.0]
        void setSpectralMode(bool spectralMode) { this->spectralMode = spectralMode; }
        void setFFTSize(size_t N) { // [256, 8192], MUST BE POWER OF 2
            const size_t fftN = clamp(N, (size_t)256, (size_t)8192);
            stft.setFFTSize(fftN); 
            spectBuf.assign(fftN, 0.0f); spectFrame.resize(fftN);
            spectPos = 0; spectHopCounter = 0;
        }
        void setHopSize(size_t hopFactor) { stft.setHopSize(clamp(hopFactor, (size_t)2, (size_t)8)); } // [2, 8]
        void setLoopRegion(float start, float end) { // [0.0, 1.0] for both
            loopStart = clamp(start, 0.0f, 1.0f);
            loopEnd = clamp(end, 0.0f, 1.0f);
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
            const size_t fftN = stft.getFFTSize(), hopN = stft.getHopSize();

            // Compute loop boundaries
            float loopStartSamples = loopStart * (float)bufSize;
            float loopEndSamples = loopEnd * (float)bufSize;
            float loopLength = loopEndSamples - loopStartSamples;
            
            for (size_t i = 0; i < n; ++i) {
                inBuf[writePos] = in[i]; // Write input to circular buffer
                ++writePos; if (writePos >= bufSize) writePos = 0;
                
                float wetSig = 0.0f;
                if (spectralMode) { // Spectral resynthesis mode

                    stft.forward(in[i]); // Forward FFT

                    // Read from OLA buffer
                    wetSig = spectBuf[spectPos];
                    spectBuf[spectPos] = 0.0f;
                    ++spectHopCounter;

                    /* SYNTHESIZE FFT FRAMES */
                    if (spectHopCounter >= hopN) { // Every hopN samples
                        spectHopCounter = 0;
                        // Map readPos to FFT frame index
                        float framePos = fmod(readPos / (float)hopN, (float)stft.getSpectSize());
                        if (framePos < 0) framePos += stft.getSpectSize();
                        // Interpolate spectral frame
                        STFT::FFTFrame interpFrame = stft.interpolateFrame(framePos);
                        // IFFT
                        stft.getFFT().inverse(interpFrame.mag.data(), interpFrame.phase.data(), spectFrame.data());
                        // Window and OLA
                        overlapAdd(spectBuf, spectFrame, HANN, spectPos);
                    }
                    
                    ++spectPos; if (spectPos >= fftN) spectPos = 0;
                }

                // Advance and wrap read pos
                readPos += rate;
                if (readPos < loopStartSamples) { readPos += loopLength; }
                else if (readPos >= loopEndSamples) { readPos -= loopLength; }
                float loopPos = (readPos - loopStartSamples) / loopLength; // Map to loop pos

                if (!spectralMode) wetSig = lerp(inBuf, readPos, bufSize); // Time domain mode

                float crossfade = 1.0f;
                if (rate != 0.0f) {
                    if (loopPos < smooth) { // Fade in
                        float t = loopPos / smooth; // 0..1
                        crossfade = getEnvelopeValue(t, 1, HANN);
                    } else if (loopPos > (1.0f - smooth)) { // Fade out
                        float t = (loopPos - (1.0f - smooth)) / smooth;
                        crossfade = getEnvelopeValue(1.0f - t, 1, HANN);
                    }
                } wetSig *= crossfade;
            
                out[i] = dryWetMix(in[i], wetSig, mix); // Mix
            }
        }
};

/* SPECTRAL */

class SpectralGate : public Spectral_Effect {
    private:
        enum Params : ParamID { THRESHOLD = 1, TILT };

        float threshold, tilt;
        
    public:
        SpectralGate(float mix = 1.0f, float thresholdDB = -10.0f, float tilt = 0.5f, int fftSize = 1024) : Spectral_Effect(mix, fftSize)
            { setThreshold(thresholdDB); setTilt(tilt); }
        
        void setThreshold(float thresholdDB) { threshold = dbAmp(clamp(thresholdDB, -100.0f, 0.0f)); } // dB, [-100.0, 0.0]
        void setTilt(float tilt) { this->tilt = clamp(tilt, -1.0f, 1.0f) * 2.0f; } // [-1.0, 1.0], + gates lows more, - gates highs more
        inline void setParam(ParamID param, float value) override {
            switch (param) {
                case THRESHOLD: setThreshold(value); break;
                case TILT: setTilt(value); break;
                default: Spectral_Effect::setParam(param, value);
            }
        }

    protected:
        void processSpectrum(float* mag, float* /*phs*/, size_t numBins) override {
            const float binTilt = tilt / (float)(numBins - 1);
            for (size_t k = 0; k < numBins; ++k) {
                float weight = 1.0f + ((k * binTilt) - (tilt * 0.5f));
                if (weight < 0.0f) weight = 0.0f;

                if ((mag[k] * weight) < threshold) mag[k] = 0.0f;
            }
        }
};

#endif // EFFECTS