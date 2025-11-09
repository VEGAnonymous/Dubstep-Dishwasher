#pragma once

#include <Arduino.h>

/* DATA */

struct Command {
    uint8_t cmd;      // Command type (0 = Add, 1 = Remove, 2 = Reorder, 3 = Set param, 4 = Bypass)
    uint8_t id1;      // EffectID
    uint8_t id2;      // ParamID
    uint8_t checksum; // Checksum (XOR)
    float value;      // Command value
} __attribute__((packed));

/* FUNCTIONS */

uint8_t computeChecksum(const Command& cmd) {
    uint8_t checksum = cmd.cmd ^ cmd.id1 ^ cmd.id2;
    const uint8_t* valueBytes = (const uint8_t*)&cmd.value;
    for(int i = 0; i < (int)sizeof(float); ++i) { checksum ^= valueBytes[i]; }
    return checksum;
}

bool verifyChecksum(const Command& cmd) {
    uint8_t storedChecksum = cmd.checksum;
    Command tempCmd = cmd;
    return (storedChecksum == computeChecksum(tempCmd));
}