#include "Teensy/Utilities/Log_Mel.h"

#include "Teensy/Defines.h"
#include "Teensy/LUTs.h"

#include "fastmath.h"

/* PRIVATE */

/*

static constexpr size_t fftSize = 512, hopFactor = 4;

STFT stft;
std::vector<float> powerSpec;
float melEnergies[NUM_MELS];
std::function<void(const float*, size_t)> melCallback; // (melEnergies, numMels)

*/

void Log_Mel::processSpectrum(STFT::FFTFrame& frame) { // Turn STFT spectrogram frames into log-mel spectrogram frames
    // Compute power spectrum
    for (size_t k = 0; k < NUM_BINS; ++k) {
        float re = frame.bins[k].r; float im = frame.bins[k].i;
        float mag; arm_sqrt_f32(re * re + im * im, &mag);
        powerSpec[k] = mag * mag;
    }

    // Init matrices
    arm_matrix_instance_f32 matMel; // Filterbank, [NUM_MELS x NUM_BINS]
    arm_matrix_instance_f32 matPower; // Power spectrum, [numBins x 1]
    arm_matrix_instance_f32 matOut; // Output mel spectrum, [NUM_MELS x 1]

    arm_mat_init_f32(&matMel, NUM_MELS, NUM_BINS, (float32_t*)MelMatrix);
    arm_mat_init_f32(&matPower, NUM_BINS, 1, powerSpec.data());
    arm_mat_init_f32(&matOut, NUM_MELS, 1, melEnergies);

    // Matrix multiply power spectrum with filterbank
    arm_mat_mult_f32(&matMel, &matPower, &matOut);

    // Take log of mel spectrum -> log-mel
    for (size_t m = 0; m < NUM_MELS; ++m) { melEnergies[m] = logf(melEnergies[m] + 1e-6f); }

    // Ready to send a new frame
    if (!melFrameReady) { // Only update if the previous frame has been sent
        melFrame.index = melFrameCounter++;
        melFrame.numMels = NUM_MELS;
        memcpy(melFrame.mel, melEnergies, NUM_MELS * sizeof(float));
        melFrameReady = true;
    }
}

/* PUBLIC */

Log_Mel::Log_Mel() : stft(fftSize, hopFactor, ((FFT_MAX_SIZE / (float)hopFactor) + 1.0f) / SAMPLE_RATE) { 
    // Set the STFT frame process callback to processSpectrum()
    powerSpec.resize(NUM_BINS);
    stft.setProcessCallback([this](STFT::FFTFrame& frame) { processSpectrum(frame); });
}

bool Log_Mel::isFrameReady() const { return melFrameReady; }
void Log_Mel::clearReady() { melFrameReady = false; }
MelFrame Log_Mel::getFrame() const { return melFrame; }

void Log_Mel::processBlock(const float* in, size_t n) {
    for (size_t i = 0; i < n; ++i) { stft.forward(in[i]); }
}