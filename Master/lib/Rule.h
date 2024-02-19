
#ifndef Rule_h
#define Rule_h

#include "Glob.h"

class Rule {
private:

  unsigned long wellTimer = 0;
  Data *mode;
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
  Rule(Read *rd, Time *tm, Buzz *tn, Data *md, Data *p1, Data *p2)
    : read(rd), time(tm), buzz(tn), mode(md), modeWell(p1), modeMain(p2), beatLed(500, AsyncDelay::MILLIS) {
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

    if (!this->checkDaytime()) {
      //
      // Stop the system
      if (spanMx.isActive())  // every second display warning
        Serial.println(F("Warning: It is not daytime!"));
      return;
    }

    //
    // Stop when is full well tank
    if (read->getWellLevel() >= LevelSensorWellMin) {
      if (spanMx.isActive())  // every second display warning
        Serial.println(F("Warning: Well tank is full!"));

      ctrlWell.setOn(false);
      read->stopWorkRead();
      return;
    }

    //
    // Turn pump OFF by timeout of mode
    if (ctrlWell.isOn() && (millis() - wellTimer >= this->calcMinutes(workMin))) {

      dbg(F("CTRL well /"));
      dbg(stopMin);
      dbg(F("min/ pump is Off "));
      dbgLn();

      wellTimer = millis();

      ctrlWell.setOn(false);
      read->stopWorkRead();
    }
    //
    // Start level read before real start
    if (!ctrlWell.isOn() && (millis() - wellTimer >= (this->calcMinutes(stopMin) - (LevelRefreshTimeWork * 2 - 50)))) {

      if (spanLg.isActive()) {

        read->startWorkRead();
        buzz->alarm();

        dbg(F("Prepare levels /well/ "));
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

    switch (mode->value()) {
      default:
      case 0:
        // noting
        beatWell(0);                     // Disables the led heartbeat
        break;

      case 1:
        // Easy
        pumpWell(8, 180);
        beatWell(1500);
        break;

      case 2:
        // Fast
        pumpWell(10, 60);
        beatWell(800);
        break;

      case 3:
        // Now!
        pumpWell(8, 25);
        // pumpWell(1, 4); // quick test mode
        beatWell(400);
        break;
    }
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
