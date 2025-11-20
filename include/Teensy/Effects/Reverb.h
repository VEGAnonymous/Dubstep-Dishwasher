#pragma once

#include "Teensy/Modules/Effect.h"
#include "Teensy/Filters/OnePole.h"
#include "Teensy/Filters/APF.h"
#include "Teensy/Generators/Wavetable.h"

class Reverb : public Effect { 
    // Datarro reverb algorithm 
    private:
        enum Params : ParamID { MIX, PREDELAY_TIME, DECAY_TIME, MOD_RATE, MOD_DEPTH };

        float mix, predelayTime, decayTime, decayGainL, decayGainR, modRate, modDepth;
        std::vector<APF> diffusers; // 8
        std::vector<OnePole> filters; // 3
        std::vector<DelayLineVector> delayLines; // 5
        Wavetable LFO;

        float tankInSig, nodeSig, tankSig1 = 0, tankSig2 = 0;

    public:
        Reverb(float mix = 0.2f, float predelayTime = 0.0f, float decayTime = 3000.0f, float modRate = 0.5f, float modDepth = 0.2f);

        void setMix(float mix); // [0.0, 1.0]
        void setPredelayTime(float predelayTime); // ms, [0.0, 100.0]
        void setDecayTime(float decayTime); // ms, [100.0, 10000.0]
        void setModRate(float modRate); // Hz, [0.05, 5.0]
        void setModDepth(float modDepth); // [0.0, 1.0]
        void setParam(ParamID param, float value) override;
        float getParam(ParamID param) const override;

        void process(const float* in, float* out, size_t n) override;
};