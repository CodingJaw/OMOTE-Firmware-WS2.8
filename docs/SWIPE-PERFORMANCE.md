# Waveshare swipe performance diagnostics

This branch is based on `waveshare-arduino3`. It does not change the stable branch or touch/scroll behavior. It adds counters that print once every three seconds, without per-frame serial printing.

## First: normal graphics buffer (PSRAM-first)
```sh
git fetch origin
git switch --track origin/test/swipe-performance-diagnostics
pio run -e waveshare-swipe-test -t upload --upload-port /dev/ttyACM0
pio device monitor -p /dev/ttyACM0 -b 115200
```

If the branch already exists locally, use `git switch test/swipe-performance-diagnostics` followed by `git pull --ff-only`. Review or stash uncommitted changes before switching. If the serial port changes after reset, run `pio device list`.

## Second: internal RAM buffer
```sh
pio run -e waveshare-swipe-internal -t upload --upload-port /dev/ttyACM0
pio device monitor -p /dev/ttyACM0 -b 115200
```

Both test targets inherit the normal Waveshare hardware and framework settings. The only buffer difference is allocation preference; each still uses one synchronous 240x32 RGB565 buffer.

## Tests
For each firmware, wait for the boot message. Run the following cases for roughly 10 seconds each, recording several complete `[SWIPE]` triplets:
1. Leave GUI idle with no touch.
2. Continuously drag a page horizontally (keep finger on screen).
3. Swipe and release, allow transition, repeat.
4. Drag a slider as the smooth-control baseline.

## Log interpretation
* `LVGL calls/s`: calls to lv_timer_handler per second (not frames per second).
* `handler avg/max`: time inside lv_timer_handler, which includes blocking screen transfers and touch reads.
* `SPI flushes/pixels/total/avg/max`: full synchronous transfers and their duration.
* `touch reads/errors/moves/I2C avg/max/gap max`: CST3530 20 ms polling cadence and I2C transfer behavior. A decoded contact record is not necessarily a pressed touch; touch reads count attempts.
* `scroll events`: GUI tab-scroll event frequency.
* `rebuild count/max`: CPU time to reconstruct OMOTE tabs after a completed swipe.
  
Potential patterns:
- High SPI total time, handler time, and low scroll event frequency during drag -> display throughput bottleneck.
- High touch read times or gaps but low SPI usage -> touch polling/loop scheduling bottleneck.
- Smooth drag but large rebuild maximum -> dynamic tab reconstruction.
- Big improvement with internal-RAM buffer -> PSRAM draw-buffer access/copy overhead.

There are no functionality changes intended by these tests. Do not enable per-scroll Serial logging while measuring, as that itself can cause lag. This diagnostic branch is meant for physical testing; the firmware has not yet been verified on the connected hardware.
