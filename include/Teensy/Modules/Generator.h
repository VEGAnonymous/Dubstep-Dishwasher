#pragma once

#include <stddef.h>

class Generator {
    public:
        virtual float next() = 0; // Generate next sample
        virtual void generate(float* out, size_t n); // Generate block of samples
};