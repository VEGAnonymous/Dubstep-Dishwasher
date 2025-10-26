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

void processCommand(const String& s, AudioChain& chain) {
    uint8_t fxID, paramID, bypass;
    float value;

    // Command structure: [Type][EffectID][ParamID/Bypass][Value], ex:
    // S 1 2 0.75
    // B 1 1
    if (sscanf(s.c_str(), "S %c %c %f", &fxID, &paramID, &value) == 3) {
        Effect* fx = chain.getEffect((EffectID)fxID);
        if (!fx) { Serial.println("ERR: Invalid EffectID"); return; }
        fx->setParam((ParamID)paramID, value);
        Serial.println("OK");
    } else if (sscanf(s.c_str(), "B %c %c", &fxID, &bypass) == 2) {
        Effect* fx = chain.getEffect((EffectID)fxID);
        if (!fx) { Serial.println("ERR: Invalid EffectID"); return; }
        fx->setBypass(bypass);
        Serial.println("OK");
    } else { Serial.println("ERR: Parse"); }
}

void setup() {
    Serial.begin(115200);
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
    // Send parameter updates via serial
    static String cmd;
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (cmd.length() > 0) { 
                processCommand(cmd, *chain); 
                cmd = ""; 
            }
        } else if (c == '\b' || c == 127) { if (cmd.length() > 0) cmd.remove(cmd.length() - 1); // backspace or DEL
        } else { cmd += c; }
    }

    if (millis() - logTime >= 1000) {
        logTime = millis();
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        if (LOG_DBG) { // Log resource usage
            Serial.printf("CPU: %f, CPU MAX: %f, Memory: %f, Memory MAX: %f\n", 
                AudioProcessorUsage(), AudioProcessorUsageMax(), AudioMemoryUsage(), AudioMemoryUsageMax()); 
        }
    }
}