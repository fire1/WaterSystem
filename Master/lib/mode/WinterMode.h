
#ifndef WinterMode_h
#define WinterMode_h

#include "../Pump.h"
#include "../Read.h"
#include "../Mode.h"


#define WORK_LEN 10
#define COOL_DOWN_MS 2700000UL // 45 minutes rest (45 * 60 * 1000)
#define WELL_RUN_TIME 12 // 12 minutes default run time

struct WorkWaterPumped {
  uint8_t time;
  uint8_t size; // CM rise
};

class WinterMode : public Mode {
private:
  WorkWaterPumped workBuffer[WORK_LEN];
  uint8_t workIndex = 0;

  unsigned long pumpOffTime = 0;
  unsigned long lastFinishTime = 0;
  uint8_t startLevelCapture = 0;

  float timeMultiplier = 1.0;
  uint8_t currentRuntime = WELL_RUN_TIME; // Start with 12 mins base

  void handleWorkBuffer(uint8_t runtime, uint8_t volume) {
    workBuffer[workIndex].time = runtime;
    workBuffer[workIndex].size = volume;
    if (++workIndex >= WORK_LEN)
      workIndex = 0;
  }

public:
  WinterMode() {}

  const __FlashStringHelper *title() override { return F("Winter"); }

  void exec() override {
    //if (!read || !rule)return;

    uint8_t wellLevel = read->getWellLevel(); // Tank we are FILLING
    uint8_t mainLevel = read->getMainLevel(); // Tank we are DRAWING from

    // --- 1. MONITOR ACTIVE PUMPING ---
    // if (ctrlWell.isOn()) {

      float driftCorrection = this->calculateLastCorrection();
      timeMultiplier = (timeMultiplier * 0.7) + (driftCorrection * 0.3);
    

    // How many cm of empty space are in the Well Tank?
    // 20cm = full, 110cm = empty, so empty space = current - full level
    uint8_t emptySpaceCm = (wellLevel < LevelSensorWellMax) ? 0 : (wellLevel - LevelSensorWellMax);

    // Add empty space in Main Tank for total missing water
    uint8_t mainEmptySpace = (mainLevel < LevelSensorMainMax) ? 0 : (mainLevel - LevelSensorMainMax);
    emptySpaceCm += mainEmptySpace;

    uint8_t effectiveWorkHours = this->workHours;
    if (emptySpaceCm > 50) {
      // Calculate number of 50cm parts (ceiling division)
      uint8_t parts = (emptySpaceCm + 49) / 50;
      // Use driftCorrection to decide days per part: 1 day if correction is close to 1.0 (accurate predictions), 2 days otherwise
      uint8_t daysMultiplier = (driftCorrection >= 0.8 && driftCorrection <= 1.2) ? 1 : 2;
      effectiveWorkHours = this->workHours * parts * daysMultiplier;
      // Limit to maximum 7 days (168 hours) to prevent excessive delays
      if (effectiveWorkHours > 168) effectiveWorkHours = 168;
    }
    // How many 3cm cycles do we need to fill the Well Tank?
    float neededCycles = (float)emptySpaceCm / (float)TARGET_RISE_CM;

    

    // Default pause (80 min hardware safety)
    uint16_t pauseMinutes = 80;

    if (neededCycles > 1.0) {
      // Spread the needed cycles across the available work hours
      // Pause = (Total Work Minutes) / Number of Cycles
      uint16_t spreadingPause =
          (uint16_t)(((float)effectiveWorkHours * 60.0) / neededCycles);

      if (spreadingPause > 45) {
        pauseMinutes = spreadingPause;
      } else {
        pauseMinutes = 45; // Minimum 45 minutes rest
      }

    }

    float calculatedRuntime = (float)WELL_RUN_TIME * timeMultiplier;

    if(cmd.show(F("mode:well:cal"), F("Mode calculated info"))) {
      dbg(F("[MODE] Winter Mode >> Space:"));
      dbg(emptySpaceCm);
      uint8_t parts = (emptySpaceCm > 50) ? ((emptySpaceCm + 49) / 50) : 1;
      uint8_t daysMultiplier = (emptySpaceCm > 50 && driftCorrection >= 0.8 && driftCorrection <= 1.2) ? 1 : (emptySpaceCm > 50 ? 2 : 1);
      dbg(F("cm, Parts:"));
      dbg(parts);
      dbg(F(", Days Mult:"));
      dbg(daysMultiplier);
      dbg(F(", Effective Hours:"));
      dbg(effectiveWorkHours);
      dbg(F(", Cycles: "));
      dbg(neededCycles);
      dbg(F(", Pause: "));
      dbg(pauseMinutes);
      dbg(F("min, Multiplier: "));
      dbg(timeMultiplier);
      dbg(F(", Runtime: "));
      dbg(calculatedRuntime);
      dbgLn();
    }

    if (calculatedRuntime < 5)
      calculatedRuntime = 5;
    if (calculatedRuntime > 20)
      calculatedRuntime = 20; 
    currentRuntime = (uint8_t)calculatedRuntime;

    // Trigger rule: Pump for X minutes, then wait Y minutes
    // rule->pumpWell(currentRuntime, pauseMinutes);

    pumpOffTime = millis() + ((unsigned long)currentRuntime * 60000UL);

    if (cmd.show(F("mode"), F("Mode info"))) {
        dbg(F("Well times On: "));
        dbg(this->getNextOn());
        dbg(F(" Off: "));   
        dbg(this->getNextOff());
        dbgLn();
      
    }
    this->pumpWell(currentRuntime, pauseMinutes);
  }
};
#endif
