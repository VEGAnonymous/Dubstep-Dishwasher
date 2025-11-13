#include "Teensy/Effects/SpectralGate.h"
#include "Teensy/Utilities/Utilities.h"

/* PRIVATE */

/*
enum Params : ParamID { THRESHOLD = 2, TILT, INVERT };

float threshold, tilt; bool invert;

*/

/* PUBLIC */
        
SpectralGate::SpectralGate(float mix, float thresholdDB, float tilt, size_t fftSize, size_t hopFactor) 
    : Phase_Vocoder(mix, fftSize, hopFactor) { setThreshold(thresholdDB); setTilt(tilt); }

void SpectralGate::setThreshold(float thresholdDB) { threshold = dbAmp(std::clamp(thresholdDB, -100.0f, 0.0f)); } // dB, [-100.0, 0.0]
void SpectralGate::setTilt(float tilt) { this->tilt = std::clamp(tilt, -1.0f, 1.0f) * 2.0f; } // [-1.0, 1.0]
void SpectralGate::setInvert(bool invert) { this->invert = invert; }
void SpectralGate::setParam(ParamID param, float value) {
    switch (param) {
        case THRESHOLD: setThreshold(value); break;
        case TILT: setTilt(value); break;
        case INVERT: setInvert(value > 0.5f); break;
        default: Phase_Vocoder::setParam(param, value);
    }
}

/* PROTECTED */

void SpectralGate::processSpectrum(STFT::FFTFrame& frame) {
    const size_t numBins = frame.bins.size();
    const float binTilt = tilt / (float)(numBins - 1);
    const float threshold_sq = threshold * threshold;
    
    for (size_t k = 0; k < numBins; ++k) {
        // + tilt gates lows more, - tilt gates highs more
        float weight = 1.0f - ((k * binTilt) - (tilt * 0.5f));
        if (weight < 0.0f) weight = 0.0f;
        
        // Compare squared magnitudes
        float mag_sq = (frame.bins[k].r * frame.bins[k].r) + (frame.bins[k].i * frame.bins[k].i);
        float weight_sq = weight * weight;
    
        bool gateCond = (mag_sq * weight_sq) < threshold_sq; // Gate bins under threshold
        if (invert) gateCond = !gateCond; // Or the other way around
        if (gateCond) { frame.bins[k].r = 0.0f; frame.bins[k].i = 0.0f; }
    }
}