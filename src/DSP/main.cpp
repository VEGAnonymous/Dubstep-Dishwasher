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

using namespace std;

// TESTING - 
const bool USB_IO = true, // <-- SET FLAGS HERE
           LOG_DBG = false;

AudioInputUSB usbIn; 
AudioOutputUSB usbOut;

uint32_t logTime = 0; // Resource usage monitoring (temp)

// IO
AudioInputI2S adcIn; // ADC input
AudioOutputI2S dacOut; // DAC output

// DSP
unique_ptr<AudioChain> chain;
unique_ptr<AudioChainStream> stream;

// Connections
unique_ptr<AudioConnection> patch1, patch2;

void processCommand(const String& s, AudioChain& chain) { // TEMP: Vibecoded slop for debugging
    String t = s;
    t.trim();
    if (t.length() == 0) return;

    if (t.startsWith("SET")) {
        int a = t.indexOf(' ') + 1;
        int b = t.indexOf(' ', a);
        int c = t.indexOf(' ', b + 1);
        if (a < 0 || b < 0 || c < 0) {
            Serial.println("ERROR: Invalid SET command format");
            return;
        }

        std::string fxName = t.substring(a, b).c_str();
        Effect* fx = chain.getEffect(fxName);
        if (!fx) {
            Serial.print("ERROR: Effect not found -> ");
            Serial.println(fxName.c_str());
            return;
        }

        std::string paramName = t.substring(b + 1, c).c_str();
        float val = t.substring(c + 1).toFloat();

        AudioNoInterrupts();
        fx->setParam(paramName.c_str(), val);
        AudioInterrupts();

        Serial.print("OK: SET ");
        Serial.print(fxName.c_str());
        Serial.print(" ");
        Serial.print(paramName.c_str());
        Serial.print(" = ");
        Serial.println(val);

    } else if (t.startsWith("BYPASS")) {
        int a = t.indexOf(' ') + 1;
        int b = t.indexOf(' ', a);
        if (a < 0 || b < 0) {
            Serial.println("ERROR: Invalid BYPASS command format");
            return;
        }

        std::string fxName = t.substring(a, b).c_str();
        Effect* fx = chain.getEffect(fxName);
        if (!fx) {
            Serial.print("ERROR: Effect not found -> ");
            Serial.println(fxName.c_str());
            return;
        }

        int bypass = t.substring(b + 1).toInt();

        AudioNoInterrupts();
        fx->setBypass(bypass);
        AudioInterrupts();

        Serial.print("OK: BYPASS ");
        Serial.print(fxName.c_str());
        Serial.print(" -> ");
        Serial.println(bypass ? "ON" : "OFF");

    } else {
        Serial.print("ERROR: Unknown command -> ");
        Serial.println(t);
    }
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
    chain = make_unique<AudioChain>();
    stream = make_unique<AudioChainStream>(*chain);

    if (USB_IO) {
        patch1 = make_unique<AudioConnection>(usbIn, 0, *stream, 0);
        patch2 = make_unique<AudioConnection>(*stream, 0, usbOut, 0);
    } else {
        patch1 = make_unique<AudioConnection>(adcIn, 0, *stream, 0);
        patch2 = make_unique<AudioConnection>(*stream, 0, dacOut, 0);
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
    // Send parameter updates via serial, ex:
    // SET Distortion Mix 0.75
    // BYPASS Distortion 1
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