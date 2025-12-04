#pragma once

#include "Teensy/Utilities/Log_Mel.h"

#include "AudioStream.h"

class LogMelStream : public AudioStream {
    private:
        audio_block_t * _inputQueueArray[1];
        Log_Mel logMel;

    public:
        LogMelStream();
        virtual ~LogMelStream();

        bool isFrameReady() const { return logMel.isFrameReady(); }
        void clearReady() { logMel.clearReady(); }
        MelFrame getFrame() const { return logMel.getFrame(); }

        virtual void update() override;
};