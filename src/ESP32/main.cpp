#include <Arduino.h>

#include "Handler.h"
#include "ESP32/Utilities.h"
#include "ESP32/InferenceBuffer.h"

#define RX_PIN 18
#define TX_PIN 17

// BLE stuff
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

Handler<Command, MelFrame> handler(Serial1); // Packet handler (send commands / receive frames)
InferenceBuffer inferenceBuffer; // Buffer and process incoming mel frames for RT inference 

class Callbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
        std::string value = pCharacteristic->getValue();
        if (value.length() == 0) return; // return on empty writes

        // sends written bytes to helper function
        processIncomingBytes((const uint8_t*)value.data(), value.length());
    }
};

void setup() {
    Serial.begin(115200);
    Serial1.begin(230400, SERIAL_8N1, RX_PIN, TX_PIN);

    /* Setup UART handler */
    handler.setCallback([](const MelFrame& frame) { inferenceBuffer.addFrame(frame); });

    /* Setup BLE */
    BLEDevice::init("ESP32 BLE");
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                          CHARACTERISTIC_UUID,
                                          BLECharacteristic::PROPERTY_READ |
                                          BLECharacteristic::PROPERTY_WRITE
                                        );

    pCharacteristic->setCallbacks(new Callbacks());

    pService->start();

    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->start();

    delay(2000);
    Serial.println("SETUP OK");
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