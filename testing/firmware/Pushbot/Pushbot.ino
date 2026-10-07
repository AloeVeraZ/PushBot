#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp32-hal-rmt.h>

#include "driver_station.h"

// ---------------------------------------------------------------------------
// Pushbot configuration
// ---------------------------------------------------------------------------

// The ESP32 creates this network. Change both values before normal use.
constexpr char AP_SSID[] = "Pushbot";
constexpr char AP_PASSWORD[] = "pushbot-drive";  // WPA2 requires 8+ characters.

// Each motor uses one H-bridge channel with two direction/PWM inputs.
// Pin map for the Lonely Binary ESP32-S3 screw terminal base. These GPIOs avoid
// the S3 strapping pins (0, 3, 45, 46), native USB (19, 20), and RGB LED (48).
// Left motors use the left terminal block; right motors use the right block.
constexpr uint8_t FRONT_LEFT_IN1 = 13;
constexpr uint8_t FRONT_LEFT_IN2 = 14;
constexpr uint8_t REAR_LEFT_IN1 = 11;
constexpr uint8_t REAR_LEFT_IN2 = 12;
constexpr uint8_t FRONT_RIGHT_IN1 = 1;
constexpr uint8_t FRONT_RIGHT_IN2 = 2;
constexpr uint8_t REAR_RIGHT_IN1 = 42;
constexpr uint8_t REAR_RIGHT_IN2 = 41;

// Flip only the motors that rotate the wrong way during the raised-wheel test.
constexpr bool INVERT_FRONT_LEFT = false;
constexpr bool INVERT_REAR_LEFT = false;
constexpr bool INVERT_FRONT_RIGHT = true;
constexpr bool INVERT_REAR_RIGHT = true;

constexpr uint32_t COMMAND_TIMEOUT_MS = 350;
constexpr float SLEW_PER_SECOND = 3.5f;  // 0 -> full output in about 0.29 seconds.
constexpr float HARD_OUTPUT_LIMIT = 1.0f;

// Lonely Binary ESP32-S3 N16R8: the on-board WS2812B is marked RGB@IO48.
// The screw-terminal base passes this same GPIO through; keep it off motors.
constexpr uint8_t STATUS_LED_PIN = 48;
constexpr uint8_t STATUS_LED_BRIGHTNESS = 40;  // Out of 255.
static_assert(STATUS_LED_PIN != FRONT_LEFT_IN1 && STATUS_LED_PIN != FRONT_LEFT_IN2 &&
              STATUS_LED_PIN != REAR_LEFT_IN1 && STATUS_LED_PIN != REAR_LEFT_IN2 &&
              STATUS_LED_PIN != FRONT_RIGHT_IN1 && STATUS_LED_PIN != FRONT_RIGHT_IN2 &&
              STATUS_LED_PIN != REAR_RIGHT_IN1 && STATUS_LED_PIN != REAR_RIGHT_IN2,
              "The RGB LED must not share a motor input GPIO");

struct MotorPins {
  uint8_t in1;
  uint8_t in2;
  bool inverted;
};

constexpr MotorPins FRONT_LEFT{FRONT_LEFT_IN1, FRONT_LEFT_IN2, INVERT_FRONT_LEFT};
constexpr MotorPins REAR_LEFT{REAR_LEFT_IN1, REAR_LEFT_IN2, INVERT_REAR_LEFT};
constexpr MotorPins FRONT_RIGHT{FRONT_RIGHT_IN1, FRONT_RIGHT_IN2, INVERT_FRONT_RIGHT};
constexpr MotorPins REAR_RIGHT{REAR_RIGHT_IN1, REAR_RIGHT_IN2, INVERT_REAR_RIGHT};

DNSServer dnsServer;
WebServer server(80);

bool robotArmed = false;
String activeClient;
String lastStopReason = "Boot";
uint32_t lastCommandMs = 0;
uint32_t lastRampMs = 0;
float targetLeft = 0.0f;
float targetRight = 0.0f;
float actualLeft = 0.0f;
float actualRight = 0.0f;
bool accessPointReady = false;
bool statusLedFault = false;
bool statusLedStopFlash = false;
uint32_t statusLedStopMs = 0;
bool statusLedReady = false;
// Async RMT needs a persistent buffer until its completion interrupt fires.
rmt_data_t statusLedData[25];
uint32_t lastWifiAttemptMs = 0;
constexpr uint32_t WIFI_RETRY_MS = 3000;

void showStatusColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness) {
  // Never let the indicator hold up boot, networking, or motor timeouts.
  if (!statusLedReady || !rmtTransmitCompleted(STATUS_LED_PIN)) return;
  const uint8_t grb[] = {
    static_cast<uint8_t>(green * brightness / 255),
    static_cast<uint8_t>(red * brightness / 255),
    static_cast<uint8_t>(blue * brightness / 255),
  };
  size_t index = 0;
  for (uint8_t color : grb) {
    for (int bit = 7; bit >= 0; --bit) {
      const bool high = color & (1 << bit);
      statusLedData[index].level0 = 1;
      statusLedData[index].duration0 = high ? 8 : 4;
      statusLedData[index].level1 = 0;
      statusLedData[index].duration1 = high ? 4 : 8;
      ++index;
    }
  }
  // A 100 us low reset/latch at 10 MHz, after the 24 GRB bits.
  statusLedData[24].level0 = statusLedData[24].level1 = 0;
  statusLedData[24].duration0 = statusLedData[24].duration1 = 500;
  rmtWriteAsync(STATUS_LED_PIN, statusLedData, 25);
}

void startAccessPoint() {
  lastWifiAttemptMs = millis();
  const IPAddress ip(192, 168, 4, 1);
  const IPAddress subnet(255, 255, 255, 0);
  accessPointReady = WiFi.mode(WIFI_AP);
  if (accessPointReady) {
    WiFi.setSleep(false);
    accessPointReady = WiFi.softAPConfig(ip, ip, subnet) &&
                       WiFi.softAP(AP_SSID, AP_PASSWORD, 6, false, 2);
  }
  if (accessPointReady) {
    dnsServer.start(53, "*", ip);
    server.begin();
  }
}

void serviceUsbDiagnostics() {
  // Only query/write a live USB console. No USB wait, flush, or boot logging.
  if (!Serial || Serial.available() <= 0 || Serial.read() != '?') return;
  char message[192];
  const int count = snprintf(message, sizeof(message),
      "Pushbot status: Wi-Fi=%s, armed=%s, left=%.3f, right=%.3f\n"
      "ESP32-S3: flash=%u MB, PSRAM=%u MB, RGB=GPIO48\n",
      accessPointReady ? "ready" : "failed", robotArmed ? "yes" : "no",
      actualLeft, actualRight, ESP.getFlashChipSize() / (1024 * 1024),
      ESP.getPsramSize() / (1024 * 1024));
  if (count > 0 && count < static_cast<int>(sizeof(message)) &&
      Serial.availableForWrite() >= count) {
    Serial.write(reinterpret_cast<const uint8_t *>(message), count);
  }
}

void updateStatusLed() {
  // Animate with millis(), never delay() or wait for a whole flash cycle.
  // Motor updates run first in loop(). The LED uses RMT, not motor PWM channels.
  static uint32_t lastLedMs = 0;
  const uint32_t now = millis();
  if (now - lastLedMs < 40) return;
  lastLedMs = now;

  if (!accessPointReady || statusLedFault) {
    // Two red flashes per second; faults stay visible until deliberate re-arm.
    const uint32_t phase = now % 1000;
    const bool on = phase < 120 || (phase >= 250 && phase < 370);
    showStatusColor(255, 0, 0, on ? STATUS_LED_BRIGHTNESS : 0);
    return;
  }
  if (statusLedStopFlash && now - statusLedStopMs >= 1200) {
    statusLedStopFlash = false;
  }
  if (!robotArmed) {
    if (statusLedStopFlash) {
      const bool on = (now - statusLedStopMs) % 240 < 120;
      showStatusColor(255, 0, 0, on ? STATUS_LED_BRIGHTNESS : 0);
    } else {
      // Slow blue breathing while disabled and waiting for a driver.
      const uint32_t phase = now % 2400;
      const uint32_t ramp = phase < 1200 ? phase : 2400 - phase;
      showStatusColor(0, 60, 255, 6 + ramp * (STATUS_LED_BRIGHTNESS - 6) / 1200);
    }
    return;
  }
  if (fabsf(actualLeft) < 0.01f && fabsf(actualRight) < 0.01f) {
    showStatusColor(255, 255, 255, STATUS_LED_BRIGHTNESS / 2);
    return;
  }

  // Use the ramped output commands, so the light follows the commanded motion.
  const uint8_t brightness = now % 500 < 300 ? STATUS_LED_BRIGHTNESS : 6;
  if (fabsf(actualLeft - actualRight) > 0.08f) {
    if (actualLeft > actualRight) showStatusColor(255, 100, 0, brightness);  // Right.
    else showStatusColor(0, 255, 255, brightness);  // Left.
  } else if (actualLeft + actualRight > 0.0f) {
    showStatusColor(0, 255, 0, brightness);  // Forward.
  } else {
    showStatusColor(200, 0, 255, brightness);  // Reverse.
  }
}

