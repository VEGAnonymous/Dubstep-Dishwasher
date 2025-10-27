#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "Defines.h"
#include "Utilities.h"
#include "Modules.h"
#include "Generators.h"
#include "Filters.h"
#include "Effects.h"
#include "Control.h"
#include "LUTs.h"

// TESTING - 
const bool USB_IO = false, // <-- SET FLAGS HERE
           LOG_DBG = false;

AudioInputUSB usbIn; 
AudioOutputUSB usbOut;

uint32_t logTime = 0; // Resource usage monitoring (temp)

// IO
AudioInputI2S adcIn; // ADC input
AudioOutputI2S dacOut; // DAC output

// DSP
std::unique_ptr<AudioChain> chain;
std::unique_ptr<AudioChainStream> stream;

// Connections
std::unique_ptr<AudioConnection> patch1, patch2;

// Command structure
struct Command {
    uint8_t cmd;        // Command type (0 = Add, 1 = Remove, 2 = Reorder, 3 = Set param, 4 = Bypass)
    uint8_t id1;        // Effect id 1
    uint8_t id2;        // Effect id 2 or ParamID
    uint8_t checksum;   // Checksum (XOR)
    float value;        // Parameter value (float)
} __attribute__((packed));

// Checksum utilities
uint8_t calculateChecksum(const Command& cmd) {
    uint8_t checksum = cmd.cmd ^ cmd.id1 ^ cmd.id2;
    const uint8_t* valueBytes = (const uint8_t*)&cmd.value;
    for(int i = 0; i < (int)sizeof(float); ++i) {
        checksum ^= valueBytes[i];
    }
    return checksum;
}

// Verify checksum of received command
bool verifyChecksum(const Command& cmd) {
    uint8_t storedChecksum = cmd.checksum;
    Command tempCmd = cmd;
    tempCmd.checksum = 0; // Zero out checksum for calculation
    return (storedChecksum == calculateChecksum(tempCmd));
}

// Parse commands
void processCommand(Command& cmd, AudioChain& chain) {
    Serial.println(cmd.cmd);
    switch (cmd.cmd) {
        case ADD: /* chain.addEffect(cmd.id1); */ Serial.printf("Added effect %d\n", cmd.id1); break; // Add effect
        case REMOVE: chain.removeEffect(cmd.id1); Serial.printf("Removed effect %d\n", cmd.id1); break; // Remove effect
        case REORDER: chain.swapEffects(cmd.id1, cmd.id2); Serial.printf("Swapped effect %d and effect %d\n", cmd.id1, cmd.id2); break; // Swap effects
        case SET: chain.getEffect(cmd.id1)->setParam(cmd.id2, cmd.value); Serial.printf("Set parameter %d for effect %d to %f\n", cmd.id2, cmd.id1, cmd.value); break; // Set effect parameter
        case BYPASS: chain.getEffect(cmd.id1)->setBypass(cmd.value); Serial.printf("Set bypass at effect %d to %f\n", cmd.id1, cmd.value); break; // Bypass effect
    }
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200); // Pin 0

    pinMode(LED_BUILTIN, OUTPUT);

    while (!Serial && millis() < 4000) {}

    // TEMP: For debugging
    if (CrashReport && LOG_DBG) {
        Serial.print("--- CRASH REPORT ---");
        Serial.print(CrashReport);
        Serial.println("--------------------");
    }
    
    /* Instantiate DSP chain */
    chain = std::make_unique<AudioChain>();
    stream = std::make_unique<AudioChainStream>(*chain);

    if (USB_IO) {
        patch1 = std::make_unique<AudioConnection>(usbIn, 0, *stream, 0);
        patch2 = std::make_unique<AudioConnection>(*stream, 0, usbOut, 0);
        // patch1 = std::make_unique<AudioConnection>(usbIn, 0, usbOut, 0);
    } else {
        patch1 = std::make_unique<AudioConnection>(usbIn, 0, *stream, 0);
        patch2 = std::make_unique<AudioConnection>(*stream, 0, dacOut, 0);
    }
    
    /* Teensy Audio setup */
    AudioNoInterrupts();
    AudioMemory(24); // <-- Unsure of how much we need
    AudioProcessorUsageMaxReset();
    AudioMemoryUsageMaxReset();

    delay(2000);
    Serial.println("SETUP OK");
    AudioInterrupts();
}

void loop() {

    if (Serial1.available()) {
        Command rcvCmd; size_t rcvBytes = 0;
        uint8_t* buf = (uint8_t*)&rcvCmd;

        rcvBytes = Serial1.readBytes((char*)buf, sizeof(Command));

        if (rcvBytes == sizeof(Command)) {
            if (verifyChecksum(rcvCmd)) {
                Serial.print("\n=== Received Echo ===\n");
                Serial.printf("cmd: %d | ", rcvCmd.cmd);
                Serial.printf("id1: %d | ", rcvCmd.id1);
                Serial.printf("id2: %d | ", rcvCmd.id2);
                Serial.printf("value: %.3f | ", rcvCmd.value);
                Serial.printf("checksum: 0x%02X (valid)\n", rcvCmd.checksum);
                Serial.println("===================");

                processCommand(rcvCmd, *chain);
            } else {
                Serial.print("\n=== Received Echo (INVALID CHECKSUM) ===\n");
                Serial.printf("cmd: %d | ", rcvCmd.cmd);
                Serial.printf("id1: %d | ", rcvCmd.id1);
                Serial.printf("id2: %d | ", rcvCmd.id2);
                Serial.printf("value: %.3f | ", rcvCmd.value);
                Serial.printf("checksum: 0x%02X (invalid)\n", rcvCmd.checksum);
                Serial.println("=====================================");
            }
        } else Serial.println("ERR: Incomplete Command");
    }

    // TESTING: Send parameter updates via serial
    // static String cmd;
    // while (Serial.available()) {
    //     char c = Serial.read();
    //     if (c == '\n' || c == '\r') {
    //         if (cmd.length() > 0) { 
    //             processCommand(cmd, *chain); 
    //             cmd = ""; 
    //         }
    //     } else if (c == '\b' || c == 127) { if (cmd.length() > 0) cmd.remove(cmd.length() - 1); // backspace or DEL
    //     } else { cmd += c; }
    // }

    if (millis() - logTime >= 1000) {
        logTime = millis();
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        if (LOG_DBG) { // Log resource usage
            Serial.printf("CPU: %f, CPU MAX: %f, Memory: %f, Memory MAX: %f\n", 
                AudioProcessorUsage(), AudioProcessorUsageMax(), AudioMemoryUsage(), AudioMemoryUsageMax()); 
        }
    }
}