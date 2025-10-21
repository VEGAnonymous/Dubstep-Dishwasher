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
        float gainFactor;
    public:
        Gain(float gainFactor) : gainFactor(gainFactor) {}

        void setGain(float gainFactor) { this->gainFactor = gainFactor; }
        inline void setParam(const string& name, float value) override {
            if (name == "Gain") { setGain(value); }
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
        distortionMode mode;
        float mix, drive;
        bool enableAAF;

        float (Distortion::*algorithm)(float, float) = nullptr; // Function pointer for distortion algorithm to use
        FIR_Filter antiAlias;

        // Distortion algorithms
        // https://www.desmos.com/calculator/qrqipgp7r4
        float tube(float in, float drive) {
            float x = in * (4.0f + (drive * 36.0f)); // d -> [4, 40]
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
        Distortion(float mix, distortionMode mode, float drive, bool enableAAF) : antiAlias(1.0f, vector<float>(begin(AAF), end(AAF))) 
        { setMix(mix); setMode(mode); setDrive(drive); setAAF(enableAAF); }

        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); }
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
        void setDrive(float drive) { this->drive = clamp(drive, 0.0f, 1.0f); }
        void setAAF(bool enableAAF) { this->enableAAF = enableAAF; }
        inline void setParam(const string& name, float value) override {
            if (name == "Mix") { setMix(value); }
            else if (name == "Mode") { setMode((distortionMode)value); }
            else if (name == "Drive") { setDrive(value); }
            else if (name == "AAF") { setAAF(value); }
        }

        void process(const float* in, float* out, size_t n) override {
            const float* in_ptr = in;
            float* out_ptr = out;

            float distortSig, wetSig;
            for (size_t i = 0; i < n; ++i) {
                wetSig = (this->*algorithm)(*in_ptr, drive) * dbAmp(-0.3f); // Apply non-linearity
                if (enableAAF) antiAlias.process(&wetSig, &wetSig, 1); // Optional AAF (~0.8s processing time)
                *out_ptr++ = lerp(*in_ptr++, wetSig, mix); // Mix
            }
        }
};

class Delay : public Effect {
    private:
        float mix, feedback;
        DelayLine delayLine;

        float delayBuffer[BUFFER_SIZE], feedbackBuffer[BUFFER_SIZE];
    public:
        Delay(float mix, float delayTime, float maxDelayTime, float feedback) : delayLine(delayTime, maxDelayTime) { 
            setMix(mix); setFeedback(feedback); }

        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); }
        void setDelayTime(float delayTime) { delayLine.setDelayTime(delayTime); }
        void setFeedback(float feedback) { this->feedback = clamp(feedback, -0.95f, 0.95f); }
        inline void setParam(const string& name, float value) override {
            if (name == "Mix") { setMix(value); }
            else if (name == "Time") { setDelayTime(value); }
            else if (name == "Feedback") { setFeedback(value); }
        }

        void process(const float* in, float* out, size_t n) override {
            const float* in_ptr = in;
            float* out_ptr = out;

            float delaySig;
            for (size_t i = 0; i < n; ++i) {
                delaySig = delayLine.read(); // Read from delay line
                delayLine.write(*in_ptr + (delaySig * feedback)); // Feedback and write new sample
                *out_ptr++ = lerp(*in_ptr++, delaySig, mix); // Mix
            }
        }
};

class Flanger : public Effect {
    private:
        DelayLine delayLine;
        Wavetable LFO;
        float mix, rate, depth, feedback;
        float wetSig = 0;
    public:
        Flanger(float mix, float rate, float depth, float feedback) : delayLine(15, 30), LFO(rate, SINE_TABLE) 
        { setMix(mix); setRate(rate); setDepth(depth); setFeedback(feedback); }

        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); }
        void setRate(float rate) { this->rate = clamp(rate, 0.0f, 20.0f); LFO.setFreq(rate); }
        void setDepth(float depth) { this->depth = clamp(depth, 0.0f, 1.0f); }
        void setFeedback(float feedback) { this->feedback = clamp(feedback, -0.95f, 0.95f); }
        inline void setParam(const string& name, float value) override {
            if (name == "Mix") { setMix(value); }
            else if (name == "Rate") { setRate(value); }
            else if (name == "Depth") { setDepth(value); }
            else if (name == "Feedback") { setFeedback(value); }
        }

        void process(const float* in, float* out, size_t n) override {
            const float* in_ptr = in;
            float* out_ptr = out;

            for (size_t i = 0; i < n; ++i) {
                delayLine.setDelayTime(15 + (LFO.next() * 10.0f * depth)); // Modulate delay with LFO, 5-25 ms
                delayLine.write(*in_ptr + (feedback * wetSig));
                
                wetSig = delayLine.read();
                *out_ptr++ = lerp(*in_ptr++, wetSig, mix); // Mix
            }
        }
};

