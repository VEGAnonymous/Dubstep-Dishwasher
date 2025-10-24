#include <Arduino.h>

// Recommended pins for UART1 on ESP32
#define RX_PIN 18
#define TX_PIN 17

// Command structure (8 bytes total)
struct Command {
    uint8_t cmd;        // Command type (0 = Add, 1 = Update, 2 = Reorder)
    uint8_t id1;        // Effect to add, parameter id, or first effect id for reorder
    uint8_t id2;        // Second effect id for reorder
    uint8_t reserved;   // Reserved/padding
    float value;        // Parameter value (float)
} __attribute__((packed));

// Buffer to store incoming command
Command cmdBuffer;
size_t bytesRead = 0;

void setup() {
    // USB/primary serial (debug input/output)
    Serial.begin(115200);
    Serial.println("UART Command Bridge");
    
    // UART1 to Teensy
    Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {
    // Reading commands from primary serial (change to BLE later)
    while (Serial.available() && bytesRead < sizeof(Command)) {
        uint8_t* buf = (uint8_t*)&cmdBuffer;
        buf[bytesRead++] = Serial.read();
        
        // Debug: Print each received byte
        if (bytesRead == 1) {
            Serial.println("\nForwarding command to Teensy:");
        }
        Serial.printf("Byte %d: 0x%02X\n", bytesRead-1, buf[bytesRead-1]);
    }

    // Forward completed command to teensy
    if (bytesRead == sizeof(Command)) {
        Serial1.write((uint8_t*)&cmdBuffer, sizeof(Command));

        // Debug: Print nicely formatted command echo
        Serial.print("\n=== Command Echo ===\n");
        Serial.printf("cmd: %c | ", cmdBuffer.cmd);
        Serial.printf("id1: %c | ", cmdBuffer.id1);
        Serial.printf("id2: %c | ", cmdBuffer.id2);
        Serial.printf("value: %.3f\n", cmdBuffer.value);
        Serial.println("==================");

        bytesRead = 0;  // Reset for next command
    }
    
    delay(10);

    // Debug: Echo back any received commands (since GPIO17/18 are connected)
    // if (Serial1.available()) {
    //     Command rcvCmd;
    //     size_t rcvBytes = 0;
        
    //     while (Serial1.available() && rcvBytes < sizeof(Command)) {
    //         uint8_t* buf = (uint8_t*)&rcvCmd;
    //         buf[rcvBytes++] = Serial1.read();
    //     }
        
    //     if (rcvBytes == sizeof(Command)) {
    //         Serial.print("\n=== Received Echo ===\n");
    //         Serial.printf("cmd: %c | ", rcvCmd.cmd);
    //         Serial.printf("id1: %c | ", rcvCmd.id1);
    //         Serial.printf("id2: %c | ", rcvCmd.id2);
    //         Serial.printf("value: %.3f\n", rcvCmd.value);
    //         Serial.println("===================");
    //     }
    // }
}