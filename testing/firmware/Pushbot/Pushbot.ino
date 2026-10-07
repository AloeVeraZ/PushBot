#include <Arduino.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp32-hal-rmt.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <driver/gpio.h>
#include <soc/gpio_periph.h>
#include <soc/io_mux_reg.h>

#include "driver_station.h"

// ---------------------------------------------------------------------------
// Pushbot configuration
// ---------------------------------------------------------------------------

// The ESP32 creates this network. Change both values before normal use.
constexpr char AP_SSID[] = "Pushbot";
constexpr char AP_PASSWORD[] = "pushbot-drive";  // WPA2 requires 8+ characters.

// A home network saved from the driver station's Wi-Fi tab is joined when in
// range; otherwise the Pushbot hotspot above starts. On the home network the
// station is at http://pushbot.local/ or the address shown in the Wi-Fi tab.
constexpr char HOSTNAME[] = "pushbot";
constexpr uint32_t HOME_JOIN_TIMEOUT_MS = 15000;
constexpr uint32_t HOME_RETRY_MS = 60000;      // Only while no one uses the hotspot.
constexpr uint32_t HOME_LOST_MS = 10000;       // Then fall back to the hotspot.
constexpr uint32_t HOTSPOT_LINGER_MS = 60000;  // After joining from the Wi-Fi tab.

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

constexpr uint32_t COMMAND_TIMEOUT_MS = 500;  // Tolerates home Wi-Fi delays.
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
bool statusLedFault = false;
bool statusLedStopFlash = false;
uint32_t statusLedStopMs = 0;
bool statusLedReady = false;
// Async RMT needs a persistent buffer until its completion interrupt fires.
rmt_data_t statusLedData[25];
constexpr uint32_t WIFI_RETRY_MS = 3000;
// Full 19.5 dBm transmit power draws current surges that a weak 5 V supply
// cannot hold. 8.5 dBm still covers driving within sight. After a brownout
// reset, wait for the supply to recover and start at the minimum instead.
constexpr wifi_power_t WIFI_TX_POWER = WIFI_POWER_8_5dBm;
constexpr wifi_power_t WIFI_TX_POWER_AFTER_BROWNOUT = WIFI_POWER_2dBm;
constexpr uint32_t WIFI_START_DELAY_AFTER_BROWNOUT_MS = 1000;

// Off: not started yet. Joining: trying the saved home network. Home: joined.
// Hotspot: serving the Pushbot network (hotspotUp says whether that worked).
enum class NetState : uint8_t { Off, Joining, Home, Hotspot };
enum class NetAction : uint8_t { None, Join, Forget };
NetState netState = NetState::Off;
NetAction pendingNetAction = NetAction::None;
uint32_t pendingNetActionMs = 0;
uint32_t netStateMs = 0;
uint32_t networkStartMs = 0;
uint32_t lastHomeAttemptMs = 0;
uint32_t homeLostMs = 0;
uint32_t hotspotOffAtMs = 0;
bool hotspotUp = false;
bool hotspotLinger = false;
bool serverStarted = false;
bool mdnsStarted = false;
String homeSsid;
String homePassword;
String joinResult = "No home network saved";
volatile uint8_t lastDisconnectReason = 0;
// Read-only request counters for the "?" diagnostic.
uint32_t pageRequests = 0, statusRequests = 0, armRequests = 0, driveRequests = 0;
const char *lastArmResult = "none";
esp_reset_reason_t bootResetReason = ESP_RST_UNKNOWN;

const char *resetReasonName(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "power-on";
    case ESP_RST_EXT: return "reset pin";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "interrupt watchdog";
    case ESP_RST_TASK_WDT: return "task watchdog";
    case ESP_RST_WDT: return "watchdog";
    case ESP_RST_DEEPSLEEP: return "deep sleep";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_USB: return "USB";
    default: return "unknown";
  }
}

