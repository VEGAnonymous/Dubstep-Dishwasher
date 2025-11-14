#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "Teensy/Control/AudioChain.h"
#include "Teensy/Control/AudioChainStream.h"

#include "Handler.h"

// TODO: Encapsulate UART I/O in Handler.h 

/* Testing - Set flags here */
const bool USB_IO = false, 
           LOG_RSE = true,
           LOG_CMD = true;

AudioInputUSB usbIn; 
AudioOutputUSB usbOut;

uint32_t logTime = 0;

/* IO */
AudioInputI2S adcIn; // ADC input
AudioOutputI2S dacOut; // DAC output

/* DSP */
std::unique_ptr<AudioChain> chain; // FX chain
std::unique_ptr<AudioChainStream> stream; // Audio stream
std::unique_ptr<AudioConnection> patch1, patch2; // Connections

/* CONTROL */
void processCommand(Command& cmd, AudioChain& chain) {
    switch (static_cast<CommandType>(cmd.cmd)) {
        case CommandType::ADD: // Add effect
            chain.addEffect(static_cast<EffectName>(cmd.id1));
            if (LOG_CMD) Serial.printf("Added effect %d\n", cmd.id1);
            break;
        case CommandType::REMOVE: // Remove effect
            chain.removeEffect(cmd.id1); 
            if (LOG_CMD) Serial.printf("Removed effect %d\n", cmd.id1);
            break;
        case CommandType::REORDER: // Reorder effects
            chain.reorderEffect(cmd.id1, static_cast<size_t>(cmd.value));
            if (LOG_CMD) Serial.printf("Moved effect %d to position %d\n", cmd.id1, cmd.id2);
            break;
        case CommandType::SET: // Set effect parameter
            chain.getEffect(cmd.id1)->setParam(cmd.id2, cmd.value);
            if (LOG_CMD) Serial.printf("Set parameter %d for effect %d to %f\n", cmd.id2, cmd.id1, cmd.value);
            break;
        case CommandType::BYPASS: // Bypass effect
            chain.getEffect(cmd.id1)->setBypass(cmd.value > 0.5f);
            if (LOG_CMD) Serial.printf("Set bypass at effect %d to %f\n", cmd.id1, cmd.value);
            break;
        case CommandType::CLEAR: // Clear chain
            chain.clear();
            if (LOG_CMD) Serial.println("Cleared effect chain");
    }
}

/* RUNTIME */
void setup() {
    Serial.begin(115200);
    Serial1.begin(115200); // Pin 0

    pinMode(LED_BUILTIN, OUTPUT);

    while (!Serial && millis() < 4000) {}

    if (CrashReport) {
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
    AudioMemory(24);
    AudioProcessorUsageMaxReset();
    AudioMemoryUsageMaxReset();

    delay(2000);
    Serial.println("SETUP OK");
    AudioInterrupts();
}

Command cmd;
size_t rcvBytes = 0;

void loop() {

    /* Parse received commands */
    while (Serial1.available()) {
        uint8_t* buf = (uint8_t*)&cmd;
        buf[rcvBytes++] = Serial1.read();

        if (rcvBytes == sizeof(Command)) {
            // Full packet received
            if (verifyChecksum(cmd)) {
                if (LOG_CMD) Serial.printf("cmd: %d | id1: %d | id2: %d | value: %.3f | checksum: 0x%02X (valid)\n", 
                cmd.cmd, cmd.id1, cmd.id2, cmd.value, cmd.checksum);
                processCommand(cmd, *chain);
            }
            // Reset for next packet
            rcvBytes = 0;
        }
    }
    
    if (millis() - logTime >= 1000) {
        logTime = millis();
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        if (LOG_RSE) Serial.printf("CPU: %f, CPU MAX: %f, Memory: %f, Memory MAX: %f\n", // Log resource usage
            AudioProcessorUsage(), AudioProcessorUsageMax(), AudioMemoryUsage(), AudioMemoryUsageMax()); 
    }
}