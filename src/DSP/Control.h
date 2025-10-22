#ifndef CONTROL
#define CONTROL

#include "Modules.h"
#include "Filters.h"
#include "Effects.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <Audio.h>

extern AudioInputI2S i2sInput;
extern AudioOutputI2S i2sOutput;

/* CONTROL */

class AudioChain {
    private:
        vector<unique_ptr<Effect>> effects;
        std::map<string, Effect*> fxMap;
    public:
        template<typename T, typename... Args>
        void addEffect(const string& name, Args&&... args) {
            effects.push_back(make_unique<T>(args...));
            fxMap[name] = effects.back().get();
        }

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
            addEffect<SpectralGate>("Spectral Gate", 0.0f, 0.0f, 0.0f, 1024);
            addEffect<Compressor>("Compressor", 0.0f, 0.0f, 1.0f, 0.0f, 50.0f, 500.0f, 0.0f, true);
            addEffect<Granulator>("Granulator", 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, HANN);
            addEffect<Freezer>("Freezer", 0.0f, 1.0f, false, 1024, 4, 0.0f, 1.0f);

            addEffect<Compressor>("Limiter", 1.0f, dbAmp(-0.6f), 100.0f, 0.0f, 1.0f, 50.0f, -0.3f, false); // DO NOT TOUCH
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

                out_ptr = (i == (size_t)lastActive) ? output : stageBuf;
                effects[i]->process(in_ptr, out_ptr, n);
                in_ptr = out_ptr;
            }
        }
};

class AudioChainStream : public AudioStream {
    private:
        audio_block_t * _inputQueueArray[1];
        AudioChain &chain;
    public:
        AudioChainStream(AudioChain &chain) : AudioStream(1, _inputQueueArray), chain(chain) {}

        virtual void update() override {
            audio_block_t *inBlock = receiveReadOnly(0); // Receive block from upstream
            if (!inBlock) return;

            // Convert int16_t samples to float
            constexpr int N = AUDIO_BLOCK_SAMPLES;
            float buf[N];
            for (int i = 0; i < N; ++i) { buf[i] = (float)inBlock->data[i] / 32768.0f; }

            // Process effect chain (in place)
            chain.processChain(buf, buf, N);

            // Convert back to int16_t
            audio_block_t *outBlock = allocate();
            if (!outBlock) { release(inBlock); return; }
            for (int i = 0; i < N; ++i) { // Clipping
                float f = buf[i] * 32767.0f;
                if (f > 32767.0f) f = 32767.0f;
                else if (f < -32768.0f) f = -32768.0f;
                outBlock->data[i] = (int16_t) (int32_t) f;
            }

            transmit(outBlock, 0); // Send block to downstream
            release(inBlock); release(outBlock);
        }
};

#endif // CONTROL