#pragma once

#include "Teensy/Defines.h"
#include "Teensy/Utilities/Utilities.h"

#include <algorithm>
#include <cmath>
#include <map>

/* DEFINES */
enum class ParamUnit {
    PERCENT,       // %
    HZ,            // Hz
    DB,            // dB
    MS,            // ms
    SEMITONES,     // st
    DIMENSIONLESS, // ...
};

struct ParameterRange {
    ParamUnit unit;
    float min, max, exponent;
    float initialValue;
    
    float fromNormalized(float norm) const {
        float lin = powf(std::clamp(norm, 0.0f, 1.0f), exponent); // Apply curve
        
        // Handle negative ranges (e.g., [-0.95, 0.95])
        if (min < 0.0f && max <= 0.0f) {
            float absMin = -max; float absMax = -min;
            float absValue = (1.0f - lin) * (absMax - absMin) + absMin;
            return -absValue;
        }
        
        return lerp(min, max, lin);
    }
    
    float toNormalized(float actual) const {
        float lin;
        if (min < 0.0f && max <= 0.0f) {
            float absV = -actual;
            float absMin = -max; float absMax = -min;
            lin = 1.0f - (absV - absMin) / (absMax - absMin);
        } else lin = (actual - min) / (max - min);

        return powf(std::clamp(lin, 0.0f, 1.0f), 1.0f / exponent);
    }
};

/* MAP */

using ParameterMap = std::map<EffectName, std::map<ParamID, ParameterRange>>; // Map: EffectName -> ParamID -> Range
ParameterMap createParameterRegistry();

const ParameterRange* getParameterRange(EffectName effect, ParamID param);