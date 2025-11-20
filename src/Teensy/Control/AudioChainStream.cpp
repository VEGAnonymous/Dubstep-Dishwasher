#include "Teensy/Control/AudioChainStream.h"

/* PRIVATE */

/*

audio_block_t * _inputQueueArray[1];

AudioChain &chain;
std::unique_ptr<ModulationEngine> modEngine;

*/

/* PUBLIC */

AudioChainStream::AudioChainStream(AudioChain &chain) 
    : AudioStream(1, _inputQueueArray), chain(chain) {
    modEngine = std::make_unique<ModulationEngine>(chain);
}

ModulationEngine* AudioChainStream::getModEngine() { return modEngine.get(); }

void AudioChainStream::update() {
    audio_block_t *inBlock = receiveReadOnly(0); // Receive block from upstream
    if (!inBlock) return;

    // Convert int16_t samples to float
    float buf[AUDIO_BLOCK_SAMPLES];
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) { buf[i] = (float)inBlock->data[i] / 32768.0f; }

    // Apply modulation
    float dt = (float)AUDIO_BLOCK_SAMPLES / SAMPLE_RATE;
    if (modEngine) modEngine->update(dt);
    
    // Process effect chain (in place)
    chain.processChain(buf, buf, AUDIO_BLOCK_SAMPLES);

    // Convert back to int16_t
    audio_block_t *outBlock = allocate();
    if (!outBlock) { release(inBlock); return; }
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) {
        float f = buf[i] * 32767.0f;
        // Clipping just in case
        if (f > 32767.0f) f = 32767.0f;
        else if (f < -32768.0f) f = -32768.0f;
        outBlock->data[i] = (int16_t) (int32_t) f;
    }

    transmit(outBlock, 0); // Send block to downstream
    release(inBlock); release(outBlock);
}