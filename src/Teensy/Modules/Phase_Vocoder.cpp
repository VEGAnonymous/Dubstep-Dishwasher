#include "Teensy/Modules/Phase_Vocoder.h"
#include "Teensy/Utilities/Utilities.h"

/* PROTECTED */

/*

enum Params : ParamID { MIX, FFT_SIZE };

float mix; size_t fftSize;

STFT stft;
DelayLine latencyComp;

virtual void processSpectrum(STFT::FFTFrame& frame) = 0; // Subclasses must implement

*/

/* PUBLIC */

Phase_Vocoder::Phase_Vocoder(float mix, size_t fftSize, size_t hopFactor) 
: stft(fftSize, hopFactor, ((FFT_MAX_SIZE / (float)hopFactor) + 1.0f) / SAMPLE_RATE), 
    latencyComp(1.0f, ((FFT_MAX_SIZE + 1.0f) * 1000.0f) / SAMPLE_RATE) { 
    setMix(mix); setFFTSize(fftSize);
    // Set the STFT frame process callback to processSpectrum()
    stft.setProcessCallback([this](STFT::FFTFrame& frame) { processSpectrum(frame); });
}

void Phase_Vocoder::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void Phase_Vocoder::setFFTSize(size_t N) { // [128, FFT_MAX_SIZE], MUST BE POWER OF 2 (please? I'm asking nicely)
    const size_t fftN = std::clamp(N, (size_t)128, (size_t)FFT_MAX_SIZE);
    fftSize = fftN;
    
    stft.setFFTSize(fftSize);
    latencyComp.setDelaySamples((float)fftSize);
}

void Phase_Vocoder::setParam(ParamID param, float value) {
    switch (param) {
        case MIX: setMix(value); break;
        case FFT_SIZE: setFFTSize((size_t)value); break;
    }
}
float Phase_Vocoder::getParam(ParamID param) const {
    switch (param) {
        case MIX: return mix;
        case FFT_SIZE: return (float)fftSize;
        default: return 0.0f;
    }
}

void Phase_Vocoder::process(const float* in, float* out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        latencyComp.write(in[i]);
        float drySig = latencyComp.read();
        
        stft.forward(in[i]);
        /* processSpectrum() called in-between */
        float wetSig = stft.inverse(); // Reconstruct
        
        out[i] = dryWetMix(drySig, wetSig, mix);
    }
}