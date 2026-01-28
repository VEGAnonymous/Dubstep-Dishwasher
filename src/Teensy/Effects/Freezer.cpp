#include "Teensy/Effects/Freezer.h"
#include "Teensy/Utilities/Utilities.h"

/* PRIVATE */

/*

enum Params : ParamID { MIX, RATE, SPECTRAL_MODE, FFT_SIZE, LOOP_START, LOOP_END };

static constexpr size_t bufSize = (size_t)3 * (size_t)SAMPLE_RATE; // 3s running buffer
static constexpr float smooth = 0.005f; // Smoothing factor at loop boundaries

float mix, rate; bool spectralMode;
float loopStart, loopEnd;

// Time domain
float* inBuf = nullptr; size_t writePos = 0; float readPos = 0.0f;

// Spectral resythesis
size_t fftSize;
static constexpr size_t hopFactor = 4;
std::unique_ptr<STFT> stft;
std::vector<float> spectBuf, spectFrame;
size_t spectPos = 0, spectHopCounter = 0;

*/

void Freezer::allocateSTFT() {
    stft = std::make_unique<STFT>(fftSize, hopFactor, 0.25f);
    spectBuf.assign(fftSize, 0.0f); spectFrame.assign(fftSize, 0.0f);
    spectPos = 0; spectHopCounter = 0;
}
void Freezer::freeSTFT() {
    stft.reset(); 
    spectBuf.clear(); spectFrame.clear();
}

/* PUBLIC */

Freezer::Freezer(float mix, float rate, bool spectralMode, size_t fftSize, 
        float loopStart, float loopEnd) : fftSize(fftSize) {
    setMix(mix); setRate(rate); setFFTSize(fftSize);
    setSpectralMode(spectralMode); setLoopRegion(loopStart, loopEnd); 
    inBuf = (float*)extmem_malloc(bufSize * sizeof(float));
    if (!inBuf) while (1) { }
    memset(inBuf, 0, bufSize * sizeof(float));
}
Freezer::~Freezer() { if (inBuf) { extmem_free(inBuf); inBuf = nullptr; } }

void Freezer::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void Freezer::setRate(float rate) { this->rate = std::clamp(rate, -4.0f, 4.0f); } // [-4.0, 4.0]
void Freezer::setSpectralMode(bool mode) { 
    if (mode == spectralMode) return;
    spectralMode = mode;

    if (spectralMode) allocateSTFT();
    else freeSTFT();
}
void Freezer::setFFTSize(size_t N) { // [128, FFT_MAX_SIZE], MUST BE POWER OF 2 (I'm not going to ask you again)
    const size_t fftN = std::clamp(N, (size_t)128, (size_t)FFT_MAX_SIZE);
    fftSize = fftN;
    if (stft) {
        stft->setFFTSize(fftN); 
        spectBuf.assign(fftN, 0.0f); spectFrame.resize(fftN);
        spectPos = 0; spectHopCounter = 0;
    }

}
void Freezer::setLoopRegion(float start, float end) { // [0.0, 1.0] for both
    loopStart = std::clamp(start, 0.0f, 1.0f);
    loopEnd = std::clamp(end, 0.0f, 1.0f);
    if (loopStart >= loopEnd) { // start < end
        loopEnd = loopStart + 0.01f;
        if (loopEnd > 1.0f) { loopEnd = 1.0f; loopStart = 0.99f; }
    }
    readPos = loopStart * (float)bufSize;
}
void Freezer::setParam(ParamID param, float value) {
    switch (param) {
        case MIX: setMix(value); break;
        case RATE: setRate(value); break;
        case SPECTRAL_MODE: setSpectralMode(value > 0.5f); break;
        case FFT_SIZE: setFFTSize((size_t)value); break;
        case LOOP_START: setLoopRegion(value, loopEnd); break;
        case LOOP_END: setLoopRegion(loopStart, value); break;
    }
}
float Freezer::getParam(ParamID param) const {
    switch (param) {
        case MIX: return mix;
        case RATE: return rate;
        case SPECTRAL_MODE: return spectralMode ? 1.0f : 0.0f;
        case FFT_SIZE: return (float)fftSize;
        case LOOP_START: return loopStart;
        case LOOP_END: return loopEnd;
        default: return 0.0f;
    }
}

void Freezer::process(const float* in, float* out, size_t n) {
    const size_t bufN = bufSize,
                 fftN = stft ? stft->getFFTSize() : 0,
                 hopN = stft ? stft->getHopSize() : 0;

    // Compute loop boundaries
    float loopStartSamples = loopStart * (float)bufN;
    float loopEndSamples = loopEnd * (float)bufN;
    float loopLength = loopEndSamples - loopStartSamples;
    
    for (size_t i = 0; i < n; ++i) {
        inBuf[writePos] = in[i];
        ++writePos; if (writePos >= bufN) writePos = 0;
        
        float wetSig = 0.0f;

        /* SPECTRAL RESYNTHESIS MODE */
        if (spectralMode && stft) { 

            stft->forward(in[i]); // Forward FFT

            // Read from OLA buffer
            wetSig = spectBuf[spectPos];
            spectBuf[spectPos] = 0.0f;
            ++spectHopCounter;

            /* SYNTHESIZE FFT FRAMES */
            if (spectHopCounter >= hopN) { // Every hopN samples
                spectHopCounter = 0;
                if (stft->getSpectSize() > 0) {
                    // Map readPos to FFT frame index
                    float framePos = fmod(readPos / (float)hopN, (float)stft->getSpectSize());
                    if (framePos < 0) framePos += stft->getSpectSize();
                    // Interpolate spectral frame
                    STFT::FFTFrame interpFrame = stft->interpolateFrame(framePos);
                    // IFFT
                    stft->getFFT().inverse(interpFrame.bins.data(), spectFrame.data());
                    // Window and OLA
                    overlapAdd(spectBuf, spectFrame, EnvelopeType::HANN, spectPos);
                }
            }
            
            ++spectPos; if (spectPos >= fftN) spectPos = 0;
        } // End spectral resynthesis logic

        // Advance and wrap read pos
        readPos += rate;
        if (readPos < loopStartSamples) { readPos += loopLength; }
        else if (readPos >= loopEndSamples) { readPos -= loopLength; }
        float loopPos = (readPos - loopStartSamples) / loopLength; // Map to loop pos

        if (!spectralMode) wetSig = lerp(inBuf, readPos, bufN); // TIME DOMAIN MODE

        // Apply crossfade
        float crossfade = 1.0f;
        if (rate != 0.0f) {
            const float fadeLen = smooth;
            if (loopPos < fadeLen) { // Fade in
                float t = loopPos / fadeLen;
                crossfade = 0.5f * (1.0f - arm_cos_f32(M_PI * t)); 
            } else if (loopPos > 1.0f - fadeLen) { // Fade out
                float t = (loopPos - (1.0f - fadeLen)) / fadeLen;
                crossfade = 0.5f * (1.0f + arm_cos_f32(M_PI * t));
            }
        } wetSig *= crossfade;
    
        out[i] = dryWetMix(in[i], wetSig, mix); // Mix
    }
}