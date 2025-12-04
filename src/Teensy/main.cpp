#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "Teensy/Control/AudioChain.h"
#include "Teensy/Control/AudioChainStream.h"
#include "Teensy/Control/LogMelStream.h"
#include "Teensy/Effects/Parallel.h"

#include "Handler.h"

/* Testing - Set flags here */
constexpr bool USB_IO = false,
               SND_SPECT = false,
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
std::unique_ptr<LogMelStream> logMelStream; // Log-mel spectrogram stream
std::unique_ptr<AudioConnection> patch1, patch2, patch3; // Connections

/* CONTROL */

Handler<Command, MelFrame> handler(Serial1); // Packet handler (send frames / receive commands)

void processCommand(const Command& cmd, AudioChain& chain, ModulationEngine& modEngine) {
    switch (static_cast<CommandType>(cmd.cmd)) {

        /* EFFECT CHAIN COMMANDS */

        case CommandType::EFFECT_ADD: { // Add effect
            chain.addEffect(static_cast<EffectName>(cmd.id1));
            if (LOG_CMD) Serial.printf("Added effect %d\n", cmd.id1);
            break;
        }

        case CommandType::EFFECT_REMOVE: { // Remove effect
            chain.removeEffect(cmd.id1); 
            if (LOG_CMD) Serial.printf("Removed effect %d\n", cmd.id1);
            break;
        }

        case CommandType::EFFECT_REORDER: { // Reorder effects
            chain.reorderEffect(cmd.id1, cmd.id2);
            if (LOG_CMD) Serial.printf("Moved effect %d to position %d\n", cmd.id1, cmd.id2);
            break;
        }

        case CommandType::EFFECT_SET_PARAMETER: { // Set effect parameter
            Effect* effect = chain.getEffect(cmd.id1);
            if (effect) {
                effect->setParam(cmd.id2, cmd.value1);
                stream->getModEngine()->setBaseValue(cmd.id1, cmd.id2, effect->getNormalized(cmd.id2)); // Update base value for modulation
                if (LOG_CMD) Serial.printf("Set parameter %d for effect %d to %.3f\n", cmd.id2, cmd.id1, cmd.value1);
            }
            break;
        }

        case CommandType::EFFECT_BYPASS: { // Bypass effect
            chain.getEffect(cmd.id1)->setBypass(cmd.value1 > 0.5f);
            if (LOG_CMD) Serial.printf("Set bypass for effect %d to %d\n", cmd.id1, static_cast<uint8_t>(cmd.value1 > 0.5f));
            break;
        }

        case CommandType::EFFECT_CLEAR: { // Clear chain
            chain.clear();
            modEngine.clearAssignments();
            if (LOG_CMD) Serial.println("Cleared effect chain");
            break;
        }

        /* MODULATION COMMANDS */
        case CommandType::MOD_SET_PARAMETER: { // Set modulator parameter
            Modulator* mod = modEngine.getModulator(cmd.id1);
            if (mod) {
                mod->setParam(cmd.id2, cmd.value1);
                if (LOG_CMD) Serial.printf("Set modulator %d param %d to %.3f\n", cmd.id1, cmd.id2, cmd.value1);
            } 
            break;
        }

        case CommandType::MOD_CLEAR_CURVE: { // Clear modulator curve (prepare for new points)
            Modulator* mod = modEngine.getModulator(cmd.id1);
            if (mod) {
                mod->clearCurve();
                mod->setPhase(0.0f); // Reset phase to sync with upstream
                if (LOG_CMD) Serial.printf("Cleared curve for modulator %d\n", cmd.id1);
            }
            break;
        }

        case CommandType::MOD_SET_CURVE_POINT: { // Set modulator curve point
            Modulator* mod = modEngine.getModulator(cmd.id1);
            if (mod) {
                mod->setCurvePoint(cmd.value1, cmd.value2, cmd.value3);
                if (LOG_CMD) Serial.printf("Set curve point for modulator %d: (%.3f, %.3f, %.3f)\n",
                    cmd.id1, cmd.value1, cmd.value2, cmd.value3);
            }
            break;
        }

        case CommandType::MOD_ASSIGNMENT_ADD: { // Add modulation assignment
            ModAssignment assignment(
                cmd.id1, // modId
                cmd.id2, // effectId
                static_cast<uint8_t>(cmd.value1), // paramId (hijacking value spot, hacky but necessary)
                cmd.value2, // Amount
                static_cast<ModPolarity>(cmd.value3) // Polarity
            );
            modEngine.addAssignment(assignment);
            if (LOG_CMD) Serial.printf("Added assignment: mod %d -> effect %d param %d (amount=%.3f, polarity=%d)\n", 
                cmd.id1, cmd.id2, static_cast<uint8_t>(cmd.value1), cmd.value2, static_cast<uint8_t>(cmd.value3));
            break;
        }

        case CommandType::MOD_ASSIGNMENT_REMOVE: {
            modEngine.removeAssignment(
                cmd.id1, // modId
                cmd.id2, // effectId
                static_cast<uint8_t>(cmd.value1) // paramId (same here)
            );
            if (LOG_CMD) Serial.printf("Removed assignment: mod %d -> effect %d param %d\n", cmd.id1, cmd.id2, static_cast<uint8_t>(cmd.value1));
            break;
        }

        case CommandType::MOD_ASSIGNMENT_SET: {
            modEngine.setAssignment(
                cmd.id1, // modId
                cmd.id2, // effectId
                static_cast<uint8_t>(cmd.value1), // paramId (and here)
                cmd.value2, // Amount
                static_cast<ModPolarity>(cmd.value3) // Polarity
            );
            if (LOG_CMD) Serial.printf("Updated assignment: mod %d -> effect %d param %d (amount=%.3f, polarity=%d)\n",
                cmd.id1, cmd.id2, static_cast<uint8_t>(cmd.value1), cmd.value2, static_cast<uint8_t>(cmd.value3));
            break;
        }

        case CommandType::MOD_MAPPING_SET_INPUT: {
            modEngine.setMappingInput(cmd.id1, cmd.value1); // value1 must be normalized input [0, 1]
            if (LOG_CMD) Serial.printf("Set mapping input for modulator %d to %.3f\n", cmd.id1, cmd.value1);
            break;
        }

        case CommandType::PARALLEL_CHAIN_COMMAND: {
            // Valid only for Parallel Effect instances
            Effect* effect = chain.getEffect(cmd.id1);
            if (!effect || !effect->isParallel()) break;

            Parallel* parallel = static_cast<Parallel*>(effect);
            
            // HACK: Have to mux paramId addressing since we ran out of command bytes
            uint8_t chainSelect = (cmd.id2 >> 4) & 0x0F; // Upper nibble
            ParamID paramId = static_cast<ParamID>(cmd.id2 & 0x0F); // Lower nibble
            
            uint8_t command = static_cast<uint8_t>(cmd.value1);
            EffectID effectId = static_cast<EffectID>(cmd.value2);
            float value = cmd.value3;
            
            parallel->chainCommand(chainSelect, command, effectId, paramId, value);
            
            if (LOG_CMD) Serial.printf("Parallel chain %c, cmd=%d, slot=%d, param=%d, value=%.3f\n",
                chainSelect ? 'B' : 'A', command, effectId, paramId, value);
            break;
        }

        default: if (LOG_CMD) { Serial.printf("Unknown command: %d\n", cmd.cmd); } break;
    }
}

