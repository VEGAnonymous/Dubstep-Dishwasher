#include <Arduino.h>

#include "Handler.h"
#include "ESP32/Utilities.h"
#include "ESP32/InferenceBuffer.h"

#define RX_PIN 18
#define TX_PIN 17

// #define DEBUG 0 // In Utilities.h

/* BLE */
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
// #define CHARACTERISTIC_UUID2 "beb54832-36e1-4688-b7f5-ea07361b26a8"

// Variables for managing connection state
bool deviceConnected = false;
bool oldDeviceConnected = false;
// Pointer to BLE server
BLEServer *pServer = NULL;

uint32_t advertiseTime = 0;

// Server callbacks (connect and disconnect)
class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("Device connected");
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("Device disconnected");
    }
};

class Callbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
        std::string value = pCharacteristic->getValue();
        // Serial.printf("Received %d bytes\n", value.length());
        // Return on invalid writes
        if (value.length() % sizeof(Command) != 0) return;

        // if (lastTime) {
        //     Serial.printf("millis %lu\n", millis() - lastTime);
        // }
        // lastTime = millis();

        // Sends written bytes to helper function
        processIncomingBytes((const uint8_t*)value.data(), value.length());
    }
};

/* UART */

Handler<MelFrame, Command> handler(Serial1); // Packet handler (send commands / receive frames)
InferenceBuffer inferenceBuffer; // Buffer and process incoming mel frames for RT inference 

void setup() {
    Serial.begin(115200);
    Serial1.begin(230400, SERIAL_8N1, RX_PIN, TX_PIN);

    /* Setup UART handler */
    handler.setCallback([](const MelFrame& frame) { inferenceBuffer.addFrame(frame); });

    /* BLE setup */
    BLEDevice::init("T8_ESP");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    
    // Create service and characteristic(s)
    BLEService *pService = pServer->createService(SERVICE_UUID);
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                          CHARACTERISTIC_UUID,
                                          BLECharacteristic::PROPERTY_WRITE
                                        );

    pCharacteristic->setCallbacks(new Callbacks());

    // Start service
    pService->start();

    // Set up advertising
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);

    advertiseTime = 0;
    pServer->startAdvertising();
    
    pAdvertising->start();

    delay(2000);
    Serial.println("SETUP OK");
}

void loop() {
    if ((!deviceConnected && oldDeviceConnected) && (millis() - advertiseTime >= 1000)) {
        advertiseTime = millis();
        pServer->startAdvertising(); // Restart advertising
        Serial.println("Started advertising");
        oldDeviceConnected = deviceConnected;
    }

    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    #if DEBUG
    // Debug: Read from Serial and process incoming bytes
    while (Serial.available()) {
      uint8_t serialData = Serial.read();
      processIncomingBytes(&serialData, 1);  
    }
    #endif

    handler.listen();
}