# PushBot

My old high-school robot, back off the floor.

I originally built PushBot sometime around my freshman or sophomore year of high school. It broke, ended up sitting on my floor, and stayed there for a while. I decided to revive it instead of leaving it there.

This repo is for my own robot: four small 5 V TT gear motors, a Lonely Binary ESP32-S3 on its screw-terminal breakout board, and two of the red L298N motor drivers. One driver handles the left pair of motors; the other handles the right pair. It's a simple tank-drive robot.

## Driving it

The robot program and the entire driver station live on the ESP32. There's no code editor, cloud service, app install, or internet connection involved in driving it.

1. Power on PushBot and join **PushBot** Wi-Fi. The initial password is `floorbot-revival`.
2. Stay connected if the phone says this network has no internet. Open **http://192.168.4.1/** in the browser.
3. Tap **Enable drive**, then hold and slide each track control. Both forward goes forward; opposite directions turns in place. Let go to stop that track.
4. **STOP & DISABLE** stops both tracks. The next drive needs another deliberate Enable.

The speed limit starts at 35%. Controls also accept up/down arrows while focused; space stops drive. Only one driver station can hold control. Closing it, losing Wi-Fi, hiding the page, or switching away disables drive. Reconnecting never automatically enables the robot.

The firmware cuts motor output after **350 ms without a valid drive command**, checked every 10 ms by a separate task even if networking stalls. Acceleration ramps up, direction changes pause at zero, and malformed commands disable drive. These are software protections, not a substitute for a reachable battery switch. The on-screen output numbers are commanded PWM counts, not measured wheel speed; there are no battery or motor sensors.

## My hardware notes

Only the right-side terminals are accessible in the chassis. This is the fixed mapping in `config.h`:

| ESP32 GPIO | L298N connection |
| --- | --- |
| 1 | Left IN1 and IN3 |
| 2 | Left IN2 and IN4 |
| 42 | Left ENA and ENB |
| 41 | Right IN1 and IN3 |
| 40 | Right IN2 and IN4 |
| 47 | Right ENA and ENB |

Each motor gets its own bridge: front motor on OUT1/OUT2, rear motor on OUT3/OUT4, for each side. Pairing the logic inputs makes those two bridges follow the same command. **Remove all ENA/ENB jumpers** before connecting the ESP32; add a 10 kΩ pull-down from each enable input to ground so the motors stay off during boot/reset. Match each motor's lead polarity so both wheels on a side drive in the same direction. `INVERT_LEFT` and `INVERT_RIGHT` reverse an entire side if needed.

The supplied pin list points to an **ESP32-S3**, not an original ESP32. Confirm that marking before flashing. GPIO19/20 are left available for native USB; GPIO48 and the uncertain “4” terminal are unused. GPIO39 is also unused. JTAG pins 40–42 are used as ordinary outputs; don't attach external JTAG to them while driving.

### Power correction before running

My original idea was a 12 V battery straight into both L298Ns and the first driver's 5 V output powering the ESP32. **That isn't the power arrangement this code assumes.** The motors are 5 V: the L298N's voltage drop and a software speed limit do not turn a 12 V supply into a regulated motor supply.

Use the 12 V battery through a fuse and switch to a **5 V motor buck converter**, then feed that converter into both drivers' motor-supply terminals (often labeled `12V`). Size the converter, wiring and fuse for the motors' measured startup/stall current. At 5 V input the L298N drops appreciable voltage, so the robot may be slower; don't compensate by blindly raising the supply. A low-loss driver is a future improvement if torque is poor.

Use a **separate regulated 5 V buck supply** for the ESP32's 5V terminal and both drivers' 5V logic terminals. Remove both drivers' **5V-EN regulator jumpers** for this arrangement; those differ from ENA/ENB. Verify the actual module's labeling first. All grounds must join: battery negative, converter negatives, both drivers and ESP32. Keep motor return currents out of the ESP32 wiring. Don't join regulator outputs, and disconnect external ESP32 5 V power before connecting USB unless the board's power isolation has been verified.

First run: wheels off the floor, speed low, check each side's direction, release the controls, press Stop, then disconnect the phone and verify the motors stop. Check driver temperature and motor current under load. Hardware operation still needs this bench check.

## Flashing my robot

Open this folder in VS Code with PlatformIO, then use **Build** and **Upload**. Or, with PlatformIO installed:

```sh
pio run
pio run --target upload
pio device monitor --baud 115200
```

The project pins the ESP32 platform and WebSockets library. It uses Arduino-ESP32 2.0.17 and a conservative ESP32-S3 DevKit configuration, using no PSRAM and fitting within 8 MB flash, including on a larger N16R8 board. The driver-station HTML is embedded automatically during build; no filesystem upload is needed. If upload doesn't connect, use the board's BOOT/RESET sequence and correct USB port/cable.

To choose a personal Wi-Fi password, create ignored `secrets.h` with `#define PUSHBOT_WIFI_PASSWORD "your-password"` (8–63 characters) and rebuild. Pin changes and side reversal are in `config.h`; control behavior is in `main.cpp` and `control.h`; the station is `index.html`.

## Checks

`node driver_station.test.cjs` checks browser control behavior without hardware. `control_test.cpp` exercises the firmware's timeout, rollover, ramp and command parser with a host C++ compiler. GitHub Actions runs both checks and compiles the firmware. Physical motor behavior and the exact breakout-board revision still need checking on my robot.

Hardware references: [Lonely Binary S3 pinout](https://learn.lonelybinary.com/pinouts/esp32-s3), [ST L298 datasheet](https://www.st.com/resource/en/datasheet/cd00000240.pdf), [typical red L298N module guide](https://www.handsontec.com/dataspecs/module/L298N%20Motor%20Driver.pdf). Module jumpers can vary.
