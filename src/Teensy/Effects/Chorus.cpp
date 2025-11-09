#include "Teensy/Effects/Chorus.h"

/* PRIVATE */

/*
enum Params : ParamID { MIX, RATE, DEPTH, DELAY_TIME, FEEDBACK };

float mix, rate, depth, delayTime, feedback;
static constexpr uint8_t voiceCount = 4; // 1-5
float sqrt_vc;

struct voice {
    float mix, depth, baseDelay;
    Random mod;
};
std::vector<voice> voices;

DelayLine delayLine;

*/

/* PUBLIC */

Chorus::Chorus(float mix, float rate, float depth, float delayTime, float feedback) {
    // Initialize voices
    for (size_t i = 0; i < voiceCount; ++i) { 
        float voicePhase = (float)i / voiceCount;
        float voiceRate = rate * (1.0f + 0.1f * ((float)i / voiceCount - 0.5f));
        voices.emplace_back(voice{
            1.0f / (float)voiceCount,
            depth,
            delayTime,
            voiceRate
        });
        voices[i].mod.setPhase(voicePhase);
    }
    sqrt_vc = sqrtf((float)voiceCount);
    setMix(mix); setRate(rate); setDepth(depth); setDelayTime(delayTime); setFeedback(feedback);
}

void Chorus::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void Chorus::setRate(float rate) { // Hz, [0.0, 20.0]
    this->rate = std::clamp(rate, 0.0f, 20.0f); 
    for (voice& vc : voices) vc.mod.setFreq(rate);
}
void Chorus::setDepth(float depth) { // ms, [0.0, 25.0]
    this->depth = std::clamp(depth, 0.0f, 25.0f);
    for (voice& vc : voices) vc.depth = depth; 
}
void Chorus::setDelayTime(float delayTime) { // ms, [0.0, 20.0]
    this->delayTime = std::clamp(delayTime, 0.0f, 20.0f); 
    for (size_t i = 0; i < voiceCount; ++i) { 
        float detune = ((float)i / voiceCount - 0.5f) * 2.0f;
        voices[i].baseDelay = delayTime * (1.0f + 0.3f * detune);
    };
}
void Chorus::setFeedback(float feedback) { this->feedback = std::clamp(feedback, -0.95f, 0.95f); } // [-0.95, 0.95]
void Chorus::setParam(ParamID param, float value) {
    switch (param) {
        case MIX: setMix(value); break;
        case RATE: setRate(value); break;
        case DEPTH: setDepth(value); break;
        case DELAY_TIME: setDelayTime(value); break;
        case FEEDBACK: setFeedback(value); break;
    }
}

void Chorus::process(const float* in, float* out, size_t n) {
    const float *in_ptr = in;
    float *out_ptr = out;
    
    for (size_t i = 0; i < n; i++) {
        float wetSig = 0.0f;
        for (voice& vc : voices) {
            float modDelay = vc.baseDelay + (vc.depth * vc.mod.next()); // Modulate delay time
            modDelay = std::clamp(modDelay, 0.01f, 50.0f);

            float voiceSig = vc.delayLine.read(modDelay * SAMPLE_RATE / 1000.0f); // Tap
            vc.delayLine.write(*in_ptr + (feedback * voiceSig)); // Write new sample and feedback
            wetSig += voiceSig * vc.mix; // Accumulate output
        }
        
        wetSig *= sqrt_vc;
        *out_ptr++ = dryWetMix(*in_ptr++, wetSig, mix); // Mix
    }
}