#pragma once

#include "Teensy/Defines.h"

#include <stddef.h>

class Effect {
    protected:
        bool bypass = false;
        EffectID id;

    public:
        virtual ~Effect() = default;
        
        void setBypass(bool state);
        bool isBypassed() const;
        void setID(EffectID id);
        EffectID getID() const;

        virtual void setParam(ParamID param, float value) = 0; // Allows setting subclass parameters from an Effect pointer

        virtual void process(const float* in, float* out, size_t n) = 0; // Process sample block, implemented per effect
};