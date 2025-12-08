#pragma once

#include <Arduino.h>
#include <Teensy/Defines.h>

/* DATA */

struct Command {
    uint16_t sync;    // 0xAA55
    uint8_t cmd;      // Command type (see include/Teensy/Defines.h)
    uint8_t id1;      // Primary ID (EffectID, ModulatorID...)
    uint8_t id2;      // Secondary ID (ParamID, CurvePoint index...)
    uint8_t checksum; // Checksum (XOR)
    float value1;
    float value2;
    float value3;
} __attribute__((packed));

struct MelFrame {
    uint16_t sync;       // 0xAA55
    uint32_t index;      // Frame counter
    uint32_t numMels;    // Number of mel bins
    float mel[NUM_MELS]; // Mel data
    uint8_t checksum;    // Checksum (XOR)
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

        // Sync helper
        template <typename T, typename = void>
        struct has_sync : std::false_type {};
        template <typename T>
        struct has_sync<T, std::void_t<decltype(std::declval<T>().sync)>> : std::true_type {};

    public:
        Handler(HardwareSerial& serial) : serial(serial), bufferPos(0) {}

        void setCallback(std::function<void(const InPacket&)> callback) { rcvCallback = callback; }

        void listen() { // Receive packets over UART
            while (serial.available()) {
                if (bufferPos < sizeof(InPacket)) buffer[bufferPos++] = serial.read(); // Write byte to buffer
                if (bufferPos == sizeof(InPacket)) { // Full packet received
                    InPacket pkt; memcpy(&pkt, buffer, sizeof(InPacket));

                    constexpr bool hasSync = has_sync<InPacket>::value; // Packet type requires sync
                    if constexpr (hasSync) {
                        // Has sync field - check and resync if invalid
                        if (pkt.sync == 0xAA55 && verifyChecksum<InPacket>(pkt)) {
                            if (rcvCallback) rcvCallback(pkt);
                            bufferPos = 0;
                        } else {
                            // Invalid - shift buffer and resync
                            memmove(buffer, buffer + 1, sizeof(InPacket) - 1);
                            bufferPos = sizeof(InPacket) - 1;
                        }
                    } else { // No sync
                        if (verifyChecksum<InPacket>(pkt)) {
                            if (rcvCallback) rcvCallback(pkt);
                        } else { bufferPos = 0; return; } // Drop invalid packets
                        bufferPos = 0;
                    }
                }
            }
        }

        void send(OutPacket& pkt) { // Send packet over UART
            constexpr bool hasSync = has_sync<OutPacket>::value;
            if constexpr (hasSync) pkt.sync = 0xAA55; // Set sync marker
            pkt.checksum = computeChecksum<OutPacket>(pkt);
            serial.write(reinterpret_cast<const uint8_t*>(&pkt), sizeof(OutPacket));
        }
};