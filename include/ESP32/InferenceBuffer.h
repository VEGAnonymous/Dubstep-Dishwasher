#pragma once

#include "Handler.h"

#include <Arduino.h>
#include <cmath>

class InferenceBuffer {
    private:
        static constexpr size_t NUM_MELS = 64;
        static constexpr size_t NUM_FRAMES = 690; // (88200 // 128) + 1
        static constexpr size_t BUFFER_SIZE = NUM_MELS * NUM_FRAMES;

        float* buffer; // Full spectrogram buffer
        int m_currentFrame;

        void processBuffer();
        
    public:
        InferenceBuffer();
        ~InferenceBuffer() { delete[] buffer; }

        void addFrame(const MelFrame& frame);
};