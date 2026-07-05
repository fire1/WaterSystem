#ifndef Main_h
#define Main_h

#include <stdint.h>

namespace mainTank {

constexpr uint8_t MAIN_CHECKPOINT_HOUR = 22;
constexpr uint8_t MAIN_CHECKPOINT_MINUTE = 10;
constexpr uint8_t MAIN_LEVEL_MAIN_MIN = 42;
constexpr uint8_t MAIN_LEVEL_MAIN_OVERRIDE = 40;

/** Well tank 1.0 x 1.5 x 1.0 m; sensor 20 cm (full) .. 110 cm (empty). */
constexpr uint8_t WELL_TANK_HEIGHT_CM = 100;
constexpr uint8_t WELL_SENSOR_FULL_CM = 20;
constexpr uint8_t WELL_SENSOR_EMPTY_CM = 110;
constexpr uint8_t WELL_PUMP_BOTTOM_MARGIN_CM = 15;
/** Minimum drawable water above the main-pump intake (not sensor reading). */
constexpr uint8_t WELL_MIN_USABLE_DEPTH_CM = 60;
constexpr uint8_t WELL_SENSOR_SPAN_CM =
    WELL_SENSOR_EMPTY_CM - WELL_SENSOR_FULL_CM;
constexpr uint8_t WELL_MIN_TOTAL_DEPTH_CM =
    WELL_MIN_USABLE_DEPTH_CM + WELL_PUMP_BOTTOM_MARGIN_CM;

/** Max well sensor reading for start: wellLevel must be strictly below this. */
constexpr uint8_t wellMaxForMinDepth(uint8_t minTotalDepthCm) {
  return WELL_SENSOR_EMPTY_CM -
         (minTotalDepthCm * WELL_SENSOR_SPAN_CM) / WELL_TANK_HEIGHT_CM;
}

constexpr uint8_t MAIN_LEVEL_WELL_MAX = wellMaxForMinDepth(WELL_MIN_TOTAL_DEPTH_CM);
constexpr uint8_t MAIN_SENSOR_BARE_MIN = 19; // LevelSensorBareMax(LevelSensorMainMax)

/** Slow main-level samples (reject 60 m cable / Slave UART spikes). */
constexpr uint8_t MAIN_STABILITY_RING = 6;
constexpr uint8_t MAIN_STABILITY_MIN = 4;
constexpr uint8_t MAIN_STABILITY_SPIKE_CM = 12;

/** Leak = steady drain (cm rises at similar rate over several sample periods). */
constexpr unsigned long MAIN_LEAK_SAMPLE_MS = 30UL * 60 * 1000;
constexpr uint8_t MAIN_LEAK_RATE_SAMPLES_NIGHT = 3;
constexpr uint8_t MAIN_LEAK_RATE_SAMPLES_DAY = 4;
constexpr uint8_t MAIN_LEAK_MIN_RATE_CM = 1;
constexpr uint8_t MAIN_LEAK_RATE_TOLERANCE_CM = 1;
constexpr uint8_t MAIN_LEAK_DAY_MIN_LEVEL_CM = 50;
constexpr uint8_t MAIN_LEAK_NIGHT_START_HOUR = 23;
constexpr uint8_t MAIN_LEAK_NIGHT_END_HOUR = 6;

enum class Intent { Default, Force, Block };
using MainTransferIntent = Intent;

struct CheckpointState {
  uint8_t lastDay = 255;
  uint8_t lastMonth = 255;
  uint16_t lastYear = 65535;
};

struct Levels {
  uint8_t mainLevel;
  uint8_t wellLevel;
};

struct ScheduleThresholds {
  uint8_t mainMin;
  uint8_t wellMax;
};

struct ClockNow {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
};

struct Stability {
  uint8_t ring[MAIN_STABILITY_RING]{};
  uint8_t count = 0;
  uint8_t head = 0;

  void reset() {
    count = 0;
    head = 0;
  }

  static uint8_t medianOf(uint8_t *values, uint8_t n) {
    for (uint8_t i = 1; i < n; ++i) {
      const uint8_t v = values[i];
      int8_t j = (int8_t)i - 1;
      while (j >= 0 && values[j] > v) {
        values[j + 1] = values[j];
        --j;
      }
      values[j + 1] = v;
    }
    if (n == 0)
      return 0;
    if (n & 1)
      return values[n / 2];
    return (values[n / 2 - 1] + values[n / 2]) / 2;
  }

  uint8_t median() const {
    if (count == 0)
      return 0;
    uint8_t tmp[MAIN_STABILITY_RING];
    for (uint8_t i = 0; i < count; ++i)
      tmp[i] = ring[i];
    return medianOf(tmp, count);
  }

  void push(uint8_t sample) {
    if (count > 0) {
      const uint8_t med = median();
      const int delta = (int)sample - (int)med;
      if (delta < 0 ? -delta : delta > MAIN_STABILITY_SPIKE_CM)
        return;
    }
    ring[head] = sample;
    head = (head + 1) % MAIN_STABILITY_RING;
    if (count < MAIN_STABILITY_RING)
      ++count;
  }

  bool isStable() const { return count >= MAIN_STABILITY_MIN; }
};

struct LeakWatch {
  static constexpr uint8_t kMaxRates = 4;

  uint8_t lastLevel = 0;
  unsigned long lastSampleMs = 0;
  bool hasLast = false;

  int8_t rates[kMaxRates]{};
  uint8_t rateCount = 0;
  uint8_t rateHead = 0;
  bool alarm = false;

  static bool isNightHour(uint8_t hour) {
    return hour >= MAIN_LEAK_NIGHT_START_HOUR ||
           hour < MAIN_LEAK_NIGHT_END_HOUR;
  }

