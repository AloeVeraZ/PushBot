# Pushbot

Pushbot is a self-contained four-motor tank-drive controller for an ESP32-S3
development board on a Lonely Binary ESP32-S3 screw terminal base. The ESP32
creates its own Wi-Fi network and serves the driver station directly; no
router, internet connection, Raspberry Pi, phone app, or separate
driver-station program is required.

The browser console intentionally follows the safety-oriented interaction of
MotionModule's driver station, but only contains what this robot needs:

- deliberate enable/disable and a large stop button;
- two on-screen touch joysticks for phones (adapted from MotionModule's
  `createTouchStick`), WASD driving, and browser Gamepad API input;
- a 40% initial speed limit;
- a single-driver ownership lock;
- immediate stop on lost focus, hidden page, disconnect, or controller loss;
- a 500 ms firmware watchdog independent of the browser;
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
   and not an unregulated motor battery. The base's **5V** terminal is the dev
   board's 5V pin, which feeds an AMS1117-3.3 linear regulator directly, with
   no diode and no reverse-polarity protection. Lonely Binary documents it as
   the intended way to power the board without USB, at 5 V ± 0.5 V. Wi-Fi
   bursts draw about 340 mA, and the pin must stay above roughly 4.1 V during
   them or the chip browns out. An L298N's on-board 78M05 output (jumper
   fitted) only meets this with at least about 7 V of motor supply. Below
   that, it sags and the board resets on battery. A dedicated 5 V buck
   converter rated 1 A or more, or a USB power bank in either USB-C socket,
   is more reliable.
3. Every driver's logic GND must share ground with the ESP32-S3. The second
   driver is covered if both drivers run from the same motor battery;
   otherwise run a wire from its GND to a base **GND** terminal.
4. Never apply 5 V to an ESP32-S3 GPIO. It is a 3.3 V device.
5. Add 10 kΩ pull-down resistors from every H-bridge input to ground so the
   motors remain stopped while the ESP32 resets or is disconnected.
6. Keep motor-current wiring short and appropriately sized. Add the driver
   manufacturer's recommended capacitors, heat sinking, and cooling.

## Build and upload

