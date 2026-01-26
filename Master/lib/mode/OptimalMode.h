#ifndef OptimalMode_h
#define OptimalMode_h

#include "../Mode.h"
#include "../Pump.h"
#include "../Read.h"

#define MIN_BREAK_TIME 48       // 1 hour
#define MAX_BREAK_TIME 480      // 8 hours
#define WELL_DEFAULT_RUNTIME 12 // minutes default run time

#define DRIFT_BAD 1.05
#define DRIFT_GOOD 0.95

class OptimalMode : public Mode {
private:
  struct PumpPlan {
    uint16_t runtime;   // minutes
    uint16_t breaktime; // minutes
    float correction;
  };

  PumpPlan plan;

  unsigned long pumpOffTime = 0;
  unsigned long lastFinishTime = 0;
  uint8_t startLevelCapture = 0;
  int8_t tuneDir = 0;
  float lastCorrection = 1.0;
  uint8_t stableCount = 0;
  

  float timeMultiplier = 1.0;
  uint8_t currentRuntime = WELL_DEFAULT_RUNTIME; // Start with 12 mins base

  PumpPlan fetchBestPlan(float drift) {
    WellPoint p = fetchEfficiencyPoint();

    PumpPlan plan;

    plan.runtime = p.work;
    plan.breaktime = p.wait;

    // ефективност = rise / runtime
    plan.correction = p.correction + 0.04; // леко увеличаваме целта

    // ако има дрифт надолу → връщаме се към доказано стабилен режим
    if (drift > 1.2) {
      plan.runtime = min(plan.runtime - 1, WELL_DEFAULT_RUNTIME);
      plan.breaktime += 15; // повече почивка
    }

    return plan;
  }

  PumpPlan defaultBootstrapPlan() {
    PumpPlan p;
    p.runtime = WELL_DEFAULT_RUNTIME; // 12 мин
    p.breaktime = 180;                // 3 часа
    p.correction = 1.0;
    return p;
  }

public:
  OptimalMode() {}

  const __FlashStringHelper *title() override { return F("Auto Adjst"); }

  RunWell well(Read *read) override {
    uint8_t wellVol = this->getWellVolume(read->getWellLevel());
    uint8_t realRise = this->fetchRise(TARGET_RISE_CM);
    float drift = constrain(this->calculateLastCorrection(), 0.6, 1.6);

    if (!hasValidHistory()) {
      plan = defaultBootstrapPlan();
      if (spanLg.active())
        dbgLn(F("[OPTIMAL] Bootstrap plan applied."));
    }
    // ⚠️ ФАЗА 2: влошаване → връщане към златната зона
    else if (drift > DRIFT_BAD) {
      // ⚠️ пада ефективността → връщаме се към златната зона
      plan = fetchBestPlan(drift);

      if (spanLg.active())
        dbgLn(F("[OPTIMAL]  Correcting BAD drift."));
    } else if (drift < DRIFT_GOOD) {

      float sessionsNeeded = (float)wellVol / (float)realRise;
      if (sessionsNeeded < 1.0)
        sessionsNeeded = 1.0;

      if (tuneDir == 0)
        tuneDir = -1;

      uint8_t trialRuntime = constrain(currentRuntime + tuneDir, 7, 16);

      plan.runtime = trialRuntime;
      plan.breaktime =
          constrain(((workHours * 60 + plan.runtime) / sessionsNeeded),
                    MIN_BREAK_TIME, MAX_BREAK_TIME);

      plan.correction = (float)realRise / plan.runtime;

      if (plan.correction < lastCorrection) {
        currentRuntime = plan.runtime;
        lastCorrection = plan.correction;
        stableCount = 0;
      } else {
        tuneDir = -tuneDir;
        stableCount++;
      }

      if (stableCount >= 3) {
        tuneDir = 0; // зона намерена
      }
    }

    if (spanLg.active()) {
      dbg(F("[OPTIMAL] Drift: "));
      dbg(drift);
      dbg(F(" | Eff: "));
      dbg(plan.correction);
      dbgLn();
    }

    return RunWell{plan.runtime, plan.breaktime};
  }
};
#endif
