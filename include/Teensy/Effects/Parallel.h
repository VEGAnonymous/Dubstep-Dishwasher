#pragma once

#include "Teensy/Modules/Effect.h"
#include "Teensy/Control/AudioChain.h"
#include "Teensy/Utilities/Utilities.h"

#include <array>

class Parallel : public Effect {
    // Parallel processing unit!
    private:
        enum Params : ParamID { MIX, MODE };

        enum Chain : uint8_t { A, B };
        enum ChainCommand : uint8_t { 
            ADD, 
            REMOVE, 
            REORDER,
            SET_PARAM, 
            BYPASS 
        };

        float mix; ParallelMode mode;
        
        AudioChain chainA, chainB; // Audio chains
        float* signalA = nullptr; float* signalB = nullptr; // Wet signal buffers
        
    public:
        Parallel(float mix = 1.0f, ParallelMode mode = ParallelMode::SUM);
        ~Parallel();

        bool isParallel() const override;
        
        void setMix(float mix); // [0.0, 1.0]
        void setMode(ParallelMode mode);
        void setParam(ParamID param, float value) override;
        float getParam(ParamID param) const override;
        
        void chainCommand(uint8_t chain, uint8_t command, EffectID effectId, ParamID paramId, float value);
        
        void process(const float* in, float* out, size_t n) override;
};