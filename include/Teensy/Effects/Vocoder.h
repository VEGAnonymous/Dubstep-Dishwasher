#pragma once

#include "Teensy/Modules/Effect.h"
#include "Teensy/Filters/FIR_Filter.h"
#include "Teensy/Filters/Biquad/BPF_Biquad.h"

class Vocoder : public Effect {
    private:
        enum Params : ParamID { MIX, N_BANDS, LOW_FREQ, HIGH_FREQ, BANDWIDTH, DEPTH, ATTACK_TIME, RELEASE_TIME };

        float mix, lowFreq, highFreq, bandwidthFactor, depth, attackTime, releaseTime;
        float attackCoeff, releaseCoeff;
        size_t nBands;

        FIR_Filter hilbert; // Hilbert transformer for phase-shifted carrier

        // RTA core
        struct Band {
            BPF_Biquad car, mod; // BPF for both carrier and modulator
            float envState, attackCoeff, releaseCoeff; // Envelope tracking

            Band(float freq = 1000.0f, float q = 1.0f, float attack = 0.1f, float release = 0.01f)
                : car(freq, q, 0.0f, true), mod(freq, q, 0.0f, true), envState(0.0f), 
                  attackCoeff(attack), releaseCoeff(release) { }

            inline float processEnvelope(float x) {
                if (x > envState) envState += (x - envState) * attackCoeff; // Attack
                else envState += (x - envState) * releaseCoeff; // Release
                return envState;
            }
        };
        std::vector<Band> bands; // Filterbanks

        // Peak-based makeup gain
        float peakDry = 0.0f, peakWet = 0.0f;
        float smoothedGain = 1.0f; 
        const float peakDecay = exp(-1.0f / msSamples(100.0f)); // 100ms
        const float gainSmooth = exp(-1.0f / msSamples(50.0f)); // 50ms

        void setBands();

    public:
        Vocoder(float mix = 1.0f, size_t nBands = 10, float lowFreq = 80.0f, float highFreq = 12000.0f, 
                float bandwidth = 0.5f, float depth = 1.0f, float attack = 20.0f, float release = 35.0f);

        void setMix(float mix); // [0.0, 1.0]
        void setNBands(size_t nBands); // [4, 20]
        void setFreqRange(float lowFreq, float highFreq); // Hz, [10.0, 16000.0]
        void setBandwidth(float bandwidthFactor); // [0.03, 4.0]
        void setDepth(float depth); // [0.0, 2.0]
        void setAttackTime(float attackTime); // ms, [10.0, 1000.0]
        void setReleaseTime(float releaseTime); // ms, [10.0, 2000.0]
        void setParam(ParamID param, float value) override;
        float getParam(ParamID param) const override;

        void process(const float* in, float* out, size_t n) override;
};