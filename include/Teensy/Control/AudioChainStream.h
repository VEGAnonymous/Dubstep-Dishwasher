#pragma once

#include "Teensy/Control/AudioChain.h"
#include "Teensy/Control/ModulationEngine.h"

#include "AudioStream.h"

class AudioChainStream : public AudioStream {
    private:
        audio_block_t * _inputQueueArray[1];

        AudioChain &chain;
        std::unique_ptr<ModulationEngine> modEngine;

    public:
        AudioChainStream(AudioChain &chain);

        ModulationEngine* getModEngine();

        virtual void update() override;
};