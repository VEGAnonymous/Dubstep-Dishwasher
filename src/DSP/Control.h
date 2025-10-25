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
        std::map<uint8_t, Effect*> fxMap;
        EffectID nextID = 0;

    public:
        template<typename T, typename... Args>
        void addEffect(Args&&... args) {
            auto effect = make_unique<T>(args...);
            effect->setID(nextID);

            fxMap[nextID] = effect.get();

            effects.push_back(move(effect));
            ++nextID;
        }

        void removeEffect(EffectID id) {
            // Search for effect by ID
            auto it = find_if(effects.begin(), effects.end(), [id](const unique_ptr<Effect>& effect){ return effect->getID() == id; });
            if (it == effects.end()) return; // Not found

            fxMap.erase((*it)->getID());
            effects.erase(it);
        }

        Effect* getEffect(EffectID id) {
            auto it = fxMap.find(id);
            return (it != fxMap.end()) ? it->second : nullptr;
        }

        void swapEffects(uint8_t idA, uint8_t idB) { // TODO: Maybe expand to also shift order? 
            auto itA = find_if(effects.begin(), effects.end(), [idA](const unique_ptr<Effect>& effect){ return effect->getID() == idA; });
            auto itB = find_if(effects.begin(), effects.end(), [idB](const unique_ptr<Effect>& effect){ return effect->getID() == idB; });
            if (itA == effects.end() || itB == effects.end()) return;
            iter_swap(itA, itB);
        }

        AudioChain() { // Build effects chain, initial order
            // BUG: Extreme DTCM memory issues - offending effects disabled until PSRAM arrives
            addEffect<Distortion>();
            addEffect<Delay>();
            addEffect<Flanger>();
            addEffect<Phaser>();
            addEffect<Chorus>();
            addEffect<Reverb>();
            addEffect<Compressor>();
            // addEffect<Granulator>();
            // addEffect<Freezer>();
            // addEffect<SpectralGate>();

            addEffect<Compressor>(1.0f, dbAmp(-0.6f), 100.0f, 0.0f, 1.0f, 50.0f, dbAmp(-0.3f), false); // LIMITER, DO NOT TOUCH

            // Bypass all except Limiter
            for (size_t i = 0; i < effects.size() - 1; ++i) effects[i]->setBypass(true);

            Serial.println("Active effects:");
            for (auto &fx : effects)
                if (!fx->isBypassed())
                    Serial.printf("  ID %d active\n", fx->getID());
        }

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