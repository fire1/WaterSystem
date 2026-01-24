
#ifndef WinterMode_h
#define WinterMode_h

#include "../Mode.h"
#include "../Pump.h"
#include "../Read.h"

#define MIN_BREAK_TIME 50
#define MAX_BREAK_TIME 480      // 8 hours
#define WELL_DEFAULT_RUNTIME 12 // minutes default run time

class SlowlyMode : public Mode {
private:
  unsigned long pumpOffTime = 0;
  unsigned long lastFinishTime = 0;
  uint8_t startLevelCapture = 0;

  float timeMultiplier = 1.0;
  uint8_t currentRuntime = WELL_DEFAULT_RUNTIME; // Start with 12 mins base

public:
  SlowlyMode() {}

  const __FlashStringHelper *title() override { return F("Slowly"); }

  void exec() override {
    if (!read || !rule)
      return;

    // Fetch levels (distance from sensor to water in cm)
    uint8_t wellLevel = read->getWellLevel();
    uint8_t mainLevel = read->getMainLevel();

    // Calculate empty space for both tanks
    uint8_t mainEmpty =
        (mainLevel < LevelSensorMainMax) ? 0 : (mainLevel - LevelSensorMainMax);
    uint8_t wellEmpty =
        (wellLevel < LevelSensorWellMax) ? 0 : (wellLevel - LevelSensorWellMax);

    // 1. Historical Efficiency Correction (+/- 20% fine-tuning)
    float driftCorrection = this->fetchWeightedCorrection();
    driftCorrection = constrain(driftCorrection, 0.8, 1.2);

    // 2. Calculate Break Interval based on demand
    // Fetch actual rise performance from previous cycles
    uint8_t realRise = this->fetchRise(TARGET_RISE_CM);
    if (realRise == 0)
      realRise = TARGET_RISE_CM;

    // Calculate how many pumping sessions are needed to fill the main tank
    float sessionsNeeded = (float)mainEmpty / (float)realRise;
    if (sessionsNeeded < 1.0)
      sessionsNeeded = 1.0;

    // Distribute sessions across the available work hours (usually 24h)
    uint16_t totalMinutesAvailable = (uint16_t)workHours * 60;
    uint16_t breakTimeInterval =
        (uint16_t)(totalMinutesAvailable / sessionsNeeded);

    // 3. Apply Safety Constraints for the Break Interval
    if (mainEmpty < 40) {
      // Main tank is satisfied (>80cm of water), sleep for 24h
      breakTimeInterval = 1440;
    } else if (wellEmpty > 100) {
      // Well is low, force a minimum 3h recovery break
      if (breakTimeInterval < 180)
        breakTimeInterval = 180;
    } else {
      // Standard minimum break to prevent rapid cycling
      if (breakTimeInterval < MIN_BREAK_TIME)
        breakTimeInterval = MIN_BREAK_TIME;
    }

    // 4. Smooth Runtime Bonus via map()
    // Longer breaks allow the Air Lift to be more efficient due to higher
    // static level. Map break interval (60-600 min) to bonus work time (0-8
    // min).
    uint16_t cappedBreak = constrain(breakTimeInterval, MIN_BREAK_TIME, MAX_BREAK_TIME);
    long bonus = map(cappedBreak, MIN_BREAK_TIME, MAX_BREAK_TIME, 0, 8);

    // 5. Final Runtime Calculation
    // Base runtime (historically adjusted) + linear bonus for long rest
    float baseRuntime = WELL_DEFAULT_RUNTIME * driftCorrection;
    uint8_t adjustedRuntime = (uint8_t)(baseRuntime + bonus);

    // Hard limits to protect the compressor from overheating
    adjustedRuntime = constrain(adjustedRuntime, 8, 22);

    // Debug output
    if (spanLg.active()) {
      dbg(F("[WINTER] MainEmpty: "));
      dbg(mainEmpty);
      dbg(F(" | WellEmpty: "));
      dbg(wellEmpty);
      dbg(F(" | Break: "));
      dbg(breakTimeInterval);
      dbg(F("m | Bonus: +"));
      dbg(bonus);
      dbg(F("m | Work: "));
      dbg(adjustedRuntime);
      dbgLn(F("m"));
    }

    // Execute pumping logic
    this->pumpWell(adjustedRuntime, (unsigned long)breakTimeInterval);
  }
};
#endif