bool initStatusLed() {
  return rmtInit(STATUS_LED_PIN, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, 10000000);
}

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
  if (!rmtWriteAsync(STATUS_LED_PIN, statusLedData, 25)) {
    // A failed RMT transmit never reports completion, which would freeze the
    // LED. Recreate the channel; the next animation frame retries.
    rmtDeinit(STATUS_LED_PIN);
    statusLedReady = initStatusLed();
  }
}

void limitWifiPower() {
  const wifi_power_t power =
      bootResetReason == ESP_RST_BROWNOUT ? WIFI_TX_POWER_AFTER_BROWNOUT : WIFI_TX_POWER;
  esp_wifi_set_max_tx_power(static_cast<int8_t>(power));
}

void stopRobot(const String &reason);

// ---------------------------------------------------------------------------
// Networking: saved home Wi-Fi first, Pushbot hotspot as the fallback.
// ---------------------------------------------------------------------------

const IPAddress HOTSPOT_IP(192, 168, 4, 1);

bool networkReady() {
  return hotspotUp || (netState == NetState::Home && WiFi.status() == WL_CONNECTED);
}

const char *netStateName() {
  switch (netState) {
    case NetState::Joining: return "joining";
    case NetState::Home: return "home";
    case NetState::Hotspot: return hotspotUp ? "hotspot" : "failed";
    default: return "starting";
  }
}

void setNetState(NetState next) {
  netState = next;
  netStateMs = millis();
}

void loadHomeNetwork() {
  Preferences prefs;
  if (!prefs.begin("pushbot", true)) return;  // Nothing saved yet.
  homeSsid = prefs.getString("ssid", "");
  homePassword = prefs.getString("pass", "");
  prefs.end();
  if (homeSsid.length()) joinResult = "Saved: " + homeSsid;
}

bool saveHomeNetwork(const String &ssid, const String &password) {
  Preferences prefs;
  if (!prefs.begin("pushbot", false)) return false;
  const bool ok = ssid.length() ? prefs.putString("ssid", ssid) == ssid.length() &&
                                      prefs.putString("pass", password) == password.length()
                                : prefs.remove("ssid") | prefs.remove("pass");
  prefs.end();
  homeSsid = ssid;
  homePassword = password;
  return ok || !ssid.length();
}

