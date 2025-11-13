#pragma once

#include "Teensy/Control/AudioChain.h"

#include "AudioStream.h"

class AudioChainStream : public AudioStream {
    private:
        audio_block_t * _inputQueueArray[1];
        AudioChain &chain;

    public:
        AudioChainStream(AudioChain &chain);

        virtual void update() override;
};