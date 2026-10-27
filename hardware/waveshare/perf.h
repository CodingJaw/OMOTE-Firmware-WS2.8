#pragma once
#include <stdint.h>

// Lightweight metrics for the Waveshare swipe diagnostic builds only.
// All values are aggregated and reported outside the touch/display callbacks.
#if defined(OMOTE_SWIPE_DIAGNOSTICS) && defined(OMOTE_BOARD_WAVESHARE_LCD_28)
namespace ws_perf {
void noteFlush(uint32_t pixels, uint32_t durationUs);
void noteTouchRead(uint32_t durationUs, bool busOk, bool moved, uint32_t gapMs);
void noteScroll();
void noteRebuild(uint32_t durationUs);
void noteLvgl(uint32_t durationUs);
}
#else
namespace ws_perf {
inline void noteFlush(uint32_t, uint32_t) {}
inline void noteTouchRead(uint32_t, bool, bool, uint32_t) {}
inline void noteScroll() {}
inline void noteRebuild(uint32_t) {}
inline void noteLvgl(uint32_t) {}
}
#endif
