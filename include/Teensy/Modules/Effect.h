#pragma once

#include "Teensy/Defines.h"

#include <stddef.h>

class Effect {
    protected:
        EffectID id;
        EffectName effectName;
        bool bypass = false;
        
    public:
        virtual ~Effect() = default;
        
        void setBypass(bool state);
        bool isBypassed() const;
        virtual bool isParallel() const;
        void setID(EffectID id);
        EffectID getID() const;
        
        void setEffectName(EffectName name);
        EffectName getEffectName() const;

        void setNormalized(ParamID param, float normalizedValue);

        virtual float getNormalized(ParamID param) const;
        
        // Subclasses must implement
        virtual void setParam(ParamID param, float value) = 0;
        virtual float getParam(ParamID param) const = 0;
        virtual void process(const float* in, float* out, size_t n) = 0;
};