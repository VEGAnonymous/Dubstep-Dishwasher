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

// TESTING
const bool USB_IO = true; // <-- SET FLAG HERE
AudioInputUSB usbIn; 
AudioOutputUSB usbOut;

uint32_t logTime = 0; // Resource usage monitoring (temp)

// IO
AudioInputI2S adcIn; // ADC input
AudioOutputI2S dacOut; // DAC output
AudioConnection* patch1 = nullptr; AudioConnection* patch2 = nullptr; // Connections

// DSP
AudioChain chain; // FX chain
AudioChainStream *stream = nullptr; // DSP stream

void processCommand(const String& s) { // TEMPORARY - vibecoded slop for testing
    String t = s; t.trim();
    if (!t.length()) return;

    if (t.startsWith("SET")) {
        int a=t.indexOf(' ')+1, b=t.indexOf(' ',a), c=t.indexOf(' ',b+1);
        if (a<0||b<0||c<0) return;
        AudioNoInterrupts();
        chain.getEffect(t.substring(a,b).c_str())->setParam(t.substring(b+1,c).c_str(), t.substring(c+1).toFloat());
        AudioInterrupts();
        Serial.println("OK");
    } else if (t.startsWith("BYPASS")) {
        int a=t.indexOf(' ')+1, b=t.indexOf(' ',a);
        if (a<0||b<0) return;
        AudioNoInterrupts();
        chain.getEffect(t.substring(a,b).c_str())->setBypass(t.substring(b+1).toInt());
        AudioInterrupts();
        Serial.println("OK");
    }
}

void setup() {
    AudioNoInterrupts();
    AudioMemoryUsageMaxReset(); 
    AudioProcessorUsageMaxReset(); 

    /* Allocate memory */
    AudioMemory(12);

    /* Setup DSP stream connections */
    // Input -> DSP -> Output
    stream = new AudioChainStream(chain);
    if (USB_IO) {
        patch1 = new AudioConnection(usbIn, 0, *stream, 0);
        patch2 = new AudioConnection(*stream, 0, usbOut, 0);
    } else {
        patch1 = new AudioConnection(adcIn, 0, *stream, 0);
        patch2 = new AudioConnection(*stream, 0, dacOut, 0);
    }

    /* BEGIN */
    delay(500);

    Serial.begin(115200);
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
            if (cmd.length() > 0) { processCommand(cmd); cmd = ""; }
        } else { cmd += c; }
    }

    // Monitor resource usage
    if (millis() - logTime >= 1000) {
        logTime = millis();
        Serial.printf("CPU: %d, CPU MAX: %d, Memory: %d, Memory MAX: %d\n", 
            AudioProcessorUsage(), AudioProcessorUsageMax(), AudioMemoryUsage(), AudioMemoryUsageMax()); 
    }
}