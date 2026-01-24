#ifndef ThermalWinterMode_h
#define ThermalWinterMode_h

#include "../Mode.h"
#include "../Pump.h"
#include "../Read.h"

#define WELL_DEFAULT_RUNTIME 12
#define FREEZE_CRITICAL -6.0
#define FREEZE_WARNING -3.0

// Sensor Range: 98cm (Empty) to 20cm (Full)
// Tank Base: 100cm x 150cm (1cm = 15 Liters)
#define TANK_SENSOR_FULL 20
#define TANK_SENSOR_EMPTY 98

// We want to keep > 700L for thermal mass (approx. 48cm from bottom)
// Sensor reading > 50cm means water level has dropped below the 50% mark.
#define CRITICAL_LOW_LEVEL 50
#define SAFE_LEVEL 35 // ~1000L mark (approx. 63cm from bottom)

class ThermalWinterMode : public Mode {
public:
  ThermalWinterMode() {}

  const __FlashStringHelper *title() override { return F("Thermal"); }

  void exec() override {
    if (!read || !rule)
      return;

    float outsideTemp = read->getOutsideTemp();
    uint8_t mainLevel = read->getMainLevel();

    // Calculate missing volume for standard logic
    uint8_t mainEmpty =
        (mainLevel <= TANK_SENSOR_FULL) ? 0 : (mainLevel - TANK_SENSOR_FULL);

    // 1. Efficiency Correction
    float driftCorrection =
        constrain(this->fetchWeightedCorrection(), 0.8, 1.2);
    uint16_t totalMinutesAvailable = (uint16_t)workHours * 60;

    uint8_t realRise = this->fetchRise(TARGET_RISE_CM);
    if (realRise == 0)
      realRise = TARGET_RISE_CM;

    float sessionsNeeded = (float)mainEmpty / (float)realRise;
    if (sessionsNeeded < 1.0)
      sessionsNeeded = 1.0;
    uint16_t breakTimeInterval =
        (uint16_t)(totalMinutesAvailable / sessionsNeeded);

    // 2. THERMAL & SAFETY LOGIC
    uint8_t finalRuntime = WELL_DEFAULT_RUNTIME;

    // PRIORITY FILL: Mass is below 50% (720 liters)
    if (mainLevel > CRITICAL_LOW_LEVEL) {
      breakTimeInterval = MIN_BREAK_TIME;
      if (spanLg.active()) {
        this->setWarn(F("Low Thermal Mass!"));
      }

    }
    // THERMAL PULSE: Tank is in Safe Zone (70% - 100% full)
    else if (mainLevel <= SAFE_LEVEL) {
      if (outsideTemp <= FREEZE_CRITICAL) {
        breakTimeInterval = 180; // 3h
        finalRuntime = 8;
      } else if (outsideTemp <= FREEZE_WARNING) {
        breakTimeInterval = 480; // 8h
        finalRuntime = 8;
      } else {
        breakTimeInterval = 1440; // 24h sleep
      }
    }

    // 3. Final Runtime with map() bonus
    if (finalRuntime == WELL_DEFAULT_RUNTIME) {
      uint16_t cappedBreak = constrain(breakTimeInterval, 60, 720);
      finalRuntime = (uint8_t)((WELL_DEFAULT_RUNTIME * driftCorrection) +
                               map(cappedBreak, 60, 720, 0, 8));
    }

    finalRuntime = constrain(finalRuntime, 8, 22);

    if (spanLg.active()) {
      dbg(F("[THERMAL] T: "));
      dbg(outsideTemp);
      dbg(F(" | Lvl: "));
      dbg(mainLevel);
      dbg(F("cm | Work: "));
      dbg(finalRuntime);
      dbgLn(F("m"));
    }

    this->pumpWell(finalRuntime, (unsigned long)breakTimeInterval);
  }
};
#endif