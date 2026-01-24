#ifndef ModeInterface_h
#define ModeInterface_h

#include "Glob.h"

// Forward declarations to avoid circular dependencies
class Read;
class Rule;
class Pump;
class Buzz;

// Defines title len for LCD
#define MODE_TITLE_LEN 5
#define WORK_LEN 10
#define TARGET_RISE_CM 3 // Target rise in cm per pumping session

struct WellState {
  bool on = false;
  uint8_t level = 0;
  unsigned long start;
  unsigned long stop;
  uint8_t mode = 0;
};

struct WellPoint {
  uint8_t work;
  uint8_t wait;
  uint8_t rise;
  bool flag = false; // Flag for calculate correction
  float correction = -1.0;
  uint8_t levelStart = 0;
  uint8_t levelStop = 0;
};

//
// Defines Mode interface structure.
class Mode {

private:
  uint8_t workIndex = 0;
  WellPoint pumpBuffer[WORK_LEN];
  WellState wellState;
  String warnMessage = "";

  unsigned long nextToOn = 0;
  unsigned long nextToOff = 0;

  const unsigned long timePrepareTurnOn = 10000;
  /**
   * Converts minutes to millis
   * @param minutes
   * @return
   */
  unsigned long calcMinutes(unsigned long minutes) {
    // UL ensures the result is treated as an unsigned long
    return minutes * 60 * 1000UL;
  }

  // Adds new entry wor buffer
  void startWorkPoint() {
    //
    // auto close work buffer
    if (isWorkPointAvailable()) {
      this->pointWorkResult();
      pumpBuffer[workIndex].flag = false;
      return;
    }
    wellState.start = millis();
    if (++workIndex >= WORK_LEN)
      workIndex = 0;
    pumpBuffer[workIndex].flag = true;
    pumpBuffer[workIndex].work = 0;
    pumpBuffer[workIndex].levelStart = read->getWellLevel();
  }

  bool isWorkPointAvailable() { return pumpBuffer[workIndex].flag; }

  /**
   * @brief  Fill with result data
   *
   * @param work
   * @param wait
   * @param rise
   */
  void pointWorkResult() {
    if (pumpBuffer[workIndex].flag) {
      pumpBuffer[workIndex].levelStop = read->getWellLevel();
      uint8_t startLevel = pumpBuffer[workIndex].levelStart;
      uint8_t endLevel = pumpBuffer[workIndex].levelStop;
      pumpBuffer[workIndex].rise =
          startLevel - endLevel; // Corrected: rise should be positive
      pumpBuffer[workIndex].correction =
          calculateCorrection(startLevel, endLevel);

      //
      // Close data entry
      pumpBuffer[workIndex].flag = false;
      dbg(F("[CTRL] Work result pointed:"));
      dbgLn(pumpBuffer[workIndex].correction);
    } else {
      // Display warn
      dbgLn(F("[ERROR] Pump buffer overflow!"));
    }
  }
  /**
   * @brief Work time for the pump
   *
   * @param msWork
   */
  void pointWorkTime(unsigned long msWork) {
    if (pumpBuffer[workIndex].flag) {
      pumpBuffer[workIndex].work = this->calcMinutes(msWork);
    } else {
      // Display warn
      dbgLn(F("[ERROR] Pump buffer overflow!"));
    }
  }
  /**
   * @brief Wait time for the pump
   *
   * @param msWait
   */
  void pointWaitTime(unsigned long msWait) {
    if (pumpBuffer[workIndex].flag) {
      pumpBuffer[workIndex].wait = this->calcMinutes(msWait);
    } else {
      // Display warn
      dbgLn(F("[ERROR] Pump buffer overflow!"));
    }
  }

  // Calculates how much the time should change based on actual performance
  // Returns a multiplier (e.g., 1.1 if pump is slow, 0.9 if pump is too fast)
  float calculateCorrection(uint8_t startLevel, uint8_t endLevel) {
    int8_t actualRise = (int8_t)startLevel - (int8_t)endLevel;
    if (actualRise <= 0)
      return 1.0;
    float rawCorrection = (float)TARGET_RISE_CM / (float)actualRise;
    return clamp(rawCorrection, 0.5, 1.5);
  }


