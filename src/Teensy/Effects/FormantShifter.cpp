#include "Teensy/Effects/FormantShifter.h"
#include "Teensy/Utilities/Utilities.h"

// Alexander Panos' GOATed formant shifter device, faithfully ported from Max/MSP (Gen) to C++!
// https://alexanderpanos.com/software

/* PRIVATE */

/*

enum Params : ParamID { FORMANT_SHIFT = 2, ENVELOPE_WIDTH };

float formantShift; size_t envelopeWidth;
size_t width; // internal value after env_compensation

std::vector<float> buf;
std::vector<float> interp; // interpolated peaks
std::vector<std::pair<float, size_t>> pk_info; // ch.0 = value of peak, ch.1 = index of peak in "buf"
std::vector<float> formants;

*/

/* PUBLIC */

FormantShifter::FormantShifter(float mix, float formantShift, size_t envelopeWidth, size_t fftSize, size_t hopFactor) 
: Phase_Vocoder(mix, fftSize, hopFactor) { 
    setFormantShift(formantShift); setEnvelopeWidth(envelopeWidth);
    
    // Allocate buffers
    const size_t numBins = (fftSize / 2) + 1;
    buf.resize(numBins); interp.resize(numBins); formants.resize(numBins);
}

void FormantShifter::setFormantShift(float formantShift) { this->formantShift = std::clamp(formantShift, -12.0f, 12.0f); } // semitones, [-12.0, 12.0]
void FormantShifter::setEnvelopeWidth(size_t envelopeWidth) { 
    this->envelopeWidth = std::clamp(envelopeWidth, (size_t)2, stft.getNumBins() / (size_t)4); 
    // env_compensation
    const size_t srComp = 1; // hardcoded for SAMPLE_RATE = 44.1 kHz
    float scaled = 0.0f;
    if (formantShift > 0) { 
        scaled = scale((float)std::clamp(formantShift, 0.0f, 12.0f), 
        0.0f, 12.0f, 10.0f, 5.0f, 2.5f); 
    } else { 
        scaled = scale((float)std::clamp(formantShift, -12.0f, 0.0f), 
        -12.0f, 0.0f, 16.0f, 10.0f, 1.0f);    
    }
    width = (size_t)scaled + (envelopeWidth - (size_t)8);
    width += srComp;
    width = std::clamp(width, (size_t)2, (size_t)32);
}
void FormantShifter::setParam(ParamID param, float value) {
    switch (param) {
        case FORMANT_SHIFT: setFormantShift(value); break;
        case ENVELOPE_WIDTH: setEnvelopeWidth((size_t)value); break;
        default: Phase_Vocoder::setParam(param, value);
    }
}

/* PROTECTED */

void FormantShifter::processSpectrum(STFT::FFTFrame& frame) {
    if (formantShift == 0.0f) return;

    const size_t len = stft.getNumBins(); // dim(buf)
    const float shift = exp2f(formantShift / 12.0f);
    const float epsilon = 1e-6f;
    
    const float cutoff = 8000.0f; // band limit to save compute
    const size_t maxBin = std::min(len, static_cast<size_t>((cutoff / (SAMPLE_RATE * 0.5f)) * (float)(len - 1)));
    const size_t num_regions = (maxBin + width - 1) / width; // number of equidistant regions within the buffer to find peaks

    // store magnitudes in "buf"
    for (size_t k = 0; k < maxBin; ++k) {
        float re = frame.bins[k].r; float im = frame.bins[k].i;
        arm_sqrt_f32(re * re + im * im, &buf[k]);
    }

    /*————— INITIALIZE BUFFERS —————*/
    // std::fill(interp.begin(), interp.end(), 0.0f); // clear
    
    pk_info.clear();
    pk_info.reserve(num_regions + 2);
    pk_info.push_back({buf[0], 0}); // set first element in pk_info to the first element in "buf" buffer
    
    /*————— PEAK DETECTION MAIN LOOP —————*/
    size_t local_len = width;
    for (size_t j = 0; j < num_regions; ++j) {
        float loc_max = 0.0f; float loc_sum = 0.0f; size_t max_idx = 0;
        
        // calculate mean of local region
        size_t start = (local_len > width) ? (local_len - width) : 0;
        size_t end = std::min(start + width, len);
        for (size_t i = start; i < end; ++i) {
            const float v = buf[i];
            loc_sum += v;
            if (v > loc_max) { loc_max = v; max_idx = i; } // local maxima
        }
        const float loc_avg = loc_sum / ((end > start) ? (float)(end - start) : 1.0f);

        // use absolute maximum if no local maxima
        size_t idx = max_idx;
        float val = loc_max;
        for (size_t i = start; i < end; ++i) {
            const float v = buf[i];
            if (v > loc_avg && v >= val) { idx = i; val = v; }
        }
        pk_info.push_back({val, idx});
        
        local_len += width;
    }
    
    pk_info.push_back({buf[len - 1], len - 1}); // set last element in pk_info to the last element in "buf"
    
    /*————— PEAK INTERPOLATION MAIN LOOP —————*/
    for (size_t j = 1; j < pk_info.size(); ++j) { // start loop at j=1; 0th index was set to the first element in "buf"
        size_t start_pos = pk_info[j - 1].second;
        size_t distance = pk_info[j].second;
        float previous_pk = pk_info[j - 1].first;
        float current_pk = pk_info[j].first; // the literal value of the peak

        if (distance <= start_pos) continue;
        for (size_t i = start_pos; i < distance; ++i) {
            float val = scale((float)i, (float)start_pos, (float)distance, previous_pk, current_pk);
            interp[i] = val;
        }
    }
    interp[len - 1] = pk_info.back().first;
    
    /*————— FORMANT SHIFT ROUTINE —————*/
    const float maxIdx = (float)(len - 1);
    const float inv_shift = 1.0f / shift;
    for (size_t i = 0; i < len; ++i) {
        float sourceIdx = (float)i * inv_shift;
        
        // poke(formants, valf, i, boundmode="clip")
        if (sourceIdx <= 0.0f) { formants[i] = interp[0];
        } else if (sourceIdx > maxIdx) { formants[i] = interp[len - 1];
        } else { formants[i] = lerp(interp, sourceIdx, len); }
    }
    
    /*————— CONVOLUTION —————*/
    for (size_t i = 1; i < maxBin; ++i) {
        if (interp[i] <= epsilon) { formants[i] = 0.0f; continue; } // avoid divide-by-zero

        float det = buf[i] / (interp[i] + epsilon); // deconvolution to get spectral detail
        float spectrum = det * formants[i]; // convolution of spectral detail & shifted spectral envelope
        if (spectrum > 10.0f * buf[i]) spectrum = 10.0f * buf[i]; // clamp extreme gains
        formants[i] = spectrum;
        
        // apply to complex FFT bins (preserve phase)
        if (buf[i] > epsilon) {
            const float scaleFactor = spectrum / buf[i];
            frame.bins[i].r *= scaleFactor; frame.bins[i].i *= scaleFactor;
        } else { frame.bins[i].r = 0.0f; frame.bins[i].i = 0.0f; }
    }
}