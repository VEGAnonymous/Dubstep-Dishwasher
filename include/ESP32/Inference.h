#pragma once

#include "Model.h"

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

constexpr int kTensorArenaSize = 250 * 1024; // 250 kB

class Inference {
    private:
        tflite::ErrorReporter* errorReporter = nullptr;
        const tflite::Model* model = nullptr;
        tflite::MicroInterpreter* interpreter = nullptr;
        TfLiteTensor* input = nullptr;
        TfLiteTensor* output = nullptr;
        tflite::AllOpsResolver resolver;
        
        uint8_t *tensorArena;

    public:
        Inference() { tensorArena = new uint8_t[kTensorArenaSize]; }

        bool setup() {
            static tflite::MicroErrorReporter microErrorReporter;
            errorReporter = &microErrorReporter;

            // Load model
            model = tflite::GetModel(model_data);
            if (model->version() != TFLITE_SCHEMA_VERSION) {
                TF_LITE_REPORT_ERROR(errorReporter, "Model version mismatch");
                return false;
            }

            // Init interpreter
            static tflite::MicroInterpreter staticInterpreter(model, resolver, tensorArena, kTensorArenaSize, errorReporter);
            interpreter = &staticInterpreter;

            // Allocate tensors
            TfLiteStatus allocateStatus = interpreter->AllocateTensors();
            if (allocateStatus != kTfLiteOk) {
                TF_LITE_REPORT_ERROR(errorReporter, "AllocateTensors() failed");
                return false;
            }

            // Get pointers
            input = interpreter->input(0);
            output = interpreter->output(0);

            return true;
        }

        float* getInputBuffer() { return input->data.f; }

        float* predict() {
            TfLiteStatus invokeStatus = interpreter->Invoke();
            if (invokeStatus != kTfLiteOk) {
                TF_LITE_REPORT_ERROR(errorReporter, "Invoke failed");
                return nullptr;
            }
            return output->data.f;
        }
};