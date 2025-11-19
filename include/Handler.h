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

struct MelFrame {
    uint32_t index;      // Frame counter
    uint32_t numMels;    // Number of mel bins
    uint8_t checksum;    // Checksum (XOR)
    float mel[NUM_MELS]; // Mel data
} __attribute__((packed));

/* FUNCTIONS */

template <typename PacketType>
inline uint8_t computeChecksum(const PacketType& pkt) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&pkt);
    uint8_t checksum = 0;
    for (size_t i = 0; i < sizeof(PacketType); ++i) { // XOR all bytes EXCEPT checksum
        if (i != offsetof(PacketType, checksum)) checksum ^= bytes[i];
    }
    return checksum;
}

template <typename PacketType>
inline bool verifyChecksum(const PacketType& pkt) { return pkt.checksum == computeChecksum(pkt); }

/* CLASSES */

template <typename InPacket, typename OutPacket>
class Handler { // Handle sending and receiving different packet types over UART
    private:
        HardwareSerial& serial;

        uint8_t buffer[sizeof(InPacket)];
        size_t bufferPos;
        std::function<void(const InPacket&)> rcvCallback;

    public:
        Handler(HardwareSerial& serial) : serial(serial), bufferPos(0) {}

        void setCallback(std::function<void(const InPacket&)> callback) { rcvCallback = callback; }

        void listen() { // Receive packets over UART
            while (serial.available()) {
                if (bufferPos < sizeof(InPacket)) buffer[bufferPos++] = serial.read(); // Write byte to buffer
                if (bufferPos == sizeof(InPacket)) { // Full packet received
                    InPacket pkt; memcpy(&pkt, buffer, sizeof(InPacket));
                    if (verifyChecksum<InPacket>(pkt)) {
                        if (rcvCallback) rcvCallback(pkt);
                    } else return; // Drop invalid packets
                    bufferPos = 0;
                }
            }
        }

        void send(OutPacket& pkt) { // Send packet over UART
            pkt.checksum = computeChecksum<OutPacket>(pkt);
            serial.write(reinterpret_cast<const uint8_t*>(&pkt), sizeof(OutPacket));
        }
};