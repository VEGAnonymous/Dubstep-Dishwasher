#pragma once

#include "Handler.h"

#include <Arduino.h>
#include <cmath>

class InferenceBuffer {
    private:
        static constexpr size_t INFERENCE_NUM_MELS = 64;
        static constexpr size_t INFERENCE_NUM_FRAMES = 690; // (88200 // 128) + 1
        static constexpr size_t INFERENCE_BUFFER_SIZE = INFERENCE_NUM_MELS * INFERENCE_NUM_FRAMES;

        float* buffer; // Full spectrogram buffer
        int m_currentFrame;

        void processBuffer();
        
    public:
        InferenceBuffer();
        ~InferenceBuffer() { delete[] buffer; }

        void addFrame(const MelFrame& frame);
};