#ifndef WinterMode_h
#define WinterMode_h

#include "../Mode.h"
#include "../Pump.h"
#include "../Read.h"

#define WELL_DEFAULT_RUNTIME 12
#define WELL_SENSOR_FULL 20
#define WELL_SAFE_ZONE 60     // Midpoint target
#define MAIN_DRAIN_LIMIT 85   // Emergency threshold for upper tank

class WinterMode : public Mode {
public:
  WinterMode() {}

  const __FlashStringHelper *title() override { return F("Winter"); }

  void exec() override {
    if (!read || !rule) return;

    // Fetch sensors
    float waterTemp = read->getWellWaterTemp(); 
    uint8_t wellLevel = read->getWellLevel();
    uint8_t mainLevel = read->getMainLevel();

    // 1. Adaptive rise calculation
    uint8_t realRise = this->fetchRise(TARGET_RISE_CM);
    if (realRise == 0) realRise = 5; 

    // 2. Volumetric calculations
    uint8_t wellEmpty = (wellLevel <= WELL_SENSOR_FULL) ? 0 : (wellLevel - WELL_SENSOR_FULL);
    float driftCorrection = constrain(this->fetchWeightedCorrection(), 0.8, 1.2);
    
    // Default break based on usage
    uint16_t breakTimeInterval = (uint16_t)((workHours * 60) / (wellEmpty / (float)realRise + 1));
    uint8_t finalRuntime = WELL_DEFAULT_RUNTIME;

    // 3. THERMAL & LEVEL LOGIC
    
    // A) PRIORITY: Mass recovery (Main < 85cm or Well < 60cm)
    if (mainLevel > MAIN_DRAIN_LIMIT || wellLevel > WELL_SAFE_ZONE) {
      breakTimeInterval = MIN_BREAK_TIME; 
      if (spanLg.active()) {
        //                "1234567890123456"
        this->setWarn(F("RECOVERING MASS ")); // Exactly 16 chars
      }
    }
    // B) FREEZE PROTECTION: Only triggers if water is near or below 1.0°C
    else if (waterTemp < 1.0) {
      // Mapping: 1.0°C -> 90min | -2.0°C -> 45min (Aggressive cycle)
      float tempIn = constrain(waterTemp, -2.0, 1.0);
      uint16_t basePulseBreak = map(tempIn * 10, -20, 10, 45, 90);
      
      float volumeFactor = (float)realRise / 5.0; 
      breakTimeInterval = (uint16_t)(basePulseBreak * volumeFactor);
      
      finalRuntime = 8; 
      if (spanLg.active()) {
        //                "1234567890123456"
        this->setWarn(F("WELL COLD WARMUP")); // Exactly 16 chars
      }
    }
    // C) STANDBY: Safe levels and water above 1°C
    else if (wellLevel <= WELL_SENSOR_FULL) {
      breakTimeInterval = 1440; 
    }

    // 4. Final adjustments
    if (finalRuntime == WELL_DEFAULT_RUNTIME) {
      uint16_t cappedBreak = constrain(breakTimeInterval, 45, 720);
      finalRuntime = (uint8_t)((WELL_DEFAULT_RUNTIME * driftCorrection) + map(cappedBreak, 45, 720, 0, 8));
    }

    finalRuntime = constrain(finalRuntime, 8, 22);

    // Serial Debug
    if (spanLg.active()) {
      dbg(F("[Winter] W-Temp: ")); dbg(waterTemp);
      dbg(F(" | Lvl: ")); dbg(wellLevel);
      dbg(F(" | Brk: ")); dbg(breakTimeInterval);
      dbgLn(F("m"));
    }

    this->pumpWell(finalRuntime, (unsigned long)breakTimeInterval);
  }
};
#endif