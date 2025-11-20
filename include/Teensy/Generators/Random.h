#pragma once

#include "Teensy/Modules/Generator.h"
#include "Teensy/Defines.h"
#include "Teensy/Utilities/Utilities.h"

class Random : public Generator {
    private:
        float freq, phase = 0.0f, currentVal = 0.0f, nextVal = 0.0f;
        RandomMode mode;
        float (Random::*algorithm)() = nullptr;

        // Noise algorithms
        float perlin();
        float sampleHold();
        float binary();
    public:
        Random(float freq, RandomMode mode);

        void setFreq(float freq); // Hz, [0.0, SAMPLE_RATE]
        void setPhase(float phase); // [0.0, 1.0]
        float getPhase() const;
        void setMode(RandomMode mode);

        float next() override;
};