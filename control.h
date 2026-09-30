#pragma once
#include <stdint.h>
#include "config.h"

// Called under the same lock by networking and the independent motor task.
struct Control {
  bool armed = false;
  int left = 0, right = 0;
  uint32_t lastCommand = 0;
  void stop() { armed = false; left = right = 0; }
  void tick(uint32_t now) {
    if (armed && uint32_t(now - lastCommand) >= COMMAND_TIMEOUT_MS) stop();
  }
  void arm(uint32_t now) { left = right = 0; lastCommand = now; armed = true; }
  bool drive(int l, int r, uint32_t now) {
    tick(now); // An expired session cannot revive through a delayed packet.
    if (!armed || l < -100 || l > 100 || r < -100 || r > 100) return false;
    left = l; right = r; lastCommand = now;
    return true;
  }
};

struct MotorRamp {
  int value = 0;
  int lastSign = 0;
  uint32_t zeroSince = 0;
  void stop(uint32_t now) {
    if (value != 0) zeroSince = now;
    value = 0;
  }
  int step(int target, uint32_t now) {
    int sign = (target > 0) - (target < 0);
    if (!sign) { stop(now); return value; }
    if (lastSign && sign != lastSign) {
      stop(now);
      if (uint32_t(now - zeroSince) < REVERSAL_PAUSE_MS) return 0;
    }
    lastSign = sign;
    int magnitude = value < 0 ? -value : value;
    int desired = target < 0 ? -target : target;
    magnitude = desired < magnitude ? desired :
                (desired < magnitude + RAMP_STEP ? desired : magnitude + RAMP_STEP);
    value = sign * magnitude;
    return value;
  }
};
