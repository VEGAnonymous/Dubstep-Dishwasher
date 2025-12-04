#include "Teensy/Effects/Scrubby.h"
#include "Teensy/Utilities/Utilities.h"

// dFX Scrubby!
// http://destroyfx.org/docs/scrubby.html

/* PRIVATE */

/*

enum Params : ParamID { MIX, SEEK_RATE_LOW, SEEK_RATE_HIGH, SEEK_RANGE, SEEK_DUR_LOW, SEEK_DUR_HIGH, 
                        OCTAVES_DOWN, OCTAVES_UP };

static constexpr size_t bufSize = (size_t)8 * (size_t)SAMPLE_RATE; // 8s running buffer

float mix, seekRateLow, seekRateHigh, seekRange, seekDurLow, seekDurHigh;
int octavesDown, octavesUp; float minSpeed, maxSpeed;

float* inBuf = nullptr; size_t writePos = 0; float readPos = 0.0f;

// State variables
size_t seekCounter = 0, currentSeekDur = 0, seekProgress = 0;
float seekInterval = 0.0f, targetPos = 0.0f, currentSpeed = 1.0f;
bool isSeeking = false;

*/

float Scrubby::constrainSpeed(float speed) { return std::clamp(speed, minSpeed, maxSpeed); } // Octave constraint

/* PUBLIC */

Scrubby::Scrubby(float mix, float seekRateLow, float seekRateHigh, float seekRange, 
                 float seekDurLow, float seekDurHigh, int octavesDown, int octavesUp) { 
        setMix(mix); setSeekRate(seekRateLow, seekRateHigh); setSeekRange(seekRange); 
        setSeekDur(seekDurLow, seekDurHigh); setOctaveRange(octavesDown, octavesUp);
        readPos = 0.0f;
        inBuf = (float*)extmem_malloc(bufSize * sizeof(float)); 
        if (!inBuf) while(1){}; 
        memset(inBuf, 0, bufSize * sizeof(float));
    }
Scrubby::~Scrubby() { if (inBuf) { extmem_free(inBuf); inBuf = nullptr; } }

void Scrubby::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void Scrubby::setSeekRate(float seekRateLow, float seekRateHigh) { // Hz, [0.3, 810.0] for both
    this->seekRateLow = std::clamp(seekRateLow, 0.3f, 810.0f);
    this->seekRateHigh = std::clamp(seekRateHigh, 0.3f, 810.0f);
    if (this->seekRateLow > this->seekRateHigh) this->seekRateHigh = this->seekRateLow;
    seekInterval = 1.0f / ((seekRateHigh + seekRateLow) / 2.0f); // Init interval to average of bounds
}
void Scrubby::setSeekRange(float seekRange) { this->seekRange = std::clamp(seekRange, 0.3f, 6000.0f); } // ms, [0.3, 6000.0]
void Scrubby::setSeekDur(float seekDurLow, float seekDurHigh) { // [0.03, 1.0] for both
    this->seekDurLow = std::clamp(seekDurLow, 0.03f, 1.0f);
    this->seekDurHigh = std::clamp(seekDurHigh, 0.03f, 1.0f);
    if (this->seekDurLow > this->seekDurHigh) this->seekDurHigh = this->seekDurLow;
}
void Scrubby::setOctaveRange(int octavesDown, int octavesUp) {
    this->octavesDown = std::clamp(octavesDown, -4, 0); // [-4, 0]
    this->octavesUp = std::clamp(octavesUp, 0, 8); // [0, 8]
    minSpeed = powf(2.0f, (float)octavesDown);
    maxSpeed = powf(2.0f, (float)octavesUp);
}
void Scrubby::setParam(ParamID param, float value) {
    switch (param) {
        case MIX: setMix(value); break;
        case SEEK_RATE_LOW: setSeekRate(value, seekRateHigh); break;
        case SEEK_RATE_HIGH: setSeekRate(seekRateLow, value); break;
        case SEEK_RANGE: setSeekRange(value); break;
        case SEEK_DUR_LOW: setSeekDur(value, seekDurHigh); break;
        case SEEK_DUR_HIGH: setSeekDur(seekDurLow, value); break;
        case OCTAVES_DOWN: setOctaveRange(value, octavesUp); break;
        case OCTAVES_UP: setOctaveRange(octavesDown, value); break;
    }
}
float Scrubby::getParam(ParamID param) const {
    switch (param) {
        case MIX: return mix;
        case SEEK_RATE_LOW: return seekRateLow;
        case SEEK_RATE_HIGH: return seekRateHigh;
        case SEEK_RANGE: return seekRange;
        case SEEK_DUR_LOW: return seekDurLow;
        case SEEK_DUR_HIGH: return seekDurHigh;
        case OCTAVES_DOWN: return (float)octavesDown;
        case OCTAVES_UP: return (float)octavesUp;
        default: return 0.0f;
    }
}

void Scrubby::process(const float* in, float* out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        inBuf[writePos] = in[i];
        
        float wetSig = 0.0f;
        if (isSeeking) { // Currently seeking
            if (seekProgress <= currentSeekDur) { 
                wetSig = lerp(inBuf, readPos, bufSize);

                // Apply envelope
                float t = (currentSeekDur > 0) ? (float)seekProgress / (float)currentSeekDur : 0.0f;
                float envelopeValue = getEnvelopeValue(t, EnvelopeType::SMOOTH_RECT); 
                wetSig *= envelopeValue;
                
                // Scrub readpos according to constant speed
                readPos += currentSpeed;
                while (readPos >= bufSize) readPos -= bufSize;
                while (readPos < 0) readPos += bufSize;
                
                ++seekProgress;
            } else isSeeking = false;
        }
        if (!isSeeking) readPos = (float)writePos;
        
        // Time to seek a new target?
        if (!isSeeking && seekCounter >= (size_t)(seekInterval * SAMPLE_RATE)) { // Only trigger new seek if NOT currently seeking
            // Set random seek duration
            float seekDur = lerp(seekDurLow, seekDurHigh, (uniform() + 1.0f) * 0.5f);
            currentSeekDur = (size_t)(seekDur * (float)(seekInterval * SAMPLE_RATE));
            
            // Set random target
            targetPos = writePos - (msSamples(seekRange) * uniform());
            while (targetPos < 0) targetPos += bufSize;
            
            // Compute constant speed
            float distance = targetPos - readPos;
            if (distance < -bufSize / 2) distance += bufSize;
            if (distance > bufSize / 2) distance -= bufSize;
            
            if (currentSeekDur > 0) currentSpeed = constrainSpeed(1.0f + (distance / (float)currentSeekDur));
            else currentSpeed = 1.0f;
            
            // Set next random seek interval and start seeking
            seekInterval = scale(uniform(), -1.0f, 1.0f, 1.0f / seekRateHigh, 1.0f / seekRateLow);
            isSeeking = true; seekProgress = 0; seekCounter = 0;
        }
        
        out[i] = dryWetMix(in[i], wetSig, mix); // Mix
        
        ++writePos; ++seekCounter; 
        if (writePos >= bufSize) writePos = 0;
    }
}