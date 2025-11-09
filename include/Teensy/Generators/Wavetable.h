#pragma once

#include "Teensy/Modules/Generator.h"
#include "Teensy/Defines.h"

#include <Arduino.h>

class Wavetable : public Generator {
    private:
        float freq = 0.0f;
        const float* table = nullptr; // Pointer to wavetable array (stored in PROGMEM)
        uint32_t phaseAccumulator = 0ULL;
        int32_t phaseIncrement = 0;

        static constexpr uint8_t TABLE_BITS = 11; // 2048 = 2^11
        static constexpr uint8_t INDEX_SHIFT = 32 - TABLE_BITS;
    public:
        Wavetable(float freq, WavetableType table);

        void setFreq(float freq); // Hz
        void setTable(WavetableType table);

        float next() override;
};