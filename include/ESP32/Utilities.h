#pragma once

#include <Arduino.h>
#include "Handler.h"

#define DEBUG 1

extern Handler<MelFrame, Command> handler;

/* processes incoming bytes from BLE
@param data Pointer to incoming byte data array
@param len Length of incoming data
*/
void processIncomingBytes(const uint8_t* data, size_t len) {
    static uint8_t buffer[sizeof(Command)];
    static size_t bufferPos = 0;

    // for loop instead of while since it's not UART
    for (size_t i = 0; i < len; ++i) {
        buffer[bufferPos++] = data[i];

        // Debug: Print each received byte
        // if (bytesRead == 1) Serial.println("\nForwarding command to Teensy:");
        // Serial.printf("Byte %d: 0x%02X\n", bytesRead-1, buf[bytesRead-1]);

        // Forward completed command to teensy
        if (bufferPos == sizeof(Command)) {
            Command cmd;
            memcpy(&cmd, buffer, sizeof(Command));
            handler.send(cmd);

            // Debug: Print forwarded command   
            Serial.printf("cmd: %d | id1: %d | id2: %d | value1: %.3f | value2: %.3f | value3: %.3f | checksum: 0x%02X\n", 
                       cmd.cmd, cmd.id1, cmd.id2, cmd.value1, cmd.value2, cmd.value3, cmd.checksum);

            bufferPos = 0; // Reset for next command
        }
    }
}