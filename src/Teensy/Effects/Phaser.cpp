#include "Teensy/Effects/Phaser.h"

/* PRIVATE */

/*

enum Params : ParamID { MIX, RATE, CENTER_FREQ, SPREAD, DEPTH, FEEDBACK };

static constexpr uint8_t order = 6; // Number of APFs
static constexpr float q = 0.8f;

float mix, rate, centerFreq, spread, depth, feedback;

std::vector<APF> apfSections; // APF bank
std::vector<float> baseFreqs; // Store APF base freqs
Wavetable LFO;
float wetSig = 0.0f;

*/

void Phaser::setupStages() { // Geometrically separate APF base freqs
    const float R = 1.0f + (spread * 16.0f); // Ratio from max/min base freqs, [1.6, 16.0]
    const float r = pow(R, 1.0f/(order-1.0f));
    baseFreqs.resize(order);
    for (size_t i = 0; i < order; ++i) {
        float freq_i = centerFreq * pow(r, i - ((order - 1.0f) / 2.0f));
        baseFreqs[i] = freq_i;
        apfSections[i].setCutoff(freq_i);
    }
}

/* PUBLIC */

Phaser::Phaser(float mix, float rate, float centerFreq, float spread, float depth, float feedback)
: LFO(rate, WavetableType::SINE) {
    for (size_t i = 0; i < order; ++i) { apfSections.push_back(APF(1.0f, centerFreq, q, false, 20.0f, true)); }
    setupStages();
    setMix(mix); setRate(rate); setCenterFreq(centerFreq); setSpread(spread), setDepth(depth); setFeedback(feedback);
}

void Phaser::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void Phaser::setRate(float rate) { this->rate = std::clamp(rate, 0.0f, 20.0f); LFO.setFreq(rate); } // Hz, [0.0, 20.0]
void Phaser::setCenterFreq(float centerFreq) { this->centerFreq = std::clamp(centerFreq, 50.0f, 8000.0f); setupStages(); } // Hz, // [50.0, 8000.0]
void Phaser::setSpread(float spread) { this->spread = std::clamp(spread, 0.1f, 1.0f); setupStages(); } // [0.1, 1.0]
void Phaser::setDepth(float depth) { this->depth = std::clamp(depth, 0.0f, 1.0f); } // [0.0, 1.0]
void Phaser::setFeedback(float feedback) { this->feedback = std::clamp(feedback, -0.95f, 0.95f); } // [-0.95, 0.95]
void Phaser::setParam(ParamID param, float value) { // [0.0, 1.0]
    switch (param) {
        case MIX: setMix(value); break;
        case RATE: setRate(value); break;
        case CENTER_FREQ: setCenterFreq(value); break;
        case SPREAD: setSpread(value); break;
        case DEPTH: setDepth(value); break;
        case FEEDBACK: setFeedback(value); break;
    }
}

void Phaser::process(const float* in, float* out, size_t n) {
    float inSig, mod;
    for (size_t i = 0; i < n; ++i) {
        mod = exp2f(depth * LFO.next());
        inSig = in[i] + (wetSig * feedback);
        
        float x = inSig, y = 0.0f;
        for (size_t j = 0; j < apfSections.size(); ++j) { // Process APFs serially, output to wetSig
            apfSections[j].setCutoff(baseFreqs[j] * mod); // Modulate each base freq with LFO
            y = apfSections[j].processSample(x);
            x = y;
        } wetSig = y;

        out[i] = dryWetMix(in[i], wetSig, mix); // Mix
    }
}