class Phaser : public Effect {
    private:
        float mix, rate, centerFreq, spread, depth, feedback;
        const uint8_t order;

        vector<APF> apfSections; // APF bank
        vector<float> baseFreqs; // Store APF base freqs
        float wetSig = 0.0f, stageSig = 0.0f;
        Wavetable LFO;
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
        Phaser(float mix, float rate, float centerFreq, float spread, float depth, float feedback, const uint8_t order = 4, const float q = 0.8f) 
        : order(order), LFO(rate, SINE_TABLE) {
            for (size_t i = 0; i < order; ++i) { apfSections.push_back(APF(1.0f, centerFreq, q, false, 1000)); }
            setupStages();
            setMix(mix); setRate(rate); setCenter(centerFreq); setSpread(spread), setDepth(depth); setFeedback(feedback);
        }

        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); }
        void setRate(float rate) { this->rate = clamp(rate, 0.0f, 20.0f); LFO.setFreq(rate); }
        void setCenter(float centerFreq) { this->centerFreq = clamp(centerFreq, 50.0f, 8000.0f); setupStages(); }
        void setSpread(float spread) { this->spread = clamp(spread, 0.1f, 1.0f); setupStages(); }
        void setDepth(float depth) { this->depth = clamp(depth, 0.0f, 1.0f); }
        void setFeedback(float feedback) { this->feedback = clamp(feedback, -0.95f, 0.95f); }
        inline void setParam(const string& name, float value) override {
            if (name == "Mix") { setMix(value); }
            else if (name == "Rate") { setRate(value); }
            else if (name == "Center") { setCenter(value); }
            else if (name == "Spread") { setSpread(value); }
            else if (name == "Depth") { setDepth(value); }
            else if (name == "Feedback") { setFeedback(value); }
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

                out[i] = lerp(in[i], wetSig, mix); // Mix
            }
        }
};

class Chorus : public Effect {
    private:
        float mix, rate, depth, delayTime, feedback;
        const uint8_t voiceCount; // 1-5
        struct voice {
            float mix, depth, baseDelay;
            Random mod;
        };

        vector<voice> voices;
        DelayLine delayLine;
    public:
        Chorus(float mix, float rate, float depth, float delayTime, float feedback, uint8_t voiceCount) : delayLine(delayTime, 50.0f * SAMPLE_RATE / 1000.0f), 
        voiceCount(voiceCount) {
            for (size_t i = 0; i < voiceCount; ++i) voices.push_back({1.0f / (float)voiceCount, depth, delayTime, Random(rate, PERLIN)});
            setMix(mix); setRate(rate); setDepth(depth); setDelayTime(delayTime); setFeedback(feedback);
        }

        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); }
        void setRate(float rate) { 
            this->rate = clamp(rate, 0.0f, 20.0f); 
            for (voice& vc : voices) vc.mod.setFreq(rate);
        }
        void setDepth(float depth) { 
            this->depth = clamp(depth, 0.0f, 1.0f);
            for (voice& vc : voices) vc.depth = depth; 
        }
        void setDelayTime(float delayTime) { 
            this->delayTime = delayTime; 
            for (size_t i = 0; i < voiceCount; ++i) { 
                float detune = ((float)i / voiceCount - 0.5f) * 2.0f; // [-1,1]
                voices[i].baseDelay = delayTime * (1.0f + 0.3f * detune);
            };
        }
        void setFeedback(float feedback) { this->feedback = clamp(feedback, -0.95f, 0.95f); }
        inline void setParam(const string& name, float value) override {
            if (name == "Mix") { setMix(value); }
            else if (name == "Rate") { setRate(value); }
            else if (name == "Depth") { setDepth(value); }
            else if (name == "Delay") { setDelayTime(value); }
            else if (name == "Feedback") { setFeedback(value); }
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
                *out_ptr++ = lerp(*in_ptr++, wetSig, mix); // Mix
            }
        }
};