/* RUNTIME */
void setup() {
    Serial.begin(115200);
    Serial1.begin(230400); // Pin 0/1

    pinMode(LED_BUILTIN, OUTPUT);

    /* Setup handler */
    handler.setCallback([](const Command& cmd) {
    if (LOG_CMD) Serial.printf("cmd: %d | id1: %d | id2: %d | value1: %.3f | value2: %.3f | value3: %.3f | checksum: 0x%02X\n", 
                                cmd.cmd, cmd.id1, cmd.id2, cmd.value1, cmd.value2, cmd.value3, cmd.checksum);
        processCommand(cmd, *chain, *stream->getModEngine());
    });

    while (!Serial && millis() < 4000) {}

    if (CrashReport) {
        Serial.print("--- CRASH REPORT ---");
        Serial.print(CrashReport);
        Serial.println("--------------------");
    }
    
    /* Instantiate DSP chain */
    chain = std::make_unique<AudioChain>();
    stream = std::make_unique<AudioChainStream>(*chain);
    if (SND_SPECT) logMelStream = std::make_unique<LogMelStream>();

    if (USB_IO) {
        patch1 = std::make_unique<AudioConnection>(usbIn, 0, *stream, 0);
        patch2 = std::make_unique<AudioConnection>(*stream, 0, usbOut, 0);
        if (SND_SPECT) patch3 = std::make_unique<AudioConnection>(usbIn, 0, *logMelStream, 0);
    } else {
        patch1 = std::make_unique<AudioConnection>(usbIn, 0, *stream, 0);
        patch2 = std::make_unique<AudioConnection>(*stream, 0, dacOut, 0);
        if (SND_SPECT) patch3 = std::make_unique<AudioConnection>(adcIn, 0, *logMelStream, 0);
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

void loop() {

    handler.listen();

    // Send log-mel frames
    if (SND_SPECT && logMelStream && logMelStream->isFrameReady()) {
        MelFrame frame = logMelStream->getFrame();
        frame.sync = 0xAA55;
        handler.send(frame);
        logMelStream->clearReady();
    }
    
    // Blink LED and log resource usage
    if (millis() - logTime >= 1000) {
        logTime = millis();
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        if (LOG_RSE) Serial.printf("CPU: %f, CPU MAX: %f, Memory: %f, Memory MAX: %f\n", // Log resource usage
            AudioProcessorUsage(), AudioProcessorUsageMax(), AudioMemoryUsage(), AudioMemoryUsageMax()); 
    }
}