float clampPower(float value) {
  return constrain(value, -HARD_OUTPUT_LIMIT, HARD_OUTPUT_LIMIT);
}

void writeMotor(const MotorPins &motor, float power) {
  power = clampPower(motor.inverted ? -power : power);
  const uint8_t duty = static_cast<uint8_t>(roundf(fabsf(power) * 255.0f));

  // Clear both legs before selecting direction. With the common EN jumper
  // fitted this produces coast at zero and PWM on the active input.
  analogWrite(motor.in1, 0);
  analogWrite(motor.in2, 0);
  if (power > 0.001f) {
    analogWrite(motor.in1, duty);
  } else if (power < -0.001f) {
    analogWrite(motor.in2, duty);
  }
}

void writeTank(float left, float right) {
  writeMotor(FRONT_LEFT, left);
  writeMotor(REAR_LEFT, left);
  writeMotor(FRONT_RIGHT, right);
  writeMotor(REAR_RIGHT, right);
}

void stopRobot(const String &reason) {
  robotArmed = false;
  activeClient = "";
  targetLeft = targetRight = 0.0f;
  actualLeft = actualRight = 0.0f;
  writeTank(0.0f, 0.0f);
  lastStopReason = reason;
  if (reason == "Boot" || reason == "Enabled by driver") statusLedFault = false;
  else if (reason == "Command watchdog expired" || reason == "Malformed drive command") statusLedFault = true;
  statusLedStopFlash = reason == "Stopped by driver";
  statusLedStopMs = millis();
}

float approach(float current, float target, float maximumStep) {
  if (current < target) return min(current + maximumStep, target);
  if (current > target) return max(current - maximumStep, target);
  return current;
}

void updateMotorOutputs() {
  const uint32_t now = millis();
  const uint32_t elapsed = now - lastRampMs;
  if (elapsed < 5) return;
  lastRampMs = now;

  if (robotArmed && now - lastCommandMs > COMMAND_TIMEOUT_MS) {
    stopRobot("Command watchdog expired");
    return;
  }

  if (!robotArmed) return;
  const float maximumStep = SLEW_PER_SECOND * (elapsed / 1000.0f);
  actualLeft = approach(actualLeft, targetLeft, maximumStep);
  actualRight = approach(actualRight, targetRight, maximumStep);
  writeTank(actualLeft, actualRight);
}

