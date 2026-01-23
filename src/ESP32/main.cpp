#include <Arduino.h>

#include "Handler.h"
#include "ESP32/Defines.h"
#include "ESP32/Utilities.h"
#include "ESP32/BLEHandler.h"
#include "ESP32/InferenceBuffer.h"

/* Testing - Set flags here */
constexpr bool LOG_DEBUG = false;

/* UART */

Handler<MelFrame, Command> uartHandler(Serial1); // Packet handler (send commands / receive frames)
InferenceBuffer* inferenceBuffer = nullptr; // Buffer and process incoming mel frames for RT inference 

/* BLE */

BLEHandler<Command, Status> bleHandler; // Packet handler (send status / receive commands)

// Connection state
bool deviceConnected = false;
bool oldDeviceConnected = false;
BLEServer *pServer = NULL;
uint32_t advertiseTime = 0;

// Callbacks
class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        bleHandler.setConnected(true);
        Serial.println("Device connected");

        // Clear on connect
        sendCommand(&uartHandler, CommandType::EFFECT_CLEAR);
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        bleHandler.setConnected(false);
        Serial.println("Device disconnected");

        // Clear on disconnect
        sendCommand(&uartHandler, CommandType::EFFECT_CLEAR);
    }
};

class WriteCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
        std::string value = pCharacteristic->getValue();
        // Process incoming commands
        if (value.length() > 0) bleHandler.processIncoming((const uint8_t*)value.data(), value.length());
    }
};

/* Expression pedal */
uint32_t exprTime = 0;
float exprValue = 0.0f;

void setup() {
    Serial.begin(115200);
    Serial1.begin(230400, SERIAL_8N1, RX_PIN, TX_PIN);

    /* Setup UART handler + inference */
    inferenceBuffer = new InferenceBuffer();
    uartHandler.setCallback([](const MelFrame& frame) { 
        inferenceBuffer->addFrame(frame); 
    });

    /* BLE setup */
    BLEDevice::init("Dubstep Dishwasher MCU");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    
    // Create service
    BLEService *pService = pServer->createService(BLE_SERVICE_UUID);
    // Setup handler characteristics
    bleHandler.setup(pService);
    bleHandler.setWriteCallback(new WriteCallbacks());

    // Set callback for received commands
    bleHandler.setReceiveCallback([](const Command& cmd) {
        if (LOG_DEBUG) Serial.printf("cmd: type=%d id1=%d id2=%d v1=%.2f\n", cmd.cmd, cmd.id1, cmd.id2, cmd.value1);
        uartHandler.send(const_cast<Command&>(cmd)); // Forward command via UART
    });

    // Start service
    pService->start();

    // Setup advertising
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    advertiseTime = 0;
    Serial.println("SETUP OK");
}

void loop() {
    // BLE advertising
    if ((!deviceConnected && oldDeviceConnected) && (millis() - advertiseTime >= BLE_ADVERTISE_TIME)) {
        advertiseTime = millis();
        pServer->startAdvertising(); // Restart advertising
        Serial.println("Started advertising");
        oldDeviceConnected = deviceConnected;
    } if (deviceConnected && !oldDeviceConnected) oldDeviceConnected = deviceConnected;

    // Expression pedal read
    if (millis() - exprTime >= EXPR_READ_TIME) {
        exprTime = millis();
        exprValue = (float)analogRead(EXPR_PIN) / 1000.0f;
        if (exprValue < 0.1f) return;
        exprValue = scale(std::clamp(exprValue, EXPR_MIN, EXPR_MAX), EXPR_MIN, EXPR_MAX, 0.0f, 1.0f);
        sendCommand(&uartHandler, CommandType::MOD_MAPPING_SET_INPUT, EXPR_MOD_ID, 0, exprValue);
        sendStatus(&bleHandler, StatusType::EXPRESSION, EXPR_MOD_ID, 0, exprValue);
    }

    // UART receive
    uartHandler.listen();
}