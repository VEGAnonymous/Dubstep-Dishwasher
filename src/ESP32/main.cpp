#include <Arduino.h>

#include "../Handler.h"

#define RX_PIN 18
#define TX_PIN 17

// Buffer to store incoming command
Command cmd;
size_t bytesRead = 0;

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {

    // Reading commands from primary serial (change to BLE later)
    while (Serial.available() && bytesRead < sizeof(Command)) {
        uint8_t* buf = (uint8_t*)&cmd;
        buf[bytesRead++] = Serial.read();
        
        // Debug: Print each received byte
        if (bytesRead == 1) {
            Serial.println("\nForwarding command to Teensy:");
        }
        Serial.printf("Byte %d: 0x%02X\n", bytesRead-1, buf[bytesRead-1]);
    }

    // Forward completed command to teensy
    if (bytesRead == sizeof(Command)) {
        cmd.checksum = computeChecksum(cmd);   
        Serial1.write((uint8_t*)&cmd, sizeof(Command));

        Serial.printf("cmd: %d | id1: %d | id2: %d | value: %.3f | checksum: 0x%02X\n", 
            cmd.cmd, cmd.id1, cmd.id2, cmd.value, cmd.checksum);

        bytesRead = 0; // Reset for next command
    }

    // Debug: Echo back any received commands
    if (Serial1.available()) {
        Command rcvCmd;
        size_t rcvBytes = 0;

        while (Serial1.available() && rcvBytes < sizeof(Command)) {
            uint8_t* buf = (uint8_t*)&rcvCmd;
            buf[rcvBytes++] = Serial1.read();
        }

        if (rcvBytes == sizeof(Command)) {
            if (verifyChecksum(rcvCmd)) {
                Serial.printf("cmd: %d | id1: %d | id2: %d | value: %.3f | checksum: 0x%02X (valid)\n");
            } else return; // Drop invalid packets
        }
    }
}