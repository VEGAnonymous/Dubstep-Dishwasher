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

#define RX_PIN 0

// TESTING - 
const bool USB_IO = true, // <-- SET FLAGS HERE
           LOG_DBG = true;

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
    uint8_t reserved;   // Reserved/padding
    float value;        // Parameter value (float)
} __attribute__((packed));

// void processCommand(const String& s, AudioChain& chain) {
//     uint8_t fxID, paramID, bypass;
//     float value;

//     // Command structure: [Type][EffectID][ParamID/Bypass][Value], ex:
//     // S 1 2 0.75
//     // B 1 1
//     if (sscanf(s.c_str(), "S %c %c %f", &fxID, &paramID, &value) == 3) {
//         Effect* fx = chain.getEffect((EffectID)fxID);
//         if (!fx) { Serial.println("ERR: Invalid EffectID"); return; }
//         fx->setParam((ParamID)paramID, value);
//         Serial.println("OK");
//     } else if (sscanf(s.c_str(), "B %c %c", &fxID, &bypass) == 2) {
//         Effect* fx = chain.getEffect((EffectID)fxID);
//         if (!fx) { Serial.println("ERR: Invalid EffectID"); return; }
//         fx->setBypass(bypass);
//         Serial.println("OK");
//     } else { Serial.println("ERR: Parse"); }
// }

// void processCommand(Command& cmd, AudioChain& chain) {
//     switch (cmd.cmd) {
//         case 0: // Add Effect
//     }
// }
void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    Serial1.begin(115200); // UART1 RX only (pin 0)

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
        patch1 = std::make_unique<AudioConnection>(adcIn, 0, *stream, 0);
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
        Command rcvCmd;
        size_t rcvBytes = 0;

        uint8_t* buf = (uint8_t*)&rcvCmd;
        Serial1.readBytes((char*)buf, sizeof(Command));

        
        
        if (rcvBytes == sizeof(Command)) {
            Serial.print("\n=== Received Command ===\n");
            Serial.printf("cmd: %c | ", rcvCmd.cmd);
            Serial.printf("id1: %c | ", rcvCmd.id1);
            Serial.printf("id2: %c | ", rcvCmd.id2);
            Serial.printf("value: %.3f\n", rcvCmd.value);
            Serial.println("===================");
            
            // processCommand(rcvCmd, *chain);
        } else {
            Serial.println("ERR: Incomplete Command");
        }
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