void startWebServices() {
  if (!serverStarted) {
    server.begin();
    serverStarted = true;
  }
  if (!mdnsStarted && MDNS.begin(HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
    mdnsStarted = true;
  }
}

// Starts the Pushbot network. With keepStation the home-network radio stays on
// alongside it, which is needed to scan or to try joining while it is up.
bool startHotspot(bool keepStation) {
  if (!WiFi.mode(keepStation ? WIFI_AP_STA : WIFI_AP)) return hotspotUp = false;
  limitWifiPower();  // As soon as the radio starts, before beacons begin.
  WiFi.setSleep(false);
  if (!hotspotUp) {
    hotspotUp = WiFi.softAPConfig(HOTSPOT_IP, HOTSPOT_IP, IPAddress(255, 255, 255, 0)) &&
                WiFi.softAP(AP_SSID, AP_PASSWORD, 6, false, 2);
    if (hotspotUp) dnsServer.start(53, "*", HOTSPOT_IP);
  }
  limitWifiPower();
  if (hotspotUp) startWebServices();
  return hotspotUp;
}

void stopHotspot() {
  if (!hotspotUp) return;
  dnsServer.stop();
  WiFi.softAPdisconnect(false);
  hotspotUp = false;
  hotspotLinger = false;
  WiFi.mode(WIFI_STA);
}

// Gives up on the home network (no further reconnect attempts, which would
// hop channels and disturb hotspot users) and serves the hotspot instead.
void fallBackToHotspot() {
  if (WiFi.getMode() & WIFI_STA) WiFi.disconnect(false, false);
  setNetState(NetState::Hotspot);
  lastHomeAttemptMs = millis();
  hotspotLinger = false;
  startHotspot(false);
}

void startHomeJoin(bool keepHotspot) {
  if (robotArmed) stopRobot("Joining home Wi-Fi");
  if (keepHotspot && hotspotUp) {
    WiFi.mode(WIFI_AP_STA);
  } else {
    if (hotspotUp) {
      dnsServer.stop();
      WiFi.softAPdisconnect(false);
      hotspotUp = false;
    }
    WiFi.mode(WIFI_STA);
  }
  limitWifiPower();
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  lastDisconnectReason = 0;
  WiFi.begin(homeSsid.c_str(), homePassword.c_str());
  lastHomeAttemptMs = millis();
  joinResult = "Joining " + homeSsid + "...";
  setNetState(NetState::Joining);
}

String joinFailureText() {
  switch (lastDisconnectReason) {
    case WIFI_REASON_NO_AP_FOUND: return homeSsid + " is not in range";
    case WIFI_REASON_AUTH_FAIL:
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
    case WIFI_REASON_HANDSHAKE_TIMEOUT:
    case WIFI_REASON_802_1X_AUTH_FAILED: return homeSsid + " rejected the password";
    default: return "Could not join " + homeSsid;
  }
}

void startNetwork() {
  networkStartMs = millis();
  if (homeSsid.length()) startHomeJoin(false);
  else fallBackToHotspot();
}

void serviceNetwork() {
  const uint32_t now = millis();
  if (pendingNetAction != NetAction::None && now - pendingNetActionMs >= 300) {
    // Deferred so the HTTP reply that requested it is delivered first.
    const NetAction action = pendingNetAction;
    pendingNetAction = NetAction::None;
    if (action == NetAction::Join) {
      hotspotLinger = hotspotUp;  // Keep the hotspot so the page can show the result.
      startHomeJoin(true);
    } else if (netState != NetState::Hotspot) {
      fallBackToHotspot();
    }
  }

  switch (netState) {
    case NetState::Off:
      if (bootResetReason != ESP_RST_BROWNOUT || now >= WIFI_START_DELAY_AFTER_BROWNOUT_MS) {
        startNetwork();
      }
      break;
    case NetState::Joining:
      if (WiFi.status() == WL_CONNECTED) {
        setNetState(NetState::Home);
        homeLostMs = 0;
        limitWifiPower();
        startWebServices();
        joinResult = "Connected to " + homeSsid + " at " + WiFi.localIP().toString();
        if (hotspotLinger) hotspotOffAtMs = now + HOTSPOT_LINGER_MS;
        else stopHotspot();
      } else if (now - netStateMs >= HOME_JOIN_TIMEOUT_MS) {
        joinResult = joinFailureText() + "; using the Pushbot hotspot";
        fallBackToHotspot();
      }
      break;
    case NetState::Home:
      if (WiFi.status() == WL_CONNECTED) {
        homeLostMs = 0;
        if (hotspotUp && hotspotLinger && static_cast<int32_t>(now - hotspotOffAtMs) >= 0) {
          stopHotspot();
        }
      } else {
        if (!homeLostMs) {
          homeLostMs = now ? now : 1;
          if (robotArmed) stopRobot("Home Wi-Fi lost");
        }
        if (now - homeLostMs >= HOME_LOST_MS) {
          joinResult = "Lost " + homeSsid + "; using the Pushbot hotspot";
          fallBackToHotspot();
        }
      }
      break;
    case NetState::Hotspot:
      if (!hotspotUp) {
        if (now - netStateMs >= WIFI_RETRY_MS) {
          stopRobot("Wi-Fi unavailable");
          WiFi.softAPdisconnect(true);
          fallBackToHotspot();
        }
      } else if (homeSsid.length() && !robotArmed && WiFi.softAPgetStationNum() == 0 &&
                 WiFi.scanComplete() != WIFI_SCAN_RUNNING &&
                 now - lastHomeAttemptMs >= HOME_RETRY_MS) {
        startHomeJoin(true);  // Nobody is using the hotspot, so look for home again.
      }
      break;
  }
}

void writeUsbLine(const char *line, int count) {
  if (count > 0 && Serial.availableForWrite() >= count) {
    Serial.write(reinterpret_cast<const uint8_t *>(line), count);
  }
}

// Reads each motor pin's actual pad level for 2 ms (two PWM periods) and
// reports the share of samples that were HIGH. Read-only: it never drives.
void reportMotorPins() {
  const uint8_t pins[] = {FRONT_LEFT_IN1, FRONT_LEFT_IN2, REAR_LEFT_IN1, REAR_LEFT_IN2,
                          FRONT_RIGHT_IN1, FRONT_RIGHT_IN2, REAR_RIGHT_IN1, REAR_RIGHT_IN2};
  uint16_t high[8] = {};
  for (uint8_t pin : pins) PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[pin]);
  const uint32_t start = micros();
  uint16_t samples = 0;
  while (micros() - start < 2000) {
    for (size_t i = 0; i < 8; ++i) high[i] += gpio_get_level(static_cast<gpio_num_t>(pins[i]));
    ++samples;
  }
  char line[128];
  const int count = snprintf(line, sizeof(line),
      "Pins %% high: FL %u=%u %u=%u | RL %u=%u %u=%u | FR %u=%u %u=%u | RR %u=%u %u=%u\n",
      pins[0], high[0] * 100 / samples, pins[1], high[1] * 100 / samples,
      pins[2], high[2] * 100 / samples, pins[3], high[3] * 100 / samples,
      pins[4], high[4] * 100 / samples, pins[5], high[5] * 100 / samples,
      pins[6], high[6] * 100 / samples, pins[7], high[7] * 100 / samples);
  writeUsbLine(line, min(count, static_cast<int>(sizeof(line)) - 1));
}

