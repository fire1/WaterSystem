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
  unsigned long compresorTimer = 0;
  Data& mode;
  Data& tank1;
  Data& tank2;

  uint16_t baud;
  uint8_t led;
  char bank2;

  bool isCompressorOn;


  unsigned long calcMinutes(unsigned int minutes) {
    return minutes * 60 * 1000UL;  // UL ensures the result is treated as an unsigned long
  }

  //
  // Defines work amplitude for Pump1
  void workAmplitudePump1(uint8_t workMin, uint8_t stopMin) {

    //
    // TODO measure bank1 before running...

    if (!isCompressorOn && (millis() - compresorTimer >= this->calcMinutes(workMin))) {
      digitalWrite(PinCompressor, HIGH);
      compresorTimer = millis();
      isCompressorOn = true;
    }

    if (isCompressorOn && (millis() - compresorTimer >= this->calcMinutes(stopMin))) {
      digitalWrite(PinCompressor, LOW);
      compresorTimer = millis();
      isCompressorOn = false;
    }

  }



  void controlCompressor() {

    switch (mode.value()) {
      default:

      case 0:
        // noting
        digitalWrite(PinCompressor, LOW);
        isCompressorOn = false;
        break;

      case 1:
        // 4 minutes works, 120 stopped
        workAmplitudePump1(4, 120);
        break;

      case 2:
        // 4 minutes works, 45 stopped
        workAmplitudePump1(4, 45);
        break;

      case 3:
        // 10 minutes works, 15 stopped
        workAmplitudePump1(10, 15);
        break;
    }
  }

  void readBank2() {
    digitalWrite(this->led, LOW);

    if (com.available()) {
      digitalWrite(this->led, HIGH);

      this->bank2 = com.read();
      dbg(F("RX: "));
      dbgLn(this->bank2);
    }
  }
public:
  Rule(uint16_t baud, uint8_t led, Data mode, Data tank1, Data tank2)
    : baud(baud), led(led), mode(mode), tank1(tank1), tank2(tank2) {}


  void begin() {
    com.begin(this->baud);
  }

  void hark() {
    this->readBank2();
  }
};


#endif