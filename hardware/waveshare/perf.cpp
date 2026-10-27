#include "perf.h"
#if defined(OMOTE_SWIPE_DIAGNOSTICS) && defined(OMOTE_BOARD_WAVESHARE_LCD_28)
#include <Arduino.h>

namespace {
struct Counters {
  uint32_t loops=0, handlerMaxUs=0;
  uint64_t handlerTotalUs=0;
  uint32_t flushes=0, flushMaxUs=0;
  uint64_t flushTotalUs=0, flushPixels=0;
  uint32_t touchReads=0, touchBusErrors=0, touchMoves=0;
  uint32_t touchMaxUs=0, touchMaxGapMs=0;
  uint64_t touchTotalUs=0;
  uint32_t scrollEvents=0, pageRebuilds=0, rebuildMaxUs=0;
} stats;
uint32_t windowStartMs=0;
uint32_t avg(uint64_t sum, uint32_t count) {
  return count ? static_cast<uint32_t>(sum/count) : 0;
}
}

namespace ws_perf {
void noteFlush(uint32_t pixels, uint32_t durationUs) {
  ++stats.flushes; stats.flushPixels+=pixels;
  stats.flushTotalUs+=durationUs;
  if(durationUs>stats.flushMaxUs) stats.flushMaxUs=durationUs;
}
void noteTouchRead(uint32_t durationUs, bool busOk, bool moved, uint32_t gapMs) {
  ++stats.touchReads;
  if(!busOk) ++stats.touchBusErrors;
  if(moved) ++stats.touchMoves;
  stats.touchTotalUs+=durationUs;
  if(durationUs>stats.touchMaxUs) stats.touchMaxUs=durationUs;
  if(gapMs>stats.touchMaxGapMs) stats.touchMaxGapMs=gapMs;
}
void noteScroll() { ++stats.scrollEvents; }
void noteRebuild(uint32_t durationUs) {
  ++stats.pageRebuilds;
  if(durationUs>stats.rebuildMaxUs) stats.rebuildMaxUs=durationUs;
}
void noteLvgl(uint32_t durationUs) {
  ++stats.loops;
  stats.handlerTotalUs+=durationUs;
  if(durationUs>stats.handlerMaxUs) stats.handlerMaxUs=durationUs;
  uint32_t now=millis();
  if(!windowStartMs) { windowStartMs=now; return; }
  uint32_t interval=now-windowStartMs;
  if(interval<3000) return;

  Serial.printf("[SWIPE] %lums LVGL calls/s=%lu handler avg/max=%lu/%lu us; scroll events=%lu; rebuild count/max=%lu/%lu us\n",
    (unsigned long)interval, (unsigned long)((uint64_t)stats.loops*1000/interval),
    (unsigned long)avg(stats.handlerTotalUs,stats.loops), (unsigned long)stats.handlerMaxUs,
    (unsigned long)stats.scrollEvents, (unsigned long)stats.pageRebuilds,
    (unsigned long)stats.rebuildMaxUs);
  Serial.printf("[SWIPE] SPI flushes=%lu pixels=%llu total=%llu ms avg/max=%lu/%lu us\n",
    (unsigned long)stats.flushes, (unsigned long long)stats.flushPixels,
    (unsigned long long)(stats.flushTotalUs/1000),
    (unsigned long)avg(stats.flushTotalUs,stats.flushes), (unsigned long)stats.flushMaxUs);
  Serial.printf("[SWIPE] touch reads=%lu bus errors=%lu coordinate moves=%lu I2C avg/max=%lu/%lu us gap max=%lu ms\n",
    (unsigned long)stats.touchReads, (unsigned long)stats.touchBusErrors,
    (unsigned long)stats.touchMoves,
    (unsigned long)avg(stats.touchTotalUs,stats.touchReads),
    (unsigned long)stats.touchMaxUs, (unsigned long)stats.touchMaxGapMs);
  stats=Counters{};
  windowStartMs=now;
}
} // namespace ws_perf
#endif