void serviceUsbDiagnostics() {
  // Only query/write a live USB console. No USB wait, flush, or boot logging.
  if (!Serial || Serial.available() <= 0) return;
  const int command = Serial.read();
  if (command == 'p') reportMotorPins();
  if (command != '?') return;
  int8_t txPower = 0;
  if (esp_wifi_get_max_tx_power(&txPower) != ESP_OK) txPower = 0;
  const String ip = netState == NetState::Home ? WiFi.localIP().toString() : HOTSPOT_IP.toString();
  char line[128];
  int count = snprintf(line, sizeof(line),
      "Pushbot status: net=%s ip=%s armed=%s left=%.3f right=%.3f reset=%s tx=%.2f dBm\n",
      netStateName(), ip.c_str(), robotArmed ? "yes" : "no", actualLeft, actualRight,
      resetReasonName(bootResetReason), txPower / 4.0f);
  writeUsbLine(line, min(count, static_cast<int>(sizeof(line)) - 1));
  count = snprintf(line, sizeof(line),
      "Web: hotspot clients=%u page=%lu status=%lu arm=%lu (%s) drive=%lu\n",
      hotspotUp ? WiFi.softAPgetStationNum() : 0, static_cast<unsigned long>(pageRequests),
      static_cast<unsigned long>(statusRequests), static_cast<unsigned long>(armRequests),
      lastArmResult, static_cast<unsigned long>(driveRequests));
  writeUsbLine(line, min(count, static_cast<int>(sizeof(line)) - 1));
  count = snprintf(line, sizeof(line), "Home Wi-Fi: %s\n", joinResult.c_str());
  writeUsbLine(line, min(count, static_cast<int>(sizeof(line)) - 1));
}

