#include "HardwareSerial.h"
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

  struct Temperature {
    float summary = 0;
    int index = 0;
    float mean;
  } TempRead;

  float heat = 0;
  uint8_t fan = 0;


  AsyncDelay beatLed;
  uint16_t beatLedLast = 0;
  bool isDaytime = false;
  bool isAlarmOn = false;

public:
  Rule(Read *rd, Time *tm, Buzz *tn, Data *mdW, Data *mdM)
    : read(rd), time(tm), buzz(tn), modeWell(mdW), modeMain(mdM), beatLed(500, AsyncDelay::MILLIS) {
  }

  void begin() {

    pinMode(pinLedBeat, OUTPUT);
    pinMode(pinWellPump, OUTPUT);
    pinMode(pinMainPump, OUTPUT);
    pinMode(pinTmpRss, INPUT);
    pinMode(pinFanRss, OUTPUT);

    analogWrite(pinFanRss, 255);
  }

  void hark() {
    this->handleWellMode();
    this->handleMainMode();

    if (spanSm.isActive())
      this->readTemp();

    this->handleHeat();
  }

  int getHeat() {
    return this->heat;
  }


  uint8_t getFanSpeed() {
    return this->fan;
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

#ifdef CHECK_DAYTIME
    if (!this->checkDaytime()) {
      //
      // Stop the system
      if (spanMx.isActive())  // every second display warning
        Serial.println(F("Warning: STOP /well/ It is not daytime!"));
      return;
    }
#endif

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
    if (!ctrlWell.isOn() && !ctrlWell.isOn() && (millis() - wellTimer >= (this->calcMinutes(stopMin) - (LevelRefreshTimeWork * 2 - 50)))) {
      if (spanLg.isActive()) {
        read->startWorkRead();
        buzz->alarm();

        dbg(F("Prepare levels /Well/ "));
        dbgLn();
      }
    }
    //
    // Turn pump on
    if (!ctrlWell.isOn() && !ctrlWell.isOn() && (millis() - wellTimer >= this->calcMinutes(stopMin))) {
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
  void handleWellMode() {
    // if (spanMd.isActive()) {
    //   uint8_t targetLevel = this->getTargetMode(this->modeWell, LevelSensorWellMin);
    //   Serial.println(targetLevel);
    // }

    //
    // Stop when Main is full
    if (ctrlMain.isOn() && LevelSensorBothMax >= read->getMainLevel()) {
      ctrlMain.setOn(false);

      dbgLn(F("CTRL /Main/ turn off the pump"));
      read->stopWorkRead();
    }

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

#ifdef CHECK_DAYTIME
    if (!this->checkDaytime()) {
      //
      // Stop the system
      if (spanMx.isActive())  // every second display warning
        Serial.println(F("Warning: STOP /main/ It is not daytime!"));
      return;
    }
#endif

    uint8_t level = read->getMainLevel();

    if (!ctrlMain.isOn() && !ctrlWell.isOn()) {

      read->startWorkRead();

      dbg(F("CTRL /Main/ at level "));
      dbg(level);
      dbg(F("cm turn ON"));
      dbgLn();

      ctrlMain.setOn(true);
    }
  }

  void handleMainMode() {
    uint8_t levelMain = read->getMainLevel();
    uint8_t levelWell = read->getWellLevel();

    //
    // Stop this function when sensor is not avialable
    if (levelMain == 0)
      return;

    // Mapping values from 20 to 95, like 20 is Full and 95 empty
    switch (modeMain->value()) {
      default:
      case 0:  // Noting
        break;
      case 1:  // Full
        if (levelMain > 32 && levelWell < 60)
          return pumpMain();
      case 2:  // Half
        if (levelMain > 47 && levelWell < 40)
          return pumpMain();
      case 3:  // Void
        if (levelMain > 80 && levelWell < 30)
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

  void readTemp() {

    if (TempRead.index < TempSampleReads) {
      TempRead.summary += analogRead(pinTmpRss);
      TempRead.index++;
    }

    if (TempRead.index >= TempSampleReads) {
      TempRead.mean = TempRead.summary / TempRead.index;
      TempRead.index = 1;
      TempRead.summary = TempRead.mean;

      //
      // Calculation based on
      //  https://solarduino.com/how-to-use-ntc-thermistor-to-measure-temperature/
      float R2 = (TempPullupResistor * TempRead.mean) / (1023 - TempRead.mean);
      float a, b, c, d, e = 2.718281828;

      a = 1 / TempTermistorT1Val;
      b = log10(TempTermistorValue / R2);
      c = b / log10(e);
      d = c / TempVoltageBValue;
      float T2 = 1 / (a - d);
      this->heat = T2 - 273.15;  // from Kelvin to Celsius
    }
  }
  //
  // Handles the overheating protection
  void handleHeat() {
    if (this->heat > stopMaxTemp) {
      if (spanLg.isActive())
        Serial.println(F("Warning: Overeating temperature for  SSR!"));
        
      ctrlMain.setOn(false);
      ctrlWell.setOn(false);

      if (spanMd.isActive())
        buzz->alarm();
    }

    if (spanSm.isActive()) {
      int pwm = map(this->heat, 35, stopMaxTemp, 100, 265);  // map temp over pwm with thresholds

      //
      // Set on/off points
      if (pwm > 255) pwm = 255;
      if (pwm < 100) pwm = 0;
      /*
      Serial.print("t:");
      Serial.print(this->heat);
      Serial.print(" pwm ");
      Serial.println(pwm);
*/
      this->fan = pwm;
      analogWrite(pinFanRss, this->fan);
    }
  }
};



#endif
