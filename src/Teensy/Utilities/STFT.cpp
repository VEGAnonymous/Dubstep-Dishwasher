#include "Teensy/Utilities/STFT.h"
#include "Teensy/Utilities/Utilities.h"

/* PUBLIC */

/*

struct FFTFrame { // Stores a frame of complex FFT bins
    std::vector<fft_cpx> bins;
    FFTFrame(size_t numBins) : bins(numBins) {}
};

*/

/* PRIVATE */

/*

const float bufDur;

FFT fft;
size_t fftSize, numBins, hopSize, hopFactor = 4;

// Time-domain
std::vector<float> inBuf, outBuf;
size_t inPos = 0, outPos = 0, hopCounter = 0;

// Spectral-domain
std::vector<FFTFrame> spectrogram; // Store FFT frames
std::vector<float> forwardFrame, inverseFrame;
size_t spectPos = 0, spectSize;
std::deque<size_t> processingQueue; // Queue frame indices ready for IFFT
std::function<void(FFTFrame&)> processCallback; // Function to process frames (for phase vocoding)

*/

/* PUBLIC */

STFT::STFT(size_t fftSize, size_t hopFactor, float bufDur) : bufDur(bufDur), fft(fftSize), hopFactor(hopFactor) { setFFTSize(fftSize); }

// Exposing this shit for external use
size_t STFT::getSpectSize() const { return spectSize; }
size_t STFT::getFFTSize() const { return fftSize; }
size_t STFT::getHopSize() const { return hopSize; }
size_t STFT::getNumBins() const { return numBins; }
FFT& STFT::getFFT() { return fft; }
STFT::FFTFrame& STFT::getFrame() { // Get most recent FFT frame (should probably add index parameter)
    size_t index = (spectPos == 0) ? (spectSize - 1) : (spectPos - 1);
    return spectrogram[index];
}

void STFT::setFFTSize(size_t N) { // [128, FFT_MAX_SIZE], MUST BE POWER OF 2 (but I can't make you)
    fftSize = std::clamp(N, (size_t)128, (size_t)FFT_MAX_SIZE); numBins = (fftSize / 2) + 1; 
    hopSize = fftSize / hopFactor;
    
    inPos = 0; outPos = 0; hopCounter = 0;
    inBuf.resize(fftSize, 0.0f); outBuf.assign(fftSize, 0.0f);

    fft.setFFTSize(fftSize);

    forwardFrame.resize(fftSize, 0.0f); inverseFrame.resize(fftSize, 0.0f);

    // Init spectrogram
    spectrogram.clear();
    spectSize = (size_t)(bufDur * ((float)SAMPLE_RATE / (float)hopSize));
    for (size_t i = 0; i < spectSize; ++i) { spectrogram.emplace_back(numBins); }
    spectPos = 0;
}
void STFT::setHopSize(size_t hopFactor) { // [2, 8]
    this->hopFactor = std::clamp(hopFactor, (size_t)2, (size_t)8); 
    setFFTSize(fftSize);
}
void STFT::setProcessCallback(std::function<void(FFTFrame&)> callback) { processCallback = callback; }

void STFT::forward(float input) { // Forward pass, store FFT frames in spectrogram
    const size_t fftN = fftSize, hopN = hopSize;
    inBuf[inPos] = input;
    ++inPos; if (inPos >= fftSize) inPos = 0;
    ++hopCounter;
    
    /* GENERATE FFT FRAMES */
    if (hopCounter >= hopN) { // Every hopN samples
        hopCounter = 0;
        
        // Extract full FFT frame from circular buffer
        size_t readPos = inPos; // Start from oldest sample
        for (size_t j = 0; j < fftN; ++j) {
            forwardFrame[j] = inBuf[readPos] * getEnvelopeValue((float)j / fftN, EnvelopeType::HANN);
            ++readPos; if (readPos >= fftN) readPos = 0;
        }
        
        // Forward FFT, store in spectrogram buffer
        fft.forward(forwardFrame.data(), spectrogram[spectPos].bins.data());

        // Process frame if applicable
        if (processCallback) processCallback(spectrogram[spectPos]);
        // Enqueue frame for IFFT 
        processingQueue.push_back(spectPos); 
        
        ++spectPos; if (spectPos >= spectSize) spectPos = 0;
    }
}

float STFT::inverse() { // Reconstruct signal from spectrogram (ideally sync this with forward() to match inPos/outPos)
    float out = outBuf[outPos];
    outBuf[outPos] = 0.0f; // Clear output for OLA

    /* RECONSTRUCT FFT FRAMES */
    if (!processingQueue.empty()) { // Process queued frames
        size_t framePos = processingQueue.front();
        
        fft.inverse(spectrogram[framePos].bins.data(), inverseFrame.data()); // Get time domain signal
        overlapAdd(outBuf, inverseFrame, EnvelopeType::HANN, outPos); // OLA into output
        
        processingQueue.pop_front();
    }

    ++outPos; if (outPos >= fftSize) outPos = 0;
    return out;
}

STFT::FFTFrame STFT::interpolateFrame(float framePos) { // Get frame from spectrogram at a fractional position
    if (spectrogram.empty()) { return FFTFrame(numBins); }
    
    int frameIndex = (int)floor(framePos);
    float frameFrac = framePos - floor(framePos);
    size_t frame0 = frameIndex % spectSize;
    size_t frame1 = (frame0 + 1) % spectSize;
    
    FFTFrame interpFrame(numBins);
    for (size_t k = 0; k < numBins; ++k) {
        // Lerp real and imaginary components
        interpFrame.bins[k].r = lerp(spectrogram[frame0].bins[k].r, spectrogram[frame1].bins[k].r, frameFrac);
        interpFrame.bins[k].i = lerp(spectrogram[frame0].bins[k].i, spectrogram[frame1].bins[k].i, frameFrac);
    }
    return interpFrame;
}