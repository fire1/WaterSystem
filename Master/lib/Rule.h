#ifndef Rule_h
#define Rule_h

#include "Arduino.h"
#include "Glob.h"
#include "HardwareSerial.h"

class Rule {
private:

  //
  // Handle well state localy in order to detect human interaction.
  struct WellState {
    unsigned long time = 0;
    bool on = false;
  };
  WellState wellCtr;

  unsigned long mainStartTime = 0;
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
  bool isLowTemp = false;
  bool isWarnDaytime = false;
  bool isWarnLowTemp = false;
  bool isWarnTopTank = false;
  uint32_t timePrepareTurnOn;
  unsigned long nextToOn = 0;
  unsigned long nextToOff = 0;

  String warnCase = "";

  unsigned long getWellWorkTimer() { return millis() - wellCtr.time; }

public:
  Rule(Read *rd, Time *tm, Buzz *tn, Data *mdW, Data *mdM)
      : read(rd), time(tm), buzz(tn), modeWell(mdW), modeMain(mdM),
        beatLed(500, AsyncDelay::MILLIS) {}

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
      

    // Wait a while...
    // NOTE: 
    // This "wait" depends strongly on collected data from sensors, 
    //  so more time will mean more accurate data before deciding to run pumps (handlers).
    if (millis() < RULE_START_WAIT) {
      isWarnStop();
      return;
    }

    this->handleWellMode();
    this->handleMainMode();

    this->handleMainStop();

    //
    // Options for detecting an overtime
    this->handleWellOvertime();
    this->handleMainOvertime();
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

private:
  /**
   * Sets warning massage to be displayed.
   * @param msg
   */
  void setWarn(String msg) { this->warnCase = msg; }

  /**
   * Converts minutes to millis
   * @param minutes
   * @return
   */
  unsigned long calcMinutes(unsigned long minutes) {
    return minutes * 60 *
           1000UL; // UL ensures the result is treated as an unsigned long
  }

  /**
   * Safe/local way to check for daytime
   * @return
   */
  bool checkDaytime() {
    //
    // Wrapping time class locally

    if (!time->isConn())
      return true;

    //
    // Check for daytime each minute
    if (spanLg.active() || millis() < RULE_START_WAIT)
      this->isDaytime = time->isDaytime(); // pass state for daytime locally

    //
    // Returns last resolve state
    return this->isDaytime;
  }

  /**
   * When temperature is too low to pump will return true
   */
  bool checkLowTemp() {

    //
    // Verify the clock is connected in order to check the temperature.
    if (!time->isConn()) {
      this->isLowTemp = false; // Reset back to default
      return true;
    }

    if (spanLg.active()) {

      if (time->getTemp() < OPT_PROTECT_COLD) {
        this->isLowTemp = true; // it is too cold to run....
        //
        //  last runtime is below 2 hours, (pump head still hot).
        if ((getWellWorkTimer()) < 7200000)
          this->isLowTemp = false;

      } else
        this->isLowTemp = false;
    }

    //
    // Returns last resolve state
    return this->isLowTemp;
  }

  /**
   * Resolve well stop from several warnings
   */
  bool isWarnStop() {

    //
    // Check well for daytime
#ifdef OPT_DAYTIME_WELL
    if (!this->checkDaytime()) {

      if (this->isWarnDaytime)
        return true;

      this->isWarnDaytime = true; // flag to display only once
      setWarn(F("Not a daytime!  "));
      dbgLn(F("Warning: STOP /well/ It is not daytime!"));
      return true;
    } else
      // Reset back to default
      this->isWarnDaytime = false;
#endif

      //
      // Check well for low temp
#ifdef OPT_PROTECT_COLD
    if (this->checkLowTemp()) {

      if (this->isWarnLowTemp)
        return true;

      this->isWarnLowTemp = true; // flag to display only once
      setWarn(F("Too cold to run!"));
      dbgLn(F("Warning: STOP /well/ Temperature too low!"));
      return true;
    } else
      // Reset back to default
      this->isWarnLowTemp = false;

#endif

    return false; // default state of the function
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
    if (ctrlWell.isOn() && (getWellWorkTimer() >= msTimeToOff)) {
      ctrlWell.setOn(false);
      wellCtr.time = millis();

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
        read->getWellLevel() < LevellSensorBareMax(LevelSensorWellMax)) {
      return;
    }

    if (isWarnStop())
      return;

    //
    // Prepare, read levels before start
    if (!ctrlWell.isOn() && !ctrlWell.isOn() &&
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
      wellCtr.time = millis();

      dbg(F("[CTRL] Well to ON"));
      dbgLn();

      ctrlWell.setOn(true);
    }
  }

  //
  // Controls well pump
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
      handleDayjob();
      break;

    case 2:
      // Fast
      beatWell(1200);
      pumpWellSchedule(ScheduleWellFast);
      handleDayjob();
      break;

