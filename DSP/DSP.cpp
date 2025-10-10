/* CHANGELOG

10-9-25:
- Initial commit

10-10-25:
- Implemented Chorus, FIR_Filter effects
- Implemented Random generator with three modes: perlin (smooth) noise, sample and hold, binary
- Refactored Flanger to use DelayLine instead of Delay
- Added (optional) anti-aliasing FIR filter to Distortion
- New Distortion algorithms: Tube, diode, rectifier

*/

#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <cmath>
#include <numeric>

#include <Filters.h>
#include <Wavetables.h>

// For testing; irrelevant to embedded implementation
#include <iostream>
#include <cstring>
#include <chrono>
#include <sndfile.h>

using namespace std;

#define BUFFER_SIZE 256
#define SAMPLE_RATE 44100
#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

enum randomMode { PERLIN, SAMPLE_HOLD, BINARY };
enum distortionMode { TUBE, SOFT_CLIP, HARD_CLIP, DIODE, BITCRUSH, RECTIFY };
enum wavetable { SINE_TABLE, TRI_TABLE, SAW_TABLE, SQUARE_TABLE };

/* UTILITIES */

float dbAmp(float dB) { return powf(10.0f, dB / 20.0f); }

class DelayLine { // Implements z^-N
    private:
        float delaySamples; 
        int writeIndex;
        vector<float> buffer;

        inline float interpolate(float readIndex) { // Linearly interpolate (fractional delay)
            int size = buffer.size();
            if (readIndex < 0) readIndex += size;

            int i0 = ((int)floor(readIndex)) % size;
            int i1 = (i0 + 1) % size;
            float frac = readIndex - floor(readIndex);

            return buffer[i0] + (frac * (buffer[i1] - buffer[i0]));
        }

    public:
        DelayLine(float delayTime, float maxDelayTime) : writeIndex(0) {
            int maxDelaySamples = (int)((maxDelayTime * SAMPLE_RATE) / 1000.0f);
            buffer.assign(maxDelaySamples + 1, 0.0f);
            setDelayTime(delayTime);
        }

        void setDelayTime(float delayTime) { delaySamples = (delayTime * SAMPLE_RATE) / 1000.0f; }
        void setDelaySamples(float delaySamples) { this->delaySamples = delaySamples; }
        int getSize() const { return buffer.size(); }

        inline float read(float offset = -1.0f) {
            float readOffset = (offset >= 0.0f) ? offset : delaySamples;
            float readIndex = (float)writeIndex - readOffset;
            if (readIndex < 0) readIndex += buffer.size();
            return interpolate(readIndex);
        }

