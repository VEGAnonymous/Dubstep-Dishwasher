#include "Teensy/Utilities/DelayLine.h"
#include "Teensy/Defines.h"

#include <Arduino.h>

/* PRIVATE */

/*

float delaySamples; 
size_t writeIndex;
float* buffer; size_t size;

*/

/* PUBLIC */

DelayLine::DelayLine(float delayTime, float maxDelayTime) : writeIndex((size_t)0) {
    size = (size_t)((maxDelayTime * SAMPLE_RATE) / 1000.0f) + 1;
    buffer = (float*)extmem_malloc(size * sizeof(float));
    if (!buffer) while (1) { }
    memset(buffer, 0, size * sizeof(float));

    setDelayTime(delayTime);
}

void DelayLine::setDelayTime(float delayTime) { delaySamples = (delayTime * SAMPLE_RATE) / 1000.0f; } // ms
void DelayLine::setDelaySamples(float delaySamples) { this->delaySamples = delaySamples; }
int DelayLine::getSize() const { return size; }