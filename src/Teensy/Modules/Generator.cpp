#include "Teensy/Modules/Generator.h"

/* PUBLIC */

void Generator::generate(float* out, size_t n) { // Generate block of samples
    for (size_t i = 0; i < n; ++i) {
        out[i] = next();
    }
}