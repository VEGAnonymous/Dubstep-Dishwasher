#include "Teensy/Utilities/DelayLineVector.h"
#include "Teensy/Utilities/Utilities.h"
#include "Teensy/Defines.h"

// HACK: Vector version of the above; this shit should be totally deprecated
// but Reverb crashes for some fucking reason without it so it's staying

/* PRIVATE */

/*

float delaySamples; 
size_t writeIndex;
std::vector<float> buffer;

*/

/* PUBLIC */

DelayLineVector::DelayLineVector(float delayTime, float maxDelayTime) : writeIndex((size_t)0) {
    buffer.resize(((maxDelayTime * SAMPLE_RATE) / 1000.0f) + 1, 0.0f);
    setDelayTime(delayTime);
}

void DelayLineVector::setDelayTime(float delayTime) { delaySamples = (delayTime * SAMPLE_RATE) / 1000.0f; } // ms
void DelayLineVector::setDelaySamples(float delaySamples) { this->delaySamples = delaySamples; }
int DelayLineVector::getSize() const { return buffer.size(); }