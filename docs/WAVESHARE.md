# Waveshare ESP32-S3-LCD-2.8

Build and upload **this target**, not an OMOTE revision target:

```sh
pio run -e waveshare-esp32-s3-lcd-28
pio run -e waveshare-esp32-s3-lcd-28 -t upload --upload-port YOUR_PORT
pio device monitor -b 115200 --port YOUR_PORT
```

On the cloud machine, first `source /workspace/omote-setup/activate.sh`. Flashing needs a machine with USB access to the board. The first serial output should identify the ST7789, report CST328 presence, and identify QMI8658 or report it missing. If upload fails, hold BOOT while tapping RESET, then release BOOT and retry. Native USB CDC is enabled; USB GPIO19/20 are never reassigned.

The target retains Arduino 3, 16 MB flash, 8 MB OPI PSRAM and the existing 16 MB no-OTA partition table. Its source filter selects a separate hardware layer and shared Wi-Fi/MQTT, BLE, preferences and LVGL code. It excludes every OMOTE display, keypad, battery, sleep, LED, SD and infrared GPIO implementation. Existing OMOTE environments retain their wiring.

| Hardware | Pins / implementation |
| --- | --- |
| ST7789, 240 × 320 portrait | SPI SCK40, MOSI45, CS42, DC41, reset39; 40 MHz, inversion enabled |
| LCD backlight | GPIO5, active high, 20 kHz PWM |
| CST328 touch | Separate Wire1: SDA1, SCL3, reset2, interrupt4; address 0x1A |
| QMI8658 and RTC bus | Wire: SDA11, SCL10; QMI address detected at 0x6B/0x6A |
| QMI8658 | Acceleration at 4 g / 30 Hz; gyro not needed for motion wake |
| Battery | ADC GPIO8, manufacturer's divider/calibration; percentage is a voltage-based estimate |
| Battery power | Latch GPIO7 asserted at startup; active-low power key GPIO6 |
| Optional SD, one-bit SD_MMC | CLK14, CMD17, D0=16, D3=21 high; no formatting on mount failure |

## Controls and behavior

- Touch operates the normal OMOTE GUI. A single touch point is decoded and bounds-checked; absent/failed touch does not block startup. The non-touch example package uses simulated input, while the touch package confirms CST328 on the separate bus.
- BOOT advances focus; a short PWR press activates the focused control. This also permits basic navigation on non-touch units. Holding PWR for three seconds saves preferences and drops the battery latch. Holding it during startup does not immediately turn the unit off: release it first. With USB connected, releasing the battery latch cannot remove USB power; the screen stays off until reset.
- Inactivity dims the screen, then turns off its backlight. Touch, buttons, and enabled QMI8658 motion sensing restore activity. Standby intentionally keeps the MCU and BLE active: this is **not deep sleep**, and battery runtime will be shorter than a verified low-power port. No untested motion-interrupt wake programming is used.
- The stock board has no OMOTE keypad, keypad lighting, IR emitter/receiver, or status LED. GPIO45 is the display's MOSI signal, so the OMOTE heartbeat LED is never initialized. The keypad-brightness and IR receiver controls are hidden. IR commands report that no transmitter is configured; IR-controlled TVs/amplifiers need an external hardware/driver addition. BLE and MQTT commands remain available.
- Charging status cannot be measured from the exposed interface and is not fabricated. The RTC, speaker/audio and SD are not used by OMOTE's current workflow; SD initialization is optional and not called at startup.
- Configure Wi-Fi/MQTT using the ignored `src/secrets_override.h` mechanism. Never commit credentials. Unconfigured network services do not prevent GUI startup.

## Checks

Portable touch-packet/battery conversion regression checks:

```sh
g++ -std=c++17 -Wall -Wextra -Werror -I . tests/waveshare/board_test.cpp -o /tmp/omote-waveshare-test
/tmp/omote-waveshare-test
```

Board-specific GUI preview on Linux (requires SDL2 and an X server):

```sh
pio run -e linux_waveshare_ui
SDL_VIDEODRIVER=x11 .pio/build/linux_waveshare_ui/program
```