    case 3:
      // Now!
      beatWell(400);
      pumpWell(WellPumpDefaultRuntime, WellPumpDefaultBreaktime);
      // pumpWell(1, 2);
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

    if (cmd.show(F("combo"), F("Shows combined level for schedule"))) {
      cmd.print(F("Combo level"), level);
    }

    //
    // Break the function when top tank is missing.
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
    // Will wait for levels to be ready.
    if (level < PumpScheduleCombinedMinLevel)
      return;

    //
    // Removing the empty space from combined level.
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

    //
    // Sets lowest value as default/backup value.
    this->wellSch.stop = schedule.stops[0];

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
   * Starts the pump for main tank.
   */
  void pumpMain() {

    uint8_t main = read->getMainLevel();

    if (ctrlMain.isFailure()) {
      if (spanSm.active())
        dbgLn(F("[CTRL] /Main/ has failure!"));
      return;
    }

    if (!ctrlMain.isOn() && !ctrlWell.isOn() && read->atNorm()) {
      read->startWorkRead();

      dbg(F("[CTRL] /Main/ at level "));
      dbg(main);
      dbg(F("cm turn ON"));
      dbgLn();

      ctrlMain.setOn(true);
    }
  }

  /**
   * Main pump work setup
   */
  void handleMainMode() {
    uint8_t levelMain = read->getMainLevel();
    uint8_t levelWell = read->getWellLevel();
    //
    // Stop this function when sensor is not available
    if (levelMain < LevellSensorBareMax(LevelSensorMainMax))
      return;

    //
    // Run the pump when it's daytime
    if (!this->isDaytime) // todo, make also a freezing temperature check
      return;

    // Mapping values from 20 to 95, like 20 is Full and 95 empty
    switch (modeMain->value()) {
    default:
    case 0: // Do noting
      break;
    case 1: // Full
      if (levelMain > 34 && levelWell < 70)
        return pumpMain();
    case 2: // Half
      if (levelMain > 52 && levelWell < 55)
        return pumpMain();
    case 3: // Void
      if (levelMain > 78 && levelWell < 30)
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
    if (ctrlMain.isOn() && LevelSensorMainMax >= levelMain) {
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
   * @brief Function to protect from overtime for well pump
   *
   */
  void handleWellOvertime() {
#ifdef OPT_WELL_OVERTIME
    if (ctrlWell.isOn() && getWellWorkTimer() > OPT_WELL_OVERTIME) {
      ctrlWell.setOn(false);
      ctrlWell.failure();
      setWarn(F("Well overtime!  "));
      dbgLn(F("Warning: STOP /well/ Overtime work detected!"));
    }
#endif
  }
  /**
   * @brief Function to protect from overtime for main pump
   *
   */
  void handleMainOvertime() {
#ifdef OPT_MAIN_OVERTIME

    //
    // Wait for main pump to start...
    if (!ctrlMain.isOn()) {
      mainStartTime = 0;
      return;
    }
    //
    // Mark starting point of the work time for main pump.
    if (mainStartTime == 0) {
      mainStartTime = millis();
      return;
    }

    if (millis() - mainStartTime > OPT_MAIN_OVERTIME) {
      ctrlMain.setOn(false);
      ctrlMain.failure();
      mainStartTime = 0;
      setWarn(F("Main overtime!  "));
      dbgLn(F("Warning: STOP /main/ Overtime work detected!"));
    }

#endif
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
#ifdef OPT_DAYJOB_WELL
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
    if (!wellHasDayjob && OPT_DAYJOB_WELL == time->getHour()) {
      ctrlWell.setOn(true);
      wellHasDayjob = true;
    }

#endif
  }

  void handleInactivityDays() {
#ifdef OPT_DAYS_JOB_WELL
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
    // Turn on well when well controll timer is above defined days (days of
    // inactivity).
    if (!wellHasDayjob && !ctrlWell.isOn() &&
        wellCtr.time > DAYS_TO_MILLIS(OPT_DAYS_JOB_WELL)) {
      ctrlWell.setOn(true);
      wellHasDayjob = true;
    }

#endif
  }

  /**
   * Handles debug IO
   */
  void handleDebug() {

    if (cmd.show(F("timer:on"), F("Shows work timer to next ON state.")))
      cmd.print("Time to on", this->getNextOn());

    if (cmd.show(F("timer:off"), F("Shows work timer to next OFF state.")))
      cmd.print("Time to off", getNextOff());

    int tmpTime = 0;
    if (cmd.set(F("timer:on"), tmpTime, F("Overwrite to \"on\" timer."))) {
      this->nextToOn = tmpTime;
    }

    if (cmd.set("timer:off", tmpTime, F("Overwrite to \"off\" timer."))) {
      this->nextToOff = tmpTime;
    }
  }
};

#endif
