#ifndef Uart_h
#define Uart_h

#include "Glob.h"

//
// OneWire setup (Power/Serial)
// The OneWire Serial is desined to provide a power and
//  communication from the serial slave sensor.
SoftwareSerial com(pinMainRx, -1);

// https://forum.arduino.cc/t/jsn-sr04t-2-0/456255/10



#define PinCompressor 20


class Rule {
private:
  bool isReadWellStart = false;
  unsigned long wellTimer = 0;
  Data *mode;
  Data *pump1;
  Data *pump2;

  struct LevelSensorAverage {
    uint8_t index = 0;
    uint32_t distances[LevelSensorReads];
    uint8_t avr;
  };

  //
  // TODO make average read of sensors
  LevelSensorAverage
    sensorWell,
    sensorMain;

  uint16_t baud;
  uint8_t well;
  uint8_t main;



  bool isWellPumpOn = false;
  bool isMainPumpOn = false;


  unsigned long calcMinutes(unsigned int minutes) {
    return minutes * 60 * 1000UL;  // UL ensures the result is treated as an unsigned long
  }

  //
  // Defines work amplitude for Pump1
  void pumpWell(uint8_t workMin, uint8_t stopMin) {

    //
    // TODO measure Well before running...
    //

    if (!isDaytime()) {
      //
      // Stop the system
      Serial.println(F("It is not daytime!"));
      return;
    }

    if (!isWellPumpOn && (millis() - wellTimer >= this->calcMinutes(workMin))) {
      digitalWrite(PinCompressor, HIGH);
      wellTimer = millis();
      isWellPumpOn = true;
    }

    if (isWellPumpOn && (millis() - wellTimer >= this->calcMinutes(stopMin))) {
      digitalWrite(PinCompressor, LOW);
      wellTimer = millis();
      isWellPumpOn = false;
    }
  }


  //
  // Controlls pump1
  void controllWellPump() {

    switch (mode->value()) {
      default:
      case 0:
        // noting
        digitalWrite(PinCompressor, LOW);
        isWellPumpOn = false;
        break;

      case 1:
        // Easy
        pumpWell(6, 180);
        break;

      case 2:
        // Fast
        pumpWell(10, 60);
        break;

      case 3:
        // Now!
        pumpWell(10, 20);
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

      unsigned long duration = pulseIn(pinWellEcho, HIGH, 120000);  // read pulse with timeout for ~150cm

      // Check for timeout
      if (duration == 0) {
        Serial.println("Timeout error: Sensor WELL reading exceeds range");
        // Handle timeout (e.g., set distance to maximum or other logic)
        return;
      }

      float distance = (duration * .0343) / 2;
      // Store distance reading
      sensorWell.distances[sensorWell.index] = distance;
      // Calculate and update average after all readings are collected
      if (sensorWell.index == (LevelSensorReads - 1)) {  // All readings received
        for (int i = 0; i < LevelSensorReads; i++) {
          sensorWell.avr += sensorWell.distances[i];
        }
        sensorWell.avr /= LevelSensorReads;  // Calculate final average
      }
      sensorWell.index++;  // Increment index for next reading

    } else {
      this->well = (uint8_t)sensorWell.avr;
      sensorWell.index = 0;
      sensorWell.avr = 0;
      dbg(F("Well tank average value: "));
      dbg(this->well);
      dbgLn();
    }
  }



  void readMain() {
    if (sensorMain.index < LevelSensorReads) {
      if (digitalRead(pinMainPower)) {
        digitalWrite(pinLed, LOW);

        // Check for available data and read value
        if (com.available()) {
          digitalWrite(pinLed, HIGH);

          uint8_t currentValue = com.read();

          // Store value in the sensorMain struct
          sensorMain.distances[sensorMain.index] = currentValue;

          // Calculate and update average if all readings are collected
          if (sensorMain.index == (LevelSensorReads - 1)) {
            for (int i = 0; i < LevelSensorReads; i++) {
              sensorMain.avr += sensorMain.distances[i];
            }
            sensorMain.avr /= LevelSensorReads;  // Calculate final average
          }

          sensorMain.index++;  // Increment index for next reading

          dbg(F("RX: "));
          dbgLn(currentValue);  // Print current value for debugging
          digitalWrite(pinLed, LOW);
        }
      } else {
        digitalWrite(pinMainPower, HIGH);
      }
    } else {
      this->main = sensorMain.avr;
      sensorMain.index = 0;
      sensorWell.avr = 0;
      dbg(F("Main tank average value: "));
      dbg(this->main);
      dbgLn();
      digitalWrite(pinMainPower, LOW);
    }
  }

  void stopMain() {
    digitalWrite(pinMainPower, LOW);
  }



  void initLevels() {
    if (this->well == 0)
      this->readWell();

    if (this->main == 0)
      this->readMain();
  }


public:
  Rule(uint16_t baud, Data *md, Data *p1, Data *p2)
    : baud(baud), mode(md), pump1(p1), pump2(p2) {}


  void begin() {
    com.begin(this->baud);

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
    this->initLevels();

    //this->controllWellPump();
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

  int getWellBars() {

    if (!this->well) return 0;
    return map(this->well, 100, 20, 0, 10);
  }


  int getMainBars() {
    if (!this->main)
      return 0;

    return map(this->main, 100, 20, 1, 10);
  }
  /**
  * Function for test dump of serial comunication.
  */
  void testMain() {
    if (digitalRead(pinMainPower)) {

      digitalWrite(pinLed, LOW);
      if (com.available()) {
        digitalWrite(pinLed, HIGH);

        this->main = com.read();
        Serial.print(F("RX: "));
        Serial.println(this->main);

        digitalWrite(pinLed, LOW);
      }
    } else {
      Serial.println(F("Waiting 100 ms"));
      delay(100);
      Serial.println(F("Turning Slave ON..."));
      digitalWrite(pinMainPower, HIGH);
    }
  }


  void testWell() {

    digitalWrite(pinWellSend, LOW);
    delayMicroseconds(2);
    digitalWrite(pinWellSend, HIGH);
    delayMicroseconds(10);
    digitalWrite(pinWellSend, LOW);

    unsigned long duration = pulseIn(pinWellEcho, HIGH, 120000);  // read pulse with timeout for ~150cm
    float distance = (duration * .0343) / 2;
    Serial.print(F("RX: "));
    Serial.print(distance);
    Serial.println();

    delay(500);
  }
};


#endif
