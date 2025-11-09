#pragma once

#include "Teensy/Modules/Effect.h"

#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <vector>

class AudioChain {
    private:
        std::vector<std::unique_ptr<Effect>> effects; // Effect chain
        std::map<EffectID, Effect*> fxMap;
        EffectID nextID = 0; std::queue<EffectID> freeIDs;

        std::map<EffectName, std::function<std::unique_ptr<Effect>()>> effectInits; // Function pointers (factories) to instantiate effects
    public:
        Effect* addEffect(EffectName id);

        void removeEffect(EffectID id);

        Effect* getEffect(EffectID id);

        void reorderEffect(EffectID id, size_t pos);

        AudioChain();

        void processChain(const float* input, float* output, size_t n);
};