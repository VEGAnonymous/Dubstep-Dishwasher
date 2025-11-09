#pragma once

#include <stddef.h>
#include "Teensy/Utilities/Utilities.h"

class DelayLine { // Implements z^-N
    private:
        float delaySamples; 
        size_t writeIndex;
        float* buffer; size_t size;

    public:
        DelayLine(float delayTime, float maxDelayTime);

        void setDelayTime(float delayTime); // ms
        void setDelaySamples(float delaySamples);
        int getSize() const;

        inline float read(float offset = -1.0f) { // Read tap
            float readOffset = (offset >= 0.0f) ? offset : delaySamples;
            float readIndex = (float)writeIndex - readOffset;
            if (readIndex < 0) readIndex += size;

            if (readOffset == floor(readOffset)) return buffer[(int)readIndex % size]; // Skip lerp if integer index
            return lerp(buffer, readIndex, size); // Fractional read
        }

        inline void write(float input) { 
            buffer[writeIndex] = input;
            if (++writeIndex == size) writeIndex = 0;
        }
};