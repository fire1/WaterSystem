#include "Arduino.h"

#ifndef Pump_h
#define Pump_h

class Pump {
private:
  bool on = false;
  byte pin;
  byte btn;
  byte led;
  bool terminated = false;


  void handleLed() {
    if (this->on) digitalWrite(led, LOW);
    else digitalWrite(led, HIGH);
  }


public:
  Pump(byte p, byte b, byte l)
    : pin(p), btn(b), led(l) {
  }

  bool isOn() {
    return this->on;
  }

  void setOn(bool state) {
    if (this->terminated) return;
    this->on = state;
    this->handleLed();
  }

  void toggle() {
    this->on = !this->on;
    this->handleLed();
  }

  void terminate() {
    this->terminated = true;
  }

  bool isTerminated() {
    return this->terminated;
  }

  byte getPin() {
    return this->pin;
  }

  byte getBtn() {
    return this->btn;
  }

  void ctrl() {
    if (this->on) digitalWrite(pin, HIGH);
    else digitalWrite(pin, LOW);
  }
};

#endif