# Pushbot

Pushbot is a self-contained four-motor tank-drive controller for an ESP32-S3
development board on a Lonely Binary ESP32-S3 screw terminal base. The ESP32
creates its own Wi-Fi network and serves the driver station directly; no
router, internet connection, Raspberry Pi, phone app, or separate
driver-station program is required.

The browser console intentionally follows the safety-oriented interaction of
MotionModule's driver station, but only contains what this robot needs:

- deliberate enable/disable and a large stop button;
- WASD driving and browser Gamepad API input;
- a 40% initial speed limit;
- a single-driver ownership lock;
- immediate stop on lost focus, hidden page, disconnect, or controller loss;
- a 350 ms firmware watchdog independent of the browser;
- slew-limited commands to reduce abrupt direction changes.

There is no code uploader, autonomous mode, configurable robot project, or
internet-facing service. The complete page lives inside the firmware.

## Supported motor-driver interface

The current pin map expects **four H-bridge channels**, one per motor. Each
channel must have two 3.3 V-compatible logic inputs. Two common dual-channel
red boards can provide four channels. If a board has separate ENA/ENB pins,
leave its enable jumpers installed; Pushbot applies PWM to the direction inputs.

Do not connect a driver until its logic-input voltage, continuous current, and
motor stall-current ratings have been checked. A listing that says "4 A" often
means peak or total current, not 4 A continuously on every channel.

## ESP32-S3 pin map

Terminal numbers are the GPIO labels printed on the screw terminal base.

| Motor | H-bridge input 1 | H-bridge input 2 | Terminal block | Default inversion |
|---|---:|---:|---|---|
| Front left | GPIO 13 | GPIO 14 | Left | No |
| Rear left | GPIO 11 | GPIO 12 | Left | No |
| Front right | GPIO 1 | GPIO 2 | Right | Yes |
| Rear right | GPIO 42 | GPIO 41 | Right | Yes |

For two dual H-bridge boards, use board 1 for the two left motors and board 2
for the two right motors. Each row above consumes one channel.

Avoid GPIO 19 and 20 (native USB), 48 (RGB LED), and the strapping pins 0, 3,
45, and 46 when moving any motor input.

### Power wiring rules

1. Power the motors from the motor battery through a fuse and physical cutoff.
2. Power the ESP32-S3 from USB or a regulated 5 V supply—not a motor output
   and not an unregulated motor battery. The current build feeds the base's
   **5V** and **GND** terminals from one driver's regulated 5 V output. On
   L298N-style boards that output only exists with the 5 V regulator jumper
   fitted and a motor supply of 12 V or less.
3. Every driver's logic GND must share ground with the ESP32-S3. The second
   driver is covered if both drivers run from the same motor battery;
   otherwise run a wire from its GND to a base **GND** terminal.
4. Never apply 5 V to an ESP32-S3 GPIO. It is a 3.3 V device.
5. Add 10 kΩ pull-down resistors from every H-bridge input to ground so the
   motors remain stopped while the ESP32 resets or is disconnected.
6. Keep motor-current wiring short and appropriately sized. Add the driver
   manufacturer's recommended capacitors, heat sinking, and cooling.

## Build and upload

Open [firmware/Pushbot/Pushbot.ino](firmware/Pushbot/Pushbot.ino) in Arduino
IDE and select **ESP32S3 Dev Module**. Match Flash Size and PSRAM to the
module fitted to the base (for an N16R8 module: 16 MB flash, OPI PSRAM).
Install the Espressif `esp32` board package if it is not already installed.
No third-party libraries are needed.

Before uploading, change these constants near the top of `Pushbot.ino`:

```cpp
constexpr char AP_SSID[] = "Pushbot";
constexpr char AP_PASSWORD[] = "pushbot-drive";
```

After upload:

1. Leave motor power off or raise all four wheels.
2. Connect a laptop to Wi-Fi **Pushbot** using password `pushbot-drive`.
3. Open `http://192.168.4.1/`. A captive-portal window may open automatically.
4. Check the safety confirmation, select **Enable**, and test at 40% or less.
5. Press **Space**, **Disable**, or **Stop all outputs** to stop immediately.

## Controls

| Input | Action |
|---|---|
| W / S | Forward / reverse |
| A / D | Turn left / right |
| Space | Disable and stop |
| Gamepad left stick Y | Forward / reverse |
| Gamepad right stick X | Turn left / right |
| Gamepad east/right face button | Disable and stop |

The gamepad connects to the laptop, not the robot. Browsers only expose a
controller after the user interacts with it, so press a button after loading
the page. Some browser versions restrict the Gamepad API on plain HTTP local
pages; WASD remains available in that case. Direct Bluetooth-controller support
can be added later for a known controller model.

## Direction test

With the chassis raised, press W briefly. All wheels should drive the robot
forward. Change only the relevant `INVERT_*` constant when a motor spins the
wrong direction. Do not swap GPIO numbers to correct direction.

The firmware assumes positive left and right commands mean forward. The right
motors are inverted in the supplied map because they are usually mounted as a
mirror image of the left motors.

