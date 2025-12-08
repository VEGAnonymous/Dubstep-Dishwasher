#pragma once

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

constexpr int kTensorArenaSize = 120 * 1024; // 120 kB

class Inference {
    private:
        tflite::ErrorReporter* errorReporter = nullptr;
        const tflite::Model* model = nullptr;
        tflite::MicroInterpreter* interpreter = nullptr;
        TfLiteTensor* input = nullptr;
        TfLiteTensor* output = nullptr;
        tflite::MicroMutableOpResolver<12> resolver;
        
        alignas(16) static uint8_t tensorArena[kTensorArenaSize];

        int8_t quantize(float x, float scale, int zero_point);

    public:
        Inference() {}
        friend class InferenceBuffer;

        bool setup();

        float* predict(const float* normalized_input, size_t input_size);
};