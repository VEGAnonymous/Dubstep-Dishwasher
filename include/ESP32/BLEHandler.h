#pragma once

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "Handler.h"

struct Status {
    uint16_t sync;    // 0x55AA
    uint8_t type;     // StatusType
    uint8_t id;       // Primary ID
    uint16_t flags;   // Reserved
    float value1;     
    float value2;     
    float value3;     
    uint8_t checksum; // Checksum (XOR)
} __attribute__((packed));

enum class StatusType : uint8_t {
    INFERENCE
    // Expandable with other statuses
};

// UUIDs
#define BLE_SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BLE_WRITE_CHARACTERISTIC "beb5483e-36e1-4688-b7f5-ea07361b26a8"  // App -> ESP32
#define BLE_NOTIFY_CHARACTERISTIC "beb5483f-36e1-4688-b7f5-ea07361b26a8"  // ESP32 -> App

template <typename InPacket, typename OutPacket>
class BLEHandler {
    private:
        BLECharacteristic* writeChar;
        BLECharacteristic* notifyChar;
        std::function<void(const InPacket&)> rcvCallback;
        
        uint8_t buffer[sizeof(InPacket)];
        size_t bufferPos;
        bool connected;

    public:
        BLEHandler() : writeChar(nullptr), notifyChar(nullptr), bufferPos(0), connected(false) {}

        void setup(BLEService* service) {
            // Write characteristic (app -> ESP32)
            writeChar = service->createCharacteristic(BLE_WRITE_CHARACTERISTIC, BLECharacteristic::PROPERTY_WRITE);

            // Notify characteristic (ESP32 -> app)
            notifyChar = service->createCharacteristic(BLE_NOTIFY_CHARACTERISTIC,BLECharacteristic::PROPERTY_NOTIFY);
            notifyChar->addDescriptor(new BLE2902());
        }

        void setWriteCallback(BLECharacteristicCallbacks* callbacks) { if (writeChar) writeChar->setCallbacks(callbacks); }
        void setReceiveCallback(std::function<void(const InPacket&)> callback) { rcvCallback = callback; }

        void processIncoming(const uint8_t* data, size_t len) {
            for (size_t i = 0; i < len; ++i) {
                if (bufferPos < sizeof(InPacket)) buffer[bufferPos++] = data[i];
                if (bufferPos == sizeof(InPacket)) {
                    InPacket pkt; memcpy(&pkt, buffer, sizeof(InPacket));
                    // Verify sync and checksum
                    if (pkt.sync == 0xAA55 && verifyChecksum(pkt)) {
                        if (rcvCallback) rcvCallback(pkt);
                        bufferPos = 0;
                    } else { // Resync - shift buffer by 1 byte
                        memmove(buffer, buffer + 1, sizeof(InPacket) - 1);
                        bufferPos = sizeof(InPacket) - 1;
                    }
                }
            }
        }

        void send(OutPacket& pkt) {
            if (!notifyChar || !connected) return;
            pkt.sync = 0x55AA;
            pkt.checksum = computeChecksum(pkt);
            notifyChar->setValue((uint8_t*)&pkt, sizeof(OutPacket));
            notifyChar->notify();
        }

        void setConnected(bool connected) { this->connected = connected; }
        bool isConnected() const { return connected; }
    };