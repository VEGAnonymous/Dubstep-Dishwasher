#pragma once

#include <Arduino.h>
#include "Handler.h"

#define DEBUG 1

extern Handler<MelFrame, Command> handler;

// Processes incoming bytes from BLE
void processIncomingBytes(const uint8_t* data, size_t len) {
    static uint8_t buffer[sizeof(CommandBLE)];
    static size_t bufferPos = 0;

    for (size_t i = 0; i < len; ++i) {
        buffer[bufferPos++] = data[i];

        // Forward completed command
        if (bufferPos == sizeof(CommandBLE)) {
            CommandBLE rcvCmd;
            memcpy(&rcvCmd, buffer, sizeof(CommandBLE));

            Command sndCmd; memset(&sndCmd, 0, sizeof(Command));
            sndCmd.sync = 0xAA55;
            sndCmd.cmd = rcvCmd.cmd;
            sndCmd.id1 = rcvCmd.id1;
            sndCmd.id2 = rcvCmd.id2;
            sndCmd.value1 = rcvCmd.value1;
            sndCmd.value2 = rcvCmd.value2;
            sndCmd.value3 = rcvCmd.value3;
            handler.send(sndCmd);
            
            // Debug: Print forwarded command 
            Serial.printf("cmd: %d | id1: %d | id2: %d | value1: %.3f | value2: %.3f | value3: %.3f | checksum: 0x%02X\n", 
                    sndCmd.cmd, sndCmd.id1, sndCmd.id2, sndCmd.value1, sndCmd.value2, sndCmd.value3, sndCmd.checksum);

            bufferPos = 0; // Reset for next command
        }
    }
}