This preview exercises board-specific UI visibility, not the ESP32 drivers. Firmware compilation and host checks cannot prove the physical display, touch, polarity, battery power latch, motion behavior, BLE or MQTT work. Before regular use: flash this target, verify the startup diagnostics; touch all four corners and hold/drag/release; check colors/orientation; adjust brightness; test blanking and touch/motion/button wake; pair/send/reconnect BLE; test configured MQTT; compare battery voltage to a meter; finally test battery-only long-press shutdown. Do not use an OMOTE board-diagnostic image on this board.

## References

Pin assignments and protocols were checked against Waveshare's [board documentation](https://docs.waveshare.com/ESP32-S3-LCD-2.8), [schematic](https://files.waveshare.com/wiki/ESP32-S3-LCD-2.8/ESP32-S3-Touch-LCD-2.8.pdf), [LCD example](https://files.waveshare.com/wiki/ESP32-S3-LCD-2.8/ESP32-S3-LCD-2.8-Demo.zip), and [CST328 touch example](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-2.8/ESP32-S3-Touch-LCD-2.8-Demo.zip). The touch protocol and ADC conversion are reimplemented here with bounded I2C reads; manufacturer source files are not compiled into this project. Later V2/B/C variants are separate boards and are not this target.

## Validation completed in cloud

- Waveshare firmware built successfully after the final power-button adjustment (ESP32-S3; about 2.44 MB application).
- Bootloader image inspection reported 16 MB flash and valid checksum/hash.
- Source-list inspection confirmed that the Waveshare build includes only the intended shared ESP32 code and new hardware layer, with no OMOTE GPIO drivers.
- OMOTE ESP32 revision 1–4 and revision 5 firmware regression builds passed, as did the normal Linux build.
- The board-specific Linux GUI rendered and selected TV → Numpad, then exited normally.
- Touch-packet, battery conversion, debounce, held-at-boot and timer-rollover checks passed, including AddressSanitizer/UndefinedBehaviorSanitizer execution.
- Physical-board flashing and peripheral validation have **not** been performed. Arduino/NimBLE SDK macro warnings remain as documented in [the migration notes](ARDUINO3.md); BLE pairing, persistent bonds and MQTT require real service/hardware checks.

## Black-screen diagnosis

Update to the latest `waveshare-arduino3` branch and first flash the standalone display test:

```sh
pio run -e waveshare-display-test -t upload --upload-port YOUR_PORT
pio device monitor -b 115200 --port YOUR_PORT
```

Press RESET after opening the monitor (the USB port may re-enumerate). The test drives backlight GPIO5 HIGH without PWM, asserts the battery power latch, and initializes the ST7789 using the manufacturer's register values at a conservative 20 MHz SPI clock. It does not initialize OMOTE, BLE, Wi-Fi, LVGL or touch. Expected output is four horizontal bands: red, green, blue, white. Serial output repeats `DISPLAY TEST running` once per second.

- **No serial heartbeat:** capture the boot/reset output and check the selected target and exact board revision. The application may not be reaching setup.
- **Heartbeat, no backlight glow:** investigate board revision/pin mapping or backlight power. This test bypasses PWM.
- **Backlight glow, no bands:** investigate the LCD reset/SPI wiring, controller or revision. This is independent of LVGL and BLE.
- **Bands appear:** LCD/backlight work with the raw vendor initialization. Then reflash `waveshare-esp32-s3-lcd-28`, capture serial output from RESET, and report the last completed startup stage. This separates the library/GUI/PWM/network initialization paths.

The power-latch startup code now preloads GPIO7 through ESP-IDF before enabling its output; Arduino 3 ignores `digitalWrite` before `pinMode`, which the original port incorrectly relied on. The normal firmware now enables a 50% backlight immediately after display-driver initialization, before touch, BLE and network startup finish. It reports the display pin mapping, initialization errors, draw-buffer allocation and entry/exit of BLE initialization. A successful SPI-driver initialization message is not panel readback: this connection is write-only. Diagnostic builds are compile-checked in cloud; the black-screen cause remains unconfirmed until physical results/serial logs are available.