class Reverb : public Effect { 
    // Datarro reverb algorithm 
    // https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf
    private:
        float mix, predelayTime, decayTime, decayGainL, decayGainR, modRate, modDepth, damping;
        vector<APF> diffusers; // 8
        vector<OnePole> filters; // 3
        vector<DelayLine> delayLines; // 5
        Wavetable LFO;
        float tankInSig, nodeSig, tankSig1 = 0, tankSig2 = 0;

    public:
        Reverb(float mix, float predelayTime, float decayTime, float modRate, float modDepth, float damping = 0.0005f) : LFO(modRate, SINE_TABLE) {
            const float inputDiffuse[2] = {0.750f, 0.625f};
            const float decayDiffuse[2] = {0.70f, 0.50f};
            const float apfDelays[8] = {142, 107, 379, 277, 672, 908, 1800, 2656};
            const float delays[5] = {predelayTime, 100.97f, 84.35f, 95.62f, 71.72f};
            const float bandwidth = 0.9995f;

            // Diffusers
            size_t apf_i = 0;
            for (float diff : inputDiffuse) {
                for (size_t i = 0; i < 2; ++i) { diffusers.push_back(APF(1.0f, 0.0f, 0.0f, false, 100.0f)); 
                diffusers.back().setDelay(apfDelays[apf_i++]); diffusers.back().setQ(1.0f / (1.0f - diff)); };
            }
            for (float diff : decayDiffuse) {
                for (size_t i = 0; i < 2; ++i) { diffusers.push_back(APF(1.0f, 0.0f, 0.0f, false, 100.0f));
                diffusers.back().setDelay(apfDelays[apf_i++]); diffusers.back().setQ(1.0f / (1.0f - diff)); };
            } diffusers[4].setInvert(true); diffusers[5].setInvert(true);

            // Filters
            filters.push_back(OnePole(1.0f, bandwidth * SAMPLE_RATE / 2.0f));
            for (size_t i = 0; i < 2; ++i) { 
                filters.push_back(OnePole(1.0f, damping * 10000.0f));
            }

            // Delays
            for (float delay : delays) { delayLines.push_back(DelayLine(delay, delay + 150.0f)); }

            setMix(mix); setPredelay(predelayTime); setDecay(decayTime); setDamping(damping);
        }

        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); }
        void setPredelay(float predelayTime) { this->predelayTime = predelayTime; delayLines.front().setDelayTime(predelayTime); }
        void setDecay(float decayTime) {
            this->decayTime = clamp(decayTime, 100.0f, 10000.0f);
            float RT60 = this->decayTime / 1000.0f;
            decayGainL = powf(0.001f, 4648.0f / (RT60 * SAMPLE_RATE));
            decayGainR = powf(0.001f, 4924.0f / (RT60 * SAMPLE_RATE));
        }
        void setModRate(float modRate) { this->modRate = clamp(modRate, 0.05f, 5.0f); LFO.setFreq(this->modRate); }
        void setModDepth(float modDepth) { this->modDepth = clamp(modDepth, 0.0f, 1.0f); }
        void setDamping(float damping) {
            this->damping = clamp(damping, 0.0f, 1.0f); 
            float cutoffFreq = 2000.0f + (damping * 10000.0f);
            for (size_t i = 1; i < 3; ++i) filters[i].setCutoff(cutoffFreq);
        }
        inline void setParam(const string& name, float value) override {
            if (name == "Mix") { setMix(value); }
            else if (name == "Predelay") { setPredelay(value); }
            else if (name == "Decay") { setDecay(value); }
            else if (name == "Mod Rate") { setModRate(value); }
            else if (name == "Mod Depth") { setModDepth(value); }
            else if (name == "Damping") { setDamping(value); } // DO NOT USE IN PRACTICE
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
                out[i] = lerp(in[i], wetSig, mix); // Total mix
            }
        }
};

class Compressor : public Effect {
    private:
        float mix, threshold, ratio, knee, makeupGain; bool autoMakeup;
        const float L = 50.0f, L_samples = L * SAMPLE_RATE / 1000.0f; // RMS window size

