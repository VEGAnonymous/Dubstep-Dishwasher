#pragma once

#include "Teensy/Utilities/Utilities.h"

#include <stddef.h>
#include <vector>

class DelayLineVector { // Implements z^-N
    private:
        float delaySamples; 
        size_t writeIndex;
        std::vector<float> buffer;
        
    public:
        DelayLineVector(float delayTime, float maxDelayTime);

        void setDelayTime(float delayTime); // ms
        void setDelaySamples(float delaySamples);
        int getSize() const;

        inline float read(float offset = -1.0f) { // Read tap
            float readOffset = (offset >= 0.0f) ? offset : delaySamples;
            float readIndex = (float)writeIndex - readOffset;
            if (readIndex < 0) readIndex += buffer.size();

            if (readOffset == floor(readOffset)) return buffer[(int)readIndex % buffer.size()]; // Skip lerp if integer index 
            return lerp(buffer, readIndex, buffer.size()); // Fractional read
        }

        inline void write(float input) { 
            buffer[writeIndex] = input;
            if (++writeIndex >= buffer.size()) writeIndex = 0;
        }
};