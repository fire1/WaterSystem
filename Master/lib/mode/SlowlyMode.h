
#ifndef SlowlyMode_h
#define SlowlyMode_h

#include "../Mode.h"
#include "../Pump.h"
#include "../Read.h"

#define MIN_BREAK_TIME 60       // 1 hour
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

  RunWell well(Read *read) override {

    uint8_t wellVol = this->getWellVolume(read->getWellLevel());
    uint8_t mainVol = this->getMainVolume(read->getMainLevel());

    uint8_t realRise = this->fetchRise(TARGET_RISE_CM);
    float drift = constrain(this->fetchWeightedCorrection(), 0.6, 1.2);

    float sessionsNeeded = (float)wellVol / (float)realRise;
    if (sessionsNeeded < 1.0)
      sessionsNeeded = 1.0;

    uint8_t runtime = (uint8_t)(WELL_DEFAULT_RUNTIME * drift);
    uint16_t breaktime =
        (uint16_t)((workHours * 60 + runtime) / sessionsNeeded);

    int8_t overtime = map(breaktime, MIN_BREAK_TIME, MAX_BREAK_TIME, -4, 6);

    runtime = constrain(runtime + overtime, 7, 16);

    // Debug output for monitoring
    if (spanLg.active()) {
      dbg(F("[SLOWLY] Work: "));
      dbg(runtime);
      dbg(F("m | Break: "));
      dbg(breaktime);
      dbg(F("m | Rise: "));
      dbg(realRise);
      dbg(F("cm | Drift: "));
      dbg(drift);
      dbg(F(" | WellVol: "));
      dbg(wellVol);
      dbgLn();
    }
    return RunWell{runtime, breaktime};
  }
};
#endif
