#include <stdint.h>

#ifndef Rule_h
#define Rule_h

#include "HardwareSerial.h"
#include "Arduino.h"
#include "Glob.h"

class Rule {
private:

  struct WellState {
    unsigned long time = 0;
    bool on = false;
  };

  WellState wellCtr;

  bool wellHasDayjob = false;


  Read *read;
  Data *modeWell;
  Data *modeMain;
  Buzz *buzz;
  Time *time;

  AsyncDelay beatLed;
  uint16_t beatLedLast = 0;
  bool isDaytime = true;
  bool isWarnDaytime = false;
  uint32_t timePrepareTurnOn;
  unsigned long nextToOn = 0;
  unsigned long nextToOff = 0;

  String warnCase = "";

public:

  Rule(Read *rd, Time *tm, Buzz *tn, Data *mdW, Data *mdM)
    : read(rd), time(tm), buzz(tn), modeWell(mdW), modeMain(mdM), beatLed(500, AsyncDelay::MILLIS) {
  }

  void begin() {

    pinMode(pinLedBeat, OUTPUT);
    pinMode(pinWellPump, OUTPUT);
    pinMode(pinMainPump, OUTPUT);

    this->timePrepareTurnOn = LevelRefreshTimeWork * LevelSensorReads - 50;
  }

  /**
    * @brief Listen for environment changes.
    * 
    */
  void hark() {
    this->handleDebug();
    this->handleWellMode();
    this->handleMainMode();
    this->handleMainStop();
    this->handleDayjob();
  }

  /**
    * Display warning message
    * @param dr
    */
  void warn(DrawInterface *dr) {

    if (this->warnCase == "") return;
    dr->warn(MenuWarn_Rule, this->warnCase);
    this->warnCase = "";
  }

  /**
    * @brief Gets next timer ON action
    */
  unsigned long getNextOn() {
    if (!wellCtr.on)
      return this->nextToOn - (millis() - this->wellCtr.time);

    return this->nextToOn;
  }

  /**
    * @brief Gets next timer OFF action
    */
  unsigned long getNextOff() {
    if (wellCtr.on)
      return this->nextToOff - (millis() - this->wellCtr.time);

    return this->nextToOff;
  }

private:

  /**
    * Sets warning massage to be displayed.
    * @param msg
    */
  void setWarn(String msg) {
    this->warnCase = msg;
  }

  /**
    * Converts minutes to millis
    * @param minutes
    * @return
    */
  unsigned long calcMinutes(unsigned int minutes) {
    return minutes * 60 * 1000UL;  // UL ensures the result is treated as an unsigned long
  }

  /**
    * Safe way to check for daytime
    * @return
    */
  bool checkDaytime() {
    //
    // Wrapping time class locally

    if (!time->isConn()) return true;

    //
    // Check for daytime each minute
    if (spanLg.active())
      this->isDaytime = time->isDaytime();  //pass state for daytime locally


    return this->isDaytime;
  }

  /**
  * Pumping well amplitude
  * @param workMin
  * @param stopMin
  */
  void pumpWell(uint8_t workMin, uint16_t stopMin) {

    unsigned long msTimeToOff = this->calcMinutes(workMin);
    unsigned long msTimeToOn = this->calcMinutes(stopMin);

    this->nextToOff = msTimeToOff;
    this->nextToOn = msTimeToOn;

    //
    // Reset the clock when pump is manually run
    if (ctrlWell.isOn() != wellCtr.on) {
      wellCtr.on = ctrlWell.isOn();
      wellCtr.time = millis();
    }

    //
    // Turn pump OFF by timeout of mode
    if (ctrlWell.isOn() && (millis() - wellCtr.time >= msTimeToOff)) {
      ctrlWell.setOn(false);
      wellCtr.time = millis();

      dbg(F("[CTRL] well to OFF"));
      dbgLn();

      read->stopWorkRead();
    }

    //
    // Ignore next code when tank is full
    if (!ctrlWell.isOn() && LevelSensorBothMax >= read->getWellLevel()) {
      return;
    }

    //
    // Check well for daytime
    if (!this->checkDaytime()) {

      if (this->isWarnDaytime) return;

      this->isWarnDaytime = true;  // flag to display only once
      setWarn(F("Not a daytime!  "));
      dbgLn(F("Warning: STOP /well/ It is not daytime!"));
      return;
    } else this->isWarnDaytime = false;

    //
    // Prepare, read levels before start
    if (!ctrlWell.isOn() && !ctrlWell.isOn() && (millis() - wellCtr.time >= (msTimeToOn - timePrepareTurnOn))) {
      if (spanLg.active()) {
        read->startWorkRead();
        buzz->alarm();
        dbg(F("[CTRL] Well prepare"));
        dbgLn();
      }
    }

    //
    // Turn the pump on
    if (!ctrlMain.isOn() && !ctrlWell.isOn() && (millis() - wellCtr.time >= msTimeToOn)) {
      wellCtr.time = millis();

      dbg(F("[CTRL] Well to ON"));
      dbgLn();

      ctrlWell.setOn(true);
    }
  }

