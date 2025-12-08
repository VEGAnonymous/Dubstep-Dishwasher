#include "ESP32/Inference.h"
#include "ESP32/Model.h"

#include <Arduino.h>

alignas(16) uint8_t Inference::tensorArena[kTensorArenaSize];

/* PRIVATE */

/*

tflite::ErrorReporter* errorReporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;
tflite::MicroMutableOpResolver<12> resolver;

alignas(16) static uint8_t tensorArena[kTensorArenaSize];

*/

int8_t Inference::quantize(float x, float scale, int zero_point) {
    float tmp = x / scale + (float)zero_point;
    return (int8_t)max(-128.0f, min(127.0f, tmp));
}

/* PUBLIC */

bool Inference::setup() {
    static tflite::MicroErrorReporter microErrorReporter;
    errorReporter = &microErrorReporter;

    // Load model
    model = tflite::GetModel(model_data);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        TF_LITE_REPORT_ERROR(errorReporter, "Model version mismatch");
        return false;
    }

    // Add ops to resolver
    resolver.AddQuantize();
    resolver.AddDequantize();

    resolver.AddPad();
    resolver.AddDepthwiseConv2D();
    resolver.AddConv2D();
    
    resolver.AddAveragePool2D();
    resolver.AddReshape();
    resolver.AddMaxPool2D();
    
    resolver.AddConcatenation();
    resolver.AddFullyConnected();
    resolver.AddMinimum();
    resolver.AddRelu();

    // Init interpreter
    static tflite::MicroInterpreter staticInterpreter(
        model, resolver, tensorArena, kTensorArenaSize, errorReporter
    );
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

    // DEBUG
    Serial.printf("Input tensor: dims=%d, bytes=%d, type=%d\n", 
                  input->dims->size, input->bytes, input->type);
    Serial.printf("  Shape: ");
    for (int i = 0; i < input->dims->size; i++) {
        Serial.printf("%d ", input->dims->data[i]);
    }
    Serial.printf("\n  Scale: %f, Zero point: %d\n", 
                  input->params.scale, input->params.zero_point);
    
    Serial.printf("Output tensor: dims=%d, bytes=%d, type=%d\n", 
                  output->dims->size, output->bytes, output->type);
    Serial.printf("  Scale: %f, Zero point: %d\n",
                  output->params.scale, output->params.zero_point);
    
    // Check shape
    if (input->dims->size != 4 || 
        input->dims->data[1] != 64 || 
        input->dims->data[2] != 87 || 
        input->dims->data[3] != 1)
        return false;

    // Check quantization
    if (input->type != kTfLiteInt8) Serial.printf("Input type: %d (expected %d for int8)\n", input->type, kTfLiteInt8);
    if (output->type != kTfLiteInt8) Serial.printf("Output type: %d (expected %d for int8)\n", output->type, kTfLiteInt8);

    return true;
}

float* Inference::predict(const float* inputNorm, size_t inputSize) {
    if (input->type != kTfLiteInt8) return nullptr;

    static float outputDecode[5];
    
    // Quantize input
    int8_t* inputTensor = input->data.int8;
    for (size_t i = 0; i < inputSize; ++i) {
        float q = (inputNorm[i] / input->params.scale) + input->params.zero_point;
        inputTensor[i] = (int8_t)max(-128.0f, min(127.0f, q));
    }

    // Run inference
    TfLiteStatus invokeStatus = interpreter->Invoke();
    if (invokeStatus != kTfLiteOk) {
        TF_LITE_REPORT_ERROR(errorReporter, "Invoke failed");
        return nullptr;
    }

    // De-quantize output
    for (int i = 0; i < 5; i++) {
        outputDecode[i] = (output->data.int8[i] - output->params.zero_point) * output->params.scale;
    }

    return outputDecode;
}