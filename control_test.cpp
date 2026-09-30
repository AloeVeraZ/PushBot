#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <initializer_list>
#include "control.h"
#include "protocol.h"

bool parses(const char *text) {
  char command; uint32_t session, sequence; int l, r;
  return parseCommand(reinterpret_cast<const uint8_t *>(text), strlen(text), command, session, sequence, l, r);
}

int main() {
  Control c;
  assert(!c.drive(100, 100, 0));
  c.arm(100); assert(c.drive(50, -50, 110));
  c.tick(459); assert(c.armed);
  c.tick(460); assert(!c.armed && c.left == 0 && c.right == 0);
  assert(!c.drive(100, 100, 470)); // No implicit re-arm after a timeout.
  c.arm(1000); assert(!c.drive(10, 10, 1350)); // Packet at expiry loses.
  c.arm(UINT32_MAX - 100); c.tick(248); assert(c.armed);
  c.tick(249); assert(!c.armed); // Unsigned millis rollover.
  c.arm(0); assert(!c.drive(101, 0, 10));
  assert(!c.drive(0, -101, 10));
  c.stop(); assert(!c.armed && !c.left && !c.right);

  MotorRamp motor;
  assert(motor.step(180, 0) == RAMP_STEP);
  for (uint32_t t = 10; t <= 500; t += 10) motor.step(180, t);
  assert(motor.value == 180);
  assert(motor.step(30, 510) == 30); // Reduction never ramps upward.
  assert(motor.step(-180, 520) == 0);
  assert(motor.step(-180, 599) == 0);
  assert(motor.step(-180, 600) == -RAMP_STEP);
  assert(motor.step(0, 610) == 0); // Stop is immediate.
  assert(motor.step(180, 689) == 0);
  assert(motor.step(180, 690) == RAMP_STEP);
  assert(motor.step(-180, UINT32_MAX - 10) == 0);
  assert(motor.step(-180, 68) == 0);
  assert(motor.step(-180, 69) == -RAMP_STEP);

  assert(parses("A,1,1,0,0"));
  assert(parses("D,4294967295,4294967295,-100,100"));
  assert(parses("S,100,20,0,0"));
  for (auto text : {"", "A", "D,1,1,101,0", "D,1,1,0,-101", "D,0,1,0,0",
       "D,-1,1,0,0", "D,1,0,0,0", "D,1,-1,0,0", "D,1,1,0,0junk",
       "D,4294967296,1,0,0", "D,1,999999999999999999,0,0", "D,1,1,0,0,0",
       "D,1,1,,0", "D,1,1,1.5,0", "X,1,1,0,0"}) assert(!parses(text));
  const uint8_t embedded[] = {'D',',','1',',','1',',','0',',','0',0,'x'};
  char cmd; uint32_t id, seq; int l, r;
  assert(!parseCommand(embedded, sizeof(embedded), cmd, id, seq, l, r));
  puts("PASS: watchdog, rollover, ramp, reversal, stop and strict protocol parsing");
}
