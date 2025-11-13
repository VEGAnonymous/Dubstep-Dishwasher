#include "Teensy/Effects/Wah.h"

/* PRIVATE */

/*

enum Params : ParamID { MIX, MIN_FREQ, MAX_FREQ, Q };

float mix, minFreq, maxFreq, q;

static constexpr float L = 50.0f, L_samples = L * SAMPLE_RATE / 1000.0f; // RMS window size
float rms = 1e-6f; DelayLine inBuffer;
BPF_Biquad bpf;

*/
    
/* PUBLIC */

Wah::Wah(float mix, float minFreq, float maxFreq, float q) 
: inBuffer(L, L + 1.0f), bpf(1000.0f, 1.6f, 0.0f) { 
    setMix(mix); setMinFreq(minFreq); setMaxFreq(maxFreq); setQ(q);
}

void Wah::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void Wah::setMinFreq(float minFreq) { this->minFreq = std::clamp(minFreq, 20.0f, 1000.0f); } // Hz, [20.0, 1000.0]
void Wah::setMaxFreq(float maxFreq) { this->maxFreq = std::clamp(maxFreq, 1000.0f, 8000.0f); } // Hz, [1000.0, 8000.0]
void Wah::setQ(float q) { this->q = std::clamp(q, 0.3f, 6.0f); bpf.setQ(this->q); } // [0.3, 6.0]
void Wah::setParam(ParamID param, float value) { 
    switch (param) {
        case MIX: setMix(value); break;
        case MIN_FREQ: setMinFreq(value); break;
        case MAX_FREQ: setMaxFreq(value); break;
        case Q: setQ(value); break;
    }
}

void Wah::process(const float* in, float* out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        const float x_i = in[i]; const float x_L = inBuffer.read();
        inBuffer.write(x_i);

        // Envelope follower (shitty)
        // Compute RMS recursively
        arm_sqrt_f32((rms * rms) + (((x_i * x_i) - (x_L * x_L)) / L_samples), &rms);
        float rmsDB = ampDB(std::max(rms, 1e-6f));

        // Map envelope to BPF cutoff
        float envNorm = std::clamp((rmsDB + 60.0f) / 60.0f, 0.0f, 1.0f); // 60->0 dB
        float cutoff = lerp(minFreq, maxFreq, powf(envNorm, 1.5f));
        bpf.setCutoff(cutoff);

        // Process BPF
        float wetSig = bpf.processSample(x_i);
        out[i] = dryWetMix(x_i, wetSig, mix); // Mix
    }
}