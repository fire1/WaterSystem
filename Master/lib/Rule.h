#ifndef Rule_h
#define Rule_h

#include "Arduino.h"
#include "Glob.h"
#include "HardwareSerial.h"
#include "Mode.h"

class Rule {
private:
  Mode *activeMode = nullptr;
  uint8_t activeModeId = 255; // Invalid mode ID in order to apply at start
  //
  // Handle well state localy in order to detect human interaction.
  struct WellState {
    unsigned long time = 0;
    bool on = false;
  };
  WellState wellCtr;

  unsigned long mainStartTime = 0;
  bool mainTimerArmed = false;
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
  bool isInit = true;
  bool isDaytime = true;
  bool isLowTemp = false;
  bool isWarnDaytime = false;
  bool isWarnLowTemp = false;
  bool isWarnAtNorm = false;
  uint32_t timePrepareTurnOn;
  unsigned long nextToOn = 0;
  unsigned long nextToOff = 0;

  String warnCase = "";

  unsigned long getWellWorkTimer() {
    if (!this->activeMode)
      return 0;

    return this->activeMode->getWellTimer();
  }

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
  void loop() {
    this->handleDebug();

    // Wait a while...
    // NOTE:
    // This "wait" depends strongly on collected data from sensors,
    //  so more time will mean more accurate data before deciding to run pumps
    //  (handlers).
    if (millis() < RULE_START_WAIT) {
      return;
    }

    applyMode(modeWell->value());

    //
    // Global Safety Layers
    this->handleMutualExclusion();
    this->handleSafety();

    //
    // Options for detecting an overtime
    this->handleWellOvertime();
    this->handleMainOvertime();
    isInit = false;
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
    if (activeMode == nullptr)
      return 0;
    return this->activeMode->getNextOn();
  }

  /**
   * @brief Gets next timer OFF action for display
   */
  unsigned long getNextOff() {
    if (activeMode == nullptr)
      return 0;
    return this->activeMode->getNextOff();
  }

private:
  /**
   * @brief Activates selected mode
   *
   * @param id
   */
  void applyMode(uint8_t id) {

    if (id >= MODE_COUNT || modes[id] == nullptr) {
      activeMode = nullptr;
      return;
    }

    if (activeModeId != id) {
      activeMode = modes[id];
      activeModeId = id;
      activeMode->init(read, buzz, modeMain, time);
    }

    if (activeMode == nullptr) {
      return;
    }

    activeMode->exec();

    // Display mode warning if any
    String warnMsg = activeMode->getWarnMessage();
    if (warnMsg.length() > 0)
      this->setWarn(warnMsg);

    activeMode->debug();
  }

  /**
   * @brief Critical fail-safe monitoring.
   * Protects hardware from explosion (overfill) or dry-run damage.
   */
  void handleSafety() {
    uint8_t levelMain = read->getMainLevel();
    uint8_t levelWell = read->getWellLevel();

    // FAIL-SAFE: Overfill Protection (Prevent tank explosion)
    if (ctrlMain.isOn() && levelMain <= LevelSensorMainMax) {
      ctrlMain.setOn(false);
      ctrlMain.terminate();
      setWarn(F("OVERFILL LIMIT! "));
      dbgLn(F("[SAFETY] Main Tank Overfill detected! Emergency Stop."));
      buzz->alarm();
    }

    // FAIL-SAFE: Well Dry-Run Protection
    if (ctrlMain.isOn() && levelWell >= LevelSensorStopWell) {
      ctrlMain.setOn(false);
      ctrlMain.terminate();
      setWarn(F("WELL DRY-RUN!  "));
      dbgLn(F("[SAFETY] Well Tank empty during transfer! Emergency Stop."));
    }
  }

  /**
   * @brief Ensures both pumps are never ON at the same time.
   * If both are detected ON, both are stopped for safety and a warning is set.
   */
  void handleMutualExclusion() {
    if (ctrlWell.isOn() && ctrlMain.isOn()) {
      dbgLn(F("[SAFETY] Mutual Exclusion Violation! Stopping both pumps."));
      ctrlWell.setOn(false);
      ctrlMain.setOn(false);
      setWarn(F("PUMP CONFLICT!  "));
      buzz->alarm();
    }
  }

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

  /** @deprecated Moved to Mode
   * Controls well pump
   * @brief Function to protect from overtime for well pump
   *
   */
  void handleWellOvertime() {
#ifdef OPT_WELL_OVERTIME
    // 1sec to try to avoid millis overflow issue
    if (ctrlWell.isOn() && activeMode != nullptr &&
        overtime::wellExceeded(activeMode->getWellStartTimer())) {
      ctrlWell.setOn(false);
      ctrlWell.failure();
      setWarn(F("Well overtime!  "));
      dbgLn(F("Warning: STOP /well/ Overtime work detected!"));
    }

    //
    // Debug info
    if (cmd.show(F("well:overtime"), F("Shows well pump work time."))) {
      dbg(F("Well work time: "));
      dbg(activeMode->getWellStartTimer());
      dbg(F(" overtime "));
      dbg(OPT_WELL_OVERTIME);
      dbg(F(" Well overtime check: "));
      dbg(activeMode->getWellStartTimer() > OPT_WELL_OVERTIME);
      dbgLn();
    }

#endif
  }

  /**
   * @brief Function to protect from overtime for main pump
   * Final fallback if sensors fail.
   */
  void handleMainOvertime() {
#ifdef OPT_MAIN_OVERTIME
    // Wait for main pump to start...
    if (!ctrlMain.isOn()) {
      mainStartTime = 0;
      mainTimerArmed = false;
      return;
    }
    if (!mainTimerArmed) {
      mainStartTime = millis();
      mainTimerArmed = true;
      return;
    }

    if (overtime::mainExceeded(millis() - mainStartTime)) {
      ctrlMain.setOn(false);
      ctrlMain.failure();
      mainStartTime = 0;
      mainTimerArmed = false;
      setWarn(F("Main overtime!  "));
      dbgLn(F("Warning: STOP /main/ Overtime work detected (Fallback)!"));
      buzz->alarm();
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

    if (cmd.set(F("timer:off"), tmpTime, F("Overwrite to \"off\" timer."))) {
      this->nextToOff = tmpTime;
    }

    if (cmd.set(F("well:pump"), tmpTime,
                F("Set well pump state, 0=off, 1=on."))) {
      if (tmpTime == 0)
        ctrlWell.setOn(false);
      else
        ctrlWell.setOn(true);
    }
  }
};

#endif
