#ifndef GENERATORS
#define GENERATORS

#include "Utilities.h"
#include "Modules.h"
#include "LUTs.h"

#include <cmath>

/* GENERATORS */

class Wavetable : public Generator {
    private:
        float freq = 0.0f;
        const float* table = nullptr; // Pointer to wavetable array (eventually stored in PROGMEM)
        uint32_t phaseAccumulator = 0ULL;
        int32_t phaseIncrement = 0;

        static constexpr uint8_t TABLE_BITS = 11; // 2048
        static constexpr uint8_t INDEX_SHIFT = 32 - TABLE_BITS;
    public:
        Wavetable(float freq, WavetableType table) : phaseAccumulator(0) { setFreq(freq); setTable(table); }

        void setFreq(float freq) { this->freq = freq; phaseIncrement = freq * ((1ULL << 32) / SAMPLE_RATE); } // Hz
        void setTable(WavetableType table) {
            switch (table) {
                case WavetableType::SINE: this->table = SineTable; break;
                case WavetableType::TRI: this->table = TriTable; break;
                case WavetableType::SAW: this->table = SawTable; break;
                case WavetableType::SQUARE: this->table = SquareTable; break;
                default: this->table = SineTable;
            }
        };

        float next() override { // Use fixed-point phase accumulator to index wavetable
            uint16_t index = phaseAccumulator >> INDEX_SHIFT; // Index with MSBs
            phaseAccumulator += phaseIncrement;
            return pgm_read_float(&table[index]);
        }
};

class Random : public Generator {
    private:
        float freq, phase = 0.0f, currentVal = 0.0f, nextVal = 0.0f;
        RandomMode mode;
        float (Random::*algorithm)() = nullptr;

        // Noise algorithms
        float perlin() {
            phase += freq / SAMPLE_RATE;
            if (phase >= 1.0f) {
                phase -= 1.0f;
                currentVal = nextVal;
                nextVal = uniform();
            }
            float smooth = phase * phase * (3.0f - (2.0f * phase)); // smoothstep(x) -> 3x^2 - 2x^3
            return currentVal + (smooth * (nextVal - currentVal)); // lerp
        }
        float sampleHold() {
            phase += freq / SAMPLE_RATE;
            if (phase >= 1.0f) {
                phase -= 1.0f;
                currentVal = uniform();
            }
            return currentVal;
        }
        float binary() {
            phase += freq / SAMPLE_RATE;
            if (phase >= 1.0f) {
                phase -= 1.0f;
                currentVal = (rand() & 1) ? 1.0f : -1.0f;
            }
            return currentVal;
        }
    public:
        Random(float freq, RandomMode mode) { setFreq(freq); setMode(mode); }

        void setFreq(float freq) { this->freq = freq; } // Hz
        void setMode(RandomMode mode) {
            this->mode = mode;
            switch (mode) {
                case RandomMode::PERLIN: algorithm = &Random::perlin; break;
                case RandomMode::SAMPLE_HOLD: algorithm = &Random::sampleHold; break;
                case RandomMode::BINARY: algorithm = &Random::binary; break;
            }
        }

        float next() override { return (this->*algorithm)(); }
};

#endif // GENERATORS