The target is the Lonely Binary **ESP32-S3 N16R8** (16 MB flash, 8 MB OPI
PSRAM) on its **Screw Terminal Base**. The base passes the dev board's GPIOs
through; it does not contain a separate controller. The on-board WS2812B RGB
LED is on **GPIO 48**, marked `RGB@IO48`, separate from all eight motor inputs.
Manufacturer references: [board identification](https://learn.lonelybinary.com/boards/esp32-s3/what-you-are-holding)
and [screw terminal pinout](https://learn.lonelybinary.com/pinouts/s3screw).

### Manual upload with Arduino IDE

1. Install the Espressif **esp32** board package (built and tested with 3.3.11).
   No third-party libraries are needed.
2. Open the sketch folder `testing/firmware/Pushbot` (the file
   [Pushbot.ino](firmware/Pushbot/Pushbot.ino); `driver_station.h` must stay
   beside it).
3. In **Tools**, select:

   | Setting | Value |
   |---|---|
   | Board | ESP32S3 Dev Module |
   | USB CDC On Boot | Enabled |
   | USB Mode | Hardware CDC and JTAG |
   | Flash Size | 16MB (128Mb) |
   | Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
   | PSRAM | OPI PSRAM |
   | Upload Mode | UART0 / Hardware CDC |
   | Port | The board's native USB port (previously COM11) |

   Leave the other menus at their defaults.
4. Turn motor power off (or raise all four wheels), then select **Upload**.
5. If the upload ends with the ROM `waiting for download`, release BOOT and
   press RESET once, or power-cycle the board.

The equivalent Arduino CLI build, run from the repository root, compiles
without uploading:

```powershell
arduino-cli compile --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB,CDCOnBoot=cdc" --build-path "$env:TEMP/pushbot-testing-build" testing/firmware/Pushbot
```

### Startup without USB

The firmware does not depend on a USB host. Startup prints nothing and never
waits for a serial monitor; the LED driver (RMT) transmits asynchronously and
never blocks; Wi-Fi is started before USB serial is initialized, and USB
writes use a zero transmit timeout. If the access point fails to start, the
LED double-flashes red and Wi-Fi is retried every three seconds.

The USB serial console (115200 baud) is optional. Send `?` to print Wi-Fi
readiness, armed state, tank outputs, the last reset reason and memory sizes.
This diagnostic cannot arm or drive. The driver station's **System** panel also
shows the last reset reason.

Brownout protection is left enabled. If the previous reset was a brownout, the
startup colour is **magenta** instead of amber, so a supply that sags when
Wi-Fi starts can be recognized without a computer attached.

Battery-only startup showed magenta (brownout) while USB power reached blue.
To lower Wi-Fi current surges, transmit power is limited to 8.5 dBm (19.5 dBm
is the default). After a brownout reset the firmware waits one second, then
starts Wi-Fi at 2 dBm. These settings only reduce the load. If magenta
persists, the 5 V supply must be improved:

- Feed the base's **5V/GND** from a dedicated 5 V buck converter rated at least
  2 A, wired straight from the battery, instead of an L298N's on-board 5 V
  regulator. That regulator needs about 7 V or more in and supplies little
  current.
- Add a 470–1000 µF electrolytic capacitor across the base's 5V and GND
  terminals.
- Use short, thick 5V/GND wires, and check the battery is charged.

### RGB status light

| Robot state | On-board LED |
|---|---|
| Starting Wi-Fi | Amber |
| Starting Wi-Fi after a brownout reset | Magenta |
| Joining the saved home network | Slow amber blink |
| Disabled / waiting on the Pushbot hotspot | Slowly breathing blue |
| Disabled / waiting on the home network | Slowly breathing green |
| Enabled, outputs idle | White |
| Forward output | Pulsing green |
| Reverse output | Pulsing purple |
| Left turn output | Pulsing cyan |
| Right turn output | Pulsing amber |
| Driver stop / disable | Red flashes for 1.2 seconds, then blue |
| Watchdog, malformed command, or Wi-Fi startup failure | Repeating double red flashes |

Watchdog and malformed-command indicators clear on a deliberate Enable (or
reset); a Wi-Fi startup failure stays red until restart succeeds. Turn colors
take priority over forward/reverse while the tank outputs differ. These colors
reflect firmware output commands, not measured wheel movement. Animation uses
`millis()` with no flash-cycle delays and runs after each motor update.

Before uploading, change these constants near the top of `Pushbot.ino`:

```cpp
constexpr char AP_SSID[] = "Pushbot";
constexpr char AP_PASSWORD[] = "pushbot-drive";
```

After upload:

1. Leave motor power off or raise all four wheels.
2. Connect a phone or laptop to Wi-Fi **Pushbot** using password
   `pushbot-drive`.
3. Open `http://192.168.4.1/`. A captive-portal window may open automatically.
4. Check the safety confirmation, select **Enable**, and test at 40% or less.
5. Press **Stop all outputs**, **Disable**, or **Space** to stop immediately.

## Home Wi-Fi

The driver station's **Wi-Fi** tab lets Pushbot join one home network, so the
phone can stay on that network instead of the Pushbot hotspot. Phones often
drop a hotspot that has no internet.

1. Connect to the Pushbot hotspot and open `http://192.168.4.1/`.
2. Open the **Wi-Fi** tab. This disables the robot. Select **Scan for
   networks**, tap your network (2.4 GHz only), enter its password, and select
   **Save & connect**.
3. When it connects, the tab shows its address. The hotspot stays on for one
   more minute. Join your home network and open `http://pushbot.local/` or
   that address. If `pushbot.local` doesn't open (some Android phones), use
   the numeric address. A router can be set to reserve that address.

The network is saved in flash. At power-on Pushbot tries it for 15 seconds.
If it is out of range or the password is rejected, Pushbot starts its own
hotspot and shows why in the Wi-Fi tab. It tries the home network again every
minute, but only while no one is connected to the hotspot and the robot is
disabled. If the home network drops for 10 seconds, the robot stops and the
hotspot returns. **Forget** clears the saved network.

Send `?` over USB to print the network mode, address, request counters and
the last home Wi-Fi result.

## Controls

| Input | Action |
|---|---|
| Left touch stick (up/down) | Forward / reverse |
| Right touch stick (left/right) | Turn left / right |
| W / S | Forward / reverse |
| A / D | Turn left / right |
| Space | Disable and stop |
| Gamepad left stick Y | Forward / reverse |
| Gamepad right stick X | Turn left / right |
| Gamepad east/right face button | Disable and stop |

### Phone touch sticks

The two sticks are tank-drive controls: the left stick only moves up and down
(drive), and the right stick only moves left and right (turn). Using both makes
a curve; turning alone rotates in place. There is no strafing.

- Touch down anywhere on a pad, then drag. Touching down alone never moves the
  robot, and each stick follows exactly one finger.
- Releasing a stick returns it to centre and its command to zero.
- Touches made while disabled do nothing, even after Enable, until the finger
  lifts and touches again.
- A cancelled touch, lost pointer capture, rotation/resize, hidden page, lost
  focus or lost connection disables the robot. After a disconnect, select
  **Enable** again.
- Landscape puts one stick at each edge with Enable, Disable and Stop between
  them. Portrait places Stop directly above both sticks.
- With a pad focused, the arrow keys also drive it.

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

## Verification status

Verified without hardware:

- `node testing/driver_station.test.cjs` passes (arming, two-thumb curves and
  pivots, release, multi-touch ownership, cancellation, stale replies,
  timeouts, reconnect, steering signs, resize stop).
- The page was exercised in a browser against mocked robot endpoints in phone
  portrait (390×664), phone landscape (844×390) and desktop layouts.
  Simultaneous touches were simulated with separate pointer IDs.
- The sketch compiles with esp32 core 3.3.11 and the settings above.

Still to verify on the robot:

1. **Battery-only startup.** Power only from the external 5V/GND supply, with
   USB unplugged, and check the LED reaches breathing blue and the **Pushbot**
   network appears. Steady amber means startup never finished. Magenta means
   the board reset on a brownout: measure the 5V rail while Wi-Fi starts
   before changing anything else. This has not been tested, so the cause of
   the earlier battery-only failure is not confirmed.
2. Repeat with motor power on and the wheels raised.
3. Real multi-touch on the phone that will drive: two thumbs at once,
   rotation, and switching apps while enabled.
4. The raised-wheel direction test, then floor driving at 40% or less.

