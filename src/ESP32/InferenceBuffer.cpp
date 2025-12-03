#include "ESP32/InferenceBuffer.h"
#include "Handler.h"

/* PRIVATE */

/*

static constexpr size_t INFERENCE_NUM_MELS = 64;
static constexpr size_t INFERENCE_NUM_FRAMES = 690; // (88200 // 128) + 1
static constexpr size_t INFERENCE_BUFFER_SIZE = INFERENCE_NUM_MELS * INFERENCE_NUM_FRAMES;

float* buffer;       // Full spectrogram buffer
int m_currentFrame;

Inference nn; // NN for inference

*/

// TODO: Remove all debug prints + time tracking

extern Handler<MelFrame, Command> handler;

// Brightness / Warmth / Intensity / Percussive / Speed
static constexpr uint8_t modulatorIds[] = {6, 7, 8, 9, 10};

void InferenceBuffer::processBuffer() {
    if (!buffer) return;

    unsigned long startTime = millis(); // TEMP

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
    
    // Normalize (in place)
    for (size_t i = 0; i < BUFFER_SIZE; ++i) buffer[i] = (buffer[i] - mean) / (std_dev + 1e-7f);

    // RUN INFERENCE
    float* output = nn.predict();

    unsigned long endTime = millis();
    if (output != nullptr) {
        Serial.printf("Inference: %lu ms\n", endTime - startTime);
        const char* qualities[] = {"Bright", "Warmth", "Intensity", "Percussive", "Speed"};

        // Send mapping values commands
        Command cmd = {};
        for (int i = 0; i < 5; ++i) {
            float value = max(0.0f, min(1.0f, output[i])); // clamp doesn't work for some reason
            uint8_t modId = modulatorIds[i];
            
            Serial.printf("%s: %.2f (-> modulator %d)\n", qualities[i], value, modId);

            // Build command
            cmd.cmd = static_cast<uint8_t>(CommandType::MOD_MAPPING_SET_INPUT); 
            cmd.id1 = modId; cmd.value1 = value; cmd.id2 = 0; cmd.value2 = 0.0f; cmd.value3 = 0.0f;

            handler.send(cmd);
        }
    }
}

/* PUBLIC */
        
InferenceBuffer::InferenceBuffer() : m_currentFrame(0), buffer(nullptr) {
    // Setup inference
    if (!nn.setup()) { Serial.println("Inference setup failed"); return; }
    buffer = nn.getInputBuffer(); // Point buffer to input tensor
    
    // DEBUG
    if (buffer == nullptr) Serial.println("Failed to get input buffer");
    else Serial.println("Inference ready");
}

void InferenceBuffer::addFrame(const MelFrame& frame) {
    if (buffer == nullptr) return;
    if (m_currentFrame >= INFERENCE_NUM_FRAMES) return;

    // Copy the incoming mel data into the correct slot in the large buffer
    memcpy(buffer + (m_currentFrame * NUM_MELS), frame.mel, NUM_MELS * sizeof(float));
    m_currentFrame++;

    // Check if the buffer is now full
    if (m_currentFrame == INFERENCE_NUM_FRAMES) {
        // Serial.println("Processing buffer");
        processBuffer();
        m_currentFrame = 0;
    }
}