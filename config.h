#pragma once
#include <stdint.h>

// The pin list identifies an ESP32-S3. GPIO numbers, not header positions.
// Each driver runs two motors, one per bridge; its inputs are paired.
constexpr uint8_t LEFT_IN1 = 1;   // Left driver IN1 + IN3
constexpr uint8_t LEFT_IN2 = 2;   // Left driver IN2 + IN4
constexpr uint8_t LEFT_EN = 42;   // Left driver ENA + ENB (remove jumpers)
constexpr uint8_t RIGHT_IN1 = 41; // Right driver IN1 + IN3
constexpr uint8_t RIGHT_IN2 = 40; // Right driver IN2 + IN4
constexpr uint8_t RIGHT_EN = 47;  // Right driver ENA + ENB (remove jumpers)
constexpr bool INVERT_LEFT = false;
constexpr bool INVERT_RIGHT = false;
constexpr uint32_t COMMAND_TIMEOUT_MS = 350;
constexpr uint32_t REVERSAL_PAUSE_MS = 80;
constexpr int MAX_DUTY = 180; // 71% ceiling; NOT a voltage regulator.
constexpr int RAMP_STEP = 6;  // PWM counts per 10 ms while accelerating

#if __has_include("secrets.h")
#include "secrets.h"
#else
// Personal isolated access point. Override locally in secrets.h.
#define PUSHBOT_WIFI_PASSWORD "floorbot-revival"
#endif
constexpr const char *WIFI_NAME = "PushBot";
constexpr const char *WIFI_PASSWORD = PUSHBOT_WIFI_PASSWORD;

