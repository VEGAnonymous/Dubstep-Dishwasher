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

        Effect* getEffect(string key) { return fxMap[key]; }
        // TODO: Overload with id getter

        AudioChain() {
            // Build effects chain, initial order
            // BUG: Extreme DTCM memory issues - offending effects disabled until PSRAM arrives
            addEffect<Distortion>("Distortion", 1.0f, TUBE, 0.25f, false);
            addEffect<Delay>("Delay", 0.3f, 200.0f, 500.0f, 0.4f);
            addEffect<Flanger>("Flanger", 1.0f, 0.08f, 1.0f, 0.5f);
            addEffect<Phaser>("Phaser", 1.0f, 0.08f, 600.0f, 1.0f, 0.5f, 0.8f, 8, 0.8f);
            addEffect<Chorus>("Chorus", 1.0f, 0.08f, 25.0f, 5.0f, 0.1f, 4);
            addEffect<Reverb>("Reverb", 0.2f, 0.0f, 3000.0f, 0.5f, 0.2f);
            addEffect<Compressor>("Compressor", 1.0f, -18.0f, 4.0f, 10.0f, 100.0f, 100.0f, 0.0f, true);
            // addEffect<Granulator>("Granulator", 1.0f, 0.5f, 0.5f, 50.0f, 0.0f, 200.0f, 0.0f, 0.8f, 0.0f, 0.0f, HANN);
            // addEffect<Freezer>("Freezer", 1.0f, 2.0f, false, 1024, 4, 0.0f, 1.0f);
            // addEffect<SpectralGate>("Spectral Gate", 1.0f, -10.0f, 1.0f, 1024);

            addEffect<Compressor>("Limiter", 1.0f, dbAmp(-0.6f), 100.0f, 0.0f, 1.0f, 50.0f, dbAmp(-0.3f), false); // DO NOT TOUCH

            // Bypass all except Limiter
            for (size_t i = 0; i < effects.size() - 1; ++i) effects[i]->setBypass(true);
        }

        void reorder(uint8_t fxA, uint8_t fxB) { swap(effects[fxA], effects[fxB]); } // TODO: Maybe expand to either swap OR shift

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