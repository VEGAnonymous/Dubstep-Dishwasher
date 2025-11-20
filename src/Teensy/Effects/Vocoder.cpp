#include "Teensy/Effects/Vocoder.h"
#include "Teensy/LUTs.h"

/* PRIVATE */

/*

enum Params : ParamID { MIX, N_BANDS, LOW_FREQ, HIGH_FREQ, BANDWIDTH, DEPTH, ATTACK_TIME, RELEASE_TIME };

float mix, lowFreq, highFreq, bandwidthFactor, depth, attackTime, releaseTime;
float attackCoeff, releaseCoeff;

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
const float peakDecay = exp(-1.0f / (100.0f * SAMPLE_RATE / 1000.0f)); // 100ms
const float gainSmooth = exp(-1.0f / (50.0f * SAMPLE_RATE / 1000.0f)); // 50ms

*/

void Vocoder::setBands() {
    std::vector<float> bandFreqs(nBands);
    // Compute BPF center frequencies
    for (size_t k = 0; k < nBands; ++k) 
        bandFreqs[k] = lowFreq * powf(highFreq / lowFreq, (float)k / (float)(nBands - 1)); // Logarithmically spaced

    if (bands.size() != nBands) bands.resize(nBands);

    for (size_t k = 0; k < nBands; ++k) {
        // Compute BPF Q
        float bw;
        if (k == 0) bw = bandFreqs[1] - bandFreqs[0];
        else if (k == nBands - 1) bw = bandFreqs[k] - bandFreqs[k - 1];
        else bw = 0.5f * (bandFreqs[k + 1] - bandFreqs[k - 1]);
        float q = std::clamp((bandFreqs[k] / bw) / bandwidthFactor, 0.025f, 40.0f);
        // float q = baseQ / powf(1.5f, (float)k / nBands); // Lower Q for high bands

        // Set band parameters
        Band& band = bands[k];
        band.car.setCutoff(bandFreqs[k]); band.car.setQ(q);
        band.mod.setCutoff(bandFreqs[k]); band.mod.setQ(q);
        band.attackCoeff = attackCoeff; band.releaseCoeff = releaseCoeff;
    }
}

/* PUBLIC */

Vocoder::Vocoder(float mix, size_t nBands, float lowFreq, float highFreq, 
                 float bandwidth, float depth, float attack, float release)
        : hilbert(0.8f, Hilbert, sizeof(Hilbert) / sizeof(float)) {
            setMix(mix); setNBands(nBands); setFreqRange(lowFreq, highFreq);
            setBandwidth(bandwidth); setDepth(depth); setAttackTime(attack); 
            setReleaseTime(release);
        }

void Vocoder::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void Vocoder::setNBands(size_t nBands) { this->nBands = std::clamp(nBands, (size_t)4, (size_t)20); setBands(); } // [4, 20]
void Vocoder::setFreqRange(float lowFreq, float highFreq) { // Hz, [10.0, 16000.0]
    this->lowFreq = std::clamp(lowFreq, 10.0f, 16000.0f);
    this->highFreq = std::clamp(highFreq, 10.0f, 16000.0f);
    if (this->lowFreq > this->highFreq) this->highFreq = this->lowFreq + 1.0f;
}
void Vocoder::setBandwidth(float bandwidthFactor) { this->bandwidthFactor = std::clamp(bandwidthFactor, 0.03f, 4.0f); setBands(); } // [0.03, 4.0]
void Vocoder::setDepth(float depth) { this->depth = std::clamp(depth, 0.0f, 2.0f); } // [0.0, 2.0]
void Vocoder::setAttackTime(float attackTime) { // ms, [10.0, 1000.0]
    this->attackTime = attackTime;
    attackCoeff = 1.0f - exp(-1.0f / (std::clamp(attackTime, 10.0f, 1000.0f) * SAMPLE_RATE / 1000.0f)); 
    for (Band& band : bands) { band.attackCoeff = attackCoeff; }
}
void Vocoder::setReleaseTime(float releaseTime) { // ms, [10.0, 2000.0]
    this->releaseTime = releaseTime;
    releaseCoeff = 1.0f - exp(-1.0f / (std::clamp(releaseTime, 10.0f, 2000.0f) * SAMPLE_RATE / 1000.0f)); 
    for (Band& band : bands) band.releaseCoeff = releaseCoeff;
}
void Vocoder::setParam(ParamID param, float value) {
    switch (param) {
        case MIX: setMix(value); break;
        case N_BANDS: setNBands(value); break;
        case LOW_FREQ: setFreqRange(value, highFreq); break;
        case HIGH_FREQ: setFreqRange(lowFreq, value); break;
        case BANDWIDTH: setBandwidth(value); break;
        case DEPTH: setDepth(value); break;
        case ATTACK_TIME: setAttackTime(value); break;
        case RELEASE_TIME: setReleaseTime(value); break;
    }
}
float Vocoder::getParam(ParamID param) const {
    switch (param) {
        case MIX: return mix;
        case N_BANDS: return (float)nBands;
        case LOW_FREQ: return lowFreq;
        case HIGH_FREQ: return highFreq;
        case BANDWIDTH: return bandwidthFactor;
        case DEPTH: return depth;
        case ATTACK_TIME: return attackTime;
        case RELEASE_TIME: return releaseTime;
        default: return 0.0f;
    }
}

void Vocoder::process(const float* in, float* out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        float carrier = 0.0f, modulator = in[i];
        hilbert.process(&modulator, &carrier, 1); // Carrier = phase-shifted modulator
        float wetSig = 0.0f;

        /* VOCODE */
        for (size_t k = 0; k < nBands; ++k) { // Split signal into bands
            Band& band = bands[k];
            float modulatorBand = band.mod.processSample(modulator);
            float carrierBand = band.car.processSample(carrier);

            // Extract modulator spectral envelope
            float modEnvelope = band.processEnvelope(modulatorBand * modulatorBand);
            arm_sqrt_f32(modEnvelope, &modEnvelope);
            float modGain = modEnvelope * (depth - (depth - 1.0f) * modEnvelope); // Cheap alternative to powf(modEnvelope, depth)

            wetSig += carrierBand * modGain; // AM and mixdown
        }

        /* MAKEUP GAIN */
        // HACK: This thing is so fucking shit I don't even know anymore

        // Track peaks with decay
        peakDry = fmaxf(fabsf(in[i]), peakDry * peakDecay);
        peakWet = fmaxf(fabsf(wetSig), peakWet * peakDecay);

        // Smooth toward target gain
        float targetGain = 1.0f;
        if (peakWet > 1e-6f) targetGain = std::clamp(peakDry / peakWet, 0.5f, 50.0f);
        smoothedGain = (gainSmooth * smoothedGain) + ((1.0f - gainSmooth) * targetGain);

        wetSig *= smoothedGain;
        out[i] = dryWetMix(in[i], wetSig, mix); // Mix
    }
}