#include "Teensy/Generators/Wavetable.h"
#include "Teensy/LUTs.h"

/* PRIVATE */

/*

float freq = 0.0f;
const float* table = nullptr; // Pointer to wavetable array (stored in PROGMEM)
uint32_t phaseAccumulator = 0ULL;
int32_t phaseIncrement = 0;

static constexpr uint8_t TABLE_BITS = 11; // 2048 = 2^11
static constexpr uint8_t INDEX_SHIFT = 32 - TABLE_BITS;

*/

/* PUBLIC */

Wavetable::Wavetable(float freq, WavetableType table) : phaseAccumulator(0) { setFreq(freq); setTable(table); }

void Wavetable::setFreq(float freq) { this->freq = freq; phaseIncrement = freq * ((1ULL << 32) / SAMPLE_RATE); } // Hz
void Wavetable::setTable(WavetableType table) {
    switch (table) {
        case WavetableType::SINE: this->table = SineTable; break;
        case WavetableType::TRI: this->table = TriTable; break;
        case WavetableType::SAW: this->table = SawTable; break;
        case WavetableType::SQUARE: this->table = SquareTable; break;
        default: this->table = SineTable;
    }
};

float Wavetable::next() { // Use fixed-point phase accumulator to index wavetable
    uint16_t index = phaseAccumulator >> INDEX_SHIFT; // Index with MSBs
    phaseAccumulator += phaseIncrement;
    return pgm_read_float(&table[index]);
}