#pragma once

#include <algorithm>
#include <cmath>

#include "ESP32/Defines.h"
#include "ESP32/BLEHandler.h"

// Generic

inline float lerp(float a, float b, float t) { return a + (t * (b - a)); } // Linearly interpolate scalars

inline float scale(float value, float inLow, float inHigh, float outLow, float outHigh, float exponent = 1.0f) { // Map value in input range to output range
    float t = std::clamp((value - inLow) / (inHigh - inLow), 0.0f, 1.0f); // Normalize to [0, 1]
    if (exponent != 1.0f) t = powf(t, exponent); // Apply exponential
    return lerp(outLow, outHigh, t);
}

// Handler

inline bool deduplicate(uint8_t* sequenceBuf, uint8_t& seqIndex, uint8_t seq) {
    if (seq == 0) return false; // Non-critical, always process

    for (uint8_t i = 0; i < SEQ_WINDOW_SIZE; i++) if (sequenceBuf[i] == seq) return true; // Duplicate

    sequenceBuf[seqIndex] = seq; // Record new sequence number
    seqIndex = (seqIndex + 1) % SEQ_WINDOW_SIZE;
    return false;
} 

inline void sendCommand(Handler<MelFrame, Command>* uartHandler, CommandType type, 
    uint8_t id1 = 0, uint8_t id2 = 0, float value1 = 0.0f, float value2 = 0.0f, float value3 = 0.0f) {
    Command cmd; memset(&cmd, 0, sizeof(Command));
    cmd.sync = 0xAA55; 
    cmd.cmd = static_cast<uint8_t>(type);
    cmd.id1 = id1; cmd.id2 = id2; cmd.value1 = value1; cmd.value2 = value2; cmd.value3 = value3;
    uartHandler->send(cmd);
}

inline void sendStatus(BLEHandler<Command, Status>* bleHandler, StatusType type, 
    uint8_t id = 0, uint16_t flags = 0, float value1 = 0.0f, float value2 = 0.0f, float value3 = 0.0f) {
    Status status; memset(&status, 0, sizeof(Status));
    
    status.sync = 0x55AA;
    status.type = static_cast<uint8_t>(type);
    status.id = id; status.flags = flags; status.value1 = value1; status.value2 = value2; status.value3 = value3;
    
    // Send via BLE
    bleHandler->send(status);
}