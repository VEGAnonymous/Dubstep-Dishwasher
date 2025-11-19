#ifndef ESPDEFS
#define ESPDEFS

#include <Arduino.h>
#include "../Handler.h"

#define DEBUG 0

Command cmd;

/* processes incoming bytes from BLE
@param data Pointer to incoming byte data array
@param len Length of incoming data
*/
void processIncomingBytes(const uint8_t* data, size_t len) {
    uint8_t* toRead;
    uint8_t* buf = (uint8_t*)&cmd;
    for (int i = 0; i < (len / 8); ++i) {
        toRead = (uint8_t*)data + (i * 8);
        memcpy(buf, toRead, 8);

        // Forward command to teensy
        cmd.checksum = computeChecksum(cmd);
        Serial1.write((uint8_t*)&cmd, sizeof(Command));

        #if DEBUG
        // Debug: Print forwarded command   
        if (i < 5) {
            Serial.printf("(%d) cmd: %d | id1: %d | id2: %d | value: %.3f | checksum: 0x%02X\n",
            i, cmd.cmd, cmd.id1, cmd.id2, cmd.value, cmd.checksum);
        }
        #endif
    }
}

#endif // ESPDEFS