#include <Arduino.h>
#include "Handler.h"
#include "ESP32/espdefs.h"

#define RX_PIN 18
#define TX_PIN 17

// BLE stuff
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

Handler<Command, MelFrame> handler(Serial1); // Packet handler (send commands / receive frames)

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
        std::string value = pCharacteristic->getValue();
        if (value.length() == 0) return; // return on empty writes

        // sends written bytes to helper function
        processIncomingBytes((const uint8_t*)value.data(), value.length());
    }
};

void setup() {
    // Serial port setup
    Serial.begin(115200);
    Serial1.begin(230400, SERIAL_8N1, RX_PIN, TX_PIN);

    /* Setup handler */
    handler.setCallback([](const MelFrame& frame) {
        #if DEBUG 
            Serial.printf("frameCounter: %d | numMels: %d | checksum: 0x%02X (valid)\n", frame.frameCounter, frame.numMels, frame.checksum);
        #endif
        // TODO: Define a function to buffer the frames eventually for TinyML inference
    });

    // BLE setup
    BLEDevice::init("ESP32 BLE");
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                          CHARACTERISTIC_UUID,
                                          BLECharacteristic::PROPERTY_READ |
                                          BLECharacteristic::PROPERTY_WRITE
                                        );

    pCharacteristic->setCallbacks(new MyCallbacks());

    pService->start();

    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->start();
    
}

void loop() {
    #if DEBUG
    // Debug: Read from Serial and process incoming bytes
    while (Serial.available()) {
      uint8_t serialData = Serial.read();
      processIncomingBytes(&serialData, 1);  
    }
    #endif

    handler.listen();
}