        float rms = 0.0f; DelayLine inBuffer;  
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
        Compressor(float mix, float threshold, float ratio, float knee, float attack, float release, float makeupGain, bool autoMakeup) 
        : inBuffer(1.0f, L + 1.0f) {
            setMix(mix); setThreshold(threshold); setRatio(ratio); setKnee(knee); 
            setAttack(attack); setRelease(release); setMakeup(makeupGain); setAutoMakeup(autoMakeup);
            inBuffer.setDelayTime(L);
            makeupCoeff = exp(-2.2f / (100.0f * SAMPLE_RATE / 1000.0f)); // Auto-makeup smoothing factor
        }

        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); }
        void setThreshold(float threshold) { this->threshold = clamp(threshold, -200.0f, 0.0f); } // dB
        void setRatio(float ratio) { this->ratio = clamp(ratio, 1.0f, 100.0f); }
        void setKnee(float knee) { this->knee = clamp(knee, 0.0f, 40.0f); }
        void setAttack(float attack) { attackCoeff = exp(-2.2f / (attack * SAMPLE_RATE / 1000.0f)); }
        void setRelease(float release) { releaseCoeff = exp(-2.2f / (release * SAMPLE_RATE / 1000.0f)); }
        void setMakeup(float makeupGain) { this->makeupGain = makeupGain; } // dB
        void setAutoMakeup(bool autoMakeup) { 
            this->autoMakeup = autoMakeup;
            if (autoMakeup) { reductionSmoothed = 0.0f; autoMakeupGain = 0.0f; }
        }
        inline void setParam(const string& name, float value) override {
            if (name == "Mix") { setMix(value); }
            else if (name == "Threshold") { setThreshold(value); }
            else if (name == "Ratio") { setRatio(value); }
            else if (name == "Knee") { setKnee(value); }
            else if (name == "Attack") { setAttack(value); }
            else if (name == "Release") { setRelease(value); }
            else if (name == "Makeup") { setMakeup(value); }
            else if (name == "Auto Makeup") { setAutoMakeup(value > 0.5f); }
        }

        void process(const float* in, float* out, size_t n) override {
            for (size_t i = 0; i < n; ++i) {
                // Compute RMS recursively
                const float x_i = in[i], x_L = inBuffer.read();
                inBuffer.write(x_i); 
                rms = sqrt((rms * rms) + (((x_i * x_i) - (x_L * x_L)) / L_samples));
                float rmsDB = ampDB(rms);

                // Gain computation
                float gDB = 0.0f;
                if (rmsDB < (threshold - (knee / 2.0f))) { gDB = rmsDB; } // Below threshold, linear
                else if (rmsDB > (threshold + (knee / 2.0f))) { gDB = threshold + ((rmsDB - threshold) / ratio); } // Above threshold, attenuate
                else { gDB = rmsDB + ((((1.0f / ratio) - 1.0f) * pow(rmsDB - threshold + (knee / 2.0f), 2.0f)) / (2.0f * knee)); } // Soft knee

                gDB -= rmsDB;

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

                // Mix
                float y_i = x_L * g;
                out[i] = lerp(x_L, y_i, mix);
            }
        }
};

class Granulator : public Effect {
    // HACK: Works, but produces clicks with larger positionRand since grains may end up reading from indices overwritten by write head
    // Non-clicking usage: No reversed grains + any positionRand, OR reversed grains + no/small positionRand
    private:
        const size_t bufSize = 3 * SAMPLE_RATE; // 3 seconds
        const int maxGrains = 64;
        
        float mix, position, time, length, reverseChance;
        float positionRand, timeRand, lengthRand;
        envelopeType envType; vector<float> envelope;
        
        vector<float> inBuf; size_t writePos = 0;
        float grainCounter = 0.0f;
        
        struct Grain {
            bool active = false;
            int startPos;
            int playhead;
            int length;
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
            
            bool reversed = ((uniform() + 1.0f) / 2.0f) <= reverseChance;
            
            // Init
            freeGrain->active = true;
            freeGrain->startPos = grainStartPos;
            freeGrain->playhead = 0;
            freeGrain->length = grainLengthSamples;
            freeGrain->reverse = reversed;
        }

        float processGrain(Grain& grain) {
            // Calculate read position in input buffer
            int offset = grain.reverse ? ((grain.length - 1) - grain.playhead) : grain.playhead;
            int readPos = (grain.startPos + offset) % bufSize;
            
            // Apply envelope
            float envelopeValue = 1.0f;
            if (!envelope.empty()) {
                int envIndex = clamp((int)(((float)grain.playhead / (float)grain.length) * envelope.size()), 0, (int)envelope.size() - 1);
                envelopeValue = envelope[envIndex];
            }
            
            ++grain.playhead;
            if (grain.playhead >= grain.length) grain.active = false; // Free if done
            
            return inBuf[readPos] * envelopeValue;
        }