  //
  // Shortcut/helper functions
  //

protected:
//
// Resolve working hours
#if defined(OPT_DAYTIME_WELL)
  const uint8_t workHours = 12;
#else
  const uint8_t workHours = 24;
#endif

  Read *read = NULL;
  Rule *rule = NULL;
  Buzz *buzz = NULL;

  bool isWarnStop() {
    // todo
    return false;
  }


  float clamp(float val, float minVal, float maxVal) {
    if (val < minVal) return minVal;
    if (val > maxVal) return maxVal;
    return val;
}

  float calculateAverageCorrection() {
    float totalCorrection = 0.0;
    uint8_t count = 0;

    for (uint8_t i = 0; i < WORK_LEN; i++) {
      if (!pumpBuffer[i].flag && pumpBuffer[i].correction != -1.0) {
        totalCorrection += pumpBuffer[i].correction;
        count++;
      }
    }

    if (count == 0)
      return 1.0; // No data, return neutral correction

    return totalCorrection / (float)count;
  }

  /**
   * @brief Returns the last correction value
   *
   * @return float The last correction value, or 1.0 if no data
   */
  float calculateLastCorrection() {
    for (int8_t i = WORK_LEN - 1; i >= 0; i--) {
      if (!pumpBuffer[i].flag && pumpBuffer[i].correction > 0) {
        return pumpBuffer[i].correction;
      }
    }
    return 1.0; // No data, return neutral correction
  }

    float fetchWeightedCorrection() {
    float avg = calculateAverageCorrection(); // Твоята функция за средно
    float last = calculateLastCorrection(); // Твоята функция за последно

    // Ако нямаме история, връщаме последното
    if (avg == 1.0)
      return last;

    // Комбинираме ги за по-голяма стабилност
    float driftCorrection = (avg * 0.7) + (last * 0.3);

    if (driftCorrection < 0.5)
      driftCorrection = 0.5;
    if (driftCorrection > 1.5)
      driftCorrection = 1.5;

    return driftCorrection;
  }

  /**
   * @brief Fetch the last rise value
   *
   * @return uint8_t
   */
  uint8_t fetchLastRise() {
    for (int8_t i = WORK_LEN - 1; i >= 0; i--) {
      if (!pumpBuffer[i].flag && pumpBuffer[i].rise != 0) {
        return pumpBuffer[i].rise;
      }
    }
    return 0; // No data
  }

  /**
   * @brief Fetch average rise value
   *
   * @return uint8_t
   */
  uint8_t fetchAverageRise() {
    uint8_t totalRise = 0;
    uint8_t count = 0;

    for (uint8_t i = 0; i < WORK_LEN; i++) {
      if (!pumpBuffer[i].flag && pumpBuffer[i].rise != 0) {
        totalRise += pumpBuffer[i].rise;
        count++;
      }
    }

    if (count == 0)
      return 0; // No data

    return totalRise / count;
  }

  uint8_t fetchRise(uint8_t defaultRise = 3) {
    uint8_t rise;
    rise = this->fetchAverageRise();
    if (rise > 0)
      return rise;

    rise = this->fetchLastRise();
    if (rise > 0)
      return rise;

    return defaultRise;
  }

  /**
   * @brief Get the Well Volume object
   *
   * @param tankLevel
   * @return uint8_t
   */
  uint8_t getWellVolume(uint8_t tankLevel) {
    // also need to be used LevelSensorMainMax
    return tankLevel - LevelSensorWellMax;
  }
  /**
   * @brief Get the Main Volume object
   *
   * @param tankLevel
   * @return uint8_t
   */
  uint8_t getMainVolume(uint8_t tankLevel) {
    // also need to be used LevelSensorMainMax
    return tankLevel - LevelSensorMainMax;
  }

