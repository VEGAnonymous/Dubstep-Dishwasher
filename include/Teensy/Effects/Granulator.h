#include "Teensy/Modules/Effect.h"

#include <vector>

class Granulator : public Effect {
    private:
        enum Params : ParamID { MIX, POSITION, POSITION_RAND, RATE, RATE_RAND, LENGTH, LENGTH_RAND, 
                                TUNE, TUNE_RAND, LEVEL, LEVEL_RAND, REVERSE_CHANCE, ENVELOPE_TYPE };

        static constexpr size_t bufSize = 3 * (size_t)SAMPLE_RATE; // 3s running buffer
        static constexpr int maxGrains = 32;
        
        float mix, position, rate, length, tune, level, reverseChance;
        float positionRand, rateRand, lengthRand, tuneRand, levelRand;
        EnvelopeType envType;
        
        float* inBuf = nullptr; size_t writePos = 0;
        float grainCounter = 0.0f;
        
        struct Grain {
            bool active = false;
            uint32_t startPos;
            float playhead;
            uint32_t length;
            float rate;
            float level;
            bool reverse;
        };
        std::vector<Grain> grains; // Grain pool

        void spawnGrain();

        float processGrain(Grain& grain);

    public:
        Granulator(float mix = 1.0f, float position = 0.5f, float positionRand = 0.5f, float time = 50.0f, float timeRand = 0.0f, 
                   float length = 200.0f, float lengthRand = 0.0f, float tune = 0.0f, float tuneRand = 0.0f, float level = 0.8f, 
                   float levelRand = 0.0f, float reverseChance = 0.0f, EnvelopeType envType = EnvelopeType::HANN);
        ~Granulator();

        void setMix(float mix); // [0.0, 1.0]
        void setPosition(float position); // [0.0, 1.0]
        void setPositionRand(float positionRand); // [0.0, 1.0]
        void setRate(float rate); // ms, [1.0, 500.0]
        void setRateRand(float rateRand); // [0.0, 1.0]
        void setLength(float length); // ms, [5.0, 500.0]
        void setLengthRand(float lengthRand); // [0.0, 1.0]
        void setTune(float tune); // semitones, [-24.0, 24.0]
        void setTuneRand(float tuneRand); // [0.0, 1.0]
        void setReverseChance(float reverseChance); // [0.0, 1.0]
        void setLevel(float level); // [0.0, 1.0]
        void setLevelRand(float levelRand); // [0.0, 1.0]
        void setEnvelopeType(EnvelopeType envType);
        void setParam(ParamID param, float value) override;

    void process(const float* in, float* out, size_t n) override;
};