    public:
        Granulator(float mix, float position, float positionRand, float time, float timeRand, 
            float length, float lengthRand, float reverseChance, envelopeType envType) {
            setMix(mix); setPosition(position); setPositionRand(positionRand); setTime(time); setTimeRand(timeRand);
            setLength(length); setLengthRand(lengthRand); setReverseChance(reverseChance); setEnvelopeType(envType);
            grains.resize(maxGrains);
            inBuf.resize(bufSize, 0.0f);
        }

        void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); }
        void setPosition(float position) { this->position = clamp(position, 0.0f, 1.0f); }
        void setPositionRand(float positionRand) { this->positionRand = clamp(positionRand, 0.0f, 1.0f); }
        void setTime(float time) { this->time = clamp(time, 1.0f, 3000.0f); }
        void setTimeRand(float timeRand) { this->timeRand = clamp(timeRand, 0.0f, 1.0f); }
        void setLength(float length) {
            this->length = clamp(length, 5.0f, 1000.0f);
            int lengthSamples = (int)(this->length * SAMPLE_RATE / 1000.0f);
            makeEnvelope(envelope, lengthSamples, envType);
        }
        void setLengthRand(float lengthRand) { this->lengthRand = clamp(lengthRand, 0.0f, 1.0f); }
        void setReverseChance(float reverseChance) { this->reverseChance = clamp(reverseChance, 0.0f, 1.0f); }
        void setEnvelopeType(envelopeType envType) { 
            this->envType = envType; 
            int lengthSamples = (int)(this->length * SAMPLE_RATE / 1000.0f);
            makeEnvelope(envelope, lengthSamples, envType);
        }
        inline void setParam(const string& name, float value) override {
            if (name == "Mix") { setMix(value); }
            else if (name == "Position") { setPosition(value); }
            else if (name == "Position Random") { setPositionRand(value); }
            else if (name == "Time") { setTime(value); }
            else if (name == "Time Random") { setTimeRand(value); }
            else if (name == "Length") { setLength(value); }
            else if (name == "Length Random") { setLengthRand(value); }
            else if (name == "Reverse Chance") { setReverseChance(value); }
            else if (name == "Envelope Type") { setEnvelopeType((envelopeType)value); }
        }

    void process(const float* in, float* out, size_t n) override {
        for (size_t i = 0; i < n; ++i) {
            inBuf[writePos] = in[i]; // Write input to circular buffer
            ++grainCounter;
            
            float timeSamples = (time + (time * timeRand * uniform())) * SAMPLE_RATE / 1000.0f;
            if (grainCounter >= timeSamples) { // Time to spawn a grain!
                spawnGrain();
                grainCounter = 0.0f;
            }
            
            // Process active grains and accumulate output
            float wetSig = 0.0f;
            for (auto& grain : grains) if (grain.active) { wetSig += processGrain(grain); }
            out[i] = ((1.0f - mix) * in[i]) + (mix * wetSig); // Mix

            ++writePos;
            if (writePos >= bufSize) writePos = 0;
        }
    }
};

class Freezer : public Effect {
private:
    const size_t bufSize = 3 * SAMPLE_RATE; // 3 seconds
    const float smooth = 0.5f;
    
    float mix, rate; bool spectralMode;
    float loopStart, loopEnd;
    
    vector<float> inBuf; size_t writePos = 0; float readPos = 0.0f;
    
    vector<float> crossfadeEnv; // Crossfade envelope
    
    // Spectral resythesis
    STFT stft;
    vector<float> spectBuf, spectFrame;
    size_t spectPos = 0, spectHopCounter = 0;

public:
    Freezer(float mix, float rate, bool spectralMode, size_t fftSize, size_t hopFactor = 4, 
        float loopStart = 0.0f, float loopEnd = 1.0f) : stft(fftSize, 4) {
        setMix(mix); setRate(rate); setSpectralMode(spectralMode); 
        setFFTSize(fftSize); setHopSize(hopFactor); setLoopRegion(loopStart, loopEnd); 
        inBuf.resize(bufSize, 0.0f);

        // Create crossfade envelope
        float crossfadeFrames = 1.0f + (smooth * smooth * 999.0f);
        size_t envSize = (size_t)(crossfadeFrames * 2.0f);
        makeEnvelope(crossfadeEnv, envSize, HANN);
    }
    