        inline void write(float in) { 
            buffer[writeIndex] = in;
            if (++writeIndex >= buffer.size()) writeIndex = 0;
        }
};

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
        virtual void generate(float* out, size_t n) {  // Generate block of samples
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

/* GENERATORS */

class Wavetable : public Generator {
    private:
        float freq;
        const float* table; // Pointer to wavetable array (eventually stored in PROGMEM)
        uint32_t phaseAccumulator, phaseIncrement;

        static constexpr uint8_t INDEX_SHIFT = 32 - 9; // 9 = log2(512)
    public:
        Wavetable(float freq, wavetable table) : phaseAccumulator(0) { setFreq(freq); setTable(table); }

        void setFreq(float freq) { this->freq = freq; phaseIncrement = freq * (pow(2, 32) / SAMPLE_RATE); }
        void setTable(wavetable table) {
            switch (table) {
                case SINE_TABLE: this->table = SineTable; break;
                case TRI_TABLE: this->table = TriTable; break;
                case SAW_TABLE: this->table = SawTable; break;
                case SQUARE_TABLE: this->table = SquareTable; break;
                default: this->table = SineTable;
            }
        };

        float next() override { // Use fixed-point phase accumulator to index wavetable
            uint16_t index = phaseAccumulator >> INDEX_SHIFT; // Index with 9 MSBs (512)
            phaseAccumulator += phaseIncrement;
            return table[index];
        }
};

class Random : public Generator {
    private:
        float freq, phase = 0.0f, currentVal = 0.0f, nextVal = 0.0f;
        randomMode mode;
        float (Random::*algorithm)() = nullptr;

        float uniform() { return ((float)rand() / RAND_MAX) * 2.0f - 1.0f; } // Random float between [-1, 1]

        // Noise algorithms
        float perlin() {
            phase += freq / SAMPLE_RATE;
            if (phase >= 1.0f) {
                phase -= 1.0f;
                currentVal = nextVal;
                nextVal = uniform();
            }
            float smooth = phase * phase * (3.0f - (2.0f * phase)); // smoothstep(x) -> 3x^2 - 2x^3
            return currentVal + (smooth * (nextVal - currentVal)); // lerp
        }
        float sampleHold() {
            phase += freq / SAMPLE_RATE;
            if (phase >= 1.0f) {
                phase -= 1.0f;
                currentVal = uniform();
            }
            return currentVal;
        }
        float binary() {
            phase += freq / SAMPLE_RATE;
            if (phase >= 1.0f) {
                phase -= 1.0f;
                currentVal = (rand() & 1) ? 1.0f : -1.0f;
            }
            return currentVal;
        }
    public:
        Random(float freq, randomMode mode) { setFreq(freq); setMode(mode); }

        void setFreq(float freq) { this->freq = freq; }
        void setMode(randomMode mode) {
            this->mode = mode;
            switch (mode) {
                case PERLIN: algorithm = &Random::perlin; break;
                case SAMPLE_HOLD: algorithm = &Random::sampleHold; break;
                case BINARY: algorithm = &Random::binary; break;
            }
        }

        float next() override { return (this->*algorithm)(); }
};

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
            // OPTI: Consider adding FFT-based filtering?
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

class LPF : public IIR_Filter { // One pole
    // TODO: Add higher orders (selectable), Q parameterization
    private:
        float b0, a1, y = 0.0f;
        float cutoff;
    public:
        LPF(float mix, float cutoff) { IIR_Filter::mix = mix; setCutoff(cutoff); }

        void setCutoff(float cutoff) {
            this->cutoff = cutoff;
            float x = expf((-2.0f * M_PI * cutoff) / SAMPLE_RATE);
            b0 = 1.0f - x;
            a1 = x;
        }
        void setParam(const string& name, float value) override { 
            IIR_Filter::setParam(name, value);
            if (name == "Cutoff") { setCutoff(value); } 
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
            IIR_Filter::setParam(name, value);
            if (name == "Cutoff") { setCutoff(value); }
            if (name == "Q") { setQ(value); }
        }

        float LCCDE(float x) override {
            // y[n] = -gy[n-N] + gx[n] + x[n-N]
            auto s = invert ? -1 : 1; // Invert sign as needed
            float y = (s * -g * bufferY.read()) + (s * g * x) + bufferX.read();
            bufferX.write(x); bufferY.write(y);
            return y;
        }
};

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
        // TODO: Add more!
        // OPTI: Use LUTs?
        float tube(float in, float drive) {
            float x = in * (1.0f + (drive * 10.0f));
            return atan(x) * (2.0f / M_PI);
        }
        float softClip(float in, float drive) { 
            float d = (1.0f + (drive * 5.0f));
            float x;
            if (in < (-1.0f/d)) { x = -2.0f/3.0f; }
            else if (in > (1.0f/d)) { x = 2.0f/3.0f; }
            else { x = in * d; x = x - ((x * x * x) / 3.0f); } // x - (x^3)/3
            return x * 1.5f;
        } 
        float hardClip(float in, float drive) {
            float x = in * (1.0f + (drive * 10.0f));
            if (x > 1.0f) { x = 1.0f; }
            else if (x < -1.0f) { x = -1.0f; }
            return x;
        }
        float diode(float in, float drive) {
            float x = in * (1.0f + (drive * 10.0f));
            if (x < 0) { x = expf(x) - 1.0f; } // Shockley diode equation, B=1
            else if (x > 0) { x = 1.0 - expf(-x); }
            else { x = 0; }
            return x;
        }
        float bitCrush(float in, float drive) {
            int bitDepth = (int)(2 + ((1.0f - drive) * (1.0f - drive) * 22.0f)); // 24-bit to 2-bit depth
            float levels = (float)(1 << bitDepth); // 2^bits
            return roundf(in * levels) / levels;
        }
        float rectify(float in, float drive) {
            float x = in * (1.0f + (drive * 5.0f));
            x = fabs(x); // Full-wave rectify
            return (x > 1) ? 1.0f : x;
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
                default: algorithm = &Distortion::hardClip;
            }
        }
        void setDrive(float drive) { this->drive = clamp(drive, 0.0f, 1.0f); }
        void setAAF(bool enableAAF) { this->enableAAF = enableAAF; }
        inline void setParam(const string& name, float value) override {
            if (name == "Mix") { setMix(value); }
            if (name == "Mode") { setMode((distortionMode)value); } // HACK: This casting is unsafe but idc lol
            if (name == "Drive") { setDrive(value); }
            if (name == "AAF") { setAAF(value); }
        }

        void process(const float* in, float* out, size_t n) override {
            const float* in_ptr = in;
            float* out_ptr = out;

            float distortSig, wetSig;
            for (size_t i = 0; i < n; ++i) {
                wetSig = (this->*algorithm)(*in_ptr, drive) * dbAmp(-0.3f); // Apply non-linearity
                if (enableAAF) antiAlias.process(&wetSig, &wetSig, 1); // Optional AAF (~0.8s processing time)
                *out_ptr++ = ((1.0f - mix) * *in_ptr++) + (mix * wetSig); // Mix
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
            if (name == "Time") { setDelayTime(value); }
            if (name == "Feedback") { setFeedback(value); }
        }

        void process(const float* in, float* out, size_t n) override {
            const float* in_ptr = in;
            float* out_ptr = out;

            float delaySig;
            for (size_t i = 0; i < n; ++i) {
                delaySig = delayLine.read(); // Read from delay line
                delayLine.write(*in_ptr + (delaySig * feedback)); // Feedback and write new sample
                *out_ptr++ = ((1.0f - mix) * *in_ptr++) + (mix * delaySig); // Mix
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
            if (name == "Rate") { setRate(value); }
            if (name == "Depth") { setDepth(value); }
            if (name == "Feedback") { setFeedback(value); }
        }

        void process(const float* in, float* out, size_t n) override {
            const float* in_ptr = in;
            float* out_ptr = out;

            for (size_t i = 0; i < n; ++i) {
                delayLine.setDelayTime(15 + (LFO.next() * 10.0f * depth)); // Modulate delay with LFO, 5-25 ms
                delayLine.write(*in_ptr + (feedback * wetSig));
                
                wetSig = delayLine.read();
                *out_ptr++ = ((1.0f - mix) * *in_ptr++) + (mix * wetSig); // Mix
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
            if (name == "Rate") { setRate(value); }
            if (name == "Center") { setCenter(value); }
            if (name == "Spread") { setSpread(value); }
            if (name == "Depth") { setDepth(value); }
            if (name == "Feedback") { setFeedback(value); }
        }

        void process(const float* in, float* out, size_t n) override {
            float inSig, mod;
            for (size_t i = 0; i < n; ++i) {
                mod = pow(2.0f, depth * LFO.next()); // OPTI: pow() aint cheap
                inSig = in[i] + (wetSig * feedback);
                
                const float* in_ptr = &inSig;
                float* out_ptr = nullptr;
                for (size_t j = 0; j < apfSections.size(); ++j) { // Process APFs serially, output to wetSig
                    apfSections[j].setCutoff(baseFreqs[j] * mod); // Modulate each base freq with LFO
                    out_ptr = (j == apfSections.size() - 1) ? &wetSig : &stageSig;
                    apfSections[j].process(in_ptr, out_ptr, 1);
                    in_ptr = out_ptr;
                }

                out[i] = ((1.0f - mix) * in[i]) + (mix * wetSig); // Mix
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
        float wetSig = 0.0f;
    public:
        Chorus(float mix, float rate, float depth, float delayTime, float feedback, uint8_t voiceCount) : delayLine(delayTime, 50.0f * SAMPLE_RATE / 1000.0f), 
        voiceCount(voiceCount) {
            for (size_t i = 0; i < voiceCount; ++i) voices.push_back({1.0f / voiceCount, depth, delayTime, Random(rate, PERLIN)});
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
            if (name == "Rate") { setRate(value); }
            if (name == "Depth") { setDepth(value); }
            if (name == "Delay") { setDelayTime(value); }
            if (name == "Feedback") { setFeedback(value); }
        }

        void process(const float* in, float* out, size_t n) override {
            const float *in_ptr = in;
            float *out_ptr = out;
            for (size_t i = 0; i < n; i++) {
                for (voice& vc : voices) {
                    float mod = vc.baseDelay + (vc.depth * vc.mod.next());
                    wetSig += delayLine.read(mod * SAMPLE_RATE / 1000.0f) * vc.mix;
                }
                delayLine.write(*in_ptr + (feedback * wetSig));

                *out_ptr++ = ((1.0f - mix) * *in_ptr++) + (mix * wetSig); // Mix
            }
        }
};

class Reverb : public Effect { // Datarro reverb algorithm
    private:
        float mix, predelayTime, decayTime, decayGainL, decayGainR, modRate, modDepth, damping;
        vector<APF> diffusers; // 8
        vector<LPF> filters; // 3
        vector<DelayLine> delayLines; // 5
        Wavetable LFO;
        float tankInSig, nodeSig, tankSig1 = 0, tankSig2 = 0;

    public:
        Reverb(float mix, float predelayTime, float decayTime, float modRate, float modDepth, float damping) : LFO(modRate, SINE_TABLE) {
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
            filters.push_back(LPF(1.0f, bandwidth * SAMPLE_RATE / 2.0f));
            for (size_t i = 0; i < 2; ++i) { 
                filters.push_back(LPF(1.0f, damping * 10000.0f));
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
            if (name == "Predelay") { setPredelay(value); }
            if (name == "Decay") { setDecay(value); }
            if (name == "Mod Rate") { setModRate(value); }
            if (name == "Mod Depth") { setModDepth(value); }
            if (name == "Damping") { setDamping(value); }
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
                out[i] = ((1.0f - mix) * in[i]) + (mix * wetSig); // Total mix
            }
        }
};

/* CONTROL */

class AudioChain {
    private:
        vector<unique_ptr<Effect>> effects;
        map<string, Effect*> fxMap;
    public:
        template<typename T, typename... Args>
        void addEffect(const string& name, Args&&... args) {
            effects.push_back(make_unique<T>(args...));
            fxMap[name] = effects.back().get();
        };

        AudioChain() {
            // Build effects chain, initial order
            addEffect<Gain>("Gain", dbAmp(0.0f));
            addEffect<LPF>("LPF", 0.0f, SAMPLE_RATE / 2.0f);
            addEffect<APF>("APF", 0.0f, SAMPLE_RATE / 2.0f, 0.5f, false, 1000.0f);
            addEffect<FIR_Filter>("FIR", 0.0f, vector<float>(64, 1.0f / 64.0f));
            addEffect<Distortion>("Distortion", 0.0f, HARD_CLIP, 0.0f, false);
            addEffect<Delay>("Delay", 0.0f, 200.0f, 3000.0f, 0.0f);
            addEffect<Flanger>("Flanger", 0.0f, 0.0f, 0.0f, 0.0f);
            addEffect<Phaser>("Phaser", 0.0f, 0.0f, 1000.0f, 1.0f, 0.0f, 0.0f, 8, 0.8f);
            addEffect<Chorus>("Chorus", 0.0f, 0.0f, 0.0f, 100.0f, 0.0f, 4);
            addEffect<Reverb>("Reverb", 0.0f, 100.0f, 100.0f, 0.0f, 0.0f, 0.0005f);
        }

        Effect* getEffect(string key) { return fxMap[key]; }

        void reorder(uint8_t fxA, uint8_t fxB) { swap(effects[fxA], effects[fxB]); } // TODO: Make this actually useful lol (and RT safe)

        void processChain(float* input, float* output, size_t n = BUFFER_SIZE) {
            // Mark last active effect
            int lastActive = -1;
            for (size_t i = 0; i < effects.size(); ++i) {
                if (!effects[i]->isBypassed()) lastActive = i; }
            if (lastActive == -1) { memcpy(output, input, n * sizeof(float)); return; } // All bypass

            const float* in_ptr = input;
            float* out_ptr = nullptr;
            float stageBuf[BUFFER_SIZE];
            for (size_t i = 0; i < effects.size(); ++i) { // Process effects serially
                if (effects[i]->isBypassed()) continue;

                out_ptr = (i == lastActive) ? output : stageBuf;
                effects[i]->process(in_ptr, out_ptr, n);
                in_ptr = out_ptr;
            }
        }
};

class AudioIO { // FIXME: Fucking useless and broken right now, integrate with SPI/I2S later
    private:
        AudioChain chain;
        float input[BUFFER_SIZE], output[BUFFER_SIZE];
    public:
        void readBuffer(float* buf) { copy(buf, buf+BUFFER_SIZE, input); } // From ADC
        void writeBuffer(float* buf) { copy(output, output+BUFFER_SIZE, buf); }; // To DAC
        void start() {
            for (;;) {
                readBuffer(input);
                chain.processChain(input, output, BUFFER_SIZE);
                writeBuffer(output);
            }
        }
};

void setTestParams(AudioChain& chain) { // Set DSP testing parameters
    chain.getEffect("Gain")->setBypass(true);
    chain.getEffect("Gain")->setParam("Gain", dbAmp(3.0f));

    chain.getEffect("LPF")->setBypass(true);
    chain.getEffect("LPF")->setParam("Mix", 1.0f);
    chain.getEffect("LPF")->setParam("Cutoff", 300.0f);

    chain.getEffect("APF")->setBypass(true);
    chain.getEffect("APF")->setParam("Mix", 1.0f);
    chain.getEffect("APF")->setParam("Cutoff", 1000.0f);
    chain.getEffect("APF")->setParam("Q", 0.5f);

    chain.getEffect("FIR")->setBypass(true);
    chain.getEffect("FIR")->setParam("Mix", 1.0f);

    chain.getEffect("Distortion")->setBypass(false);
    chain.getEffect("Distortion")->setParam("Mix", 1.0f);
    chain.getEffect("Distortion")->setParam("Mode", RECTIFY);
    chain.getEffect("Distortion")->setParam("Drive", 1.0f);
    chain.getEffect("Distortion")->setParam("AAF", false);

    chain.getEffect("Delay")->setBypass(true);
    chain.getEffect("Delay")->setParam("Mix", 0.5f);
    chain.getEffect("Delay")->setParam("Time", 500.0f);
    chain.getEffect("Delay")->setParam("Feedback", 0.5f);
    
    chain.getEffect("Flanger")->setBypass(true);
    chain.getEffect("Flanger")->setParam("Mix", 1.0f);
    chain.getEffect("Flanger")->setParam("Rate", 0.2f);
    chain.getEffect("Flanger")->setParam("Depth", 1.0f);
    chain.getEffect("Flanger")->setParam("Feedback", 0.7f);

    chain.getEffect("Phaser")->setBypass(true);
    chain.getEffect("Phaser")->setParam("Mix", 1.0f);
    chain.getEffect("Phaser")->setParam("Rate", 0.2f);
    chain.getEffect("Phaser")->setParam("Center", 500.0f);
    chain.getEffect("Phaser")->setParam("Spread", 0.3f);
    chain.getEffect("Phaser")->setParam("Depth", 1.0f);
    chain.getEffect("Phaser")->setParam("Feedback", 0.5f);

    chain.getEffect("Chorus")->setBypass(true);
    chain.getEffect("Chorus")->setParam("Mix", 0.5f);
    chain.getEffect("Chorus")->setParam("Rate", 0.2f);
    chain.getEffect("Chorus")->setParam("Depth", 1.0f);
    chain.getEffect("Chorus")->setParam("Delay", 30.0f);
    chain.getEffect("Chorus")->setParam("Feedback", 0.5f);

    chain.getEffect("Reverb")->setBypass(true);
    chain.getEffect("Reverb")->setParam("Mix", 0.5f);
    chain.getEffect("Reverb")->setParam("Predelay", 50.0f);
    chain.getEffect("Reverb")->setParam("Decay", 5000.0f);
    chain.getEffect("Reverb")->setParam("Mod Rate", 0.2f);
    chain.getEffect("Reverb")->setParam("Mod Depth", 0.5f);
    chain.getEffect("Reverb")->setParam("Damping", 0.0005f);
};

/* TESTBENCH */
int main() {
    SF_INFO sfInfo;
    memset(&sfInfo, 0.0f, sizeof(sfInfo));

    // Open audio in/out files
    SNDFILE* inFile = sf_open("test.wav", SFM_READ, &sfInfo);
    SNDFILE* outFile = sf_open("result.wav", SFM_WRITE, &sfInfo);
    printf("Input: %d Hz, %d channels\n", sfInfo.samplerate, sfInfo.channels);

    // Init effects chain
    AudioChain chain;
    float input[BUFFER_SIZE], output[BUFFER_SIZE];
    sf_count_t readCount;

    setTestParams(chain);

    auto start = chrono::high_resolution_clock::now();
    // Process buffers sequentially and write to output
    while ((readCount = sf_read_float(inFile, input, BUFFER_SIZE)) > 0) {
        if (readCount < BUFFER_SIZE) { memset(input + readCount, 0, (BUFFER_SIZE - readCount) * sizeof(float)); } // Zero-pad ending
        chain.processChain(input, output, BUFFER_SIZE);
        // cout << *input << endl;
        // cout << *output << endl;
        sf_write_float(outFile, output, readCount);
    }

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;

    sf_close(inFile); sf_close(outFile);
    printf("Successfully processed chain in %fs\n", elapsed.count());
    return 0;
}