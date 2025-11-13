#include <Arduino.h>
#include "Handler.h"
#include "ESP32/espdefs.h"

#define RX_PIN 18
#define TX_PIN 17

// BLE stuff
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
// #define CHARACTERISTIC_UUID2 "beb54832-36e1-4688-b7f5-ea07361b26a8"

// variables for managing connection state
bool deviceConnected = false;
bool oldDeviceConnected = false;
// pointer to BLE server
BLEServer *pServer = NULL;

// handle server callbacks (connect and disconnect)
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("Device connected");
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("Device disconnected");
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        // return on empty writes
        if (value.length() == 0) return;

        // sends written bytes to helper function
        processIncomingBytes((const uint8_t*)value.data(), value.length());
    }
};

void setup() {
    // Serial port setup
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

    // BLE setup (I copied this from platformio setup tutorial)
    BLEDevice::init("Team 8 ESP32 BLE");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    // create service and characteristic(s)
    BLEService *pService = pServer->createService(SERVICE_UUID);
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                          CHARACTERISTIC_UUID,
                                          BLECharacteristic::PROPERTY_READ |
                                          BLECharacteristic::PROPERTY_WRITE
                                        );
    // BLECharacteristic *pCharacteristic2 = pService->createCharacteristic(
    //                                       CHARACTERISTIC_UUID2,
    //                                       BLECharacteristic::PROPERTY_WRITE
    //                                     );

    pCharacteristic->setCallbacks(new MyCallbacks());
    // pCharacteristic2->setCallbacks(new MyCallbacks());

    // start service
    pService->start();

    // set up advertising
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pServer->startAdvertising();
    
}

void loop() {
    // 
    if (!deviceConnected && oldDeviceConnected) {
        delay(1000); // wait a bit for bluetooth stack to clear
        pServer->startAdvertising(); // restart advertising
        Serial.println("Started advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    #if DEBUG
    // Debug: Read from Serial and process incoming bytes
    while (Serial.available()) {
      uint8_t serialData = Serial.read();
      processIncomingBytes(&serialData, 1);  
    }

    // Debug: Echo back any received commands
    // not needed atm since no data is being received from the teensy
    if (Serial1.available()) {
        Command rcvCmd;
        size_t rcvBytes = 0;

        while (Serial1.available() && rcvBytes < sizeof(Command)) {
            uint8_t* buf = (uint8_t*)&rcvCmd;
            buf[rcvBytes++] = Serial1.read();
        }

        if (rcvBytes == sizeof(Command)) {
            if (verifyChecksum(rcvCmd)) {
                Serial.printf("cmd: %d | id1: %d | id2: %d | value: %.3f | checksum: 0x%02X (valid)\n", 
                    rcvCmd.cmd, rcvCmd.id1, rcvCmd.id2, rcvCmd.value, rcvCmd.checksum);
            } else return; // Drop invalid packets
        }
    }
    #endif
}