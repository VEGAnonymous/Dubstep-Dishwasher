#ifndef HANDLER
#define HANDLER

#include <Arduino.h>

class Packet { // Interface for other packet types
    public:
        virtual ~Packet() = default;

        // Checksum and validation
        virtual uint8_t computeChecksum() const = 0;
        virtual bool verifyChecksum() const = 0;
};

struct Command : public Packet {
    uint8_t cmd;        // Command type (0 = Add, 1 = Remove, 2 = Reorder, 3 = Set param, 4 = Bypass)
    uint8_t id1;        // EffectID 1
    uint8_t id2;        // ParamID
    uint8_t checksum;   // Checksum (XOR)
    float value;        // Command value

    Command() : cmd(0), id1(0), id2(0), checksum(0), value(0.0f) {}

    uint8_t computeChecksum() const override {
        uint8_t chk = cmd ^ id1 ^ id2;
        const uint8_t* valueBytes = (const uint8_t*)(&value);
        for (size_t i = 0; i < (size_t)sizeof(float); ++i) chk ^= valueBytes[i];
        return chk;
    }

    bool verifyChecksum() const override {
        Command temp = *this;
        return (checksum == temp.computeChecksum());
    }
} __attribute__((packed));

#endif // HANDLER