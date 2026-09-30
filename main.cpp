#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <esp_system.h>
#include "config.h"
#include "control.h"
#include "protocol.h"
#include "driver_station.h"

WebServer http(80);
WebSocketsServer socketServer(81);
portMUX_TYPE controlMux = portMUX_INITIALIZER_UNLOCKED;
Control control;
int owner = -1;
uint32_t session = 0;
uint32_t lastSequence = 0;
int actualLeft = 0, actualRight = 0;

void stopRobot() {
  portENTER_CRITICAL(&controlMux);
  control.stop();
  portEXIT_CRITICAL(&controlMux);
}

void writeMotor(uint8_t a, uint8_t b, uint8_t channel, int value, bool invert) {
  if (invert) value = -value;
  // Disable the bridge before changing either direction input.
  ledcWrite(channel, 0);
  digitalWrite(a, value > 0 ? HIGH : LOW);
  digitalWrite(b, value < 0 ? HIGH : LOW);
  ledcWrite(channel, abs(value));
}

void motorTask(void *) {
  MotorRamp leftRamp, rightRamp;
  TickType_t wake = xTaskGetTickCount();
  for (;;) {
    uint32_t now = millis();
    portENTER_CRITICAL(&controlMux);
    control.tick(now);
    // GPIO writes are outside the lock. This loop never waits for networking.
    int left = control.armed ? control.left * MAX_DUTY / 100 : 0;
    int right = control.armed ? control.right * MAX_DUTY / 100 : 0;
    portEXIT_CRITICAL(&controlMux);
    int l = leftRamp.step(left, now), r = rightRamp.step(right, now);
    writeMotor(LEFT_IN1, LEFT_IN2, 0, l, INVERT_LEFT);
    writeMotor(RIGHT_IN1, RIGHT_IN2, 1, r, INVERT_RIGHT);
    portENTER_CRITICAL(&controlMux);
    actualLeft = l; actualRight = r;
    portEXIT_CRITICAL(&controlMux);
    vTaskDelayUntil(&wake, pdMS_TO_TICKS(10));
  }
}

void sendState(uint8_t client) {
  portENTER_CRITICAL(&controlMux);
  bool armed = control.armed;
  int l = actualLeft, r = actualRight;
  portEXIT_CRITICAL(&controlMux);
  char message[160];
  snprintf(message, sizeof(message),
           "{\"type\":\"state\",\"armed\":%s,\"left\":%d,\"right\":%d,\"uptime\":%lu,\"ack\":%lu}",
           armed ? "true" : "false", l, r, (unsigned long)(millis() / 1000),
           (unsigned long)lastSequence);
  socketServer.sendTXT(client, message);
}

void onSocket(uint8_t client, WStype_t type, uint8_t *payload, size_t length) {
  if (type == WStype_CONNECTED) {
    if (owner >= 0) {
      socketServer.sendTXT(client, "{\"type\":\"busy\"}");
      socketServer.disconnect(client);
      return;
    }
    stopRobot(); owner = client; lastSequence = 0;
    do { session = esp_random(); } while (!session);
    char hello[100];
    snprintf(hello, sizeof(hello), "{\"type\":\"hello\",\"session\":%lu,\"maxDuty\":%d}",
             (unsigned long)session, MAX_DUTY);
    socketServer.sendTXT(client, hello);
    sendState(client);
    return;
  }
  if (client != owner) return;
  if (type == WStype_DISCONNECTED) { stopRobot(); owner = -1; return; }
  if (type == WStype_ERROR || type == WStype_BIN || type == WStype_FRAGMENT_TEXT_START ||
      type == WStype_FRAGMENT_BIN_START || type == WStype_FRAGMENT || type == WStype_FRAGMENT_FIN) {
    stopRobot(); socketServer.disconnect(client); return;
  }
  if (type != WStype_TEXT) return;
  char command; uint32_t id, sequence; int left, right;
  if (!parseCommand(payload, length, command, id, sequence, left, right) ||
      id != session || sequence <= lastSequence) {
    stopRobot(); sendState(client); return;
  }
  lastSequence = sequence;
  portENTER_CRITICAL(&controlMux);
  if (command == 'S') control.stop();
  else if (command == 'A') {
    // Arming is an explicit neutral-only transition.
    if (left == 0 && right == 0 && !control.armed) control.arm(millis());
    else control.stop();
  } else if (!control.drive(left, right, millis())) control.stop();
  portEXIT_CRITICAL(&controlMux);
  sendState(client);
}

void setup() {
  Serial.begin(115200);
  for (uint8_t pin : {LEFT_IN1, LEFT_IN2, RIGHT_IN1, RIGHT_IN2, LEFT_EN, RIGHT_EN}) {
    pinMode(pin, OUTPUT); digitalWrite(pin, LOW);
  }
  ledcSetup(0, 1000, 8); ledcSetup(1, 1000, 8);
  ledcAttachPin(LEFT_EN, 0); ledcAttachPin(RIGHT_EN, 1);
  ledcWrite(0, 0); ledcWrite(1, 0);
  if (xTaskCreatePinnedToCore(motorTask, "motor-watchdog", 3072, nullptr, 3, nullptr, 1) != pdPASS) {
    Serial.println("Motor task failed. Outputs stay disabled.");
    for (;;) delay(1000);
  }
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  if (!WiFi.softAP(WIFI_NAME, WIFI_PASSWORD, 6, false, 1)) {
    Serial.println("Wi-Fi failed. Outputs stay disabled.");
    for (;;) delay(1000);
  }
  http.on("/", HTTP_GET, []() {
    http.sendHeader("Cache-Control", "no-store");
    http.send_P(200, "text/html; charset=utf-8", DRIVER_STATION);
  });
  http.onNotFound([]() { http.send(404, "text/plain", "Open http://192.168.4.1/"); });
  socketServer.begin();
  socketServer.onEvent(onSocket);
  socketServer.enableHeartbeat(1000, 1000, 2);
  http.begin();
  Serial.print("PushBot driver station: http://"); Serial.println(WiFi.softAPIP());
}

void loop() {
  socketServer.loop();
  http.handleClient();
  static uint32_t lastStatus = 0;
  if (millis() - lastStatus >= 100) {
    lastStatus = millis();
    if (owner >= 0) sendState(uint8_t(owner));
  }
  delay(1);
}
