#include "hardware/waveshare/board.h"
#include <cassert>
#include <cstdio>
int main() {
  waveshare::DebouncedButton key;
  key.reset(false,0);
  assert(!key.update(true,10));
  assert(!key.update(false,15));
  assert(!key.update(true,20));
  assert(!key.update(true,44));
  assert(key.update(true,45));
  assert(key.update(false,50));
  assert(!key.update(false,75));
  // Holding PWR through boot is initialized as held, not a false release.
  key.reset(true,100); assert(key.update(true,101));
  // Unsigned time arithmetic must also debounce correctly across millis wrap.
  key.reset(false,0xfffffff0u);
  assert(!key.update(true,0xfffffff5u));
  assert(key.update(true,20));
  uint16_t x=0,y=0;
  // Manufacturer's packed first-point format: high X/Y bytes, shared low nibbles.
  uint8_t topLeft[5]={0x06,0,0,0,80};
  uint8_t bottomRight[5]={0x06,0x0e,0x13,0xff,80};
  uint8_t outside[5]={0x06,0x0f,0x13,0x0f,80};
  assert(waveshare::decodeTouch(topLeft,1,x,y) && x==0 && y==0);
  assert(waveshare::decodeTouch(bottomRight,1,x,y) && x==239 && y==319);
  assert(waveshare::decodeTouch(bottomRight,5,x,y));
  assert(!waveshare::decodeTouch(outside,1,x,y));
  assert(!waveshare::decodeTouch(nullptr,0,x,y)); // Release packets have no point data.
  assert(!waveshare::decodeTouch(nullptr,6,x,y)); // Corrupt count must never be decoded.
  assert(waveshare::batteryMillivolts(1320)>=3997 && waveshare::batteryMillivolts(1320)<=4000);
  assert(waveshare::batteryPercent(0)==0 && waveshare::batteryPercent(3300)==0);
  assert(waveshare::batteryPercent(3750)==50);
  assert(waveshare::batteryPercent(4200)==100 && waveshare::batteryPercent(5000)==100);
  static_assert(waveshare::sda!=19 && waveshare::sda!=20 && waveshare::scl!=19 && waveshare::scl!=20,"USB pins must remain free");
  puts("Waveshare debounce, touch packet and battery conversion checks passed");
}
