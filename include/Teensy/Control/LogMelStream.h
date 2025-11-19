#pragma once

#include "Teensy/Utilities/Log_Mel.h"

#include "AudioStream.h"

class LogMelStream : public AudioStream {
    private:
        audio_block_t * _inputQueueArray[1];
        Log_Mel logMel;

    public:
        LogMelStream();

        virtual void setMelCallback(std::function<void(const float*, size_t)> callback);
        virtual void update() override;
};