    void setMix(float mix) { this->mix = clamp(mix, 0.0f, 1.0f); }
    void setRate(float rate) { this->rate = clamp(rate, -4.0f, 4.0f); }
    void setSpectralMode(bool spectralMode) { this->spectralMode = spectralMode; }
    void setFFTSize(size_t N) { 
        stft.setFFTSize(N); 
        spectBuf.assign(N, 0.0f); spectFrame.resize(N);
        spectPos = 0; spectHopCounter = 0;
    }
    void setHopSize(size_t hopFactor) { stft.setHopSize(max(hopFactor, (size_t)2)); }
    void setLoopRegion(float start, float end) {
        loopStart = clamp(start, 0.0f, 1.0f);
        loopEnd = clamp(end, 0.0f, 1.0f);
        if (loopStart >= loopEnd) { // start < end
            loopEnd = loopStart + 0.01f;
            if (loopEnd > 1.0f) { loopEnd = 1.0f; loopStart = 0.99f; }
        }
        readPos = loopStart * (float)bufSize;
    }
    inline void setParam(const string& name, float value) override {
        if (name == "Mix") { setMix(value); }
        else if (name == "Rate") { setRate(value); }
        else if (name == "Spectral Mode") { setSpectralMode(value > 0.5f); }
        else if (name == "FFT Size") { setFFTSize((size_t)value); }
        else if (name == "Hop Size") { setHopSize((size_t)value); }
        else if (name == "Loop Start") { setLoopRegion(value, loopEnd); }
        else if (name == "Loop End") { setLoopRegion(loopStart, value); }
    }
    
    void process(const float* in, float* out, size_t n) override {
        const size_t fftN = stft.getFFTSize(), hopN = stft.getHopSize();
        const auto& window = stft.getWindow();

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
                    overlapAdd(spectBuf, spectFrame, window, spectPos);
                }
                
                ++spectPos; if (spectPos >= fftN) spectPos = 0;
                
            } else { wetSig = lerp(inBuf, readPos, bufSize); } // Time-domain mode

            // Advance and wrap read pos
            readPos += rate;
            if (readPos < loopStartSamples) { readPos += loopLength; }
            else if (readPos >= loopEndSamples) { readPos -= loopLength; }
            
            float loopPos = (readPos - loopStartSamples) / loopLength; // Map to loop pos

            // Calculate and apply crossfade envelope
            float crossfade = 1.0f;
            if (rate != 0.0f) {
                float fadeSize = (float)crossfadeEnv.size() / (2.0f * loopLength);
                if (loopPos < fadeSize) { // Fade in
                    float envIndex = (loopPos / fadeSize) * (crossfadeEnv.size() / 2.0f);
                    crossfade = lerp(crossfadeEnv, envIndex, crossfadeEnv.size());
                } else if (loopPos > (1.0f - fadeSize)) { // Fade out
                    float t = (loopPos - (1.0f - fadeSize)) / fadeSize;
                    float envIndex = (crossfadeEnv.size() / 2.0f) + (t * (crossfadeEnv.size() / 2.0f));
                    crossfade = lerp(crossfadeEnv, envIndex, crossfadeEnv.size());
                }
            } wetSig *= crossfade;
        
            out[i] = lerp(in[i], wetSig, mix); // Mix
        }
    }
};

/* SPECTRAL */

class SpectralGate : public Spectral_Effect {
    private:
        float threshold;
    public:
        SpectralGate(float mix, float thresholdDB, int fftSize) : Spectral_Effect(mix, fftSize)
            { setThreshold(thresholdDB); }
        
        void setThreshold(float thresholdDB) { threshold = dbAmp(thresholdDB); }
        inline void setParam(const string& name, float value) override {
            if (name == "Threshold") { setThreshold(value); }
            else { Spectral_Effect::setParam(name, value); }
        }
    protected:
        void processSpectrum(float* mag, float* phs, size_t numBins) override {
            for (size_t k = 0; k < numBins; ++k) {
                if (mag[k] < threshold) mag[k] = 0.0f; 
            }
        }
};

#endif // EFFECTS