  //
  // Controls pump1
  void handleWellMode() {

    switch (modeWell->value()) {
      default:
      case 0:
        // Noting
        beatWell(0);  // Disables the led heartbeat
        break;

      case 1:
        // Easy
        beatWell(2400);
        pumpWellSchedule(ScheduleWellEasy);

        break;

      case 2:
        // Fast
        beatWell(1200);
        pumpWellSchedule(ScheduleWellFast);
        break;

      case 3:
        // Now!
        beatWell(400);
        pumpWell(WellPumpDefaultRuntime, WellPumpDefaultBreaktime);
        //pumpWell(1, 2);
        break;
    }
  }


  /**
    * Pump schedule for the well mode
    * @param schedule
    */
  void pumpWellSchedule(const PumpSchedule &schedule) {
    uint8_t comb = read->getWellLevel() + read->getMainLevel();
    uint16_t stop = 180;  // just defining some foo value

    for (int i = 0; i < schedule.stops; ++i) {
      if (schedule.levels[i] > comb)
        stop = schedule.stops[i];
    }

    if (cmd.show(F("schedule"))) {
      cmd.print(F("[Schedule] well work:"), schedule.runtime);
      cmd.print(F("[Schedule] well stop:"), stop);
    }

    pumpWell(schedule.runtime, stop);
  }

  /**
    * Just turns on the pump to main
    */
  void pumpMain() {

    uint8_t main = read->getMainLevel();

    if (!ctrlMain.isOn() && !ctrlWell.isOn()) {
      read->startWorkRead();

      dbg(F("CTRL /Main/ at level "));
      dbg(main);
      dbg(F("cm turn ON"));
      dbgLn();

      ctrlMain.setOn(true);
    }
  }

  void handleMainMode() {
    uint8_t levelMain = read->getMainLevel();
    uint8_t levelWell = read->getWellLevel();
    //
    // Stop this function when sensor is not available
    if (levelMain == 0)
      return;

    // Mapping values from 20 to 95, like 20 is Full and 95 empty
    switch (modeMain->value()) {
      default:
      case 0:  // Noting
        break;
      case 1:  // Full
        if (levelMain > 32 && levelWell < 70)
          return pumpMain();
      case 2:  // Half
        if (levelMain > 45 && levelWell < 55)
          return pumpMain();
      case 3:  // Void
        if (levelMain > 75 && levelWell < 40)
          return pumpMain();
    }
  }

  /**
    * Monitors the levels and turn off on Low or Full tank state
    */
  void handleMainStop() {
    uint8_t levelMain = read->getMainLevel();
    uint8_t levelWell = read->getWellLevel();

    //
    // After 5min clear terminate.
    if (spanLg.active()) {
      ctrlMain.clearTerminate();
      ctrlWell.clearTerminate();
    }


    // MAIN
    // Stop Main when Main is full
    if (ctrlMain.isOn() && LevelSensorBothMax >= levelMain) {
      ctrlMain.setOn(false);
      ctrlMain.terminate();

      setWarn(F(" TOP tank FULL! "));
      dbgLn(F("CTRL /Main/ turn off  /TOP FULL/"));

      read->stopWorkRead();
    }
    // MAIN
    // Stop when well is empty
    if (ctrlMain.isOn() && LevelSensorStopWell <= levelWell) {
      ctrlMain.setOn(false);
      ctrlMain.terminate();

      setWarn(F(" WELL tank VOID!"));
      dbgLn(F("CTRL /Main/ turn off /Well empty/"));

      read->stopWorkRead();
    }
  }


  /**
    * Led beet for indicating the modes
    * @param ms
    */
  void beatWell(int ms) {

    if (ms == 0) {
      //
      // Turn led off
      digitalWrite(pinLedBeat, HIGH);
      return;
    }

    if (ms != beatLedLast) {
      beatLed.start(ms, AsyncDelay::MILLIS);
      beatLedLast = ms;
    }
    if (beatLed.isExpired()) {
      digitalWrite(pinLedBeat, !digitalRead(pinLedBeat));  // Toggle LED state
      if (ms == beatLedLast && ms != 0) {
        beatLed.repeat();
      }
    }
  }
  void handleDayjob() {
    if (!time->isConn()) return;  // clock is not connected...

    if (!wellHasDayjob && WellDayjobHour == time->getHour()) {
      ctrlWell.setOn(true);
      wellHasDayjob = true;
    }
    // Reset dayjob
    if (!time->isDaytime()) wellHasDayjob = false;
  }

  /**
    * Handles debug IO
    */
  void handleDebug() {
  }
};


#endif
