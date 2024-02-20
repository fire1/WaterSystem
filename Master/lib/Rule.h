#include <stdint.h>
#include "Arduino.h"

#ifndef Rule_h
#define Rule_h

#include "Glob.h"

class Rule {
private:

  unsigned long wellTimer = 0;

  Read *read;
  Data *modeWell;
  Data *modeMain;
  Buzz *buzz;
  Time *time;


  AsyncDelay beatLed;
  uint16_t beatLedLast = 0;
  bool isDaytime = false;
  bool isAlarmOn = false;

public:
  Rule(Read *rd, Time *tm, Buzz *tn, Data *mdW,  Data *mdM)
    : read(rd), time(tm), buzz(tn), modeWell(mdW), modeMain(mdM), beatLed(500, AsyncDelay::MILLIS) {
  }

  void begin() {
    pinMode(pinLedBeat, OUTPUT);
    pinMode(pinWellPump, OUTPUT);
    pinMode(pinMainPump, OUTPUT);
  }

  void hark() {
    this->handleDataMode();
  }

private:

  unsigned long calcMinutes(unsigned int minutes) {
    return minutes * 60 * 1000UL;  // UL ensures the result is treated as an unsigned long
  }

  bool checkDaytime() {
    //
    // Wrapping time class locally
    if (time->isConn()) {
      //
      // Check for daytime each minutes
      if (spanMx.isActive())
        this->isDaytime = time->isDaytime();  //pass state for daytime locally

    } else {
      this->isDaytime = true;  // skip daytime check since there is no clock
    }

    return this->isDaytime;
  }

  //
  // Defines work amplitude for Pump1
  void pumpWell(uint8_t workMin, uint8_t stopMin) {
    /*
    if (!this->checkDaytime()) {
      //
      // Stop the system
      if (spanMx.isActive())  // every second display warning
        Serial.println(F("Warning: STOP It is not daytime!"));
      return;
    }
*/
    //
    // Stop when is full well tank
    if (ctrlWell.isOn() && LevelSensorBothMax >= read->getWellLevel()) {
      Serial.println(F("Warning: STOP Well tank is full!"));
      dbg(read->getWellLevel());
      dbg(F("cm / "));
      dbg(LevelSensorBothMax);
      dbg(F("cm "));
      dbgLn();
      ctrlWell.setOn(false);
      read->stopWorkRead();
      return;
    }

    //
    // Turn pump OFF by timeout of mode
    if (ctrlWell.isOn() && (millis() - wellTimer >= this->calcMinutes(workMin))) {
      ctrlWell.setOn(false);

      dbg(F("CTRL well /"));
      dbg(stopMin);
      dbg(F("min/ pump is Off "));
      dbgLn();

      wellTimer = millis();
      read->stopWorkRead();
    }
    //
    // Start level read before real start
    if (!ctrlWell.isOn() && (millis() - wellTimer >= (this->calcMinutes(stopMin) - (LevelRefreshTimeWork * 2 - 50)))) {
      if (spanLg.isActive()) {
        read->startWorkRead();
        buzz->alarm();

        dbg(F("Prepare levels /Well/ "));
        dbgLn();
      }
    }
    //
    // Turn pump on
    if (!ctrlWell.isOn() && (millis() - wellTimer >= this->calcMinutes(stopMin))) {
      this->isAlarmOn = false;
      dbg(F("CTRL well /"));
      dbg(workMin);
      dbg(F("min/ pump  is On "));
      dbgLn();

      wellTimer = millis();
      ctrlWell.setOn(true);
    }
  }

  //
  // Controlls pump1
  void handleDataMode() {
    // if (spanMd.isActive()) {
    //   uint8_t targetLevel = this->getTargetMode(this->modeWell, LevelSensorWellMin);
    //   Serial.println(targetLevel);
    // }
    switch (modeWell->value()) {
      default:
      case 0:
        // Noting
        beatWell(0);  // Disables the led heartbeat
        break;

      case 1:
        // Easy
        beatWell(1500);
        pumpWell(8, 180);
        break;

      case 2:
        // Fast
        beatWell(800);
        pumpWell(10, 60);
        break;

      case 3:
        // Now!
        beatWell(400);
        pumpWell(8, 25);
        break;
    }
  }

  void pumpMain() {

    uint8_t level = read->getMainLevel();
    //
    // Stop when Main is full
    if (ctrlMain.isOn() && LevelSensorBothMax >= level) {
      ctrlMain.setOn(false);
      read->stopWorkRead();
    }

    if (!ctrlMain.isOn()) {

      read->startWorkRead();

      dbg(F("CTRL /Main/ at level "));
      dbg(level);
      dbg(F("cm turn ON"));
      dbgLn();

      ctrlMain.setOn(true);
    }
  }

  void handleMainData() {
    uint8_t level = read->getMainLevel();
    if (level == 0) {
      if (spanLg.isActive()) {
        dbg(F("Warning: Main tank level not available!"));
        dbgLn();
      }
      return;
    }

    // Mapping values from 20 to 95, like 20 is Full and 95 empty
    switch (modeMain->value()) {
      default:
      case 0:  // Noting
        break;
      case 1:  // Full
        if (level > 30)
          return pumpMain();
      case 2:  // Half
        if (level > LevelSensorMainMin / 2)
          return pumpMain();
      case 3:  // Void
        if (level > 80)
          return pumpMain();
    }
  }




  uint8_t getTargetMode(Data *mode, const uint8_t levelMin) {
    return map(mode->value(), 0, mode->length(), LevelSensorBothMax, levelMin);
  }

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
};



#endif
