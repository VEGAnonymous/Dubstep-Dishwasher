#pragma once

#include "Teensy/Modules/Effect.h"
#include "Teensy/Control/AudioChain.h"
#include "Teensy/Utilities/Utilities.h"

#include <array>

class Parallel : public Effect {
    // Parallel processing unit!
    private:
        enum Params : ParamID { MIX = 254, MODE = 255 }; // Meta parameters
        enum LocalCommand : uint8_t { // Command slots per block
            ADD = 13,
            REMOVE = 14,
            REORDER = 15,
            BYPASS = 16
        };

        static constexpr uint8_t MAX_CHAIN_EFFECTS = 5; // For the preservation of my sanity
        static constexpr uint8_t PARAMS_PER_EFFECT = 17; // Max 13 parameters + 4 command slots
        static constexpr uint8_t CHAIN_BLOCK = MAX_CHAIN_EFFECTS * PARAMS_PER_EFFECT; // = 85
        // Total addressing space = 2 * CHAIN_BLOCK = 170 (< 255)

        float mix; ParallelMode mode;

        struct Slot { // Store info about each slot in a chain
            bool occupied = false;
            EffectID id = 255;
            EffectName name = EffectName::GAIN;
        };

        AudioChain chainA, chainB; // Audio chains
        std::array<Slot, MAX_CHAIN_EFFECTS> slotsA, slotsB; // Chain effect slot info
        float* signalA = nullptr; float* signalB = nullptr; // Wet signal buffers

        // Addressing / routing helpers
        bool isChainB(ParamID pid);
        uint8_t localIndex(ParamID pid);
        uint8_t effectIndex(ParamID pid);
        uint8_t paramIndex(ParamID pid);

    public:
        Parallel(float mix = 1.0f, ParallelMode mode = ParallelMode::SUM);
        ~Parallel();

        void setMix(float mix); // [0.0, 1.0]
        void setMode(ParallelMode mode);
        void setParam(ParamID pid, float value) override;

        void process(const float* in, float* out, size_t n);
};