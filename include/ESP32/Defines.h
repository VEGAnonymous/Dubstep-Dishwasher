#pragma once

// UART
#define RX_PIN 18
#define TX_PIN 17

#define EXPR_PIN 4

constexpr uint32_t ADVERTISE_INTERVAL = 1000, // ms
                   HEARTBEAT_INTERVAL = 500,
                   EXPR_READ_INTERVAL = 200;

constexpr uint8_t SEQ_WINDOW_SIZE = 32;

constexpr float EXPR_MIN = 0.50f, // V
                EXPR_MAX = 1.65f,
                EXPR_TOL = 0.05f;

#define EXPR_MOD_ID 11