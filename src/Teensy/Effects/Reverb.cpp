#include "Teensy/Effects/Reverb.h"

// Datarro reverb algorithm 
// https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf

/* PRIVATE */

/*

enum Params : ParamID { MIX, PREDELAY_TIME, DECAY_TIME, MOD_RATE, MOD_DEPTH };

float mix, predelayTime, decayTime, decayGainL, decayGainR, modRate, modDepth;
std::vector<APF> diffusers; // 8
std::vector<OnePole> filters; // 3
std::vector<DelayLineVector> delayLines; // 5
Wavetable LFO;

float tankInSig, nodeSig, tankSig1 = 0, tankSig2 = 0;

*/

/* PUBLIC */

Reverb::Reverb(float mix, float predelayTime, float decayTime, float modRate, float modDepth) 
: LFO(modRate, WavetableType::SINE) {
    const float inputDiffuse[2] = {0.750f, 0.625f};
    const float decayDiffuse[2] = {0.70f, 0.50f};
    const size_t apfDelays[8] = {142, 107, 379, 277, 672, 908, 1800, 2656};
    const float delays[5] = {predelayTime, 100.97f, 84.35f, 95.62f, 71.72f};
    const float bandwidth = 0.9995f, damping = 0.0005f;

    // Diffusers
    size_t apf_i = 0;
    for (float diff : inputDiffuse) {
        for (size_t i = 0; i < 2; ++i) { 
            diffusers.push_back(APF(1.0f, 0.0f, 0.0f, false, 10.0f, true, false)); 
            diffusers.back().setDelay(apfDelays[apf_i++]); 
            diffusers.back().setQ(1.0f / (1.0f - diff)); 
        };
    }
    for (float diff : decayDiffuse) {
        for (size_t i = 0; i < 2; ++i) { 
            diffusers.push_back(APF(1.0f, 0.0f, 0.0f, false, 61.0f, true, false));
            diffusers.back().setDelay(apfDelays[apf_i++]); 
            diffusers.back().setQ(1.0f / (1.0f - diff)); 
        };
    } 
    diffusers[4].setInvert(true); diffusers[5].setInvert(true);

    // Filters
    filters.push_back(OnePole(1.0f, bandwidth * SAMPLE_RATE / 2.0f));
    for (size_t i = 0; i < 2; ++i) { filters.push_back(OnePole(1.0f, 2000.0f + (damping * 10000.0f))); }

    // Delays
    for (float delay : delays) { delayLines.push_back(DelayLineVector(delay, 102.0f)); }

    setMix(mix); setPredelayTime(predelayTime); setDecayTime(decayTime); setModRate(modRate); setModDepth(modDepth);
}

void Reverb::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void Reverb::setPredelayTime(float predelayTime) { // ms, [0.0, 100.0]
    this->predelayTime = std::clamp(predelayTime, 0.0f, 100.0f); 
    delayLines.front().setDelayTime(this->predelayTime); 
} 
void Reverb::setDecayTime(float decayTime) { // ms, [100.0, 10000.0]
    this->decayTime = std::clamp(decayTime, 100.0f, 10000.0f);
    // Decay gain computed via RT60
    float RT60 = this->decayTime / 1000.0f;
    decayGainL = powf(0.001f, 4648.0f / (RT60 * SAMPLE_RATE));
    decayGainR = powf(0.001f, 4924.0f / (RT60 * SAMPLE_RATE));
}
void Reverb::setModRate(float modRate) { this->modRate = std::clamp(modRate, 0.05f, 5.0f); LFO.setFreq(this->modRate); } // Hz, [0.05, 5.0]
void Reverb::setModDepth(float modDepth) { this->modDepth = std::clamp(modDepth, 0.0f, 1.0f); } // [0.0, 1.0]
void Reverb::setParam(ParamID param, float value) {
    switch (param) {
        case MIX: setMix(value); break;
        case PREDELAY_TIME: setPredelayTime(value); break;
        case DECAY_TIME: setDecayTime(value); break;
        case MOD_RATE: setModRate(value); break;
        case MOD_DEPTH: setModDepth(value); break;
    }
}

void Reverb::process(const float* in, float* out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        nodeSig = delayLines.front().read(); delayLines.front().write(in[i]); // Predelay
        tankInSig = filters.front().processSample(nodeSig); // Input-bandwidth filter

        float x = tankInSig, y = 0.0f;
        for (size_t apf_i = 0; apf_i < 4; ++apf_i) { // Input diffusion
            y = diffusers[apf_i].processSample(x);
            x = y;
        }

        // Tank left
        tankSig1 = tankInSig + tankSig2;

        float tankMod = LFO.next() * modDepth * 16.0f; // EXCURSION = 16 samples
        diffusers[4].setDelay(672.0f + tankMod);
        tankSig1 = diffusers[4].processSample(tankSig1); // Decay diffusion 1L

        nodeSig = delayLines[1].read(); delayLines[1].write(tankSig1); tankSig1 = nodeSig;
        tankSig1 = filters[1].processSample(tankSig1); // Damping L

        tankSig1 *= decayGainL; // Decay L
        tankSig1 = diffusers[6].processSample(tankSig1); // Decay diffusion 2L
        nodeSig = delayLines[2].read(); delayLines[2].write(tankSig1); tankSig1 = nodeSig; // END

        // Tank right
        tankSig2 = tankInSig + tankSig1;

        diffusers[5].setDelay(908.0f + tankMod);
        tankSig2 = diffusers[5].processSample(tankSig2); // Decay diffusion 1L

        nodeSig = delayLines[3].read(); delayLines[3].write(tankSig2); tankSig2 = nodeSig;
        tankSig2 = filters[2].processSample(tankSig2); // Damping L

        tankSig2 *= decayGainR; // Decay R
        tankSig2 = diffusers[7].processSample(tankSig2); // Decay diffusion 2L
        nodeSig = delayLines[4].read(); delayLines[4].write(tankSig2); tankSig2 = nodeSig; // END

        // Mixdown output taps
        const float tapGain = 0.6f;
        float accumulatorL = (
            delayLines[3].read(266.0f) +
            delayLines[3].read(2974.0f) +
            delayLines[4].read(1996.0f)
        ) - (
            diffusers[7].readTap(1913.0f) +
            delayLines[1].read(1990.0f) +
            diffusers[6].readTap(187.0f) +
            delayLines[2].read(1066.0f)
        ); accumulatorL *= tapGain;

        float accumulatorR = (
            delayLines[1].read(353.0f) + 
            delayLines[1].read(3627.0f) +
            delayLines[2].read(2673.0f) 
        ) - (
            diffusers[6].readTap(1228.0f) +
            delayLines[3].read(2111.0f) +
            diffusers[7].readTap(335.0f) +
            delayLines[4].read(121.0f)
        ); accumulatorR *= tapGain;

        float wetSig = (accumulatorL + accumulatorR) * 0.5f;
        out[i] = dryWetMix(in[i], wetSig, mix); // Total mix
    }
}