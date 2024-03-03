#include "Arduino.h"

#ifndef Pump_h
#define Pump_h

class Pump {
private:
  bool on = false;
  byte pin;
  byte btn;
  byte led;

  void handlePins() {
    if (this->on) {
      digitalWrite(led, LOW);
      digitalWrite(pin, HIGH);
    } else {
      digitalWrite(led, HIGH);
      digitalWrite(pin, LOW);
    }
  }


public:
  Pump(byte p, byte b, byte l)
    : pin(p), btn(b), led(l) {
  }

  bool isOn() {
    return this->on;
  }

  void setOn(bool state) {
    this->on = state;
    this->handlePins();
  }
  void toggle() {
    this->on = !this->on;
    this->handlePins();
  }

  byte getPin() {
    return this->pin;
  }

  byte getBtn() {
    return this->btn;
  }
};

#endif