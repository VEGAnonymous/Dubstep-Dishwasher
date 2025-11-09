#pragma once

#include <Arduino.h>
#include "Handler.h"

#define DEBUG 1

size_t bytesRead = 0;
Command cmd;

/* processes incoming bytes from BLE
@param data Pointer to incoming byte data array
@param len Length of incoming data
*/
void processIncomingBytes(const uint8_t* data, size_t len) {
    // for loop instead of while since it's not UART
    for (size_t i = 0; i < len; ++i) {
        // break after full command
        if (bytesRead >= sizeof(Command)) {
            break;
        }

        uint8_t* buf = (uint8_t*)&cmd;
        buf[bytesRead++] = data[i];

        #if DEBUG   
        // Debug: Print each received byte
        if (bytesRead == 1) {
            Serial.println("\nForwarding command to Teensy:");
        }
        Serial.printf("Byte %d: 0x%02X\n", bytesRead-1, buf[bytesRead-1]);
        #endif
    }

    // Forward completed command to teensy
    if (bytesRead == sizeof(Command)) {
        cmd.checksum = computeChecksum(cmd);
        Serial1.write((uint8_t*)&cmd, sizeof(Command));

        #if DEBUG
        // Debug: Print forwarded command   
        Serial.printf("cmd: %d | id1: %d | id2: %d | value: %.3f | checksum: 0x%02X\n",
            cmd.cmd, cmd.id1, cmd.id2, cmd.value, cmd.checksum);
        #endif

        bytesRead = 0; // Reset for next command
    }
}