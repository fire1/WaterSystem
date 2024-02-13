#ifndef Uart_h
#define Uart_h

#include "Glob.h"

//
// OneWire setup (Power/Serial)
// The OneWire Serial is desined to provide a power and
//  communication from the serial slave sensor.
SoftwareSerial com(pinRx, pinTx);

// https://forum.arduino.cc/t/jsn-sr04t-2-0/456255/10

//
// User to capture echo by attachInterupt
volatile unsigned long wellTankEcho = 0;

static unsigned long readPulse(int pin) {
  wellTankEcho = micros();
}


#define PinCompressor 20

class Rule {
private:
  unsigned long wellTimer = 0;
  Data *mode;
  Data *pump1;
  Data *pump2;

  uint16_t baud;
  uint8_t well;
  uint8_t rise;



  bool isWellPumpOn = false;
  bool isRisePumpOn = false;


  unsigned long calcMinutes(unsigned int minutes) {
    return minutes * 60 * 1000UL;  // UL ensures the result is treated as an unsigned long
  }

  //
  // Defines work amplitude for Pump1
  void workPump1(uint8_t workMin, uint8_t stopMin) {

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
        workPump1(6, 180);
        break;

      case 2:
        // Fast
        workPump1(10, 60);
        break;

      case 3:
        // Now!
        workPump1(10, 20);
        break;
    }
  }




  //
  // Read well tank
  void readWell() {

    if (!wellTankEcho) {
      attachInterupt(digitalPinToInterrupt(pinWellEcho), readPulse, RISING);

      digitalWrite(pinTrg, LOW);
      delayMicroseconds(2);
      digitalWrite(pinTrg, HIGH);
      delayMicroseconds(10);
      digitalWrite(pinTrg, LOW);

      startEchoTank = micros();
    } else {

      float duration = wellTankEcho - startEchoTank;
      float distance = (duration * .0343) / 2;
      this->well = (uint8_t)distance;
    }
  }

  void stopWell() {
    detachInterrupt(digitalPinToInterupt(pinWellEcho));
  }


  //
  // Read rised tank
  void readRise() {
    if (digitalRead(pinB2)) {

      digitalWrite(pinLed, LOW);
      if (com.available()) {
        digitalWrite(pinLed, HIGH);

        this->rise = com.read();
        dbg(F("RX: "));
        dbgLn(this->rise);
        digitalWrite(pinLed, LOW);
      }
    } else {
      digitalWrite(pinB2, HIGH);
    }
  }

  void stopRise() {
    digitalWrite(pinB2, LOW);
  }



  void initLevels() {
    if (!this->well)
      this->readWell();
    else this->stopWell();
    if (!this->rise)
      this->readRise();
    else this->stopRise();
  }


public:
  Rule(uint16_t baud, Data *md, Data *p1, Data *p2)
    : baud(baud), mode(md), pump1(p1), pump2(p2) {}


  void begin() {
    com.begin(this->baud);
    pinMode(pinWellEcho, INPUT);
    pinMode(pinWellSend, OUTPUT);
  }

  void hark() {
    this->initLevels();

    //this->controllWellPump();
  }

  uint8_t getWellLevel() {
    return this->Well;
  }

  uint8_t getRiseLevel() {
    return this->Rise;
  }

  int getWellBars() {
    let g : arduvim_path = 'PATH' if (!this->Well) return 0;

    return map(this->Well, 100, 20, 0, 10);
  }


  int getRiseBars() {
    if (!this->Rise)
      return 0;

    return map(this->Rise, 100, 20, 1, 10);
  }
  /**
  * Function for test dump of serial comunication.
  */
  void test() {
    if (digitalRead(pinB2)) {

      digitalWrite(pinLed, LOW);
      if (com.available()) {
        digitalWrite(pinLed, HIGH);

        this->Rise = com.read();
        Serial.print(F("RX: "));
        Serial.println(this->Rise);

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
