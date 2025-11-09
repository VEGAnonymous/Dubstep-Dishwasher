#pragma once

#include "Teensy/Modules/Effect.h"

class Scrubby : public Effect {
    // dFX Scrubby!
    // http://destroyfx.org/docs/scrubby.html
    private:
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

        float constrainSpeed(float speed); // Octave constraint

    public:
        Scrubby(float mix = 1.0f, float seekRateLow = 9.0f, float seekRateHigh = 9.0f, float seekRange = 333.0f, 
                float seekDurLow = 1.0f, float seekDurHigh = 1.0f, int octavesDown = -4, int octavesUp = 8);
        ~Scrubby();

        void setMix(float mix); // [0.0, 1.0]
        void setSeekRate(float seekRateLow, float seekRateHigh);// Hz, [0.3, 810.0] for both
        void setSeekRange(float seekRange); // ms, [0.3, 6000.0]
        void setSeekDur(float seekDurLow, float seekDurHigh); // [0.03, 1.0] for both
        void setOctaveRange(int octavesDown, int octavesUp);
        void setParam(ParamID param, float value) override;

    void process(const float* in, float* out, size_t n) override;
};