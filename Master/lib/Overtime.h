#ifndef Overtime_h
#define Overtime_h

#include <stdint.h>

/**
 * Pure overtime predicates shared by Rule, Mode, and host unit tests.
 * Limits match Glob.h (15 min well, 30 min main).
 */
namespace overtime {

constexpr uint32_t WELL_LIMIT_MS = 900000UL;   // 15 minutes
constexpr uint32_t MAIN_LIMIT_MS = 1800000UL;  // 30 minutes

/** Well pump: trip when continuous runtime strictly exceeds the limit. */
inline bool wellExceeded(uint32_t runMs, uint32_t limit = WELL_LIMIT_MS) {
  return runMs > limit;
}

/** Main pump: trip when elapsed time strictly exceeds the limit. */
inline bool mainExceeded(uint32_t elapsedMs, uint32_t limit = MAIN_LIMIT_MS) {
  return elapsedMs > limit;
}

/**
 * Mirrors Rule::handleMainOvertime() state (mainStartTime + two-phase arm).
 * First on-tick only arms the timer; trip is evaluated from the second tick.
 */
class RuleMainTracker {
  uint32_t startMs_ = 0;
  bool armed_ = false;

public:
  enum class Event { None, Armed, Tripped };

  Event step(bool pumpOn, uint32_t nowMs, uint32_t limit = MAIN_LIMIT_MS) {
    if (!pumpOn) {
      startMs_ = 0;
      armed_ = false;
      return Event::None;
    }
    if (!armed_) {
      startMs_ = nowMs;
      armed_ = true;
      return Event::Armed;
    }
    if (mainExceeded(nowMs - startMs_, limit)) {
      armed_ = false;
      startMs_ = 0;
      return Event::Tripped;
    }
    return Event::None;
  }

  uint32_t startMs() const { return startMs_; }
  bool armed() const { return armed_; }
  void reset() {
    startMs_ = 0;
    armed_ = false;
  }
};

/**
 * Mirrors Mode::handleMainStop() overtime branch (mainState.start).
 * Arms on first on-tick while running; trip on later ticks.
 */
class ModeMainTracker {
  uint32_t startMs_ = 0;
  bool armed_ = false;

public:
  enum class Event { None, Armed, Tripped };

  Event step(bool pumpOn, uint32_t nowMs, uint32_t limit = MAIN_LIMIT_MS) {
    if (!pumpOn) {
      startMs_ = 0;
      armed_ = false;
      return Event::None;
    }
    if (!armed_) {
      startMs_ = nowMs;
      armed_ = true;
      return Event::Armed;
    }
    if (mainExceeded(nowMs - startMs_, limit)) {
      armed_ = false;
      startMs_ = 0;
      return Event::Tripped;
    }
    return Event::None;
  }

  uint32_t startMs() const { return startMs_; }
  bool armed() const { return armed_; }
  void reset() {
    startMs_ = 0;
    armed_ = false;
  }
};

} // namespace overtime

#endif
