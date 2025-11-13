#include "ESP32/InferenceBuffer.h"

/* PRIVATE */

/*

static constexpr float DUR_S = 2.0f;
static constexpr size_t NUM_MELS = 64;
static constexpr size_t NUM_FRAMES = 690; // (88200 // 128) + 1
static constexpr size_t BUFFER_SIZE = NUM_MELS * NUM_FRAMES;

float* buffer;       // Full spectrogram buffer
int m_currentFrame;

*/

void InferenceBuffer::processBuffer() {
    // Compute mean and std. dev.
    float sum = 0.0f; float sum_sq = 0.0f;

    for (size_t i = 0; i < BUFFER_SIZE; ++i) {
        float val = buffer[i];
        sum += val;
        sum_sq += val * val;
    }

    float mean = sum / BUFFER_SIZE;
    float variance = (sum_sq / BUFFER_SIZE) - (mean * mean);
    float std_dev = sqrt(fmaxf(0.0f, variance));

    Serial.printf("Mean: %.4f, Std: %.4f\n", mean, std_dev);

    // Normalize
    for (size_t i = 0; i < BUFFER_SIZE; ++i) buffer[i] = (buffer[i] - mean) / (std_dev + 1e-7f);

    // TODO: run inference
}

/* PUBLIC */
        
InferenceBuffer::InferenceBuffer() : m_currentFrame(0) {
    // Allocate the buffer on the heap
    buffer = new (std::nothrow) float[BUFFER_SIZE];
    if (buffer == nullptr) Serial.println("Failed to allocate"); // Probably already crashed lmao
    else Serial.println("Inference buffer allocated");
}

void InferenceBuffer::addFrame(const MelFrame& frame) {
    if (buffer == nullptr) return;

    // Copy the incoming mel data into the correct slot in the large buffer
    memcpy(buffer + (m_currentFrame * NUM_MELS), frame.mel, NUM_MELS * sizeof(float));
    m_currentFrame++;

    // Check if the buffer is now full
    if (m_currentFrame == NUM_FRAMES) {
        // Serial.println("Processing buffer");
        processBuffer();
        
        m_currentFrame = 0;
    }
}