#ifndef CONTROL
#define CONTROL

#include <Audio.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Modules.h"
#include "Filters.h"
#include "Effects.h"

#include "../Handler.h"

/* CONTROL */

class AudioChain {
    private:
        std::vector<std::unique_ptr<Effect>> effects;
        std::map<EffectID, Effect*> fxMap;
        EffectID nextID = 0;

        std::map<EffectName, std::function<std::unique_ptr<Effect>()>> effectInits; // Function pointers to instantiate effects
    public:
        void addEffect(EffectName id) {
            // Search for effect initializer via EffectName mapping
            auto it = effectInits.find(id);
            if (it == effectInits.end()) return; // Not found

            auto effect = it->second();
            effect->setID(nextID);

            fxMap[nextID++] = effect.get();

            effects.push_back(std::move(effect));
        }

        void removeEffect(EffectID id) {
            // Search for effect by ID
            auto it = std::find_if(effects.begin(), effects.end(), [id](const std::unique_ptr<Effect>& effect){ return effect->getID() == id; });
            if (it == effects.end()) return; // Not found

            fxMap.erase((*it)->getID());
            effects.erase(it);
        }

        Effect* getEffect(EffectID id) {
            auto it = fxMap.find(id);
            return (it != fxMap.end()) ? it->second : nullptr;
        }

        void reorderEffect(EffectID id, size_t pos) {
            if (pos < 0) pos = 0;
            if (pos >= (size_t)(effects.size()) - 1) pos = effects.size() - 2; // Prevent moving the Limiter

            auto it = std::find_if(effects.begin(), effects.end(), [id](const std::unique_ptr<Effect>& effect) { return effect->getID() == id; });
            if (it == effects.end()) return;

            // Extract the effect from the vector
            auto effectPtr = std::move(*it);
            effects.erase(it);

            // Insert at new position
            effects.insert(effects.begin() + pos, std::move(effectPtr));
        }

        AudioChain() { 
            effectInits = {
                {EffectName::DISTORTION, [](){ return std::make_unique<Distortion>(); }},
                {EffectName::DELAY, [](){ return std::make_unique<Delay>(); }},
                {EffectName::FLANGER, [](){ return std::make_unique<Flanger>(); }},
                {EffectName::PHASER, [](){ return std::make_unique<Phaser>(); }},
                {EffectName::CHORUS, [](){ return std::make_unique<Chorus>(); }},
                {EffectName::REVERB, [](){ return std::make_unique<Reverb>(); }},
                {EffectName::COMPRESSOR, [](){ return std::make_unique<Compressor>(); }},
                {EffectName::EQUALIZER, [](){ return std::make_unique<Equalizer>(); }},
                {EffectName::GRANULATOR, [](){ return std::make_unique<Granulator>(); }},
                {EffectName::FREEZER, [](){ return std::make_unique<Freezer>(); }},
                {EffectName::SPECTRAL_GATE, [](){ return std::make_unique<SpectralGate>(); }},
                {EffectName::LIMITER, [](){ return std::make_unique<Compressor>(1.0f, dbAmp(-0.6f), 100.0f, 0.0f, 1.0f, 50.0f, dbAmp(-0.3f), false); }}
            };

            // Build effects chain, initial order
            // BUG: DTCM memory issues - some effects disabled until PSRAM arrives
            addEffect(EffectName::DISTORTION);
            addEffect(EffectName::DELAY);
            addEffect(EffectName::FLANGER);
            addEffect(EffectName::PHASER);
            addEffect(EffectName::CHORUS);
            addEffect(EffectName::REVERB);
            // addEffect(EffectName::COMPRESSOR);
            addEffect(EffectName::EQUALIZER);
            addEffect(EffectName::GRANULATOR);
            // addEffect(EffectName::FREEZER);
            addEffect(EffectName::SPECTRAL_GATE);

            addEffect(EffectName::LIMITER); // DO NOT TOUCH

            // Bypass all except Limiter
            for (size_t i = 0; i < effects.size() - 1; ++i) effects[i]->setBypass(true);

            Serial.println("Active effects:");
            for (auto &fx : effects) if (!fx->isBypassed()) Serial.printf("  ID %d active\n", fx->getID());
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
            float buf[AUDIO_BLOCK_SAMPLES];
            for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) { buf[i] = (float)inBlock->data[i] / 32768.0f; }

            // Process effect chain (in place)
            chain.processChain(buf, buf, AUDIO_BLOCK_SAMPLES);

            // Convert back to int16_t
            audio_block_t *outBlock = allocate();
            if (!outBlock) { release(inBlock); return; }
            for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) { // Clipping
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