#include "HardwareSerial.h"
#ifndef Uart_h
#define Uart_h

#include "Glob.h"

//
// OneWire setup (Power/Serial)
// The OneWire Serial is desined to provide a power and
//  communication from the serial slave sensor.
SoftwareSerial com(pinMainRx, -1);

// https://forum.arduino.cc/t/jsn-sr04t-2-0/456255/10








class Rule {
private:


  unsigned long wellTimer = 0;
  Data *mode;
  Data *pump1;
  Data *pump2;
  Tone *sound;
  AsyncDelay beatLed;
  AsyncDelay refreshLevels;
  uint16_t beatLedLast = 0;

  struct LevelSensorAverage {
    uint8_t index = 0;
    uint32_t average = 0;
    bool done = false;
  };

  //
  // TODO make average read of sensors
  LevelSensorAverage
    sensorWell,
    sensorMain;

  uint8_t well;
  uint8_t main;



  unsigned long calcMinutes(unsigned int minutes) {
    return minutes * 60 * 1000UL;  // UL ensures the result is treated as an unsigned long
  }

  //
  // Defines work amplitude for Pump1
  void pumpWell(uint8_t workMin, uint8_t stopMin) {

#ifdef UseRtl
    if (!isDaytime()) {
      //
      // Stop the system
      Serial.println(F("It is not daytime!"));
      return;
    }
#endif


    //
    // TODO check levels for running the pumps
    //

    if (ctrlWell.isOn() && (millis() - wellTimer >= this->calcMinutes(workMin))) {

      dbg(F("CTRL well /"));
      dbg(stopMin);
      dbg(F("min/ pump is Off "));
      dbgLn();

      wellTimer = millis();
      ctrlWell.setOn(false);
    }

    if (!ctrlWell.isOn() && (millis() - wellTimer >= this->calcMinutes(stopMin))) {

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
  void controllWellPump() {

    switch (mode->value()) {
      default:
      case 0:
        // noting
        digitalWrite(pinWellPump, LOW); // Force disable immediately!
        ctrlWell.setOn(false); // Disables the controll of the well pump
        beatWell(0); // Disables the led heartbeat 
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



  //
  // Read well tank
  void readWell() {

    if (this->sensorWell.index < LevelSensorReads) {

      digitalWrite(pinWellSend, LOW);
      delayMicroseconds(2);
      digitalWrite(pinWellSend, HIGH);
      delayMicroseconds(10);
      digitalWrite(pinWellSend, LOW);

      unsigned long duration = pulseIn(pinWellEcho, HIGH, 130000);  // read pulse with timeout for ~150cm

      // Check for timeout
      if (duration == 0) {
        //     Serial.println("Timeout error: Sensor WELL reading exceeds range");
        // Handle timeout (e.g., set distance to maximum or other logic)
        return;
      }

      float distance = (duration * .0343) / 2;


      // Store distance reading
      sensorWell.average += distance;
      sensorWell.index++;  // Increment index for next reading

    } else {
      this->well = sensorWell.average / sensorWell.index;
      sensorWell.index = 0;
      sensorWell.average = 0;
      sensorWell.done = true;
      dbg(F("Well tank average value: "));
      dbg(this->well);
      dbgLn();
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
  //
  // Read Main tank using slave MCU
  void readMain() {
    if (sensorMain.index < LevelSensorReads) {
      if (digitalRead(pinMainPower)) {
        digitalWrite(pinLed, LOW);

        // Check for available data and read value
        if (com.available()) {
          digitalWrite(pinLed, HIGH);

          uint8_t currentValue = com.read();

          // Store value in the sensorMain struct
          sensorMain.average += currentValue;
          sensorMain.index++;  // Increment index for next reading

          dbg(F("RX: "));
          dbgLn(currentValue);  // Print current value for debugging
          digitalWrite(pinLed, LOW);
        }
      } else {
        digitalWrite(pinMainPower, HIGH);
      }
    } else {
      this->main = sensorMain.average / sensorMain.index;
      sensorMain.index = 0;
      sensorMain.average = 0;
      sensorMain.done = true;
      dbg(F("Main tank average value: "));
      dbg(this->main);
      dbgLn();
      digitalWrite(pinMainPower, LOW);
    }
  }

  void stopMain() {
    digitalWrite(pinMainPower, LOW);
  }



  void readLevels() {
    if (!sensorWell.done) {
      this->readWell();
    }

    if (!sensorMain.done) {
      this->readMain();
    }

    if (refreshLevels.isExpired()) {
      sensorWell.done = false;
      sensorMain.done = false;
      refreshLevels.repeat();
    }
  }


public:
  Rule(Tone *tn, Data *md, Data *p1, Data *p2)
    : sound(tn), mode(md), pump1(p1), pump2(p2), beatLed(500, AsyncDelay::MILLIS) {
  }



  void begin() {

    refreshLevels.start(LevelsRefreshTime, AsyncDelay::MILLIS);
    com.begin(BaudSlaveRx);

    //
    // Well tank sensor pins
    pinMode(pinWellEcho, INPUT);
    pinMode(pinWellSend, OUTPUT);
    digitalWrite(pinWellSend, LOW);

    //
    // Main tank Slave pins
    pinMode(pinMainPower, OUTPUT);
    digitalWrite(pinMainPower, LOW);
  }

  void hark() {
    this->readLevels();

    this->controllWellPump();
  }

  //
  // Ouput to pass  information from this methods
  //
  uint8_t getWellLevel() {
    return this->well;
  }

  uint8_t getMainLevel() {
    return this->main;
  }
};



#endif
