#include <stdint.h>

#ifndef Rule_h
#define Rule_h

#include "HardwareSerial.h"
#include "Arduino.h"
#include "Glob.h"

class Rule {
private:
  //
  // Handle well state localy in order to detect human interaction.
  struct WellState {
    unsigned long time = 0;
    bool on = false;
  };
  WellState wellCtr;

  //
  // Used to handle the schedule time properly
  struct WellSchedule {
    uint16_t level = 0;
    unsigned long runtime;
    unsigned long stop;
    uint8_t mode = 0;
  };

  WellSchedule wellSch;



  //
  // Handles the state of "dayjob" for the well.
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
  bool isWarnTopTank = false;
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

    if (millis() < 6000)
      return;

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

    if (this->warnCase == "")
      return;
    dr->warn(MenuWarn_Rule, this->warnCase);
    this->warnCase = "";
  }

  /**
    * @brief Gets next timer ON action
    */
  unsigned long getNextOn() {
    if (!wellCtr.on)
      return this->nextToOn - (millis() - wellCtr.time);

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
  unsigned long calcMinutes(unsigned long minutes) {
    return minutes * 60 * 1000UL; // UL ensures the result is treated as an unsigned long
  }

  /**
    * Safe way to check for daytime
    * @return
    */
  bool checkDaytime() {
    //
    // Wrapping time class locally

    if (!time->isConn())
      return true;

    //
    // Check for daytime each minute
    if (spanLg.active())
      this->isDaytime = time->isDaytime(); //pass state for daytime locally


    return this->isDaytime;
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

    // Data is not ready
    if (!ctrlWell.isOn() && read->getWellLevel() < 19) {
      return;
    }

    //
    // Check well for daytime
    if (!this->checkDaytime()) {

      if (this->isWarnDaytime)
        return;

      this->isWarnDaytime = true; // flag to display only once
      setWarn(F("Not a daytime!  "));
      dbgLn(F("Warning: STOP /well/ It is not daytime!"));
      return;
    } else
      this->isWarnDaytime = false;

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
      beatWell(0); // Disables the led heartbeat
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
  void pumpWellSchedule(PumpSchedule schedule) {
    int16_t level = read->getWellLevel() + read->getMainLevel();
    uint8_t mode = modeWell->value();

    if (cmd.show(F("combo"), F("Shows combined level for scedule"))) {
      cmd.print(F("Combo level"), level);
    }

    if (!read->atNorm()) {
      if (!isWarnTopTank) {
        setWarn(F("Top tank missing"));
        isWarnTopTank = true;
        //
        // Reset warning message to be displayed again.
      } else if (spanLg.active()) {
        isWarnTopTank = false;
      }

      return;
    }

    //
    // Will wait for levels to be ready
    if (level < PumpScheduleCombinedMinLevel)
      return;

    //
    // Removeing the empty space from combined level
    level = level - PumpScheduleCombinedAbsence;
    if (level < 0)
      level = 0;

    //
    // Compare the lavels at "next" loop index (skips for loop each time)
    if (level == this->wellSch.level && this->wellSch.mode == mode) {
      if (cmd.show(F("schedule"))) {
        Serial.print(F("[Schedule] well work: "));
        Serial.print(this->wellSch.runtime);
        Serial.print(" stop: ");
        Serial.println(this->wellSch.stop);
      }

      pumpWell(wellSch.runtime, wellSch.stop);
      return;
    }


    this->wellSch.stop = schedule.stops[0]; // Sets lowest value as default

    for (uint8_t i = 0; i < schedule.intervals; ++i) {
      if (schedule.levels[i] > level) {

        dbg(F("[Schedule] lvl below "));
        dbg(schedule.levels[i]);
        dbg(F(" Raw "));
        dbg(level);
        dbg(F(" stop "));
        dbg(schedule.stops[i]);
        dbg(F(" run "));
        dbg(schedule.runtime);
        dbg(F(" index: "));
        dbgLn(i);

        this->wellSch.stop = schedule.stops[i];
      }
    }

    this->wellSch.runtime = schedule.runtime;
    this->wellSch.level = level;
    this->wellSch.mode = mode;
  }

  /**
    * Just turns on the pump to main
    */
  void pumpMain() {

    uint8_t main = read->getMainLevel();

    if (!ctrlMain.isOn() && !ctrlWell.isOn() && read->atNorm()) {
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
    if (levelMain < 19)
      return;

    // Mapping values from 20 to 95, like 20 is Full and 95 empty
    switch (modeMain->value()) {
    default:
    case 0: // Noting
      break;
    case 1: // Full
      if (levelMain > 32 && levelWell < 70)
        return pumpMain();
    case 2: // Half
      if (levelMain > 47 && levelWell < 55)
        return pumpMain();
    case 3: // Void
      if (levelMain > 75 && levelWell < 30)
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
      digitalWrite(pinLedBeat, !digitalRead(pinLedBeat)); // Toggle LED state
      if (ms == beatLedLast && ms != 0) {
        beatLed.repeat();
      }
    }
  }

  /**
  	*  The well pump will be turned on once a day when it has not been running.
  	*/
  void handleDayjob() {

    //
    // This function will be active only when clock is active.
    if (!time->isConn())
      return;

    //
    // Reset dayjob for the next day
    if (!time->isDaytime())
      wellHasDayjob = false;

    //
    // Pass the "On" pump state to dayjob state...
    if (wellCtr.on && !wellHasDayjob)
      wellHasDayjob = true;

    //
    // Skip dayjob when levels are not available.
    if (!read->atNorm())
      return;

    //
    // Turn on well for the dayjob
    if (!wellHasDayjob && WellDayjobHour == time->getHour()) {
      ctrlWell.setOn(true);
      wellHasDayjob = true;
    }
  }

  /**
      * Handles debug IO
      */
  void handleDebug() {

    if (cmd.show("timer:on", F("Shows work timer to next ON state.")))
      cmd.print("Time to on", getNextOn());


    if (cmd.show("timer:off", F("Shows work timer to next OFF state.")))
      cmd.print("Time to off", getNextOff());
  }
};


#endif