void updateStatusLed() {
  // Animate with millis(), never delay() or wait for a whole flash cycle.
  // Motor updates run first in loop(). The LED uses RMT, not motor PWM channels.
  static uint32_t lastLedMs = 0;
  const uint32_t now = millis();
  if (now - lastLedMs < 40) return;
  lastLedMs = now;

  if (netState == NetState::Off) return;  // Keep the startup colour until Wi-Fi starts.
  if (netState == NetState::Joining && !hotspotUp) {
    // Slow amber blink while looking for the saved home network.
    showStatusColor(255, 140, 0, now % 1000 < 500 ? STATUS_LED_BRIGHTNESS : 4);
    return;
  }
  if (!networkReady() || statusLedFault) {
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
      // Slow breathing while disabled and waiting for a driver: blue on the
      // Pushbot hotspot, green on the home network.
      const uint32_t phase = now % 2400;
      const uint32_t ramp = phase < 1200 ? phase : 2400 - phase;
      const uint8_t level = 6 + ramp * (STATUS_LED_BRIGHTNESS - 6) / 1200;
      if (netState == NetState::Home) showStatusColor(0, 255, 60, level);
      else showStatusColor(0, 60, 255, level);
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
  ++statusRequests;
  const String client = server.arg("cid");
  const bool owner = robotArmed && client == activeClient;
  const uint32_t age = robotArmed ? millis() - lastCommandMs : 0;
  String json;
  json.reserve(320);
  json += "{\"armed\":";
  json += robotArmed ? "true" : "false";
  json += ",\"owner\":";
  json += owner ? "true" : "false";
  json += ",\"net\":\"" + String(netStateName()) + "\"";
  json += ",\"clients\":" + String(hotspotUp ? WiFi.softAPgetStationNum() : 0);
  json += ",\"commandAgeMs\":" + String(age);
  json += ",\"uptimeMs\":" + String(millis());
  json += ",\"left\":" + String(actualLeft, 3);
  json += ",\"right\":" + String(actualRight, 3);
  json += ",\"reset\":\"" + String(resetReasonName(bootResetReason)) + "\"";
  json += ",\"reason\":\"" + lastStopReason + "\"}";
  sendJson(200, json);
}

void handleArm() {
  ++armRequests;
  if (!networkReady()) {
    lastArmResult = "wifi not ready";
    sendJson(503, "{\"error\":\"Wi-Fi is not ready\"}");
    return;
  }
  const String client = server.arg("cid");
  if (server.arg("safe") != "1") {
    lastArmResult = "no safety box";
    sendJson(400, "{\"error\":\"Safety confirmation is required\"}");
    return;
  }
  if (!validClient(client)) {
    lastArmResult = "bad driver id";
    sendJson(400, "{\"error\":\"Invalid driver identity\"}");
    return;
  }
  if (robotArmed && client != activeClient) {
    lastArmResult = "other driver";
    sendJson(409, "{\"error\":\"Another driver already owns control\"}");
    return;
  }

  lastArmResult = "enabled";
  stopRobot("Enabled by driver");
  activeClient = client;
  robotArmed = true;
  lastCommandMs = millis();
  sendStatus();
}

void handleDrive() {
  ++driveRequests;
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

String jsonString(const String &value) {
  String out = "\"";
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    if (c == '"' || c == '\\') {
      out += '\\';
      out += c;
    } else if (static_cast<uint8_t>(c) < 0x20) {
      char escaped[7];
      snprintf(escaped, sizeof(escaped), "\\u%04x", c);
      out += escaped;
    } else {
      out += c;
    }
  }
  return out + "\"";
}

void handleWifiStatus() {
  const bool home = netState == NetState::Home && WiFi.status() == WL_CONNECTED;
  String json = "{\"state\":\"" + String(netStateName()) + "\"";
  json += ",\"saved\":" + jsonString(homeSsid);
  json += ",\"ip\":\"" + (home ? WiFi.localIP().toString() : String("")) + "\"";
  json += ",\"rssi\":" + String(home ? WiFi.RSSI() : 0);
  json += ",\"hotspot\":" + String(hotspotUp ? "true" : "false");
  json += ",\"hotspotName\":" + jsonString(AP_SSID);
  json += ",\"hotspotIp\":\"" + HOTSPOT_IP.toString() + "\"";
  json += ",\"hostname\":\"" + String(HOSTNAME) + ".local\"";
  json += ",\"result\":" + jsonString(joinResult) + "}";
  sendJson(200, json);
}

void handleWifiScan() {
  if (robotArmed) {
    sendJson(409, "{\"error\":\"Disable the robot before scanning\"}");
    return;
  }
  if (netState == NetState::Joining || pendingNetAction != NetAction::None) {
    sendJson(409, "{\"error\":\"Pushbot is joining a network; try again shortly\"}");
    return;
  }
  if (WiFi.scanComplete() != WIFI_SCAN_RUNNING) {
    // Scanning needs the station radio; the hotspot stays up alongside it.
    if (!(WiFi.getMode() & WIFI_STA)) WiFi.mode(WIFI_AP_STA);
    WiFi.scanDelete();
    if (WiFi.scanNetworks(true) == WIFI_SCAN_FAILED) {
      sendJson(500, "{\"error\":\"Scan could not start\"}");
      return;
    }
  }
  sendJson(200, "{\"ok\":true}");
}

void handleWifiNetworks() {
  const int16_t found = WiFi.scanComplete();
  if (found == WIFI_SCAN_RUNNING) {
    sendJson(200, "{\"scanning\":true}");
    return;
  }
  if (found < 0) {
    sendJson(200, "{\"scanning\":false,\"networks\":[]}");
    return;
  }
  // Strongest entry per name, strongest first, at most 20.
  int16_t order[64];
  int16_t count = 0;
  for (int16_t i = 0; i < found && count < 64; ++i) {
    const String ssid = WiFi.SSID(i);
    if (!ssid.length()) continue;
    bool duplicate = false;
    for (int16_t j = 0; j < count; ++j) {
      if (WiFi.SSID(order[j]) == ssid) {
        if (WiFi.RSSI(i) > WiFi.RSSI(order[j])) order[j] = i;
        duplicate = true;
        break;
      }
    }
    if (!duplicate) order[count++] = i;
  }
  for (int16_t i = 1; i < count; ++i) {
    for (int16_t j = i; j > 0 && WiFi.RSSI(order[j]) > WiFi.RSSI(order[j - 1]); --j) {
      const int16_t swap = order[j];
      order[j] = order[j - 1];
      order[j - 1] = swap;
    }
  }
  String json = "{\"scanning\":false,\"networks\":[";
  for (int16_t i = 0; i < count && i < 20; ++i) {
    if (i) json += ',';
    json += "{\"ssid\":" + jsonString(WiFi.SSID(order[i]));
    json += ",\"rssi\":" + String(WiFi.RSSI(order[i]));
    json += ",\"secure\":" + String(WiFi.encryptionType(order[i]) == WIFI_AUTH_OPEN ? "false" : "true");
    json += "}";
  }
  json += "]}";
  WiFi.scanDelete();
  // Put the radio back how the hotspot normally runs.
  if (netState == NetState::Hotspot && hotspotUp) WiFi.mode(WIFI_AP);
  sendJson(200, json);
}

void handleWifiSave() {
  const String ssid = server.arg("ssid");
  const String password = server.arg("password");
  if (!ssid.length() || ssid.length() > 32) {
    sendJson(400, "{\"error\":\"Network name must be 1 to 32 characters\"}");
    return;
  }
  if (password.length() > 63 || (password.length() && password.length() < 8)) {
    sendJson(400, "{\"error\":\"Wi-Fi passwords are 8 to 63 characters (or empty for open networks)\"}");
    return;
  }
  stopRobot("Wi-Fi settings changed");
  if (!saveHomeNetwork(ssid, password)) {
    sendJson(500, "{\"error\":\"Could not save the network\"}");
    return;
  }
  joinResult = "Saved " + ssid + "; joining...";
  pendingNetAction = NetAction::Join;
  pendingNetActionMs = millis();
  handleWifiStatus();
}

void handleWifiForget() {
  stopRobot("Wi-Fi settings changed");
  saveHomeNetwork("", "");
  joinResult = "No home network saved";
  pendingNetAction = NetAction::Forget;
  pendingNetActionMs = millis();
  handleWifiStatus();
}

void serveDriverStation() {
  ++pageRequests;
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", DRIVER_STATION_HTML);
}

void setupRoutes() {
  server.on("/", HTTP_GET, serveDriverStation);
  server.on("/api/status", HTTP_GET, sendStatus);
  server.on("/api/arm", HTTP_POST, handleArm);
  server.on("/api/drive", HTTP_POST, handleDrive);
  server.on("/api/stop", HTTP_POST, handleStop);
  server.on("/api/wifi", HTTP_GET, handleWifiStatus);
  server.on("/api/wifi/scan", HTTP_POST, handleWifiScan);
  server.on("/api/wifi/networks", HTTP_GET, handleWifiNetworks);
  server.on("/api/wifi/save", HTTP_POST, handleWifiSave);
  server.on("/api/wifi/forget", HTTP_POST, handleWifiForget);

  // Captive-portal probes and unknown URLs all return the station page.
  server.on("/generate_204", HTTP_ANY, serveDriverStation);
  server.on("/gen_204", HTTP_ANY, serveDriverStation);
  server.on("/hotspot-detect.html", HTTP_ANY, serveDriverStation);
  server.on("/connecttest.txt", HTTP_ANY, serveDriverStation);
  server.onNotFound(serveDriverStation);
}

void setup() {
  // 80 MHz is the lowest clock Wi-Fi supports and is ample for the web server.
  // It lowers the current drawn alongside Wi-Fi bursts on a weak 5 V supply.
  setCpuFrequencyMhz(80);
  const MotorPins motors[] = {FRONT_LEFT, REAR_LEFT, FRONT_RIGHT, REAR_RIGHT};
  for (const MotorPins &motor : motors) {
    pinMode(motor.in1, OUTPUT);
    pinMode(motor.in2, OUTPUT);
    digitalWrite(motor.in1, LOW);
    digitalWrite(motor.in2, LOW);
  }
  stopRobot("Boot");
  bootResetReason = esp_reset_reason();
  statusLedReady = initStatusLed();
  // Amber during startup; magenta if the previous reset was a brownout, so a
  // supply that sags when Wi-Fi starts is visible without USB attached.
  if (bootResetReason == ESP_RST_BROWNOUT) {
    showStatusColor(255, 0, 160, STATUS_LED_BRIGHTNESS);
  } else {
    showStatusColor(255, 140, 0, STATUS_LED_BRIGHTNESS);
  }
  setupRoutes();
  loadHomeNetwork();
  WiFi.setHostname(HOSTNAME);
  WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info) {
    lastDisconnectReason = info.wifi_sta_disconnected.reason;
  }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  // After a brownout, loop() starts Wi-Fi once the delay has passed.
  if (bootResetReason != ESP_RST_BROWNOUT) startNetwork();
  lastRampMs = millis();
  updateStatusLed();

  // USB diagnostics are optional and initialized after the driver station.
  Serial.begin(115200);
#if ARDUINO_USB_MODE && ARDUINO_USB_CDC_ON_BOOT
  Serial.setTxTimeoutMs(0);
#endif
}

void loop() {
  serviceNetwork();
  if (hotspotUp) dnsServer.processNextRequest();
  if (serverStarted) server.handleClient();
  updateMotorOutputs();
  updateStatusLed();
  serviceUsbDiagnostics();
  delay(1);
}

