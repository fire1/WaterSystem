#ifndef ModeInterface_h
#define ModeInterface_h

#include "../Glob.h"

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
};



//
// Defines Mode interface structure.
class ModeInterface {

private:
  uint8_t workIndex = 0;
  WellPoint pumpBuffer[WORK_LEN];
  WellState wellState;

  unsigned long nextToOn = 0;
  unsigned long nextToOff = 0;

  unsigned long timePrepareTurnOn = 0;
  /**
   * Converts minutes to millis
   * @param minutes
   * @return
   */
  unsigned long calcMinutes(unsigned long minutes) {
    // UL ensures the result is treated as an unsigned long
    return minutes * 60 * 1000UL;
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

  bool isWarnStop(){

  }

  uint8_t getWellVolume(uint8_t tankLevel) {
    // also need to be used LevelSensorMainMax
    return tankLevel - LevelSensorWellMax;
  }

  uint8_t getMainVolume(uint8_t tankLevel) {
    // also need to be used LevelSensorMainMax
    return tankLevel - LevelSensorMainMax;
  }

  // Calculates how much the time should change based on actual performance
  // Returns a multiplier (e.g., 1.1 if pump is slow, 0.9 if pump is too fast)
  float calculateCorrection(uint8_t startLevel, uint8_t endLevel) {
    // Since 19cm is FULL and 110cm is EMPTY, rise = start - end
    int8_t actualRise = (int8_t)startLevel - (int8_t)endLevel;
    if (actualRise <= 0)
      return 1.0; // Avoid division by zero or negative

    return (float)TARGET_RISE_CM / (float)actualRise;
  }

  void setWorkBuffer(uint8_t work, uint8_t wait, uint8_t rise) {
    pumpBuffer[workIndex].work = work;
    pumpBuffer[workIndex].wait = wait;
    pumpBuffer[workIndex].rise = rise;
    if (++workIndex >= WORK_LEN)
      workIndex = 0;
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
    }

    //
    // Turn pump OFF by timeout of mode
    if (ctrlWell.isOn() && (getWellWorkTimer() >= msTimeToOff)) {
      ctrlWell.setOn(false);
      wellState.stop = millis();

      dbg(F("[CTRL] well to OFF"));
      dbgLn();

      read->stopWorkRead();
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
      wellState.start = millis();

      dbg(F("[CTRL] Well to ON"));
      dbgLn();

      ctrlWell.setOn(true);
    }
  }

public:
  void init(Rule *rl, Read *rd,  Buzz *bz) {
    read = rd;
    rule = rl;
  }
  virtual void exec() = 0;
  // Return flash string helper so implementations can return F("...")
  virtual const __FlashStringHelper *title() = 0;

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
    if (!wellCtr.on)
      return this->nextToOn - (getWellWorkTimer());

    return this->nextToOn;
  }

  /**
   * @brief Gets next timer OFF action for display
   */
  unsigned long getNextOff() {
    if (wellCtr.on)
      return this->nextToOff - (getWellWorkTimer());

    return this->nextToOff;
  }


};

#endif