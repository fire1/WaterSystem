
#ifndef WinterMode_h
#define WinterMode_h

#include "../Mode.h"
#include "../Pump.h"
#include "../Read.h"

#define WELL_BREAK_TIME 80 // minutes minimum rest
#define WELL_RUNTIME 12    // minutes default run time

class WinterMode : public Mode {
private:
  unsigned long pumpOffTime = 0;
  unsigned long lastFinishTime = 0;
  uint8_t startLevelCapture = 0;

  float timeMultiplier = 1.0;
  uint8_t currentRuntime = WELL_RUNTIME; // Start with 12 mins base

public:
  WinterMode() {}

  const __FlashStringHelper *title() override { return F("Winter"); }

  void exec() override {
    if (!read || !rule)
      return;

    uint8_t wellLevel = read->getWellLevel(); // Tank we are FILLING
    uint8_t mainLevel = read->getMainLevel(); // Tank we are DRAWING from

    // Update multiplier from measured drift
    float driftCorrection = this->calculateLastCorrection();
    uint8_t emptySpaceCm =
        (wellLevel < LevelSensorWellMax) ? 0 : (wellLevel - LevelSensorWellMax);
    uint8_t levelRise = this->fetchRise(TARGET_RISE_CM);

    if (spanLg.active()) {
      dbg(F("LevelRs:"));
      dbg(levelRise);
      dbg(F(" Corr:"));
      dbg(driftCorrection);
      dbgLn();
    }

    uint16_t workMinutes = workHours * 60;

    uint8_t breakTimeInterval = workMinutes / (emptySpaceCm / TARGET_RISE_CM);
    uint8_t workTimeInterval = workMinutes;
    //
    // emptySpace = 60 cm, TARGET_RISE_CM = 3 cm, ако levelRise = 6 cm:
    // sessions = ceil(60/6) = 10 → interval = 24h/10 = 144 min → pause ≈ 144 -
    // runtime.
    if (levelRise > TARGET_RISE_CM) {
       breakTimeInterval = ceil(workMinutes / (emptySpaceCm / levelRise));
    }

    if(breakTimeInterval < WELL_BREAK_TIME){
      breakTimeInterval = WELL_BREAK_TIME;
    }

    this->pumpWell(currentRuntime, breakTimeInterval);
  }
};
#endif
