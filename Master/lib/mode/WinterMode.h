#ifndef WinterMode_h
#define WinterMode_h

#include "../Mode.h"
#include "../Pump.h"
#include "../Read.h"

#define WELL_DEFAULT_RUNTIME 12
#define WELL_SENSOR_FULL 20
#define WELL_MIDPOINT 50    // Target thermal mass level
#define MAIN_DRAIN_LIMIT 85 // Emergency house supply limit
#define MIN_BREAK_TIME 48
#define MAX_BREAK_TIME 2160 // 36 hours

class WinterMode : public Mode {
public:
  WinterMode() {}

  const __FlashStringHelper *title() override { return F("Winter"); }

  RunWell well(Read *read) override {

    // 1. Basic Data Acquisition
    float waterTemp = read->getWellWaterTemp();
    uint8_t wellLevel = read->getWellLevel();
    uint8_t mainLevel = read->getMainLevel();

    uint8_t realRise = this->fetchRise(TARGET_RISE_CM) ?: 5;
    uint8_t wellEmpty =
        (wellLevel <= WELL_SENSOR_FULL) ? 0 : (wellLevel - WELL_SENSOR_FULL);
    float driftCorrection =
        constrain(this->fetchWeightedCorrection(), 0.8, 1.2);

    uint16_t breakTimeInterval =
        (uint16_t)((workHours * 60) / (wellEmpty / (float)realRise + 1));
    uint8_t finalRuntime = WELL_DEFAULT_RUNTIME;

    // 2. CHECK FOR SYSTEM LOAD (Power Safety)
    // Avoid starting the compressor if any pump/valve is already active
    bool systemBusy = ctrlWell.isOn() || ctrlMain.isOn();

    // 3. STRATEGIC WINTER LOGIC

    // A) EMERGENCY: House tank is low
    if (mainLevel > MAIN_DRAIN_LIMIT) {
      breakTimeInterval = MIN_BREAK_TIME;
      this->setWarn(F("HOUSE WATER LOW "));
    }
    // B) FREEZE PROTECTION: Water near 0°C
    else if (waterTemp < 1.0) {
      float tempIn = constrain(waterTemp, -2.0, 1.0);
      uint16_t basePulseBreak = map(tempIn * 10, -20, 10, 45, 120);

      // CRITICAL: Force start ONLY if system is NOT busy (Power Safety)
      if (waterTemp < 0 && !systemBusy) {
        ctrlWell.setOn(true); // enters in infinity loop until temp rises
        //                "1234567890123456"
        this->setWarn(F("WELL FREEZING!!!"));
      }

      float volumeFactor = (float)realRise / 5.0;
      breakTimeInterval = (uint16_t)(basePulseBreak * volumeFactor);
      finalRuntime = 8;
    }
    // C) THERMAL CONSERVATION: Mass is OK (Level > 50cm), stay in standby
    else if (wellLevel <= WELL_MIDPOINT) {
      breakTimeInterval = MAX_BREAK_TIME; //  sleep time
      if (spanLg.active())
        dbgLn(F("[Winter] Mass OK, sleeping."));
    }
    // D) REPLENISHING MASS: Well is emptier than 50cm
    else if (wellLevel > WELL_MIDPOINT) {
      // Slow refill (min 6 hours break) to keep ground water warm
      breakTimeInterval = max(breakTimeInterval, (uint16_t)360);
      if (spanLg.active())
        dbgLn(F("[Winter] Refilling mass."));
    }

    // 4. Final adjustments
    if (finalRuntime == WELL_DEFAULT_RUNTIME) {
      uint16_t cappedBreak = constrain(breakTimeInterval, 45, MAX_BREAK_TIME);
      finalRuntime = (uint8_t)((WELL_DEFAULT_RUNTIME * driftCorrection) +
                               map(cappedBreak, 45, MAX_BREAK_TIME, 0, 8));
    }

    finalRuntime = constrain(finalRuntime, 8, 15);

    // Final Power Safety Check: If we calculated a start now but system is
    // busy, the pumpWell method handles the timing, but we log it.

    return RunWell{finalRuntime, breakTimeInterval};
  }
};
#endif