#pragma once

#include "Handler.h"
#include "Inference.h"

#include <Arduino.h>
#include <cmath>

class InferenceBuffer {
    private:
        static constexpr size_t INFERENCE_NUM_MELS = 64;
        static constexpr size_t INFERENCE_NUM_FRAMES = 87; // ((44100 * 0.25s) // 128) + 1
        static constexpr size_t INFERENCE_BUFFER_SIZE = INFERENCE_NUM_MELS * INFERENCE_NUM_FRAMES;

        float* buffer; // Full spectrogram buffer
        int m_currentFrame;

        Inference nn; // NN for inference

        void processBuffer();
        
    public:
        InferenceBuffer();
        ~InferenceBuffer() { delete[] buffer; }

        void addFrame(const MelFrame& frame);
};