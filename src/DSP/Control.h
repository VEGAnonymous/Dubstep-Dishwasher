#ifndef CONTROL
#define CONTROL

#include "Modules.h"
#include "Filters.h"
#include "Effects.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

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
            addEffect<FIR_Filter>("FIR", 0.0f, vector<float>(64, 1.0f / 64.0f));
            addEffect<Distortion>("Distortion", 0.0f, HARD_CLIP, 0.0f, false);
            addEffect<Delay>("Delay", 0.0f, 200.0f, 3000.0f, 0.0f);
            addEffect<Flanger>("Flanger", 0.0f, 0.0f, 0.0f, 0.0f);
            addEffect<Phaser>("Phaser", 0.0f, 0.0f, 1000.0f, 1.0f, 0.0f, 0.0f, 8, 0.8f);
            addEffect<Chorus>("Chorus", 0.0f, 0.0f, 0.0f, 100.0f, 0.0f, 4);
            addEffect<Reverb>("Reverb", 0.0f, 100.0f, 100.0f, 0.0f, 0.0f);
            addEffect<SpectralGate>("Spectral Gate", 0.0f, 0.0f, 1024);
            addEffect<Compressor>("Compressor", 0.0f, 0.0f, 1.0f, 0.0f, 50.0f, 500.0f, 0.0f, true);
            addEffect<Granulator>("Granulator", 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 5.0f, 0.0f, 0.0f, HANN);
            addEffect<Freezer>("Freezer", 0.0f, 1.0f, false, 1024, 4, 0.0f, 1.0f);
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

#endif // CONTROL