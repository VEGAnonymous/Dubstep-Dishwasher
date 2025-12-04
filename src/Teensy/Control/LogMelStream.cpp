#include "Teensy/Control/LogMelStream.h"

/* PRIVATE */

/*

audio_block_t * _inputQueueArray[1];
Log_Mel logMel;

*/

/* PUBLIC */

LogMelStream::LogMelStream() : AudioStream(1, _inputQueueArray), logMel() {}
LogMelStream::~LogMelStream() {}

void LogMelStream::update() {
    audio_block_t *inBlock = receiveReadOnly(0); // Receive block from upstream
    if (!inBlock) return;

    // Convert int16_t samples to float
    float buf[AUDIO_BLOCK_SAMPLES];
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) { buf[i] = (float)inBlock->data[i] / 32768.0f; }

    // Process log-mel spectrogram
    logMel.processBlock(buf, AUDIO_BLOCK_SAMPLES);

    release(inBlock);
}