  static uint8_t requiredSamples(uint8_t hour) {
    return isNightHour(hour) ? MAIN_LEAK_RATE_SAMPLES_NIGHT
                             : MAIN_LEAK_RATE_SAMPLES_DAY;
  }

  void clearRates() {
    rateCount = 0;
    rateHead = 0;
  }

  void resetSample(uint8_t level, unsigned long nowMs) {
    lastLevel = level;
    lastSampleMs = nowMs;
    hasLast = true;
    clearRates();
  }

  static bool ratesAreConstant(const int8_t *samples, uint8_t n) {
    if (n == 0)
      return false;
    int8_t minR = 127;
    int8_t maxR = -128;
    for (uint8_t i = 0; i < n; ++i) {
      if (samples[i] < (int8_t)MAIN_LEAK_MIN_RATE_CM)
        return false;
      if (samples[i] < minR)
        minR = samples[i];
      if (samples[i] > maxR)
        maxR = samples[i];
    }
    return (uint8_t)(maxR - minR) <= MAIN_LEAK_RATE_TOLERANCE_CM;
  }

  void pushRate(int8_t rate) {
    rates[rateHead] = rate;
    rateHead = (rateHead + 1) % kMaxRates;
    if (rateCount < kMaxRates)
      ++rateCount;
  }

  void tick(uint8_t stableLevel, bool mainPumpOn, unsigned long nowMs,
            uint8_t hour,
            unsigned long sampleMs = MAIN_LEAK_SAMPLE_MS) {
    if (mainPumpOn) {
      alarm = false;
      hasLast = false;
      clearRates();
      return;
    }

    if (!hasLast) {
      resetSample(stableLevel, nowMs);
      return;
    }

    if (nowMs - lastSampleMs < sampleMs)
      return;

    const int delta = (int)stableLevel - (int)lastLevel;
    lastLevel = stableLevel;
    lastSampleMs = nowMs;

    if (delta < (int)MAIN_LEAK_MIN_RATE_CM) {
      clearRates();
      return;
    }

    if (!isNightHour(hour) && stableLevel < MAIN_LEAK_DAY_MIN_LEVEL_CM) {
      clearRates();
      return;
    }

    pushRate((int8_t)delta);

    const uint8_t need = requiredSamples(hour);
    if (rateCount >= need) {
      int8_t recent[kMaxRates];
      for (uint8_t i = 0; i < need; ++i) {
        const uint8_t idx = (rateHead + kMaxRates - need + i) % kMaxRates;
        recent[i] = rates[idx];
      }
      if (ratesAreConstant(recent, need))
        alarm = true;
    }
  }

  bool isAlarm() const { return alarm; }
};

inline CheckpointState checkpointState{};
inline Stability stability{};
inline LeakWatch leakWatch{};

inline void observeMainSample(uint8_t sample) { stability.push(sample); }

inline bool hasStableMain() { return stability.isStable(); }

inline uint8_t stabilizedMain() { return stability.median(); }

inline void resetStability() { stability.reset(); }

inline ScheduleThresholds thresholdsForMode(uint8_t modeMainValue) {
  switch (modeMainValue) {
  case 1:
    return {MAIN_LEVEL_MAIN_MIN, MAIN_LEVEL_WELL_MAX};
  case 2:
    return {52, 55};
  case 3:
    return {78, 30};
  default:
    return {255, 0};
  }
}

inline bool levelsOk(const Levels &levels, ScheduleThresholds thresholds) {
  return levels.mainLevel > thresholds.mainMin &&
         levels.wellLevel < thresholds.wellMax;
}

inline bool levelsOkScheduled(const Levels &levels, uint8_t modeMainValue) {
  return levelsOk(levels, thresholdsForMode(modeMainValue));
}

inline bool levelsOkOverride(const Levels &levels) {
  return levels.mainLevel > MAIN_LEVEL_MAIN_OVERRIDE &&
         levels.wellLevel < MAIN_LEVEL_WELL_MAX;
}

inline bool sameCalendarDay(const CheckpointState &state, const ClockNow &now) {
  return state.lastYear == now.year && state.lastMonth == now.month &&
         state.lastDay == now.day;
}

inline void markCheckpointFired(CheckpointState &state, const ClockNow &now) {
  state.lastYear = now.year;
  state.lastMonth = now.month;
  state.lastDay = now.day;
}

/** Once per day at MAIN_CHECKPOINT_HOUR:MINUTE; always latches (skip if levels bad). */
inline bool shouldFireCheckpoint(const ClockNow &now, CheckpointState &state,
                                 bool levelsOkNow) {
  if (now.hour != MAIN_CHECKPOINT_HOUR)
    return false;
  if (now.minute < MAIN_CHECKPOINT_MINUTE)
    return false;
  if (sameCalendarDay(state, now))
    return false;

  markCheckpointFired(state, now);
  return levelsOkNow;
}

inline bool sensorReady(const Levels &levels) {
  return levels.mainLevel >= MAIN_SENSOR_BARE_MIN;
}

inline bool shouldStartMain(Intent intent, bool rtcOk, const ClockNow &now,
                            const Levels &levels, CheckpointState &state,
                            uint8_t modeMainValue) {
  if (!sensorReady(levels))
    return false;

  if (modeMainValue == 0)
    return false;

  if (intent == Intent::Block)
    return false;

  if (intent == Intent::Force)
    return levelsOkOverride(levels);

  if (!rtcOk)
    return false;

  const ScheduleThresholds thresholds = thresholdsForMode(modeMainValue);
  return shouldFireCheckpoint(now, state, levelsOk(levels, thresholds));
}

} // namespace mainTank

#endif
