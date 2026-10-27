#pragma once
#include <stdint.h>
namespace waveshare {
constexpr int lcdSck=40, lcdMosi=45, lcdCs=42, lcdDc=41, lcdReset=39, backlight=5;
constexpr int touchSda=1, touchScl=3, touchReset=2, touchInterrupt=4;
constexpr int sda=11, scl=10, powerKey=6, powerLatch=7, batteryAdc=8, bootKey=0;
constexpr int sdClock=14, sdCommand=17, sdData=16, sdD3=21;
constexpr uint8_t touchAddress=0x1a;
class DebouncedButton {
  bool raw=false, stable=false;
  uint32_t changedAt=0;
public:
  void reset(bool down,uint32_t now) { raw=stable=down; changedAt=now; }
  bool update(bool down,uint32_t now) {
    if(down!=raw) { raw=down; changedAt=now; }
    if(uint32_t(now-changedAt)>=25) stable=raw;
    return stable;
  }
};
// First point in a CST328 D000 frame; the count is read separately at D005.
inline bool decodeTouch(const uint8_t *frame, uint8_t count, uint16_t &x, uint16_t &y) {
  if (count==0 || count>5) return false;
  x=(uint16_t(frame[1])<<4)|(frame[3]>>4);
  y=(uint16_t(frame[2])<<4)|(frame[3]&15);
  return x<240 && y<320;
}
inline int batteryMillivolts(int adcMv) { return int(adcMv*3.0/0.990476); }
inline int batteryPercent(int mv) { return mv<=3300?0:mv>=4200?100:(mv-3300)*100/900; }
}
