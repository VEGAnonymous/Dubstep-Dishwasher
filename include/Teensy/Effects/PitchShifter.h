#include "Teensy/Modules/Effect.h"
#include "Teensy/Generators/Random.h"

#include <vector>

class PitchShifter : public Effect {
    // Partially based on Kilohearts grain-based pitch shifter
    // https://kilohearts.com/products/pitch_shifter
    private:
        enum Params : ParamID { MIX, PITCH_SHIFT, GRAIN_SIZE, GRAIN_OVERLAP, JITTER };
        static constexpr size_t bufSize = (SAMPLE_RATE * 501) / 1000; // 500ms
        const size_t maxGrains = 16;

        float mix, pitchShift, grainSize, grainOverlap, jitter;
        float grainSizeSamples, grainInterval, pitchRatio, overlapGain;

        float* inBuf = nullptr;
        size_t writePos = 0;
        float grainCounter = 0.0f;

        struct Grain {
            bool active = false;
            float startPos;
            float playhead;
        };

        std::vector<Grain> grains; // Grain pool

        Random jitterer; // Jitter noise generator

        void spawnGrain();

        float processGrain(Grain& grain);

        void updateInterval();

    public:
        PitchShifter(float mix = 1.0f, float pitchShift = 0.0f, float grainSize = 200.0f, float grainOverlap = 0.5f, float jitter = 0.0f);
        ~PitchShifter();

        void setMix(float mix); // [0.0, 1.0]
        void setPitchShift(float pitchShift); // semitones, [-24.0, 24.0]
        void setGrainSize(float grainSize); // ms, [20.0, 500.0]
        void setGrainOverlap(float grainOverlap); // [0.25, 0.75]
        void setJitter(float jitter); // [0.0, 1.0]
        void setParam(ParamID param, float value) override;
        float getParam(ParamID param) const override;
        
        void process(const float* in, float* out, size_t n) override;
};