#ifndef Uart_h
#define Uart_h

#include "Glob.h"

//
// OneWire setup (Power/Serial)
// The OneWire Serial is desined to provide a power and
//  communication from the serial slave sensor.
SoftwareSerial com(pinRx, pinTx);

// https://forum.arduino.cc/t/jsn-sr04t-2-0/456255/10


#define PinCompressor 20

class Rule {
private:
  unsigned long wellTimer = 0;
  Data& mode;
  Data& pump1;
  Data& pump2;

  uint16_t baud;
  uint8_t bank2;

  bool isWellPumpOn = false;
  bool isRisePumpOn = false;


  unsigned long calcMinutes(unsigned int minutes) {
    return minutes * 60 * 1000UL;  // UL ensures the result is treated as an unsigned long
  }

  //
  // Defines work amplitude for Pump1
  void workPump1(uint8_t workMin, uint8_t stopMin) {

    //
    // TODO measure bank1 before running...
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

    switch (mode.value()) {
      default:
      case 0:
        // noting
        digitalWrite(PinCompressor, LOW);
        isWellPumpOn = false;
        break;

      case 1:
        // 4 minutes works, 120 stopped
        workPump1(4, 120);
        break;

      case 2:
        // 4 minutes works, 45 stopped
        workPump1(4, 45);
        break;

      case 3:
        // 6 minutes works, 15 stopped
        workPump1(6, 15);
        break;
    }
  }

  void readBank1() {
    //
    // TODO check level bank 1
    //
  }

  void readBank2() {


    if (digitalRead(pinB2)) {

      digitalWrite(pinLed, LOW);
      if (com.available()) {
        digitalWrite(pinLed, HIGH);

        this->bank2 = com.read();
        dbg(F("RX: "));
        dbgLn(this->bank2);
      }
    }

    digitalWrite(pinB2, HIGH);
  }



  void resolveLevels() {
    this->readBank1();
    this->readBank2();

    //
    //
  }


public:
  Rule(uint16_t baud, Data mode, Data p1, Data p2)
    : baud(baud), mode(mode), pump1(p1), pump2(p2) {}


  void begin() {
    com.begin(this->baud);
  }

  void hark() {
    this->resolveLevels();
    //this->controllWellPump();
  }

  /**
  * Function for test dump of serial comunication.
  */
  void test() {
    if (digitalRead(pinB2)) {

      digitalWrite(pinLed, LOW);
      if (com.available()) {
        digitalWrite(pinLed, HIGH);

        this->bank2 = com.read();
        Serial.print(F("RX: "));
        Serial.println(this->bank2);
        
        digitalWrite(pinLed, LOW);
      }
    } else {
      Serial.println(F("Waiting 100 ms"));
      delay(100);
      Serial.println(F("Turning Slave ON..."));
      digitalWrite(pinB2, HIGH);
    }
  }
};


#endif