  /**
   * Pumping well amplitude
   * @param workMin
   * @param stopMin
   */
  void pumpWell(uint8_t workMin, unsigned long stopMin) {

    unsigned long msTimeToOff = this->calcMinutes(workMin);
    unsigned long msTimeToOn = this->calcMinutes(stopMin);

    this->nextToOff = msTimeToOff;
    this->nextToOn = msTimeToOn;

    if (cmd.show(F("dump:off"))) {
      Serial.print(F("Stop min: "));
      Serial.print(stopMin);
      Serial.print(F(" ms: "));
      Serial.println(msTimeToOn);
    }
    //
    // Reset the clock when pump is manually run
    if (ctrlWell.isOn() != wellState.on) {
      wellState.on = ctrlWell.isOn();
      wellState.start = millis();

      this->startWorkPoint();
    }

    //
    // Turn pump OFF by timeout of mode
    if (ctrlWell.isOn() && (getWellWorkTimer() >= msTimeToOff)) {
      ctrlWell.setOn(false);
      wellState.stop = millis();
      dbg(F("[CTRL] well to OFF"));
      dbgLn();
      this->pointWorkTime(wellState.stop - wellState.start);
      read->stopWorkRead();
    }

    //
    // Calculate final for work point
    if (isWorkPointAvailable() && !ctrlWell.isOn() && spanLg.active()) {
      this->pointWorkResult();
    }

    //
    // Ignore next code when tank is full
    if (!ctrlWell.isOn() && LevelSensorWellMax >= read->getWellLevel()) {
      return;
    }

    if (ctrlWell.isFailure()) {
      if (spanSm.active())
        dbgLn(F("[CTRL] /Well/ has failure!"));
      return;
    }
    //
    // Data is not ready, brake the function
    if (!ctrlWell.isOn() &&
        read->getWellLevel() < LevelSensorBareMax(LevelSensorWellMax)) {
      return;
    }

    if (isWarnStop())
      return;

    //
    // Prepare, read levels before start
    if (!ctrlWell.isOn() &&
        (getWellWorkTimer() >= (msTimeToOn - timePrepareTurnOn))) {
      if (spanLg.active()) {
        read->startWorkRead();
        buzz->alarm();
        dbg(F("[CTRL] Well prepare"));
        dbgLn();
      }
    }

    //
    // Turn the pump on
    if (!ctrlMain.isOn() && !ctrlWell.isOn() &&
        (getWellWorkTimer() >= msTimeToOn)) {

      startWorkPoint();
      dbg(F("[CTRL] Well to ON"));
      dbgLn();

      ctrlWell.setOn(true);
    }
  }

public:
  void init(Rule *rl, Read *rd, Buzz *bz) {
    read = rd;
    rule = rl;
  }
  virtual void exec() = 0;
  // Return flash string helper so implementations can return F("...")
  virtual const __FlashStringHelper *title() = 0;

  void debug() {
    int testMode = 0;
    if (cmd.set(F("mode:test"), testMode, F("Set mode to TEST"))) {

      pumpBuffer[0].flag = false; // Clear buffer
      pumpBuffer[0].levelStart = 90;
      pumpBuffer[0].levelStop = 83;
      pumpBuffer[0].rise = 7;
      pumpBuffer[0].wait = 180;
      pumpBuffer[0].work = 20;
      float correction = calculateCorrection(90, 83);
      pumpBuffer[0].correction = correction;
      dbgLn(F("Mode set to TEST"));
    }

    if (cmd.show(F("mode:test"), F("Shows work timer to next ON state.")))
      cmd.print(F("Internal correction"), pumpBuffer[0].correction);
  }
  /**
   * Gets title limited by length
   */
  const char *getTitle() {
    static char buffer[MODE_TITLE_LEN + 1];
    // Prefill with spaces so the LCD line is cleared/padded
    memset(buffer, ' ', MODE_TITLE_LEN);
    const __FlashStringHelper *flash = title();
    strncpy_P(buffer, (PGM_P)flash, MODE_TITLE_LEN);

    buffer[MODE_TITLE_LEN] = '\0';
    return buffer;
  }

  // Returns the flash-resident title directly. Callers that accept
  // __FlashStringHelper* (e.g., lcd.print) can use this to avoid copying.
  const __FlashStringHelper *getTitleFlash() { return title(); }

  unsigned long getWellWorkTimer() { return millis() - wellState.start; }
  /**
   * @brief Gets next timer ON action for display
   */
  unsigned long getNextOn() {
    if (!wellState.on)
      return this->nextToOn - (getWellWorkTimer());

    return this->nextToOn;
  }

  /**
   * @brief Gets next timer OFF action for display
   */
  unsigned long getNextOff() {
    if (wellState.on)
      return this->nextToOff - (getWellWorkTimer());

    return this->nextToOff;
  }
};

#endif