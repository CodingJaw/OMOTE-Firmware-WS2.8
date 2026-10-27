# Arduino 3 migration

The ESP32 environments use PlatformIO `espressif32@8.0.0` (Arduino 3.3.12 / ESP-IDF 5). Arduino_GFX is pinned to 1.6.9 for the updated ESP32 SPI APIs. NimBLE-Arduino is pinned to 2.5.2: older stacks can compile but crash on controller initialization with Arduino 3.3.7+; the HID wrapper rejects versions older than the upstream initialization fix in 2.3.8. LVGL remains on version 8; this migration does not change the GUI API.

Build the same targets as Ubuntu CI from the repository root:

```sh
pio run -j 4 -e esp32-Rev1toRev4 -e esp32-s3-Rev5andHigher \
  -e esp32_testboard-Rev1toRev4 -e esp32-s3_testboard-Rev5andHigher \
  -e linux_64bit
```

On the configured cloud machine, first run `source /workspace/omote-setup/activate.sh`.
Do not use the old Arduino 2 compatibility override (`platformio-cloud.ini`).

## PWM behavior

Arduino 3's `ledcWrite` accepts a GPIO pin rather than a channel number. The LCD uses direct ESP-IDF configuration to preserve active-low output without a power-on flash; Arduino's internal pin registration does not own that channel. Display and keypad duty updates therefore use ESP-IDF throughout. Display channel 5 retains timer 1, its board-specific speed mode, 640 Hz and output inversion. Keypad channel 6 uses low-speed timer 3 at 5 kHz. Both use 8-bit duty values. Timer/channel structures are zero-initialized, and timers are configured before channels.

## Verification limits

Firmware compilation checks API compatibility; it cannot verify electrical outputs. The Linux simulator can validate GUI rendering and input but does not run ESP32 peripheral drivers.

Before deploying to a remote used daily, check on the matching OMOTE hardware:

- Cold boot: no unexpected backlight flash; display and keypad fade in correctly.
- Change display/keypad brightness through minimum, intermediate and maximum settings. Check dimming, sleep, wake and repeated brightness changes.
- Pair a BLE keyboard client, send keys, reconnect, and verify bonds survive a reboot.
- Send/receive a known IR command, and exercise MQTT against a configured test broker.
- Verify battery readings and motion/key wake; run the matching board-diagnostic firmware.

The original environments describe OMOTE revision 1–5 wiring. The subsequent Waveshare hardware port uses the dedicated `waveshare-esp32-s3-lcd-28` environment; see [Waveshare setup and hardware checks](WAVESHARE.md). Do not use an OMOTE revision image on the Waveshare board.

## Cloud verification

The default firmware and board-diagnostic build matrix is checked with Arduino 3. The Linux GUI smoke check starts Xvfb with `SDL_VIDEODRIVER=x11`, verifies that both windows render, clicks TV, checks that the Numpad view appears, and closes the application normally. Live MQTT is not covered: the default broker is a placeholder. The optional Arduino_GFX driver 1/2 source files also receive ESP32-S3 compiler syntax checks; they are not hardware-tested or full alternative-driver firmware builds.

The framework and NimBLE currently emit macro-redefinition warnings, including the SDK's CCCD limit overriding the command-line value. Do not treat compilation as validation of multi-client BLE subscriptions or persistent bonds; test those explicitly on hardware before release.

Migration validation completed on 2026-10-27 UTC:

| Check | Outcome |
| --- | --- |
| esp32-Rev1toRev4 | Build passed |
| esp32-s3-Rev5andHigher | Build passed |
| esp32_testboard-Rev1toRev4 | Build passed |
| esp32-s3_testboard-Rev5andHigher | Build passed |
| linux_64bit | Build passed |
| GUI startup, both windows, TV → Numpad, normal close | Passed under Xvfb |
| Arduino_GFX drivers 1 and 2 | ESP32-S3 syntax checks passed |
| Physical peripherals / live MQTT | Not run; hardware/services required |