bool validClient(const String &client) {
  if (client.length() < 8 || client.length() > 64) return false;
  for (size_t i = 0; i < client.length(); ++i) {
    const char c = client[i];
    if (!(isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_')) return false;
  }
  return true;
}

void sendJson(int code, const String &body) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", body);
}

void sendStatus() {
  const String client = server.arg("cid");
  const bool owner = robotArmed && client == activeClient;
  const uint32_t age = robotArmed ? millis() - lastCommandMs : 0;
  String json;
  json.reserve(240);
  json += "{\"armed\":";
  json += robotArmed ? "true" : "false";
  json += ",\"owner\":";
  json += owner ? "true" : "false";
  json += ",\"clients\":" + String(WiFi.softAPgetStationNum());
  json += ",\"commandAgeMs\":" + String(age);
  json += ",\"uptimeMs\":" + String(millis());
  json += ",\"left\":" + String(actualLeft, 3);
  json += ",\"right\":" + String(actualRight, 3);
  json += ",\"reason\":\"" + lastStopReason + "\"}";
  sendJson(200, json);
}

void handleArm() {
  if (!accessPointReady) {
    sendJson(503, "{\"error\":\"Wi-Fi is not ready\"}");
    return;
  }
  const String client = server.arg("cid");
  if (server.arg("safe") != "1") {
    sendJson(400, "{\"error\":\"Safety confirmation is required\"}");
    return;
  }
  if (!validClient(client)) {
    sendJson(400, "{\"error\":\"Invalid driver identity\"}");
    return;
  }
  if (robotArmed && client != activeClient) {
    sendJson(409, "{\"error\":\"Another driver already owns control\"}");
    return;
  }

  stopRobot("Enabled by driver");
  activeClient = client;
  robotArmed = true;
  lastCommandMs = millis();
  sendStatus();
}

void handleDrive() {
  const String client = server.arg("cid");
  if (!robotArmed || client != activeClient) {
    sendJson(403, "{\"error\":\"Robot is disabled or owned by another driver\"}");
    return;
  }
  if (!server.hasArg("left") || !server.hasArg("right")) {
    stopRobot("Malformed drive command");
    sendJson(400, "{\"error\":\"Both tank commands are required\"}");
    return;
  }

  const long left = constrain(server.arg("left").toInt(), -1000L, 1000L);
  const long right = constrain(server.arg("right").toInt(), -1000L, 1000L);
  targetLeft = clampPower(left / 1000.0f);
  targetRight = clampPower(right / 1000.0f);
  lastCommandMs = millis();
  sendJson(200, "{\"ok\":true}");
}

void handleStop() {
  stopRobot("Stopped by driver");
  sendStatus();
}

void serveDriverStation() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", DRIVER_STATION_HTML);
}

void setupRoutes() {
  server.on("/", HTTP_GET, serveDriverStation);
  server.on("/api/status", HTTP_GET, sendStatus);
  server.on("/api/arm", HTTP_POST, handleArm);
  server.on("/api/drive", HTTP_POST, handleDrive);
  server.on("/api/stop", HTTP_POST, handleStop);

  // Captive-portal probes and unknown URLs all return the station page.
  server.on("/generate_204", HTTP_ANY, serveDriverStation);
  server.on("/gen_204", HTTP_ANY, serveDriverStation);
  server.on("/hotspot-detect.html", HTTP_ANY, serveDriverStation);
  server.on("/connecttest.txt", HTTP_ANY, serveDriverStation);
  server.onNotFound(serveDriverStation);
}

void setup() {
  const MotorPins motors[] = {FRONT_LEFT, REAR_LEFT, FRONT_RIGHT, REAR_RIGHT};
  for (const MotorPins &motor : motors) {
    pinMode(motor.in1, OUTPUT);
    pinMode(motor.in2, OUTPUT);
    digitalWrite(motor.in1, LOW);
    digitalWrite(motor.in2, LOW);
  }
  stopRobot("Boot");
  statusLedReady = rmtInit(STATUS_LED_PIN, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, 10000000);
  showStatusColor(255, 140, 0, STATUS_LED_BRIGHTNESS);  // Amber during startup.
  setupRoutes();
  startAccessPoint();
  lastRampMs = millis();
  updateStatusLed();

  // USB diagnostics are optional and initialized after the driver station.
  Serial.begin(115200);
#if ARDUINO_USB_MODE && ARDUINO_USB_CDC_ON_BOOT
  Serial.setTxTimeoutMs(0);
#endif
}

void loop() {
  if (accessPointReady) {
    dnsServer.processNextRequest();
    server.handleClient();
  } else if (millis() - lastWifiAttemptMs >= WIFI_RETRY_MS) {
    stopRobot("Wi-Fi unavailable");
    WiFi.softAPdisconnect(true);
    startAccessPoint();
  }
  updateMotorOutputs();
  updateStatusLed();
  serviceUsbDiagnostics();
  delay(1);
}

