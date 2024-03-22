#include "Arduino.h"

#ifndef Pump_h
#define Pump_h

class Pump {
private:
  bool on = false;
  byte pin;
  byte btn;
  byte led;
  bool isTerminate = false;
  bool isOverwrite = false;

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
    if (this->isTerminate) return;
    this->on = state;
    this->handleLed();
  }
  /**
    * Toggle pump state off/on.
    *  By default access is frом a human interaction.
    * @param overwrite 
    * @return 
    */
  void toggle(bool overwrite = false) {
    this->isOverwrite = overwrite;
    this->on = !this->on;
    this->handleLed();
  }

  void terminate() {
    this->isTerminate = true;
  }

  bool isTerminated() {
    return this->isTerminate;
  }

  bool isOverwrited() {
    return this->isOverwrite;
  }

  void clearTerminate() {
    this->isTerminate = false;
  }

  byte getPin() {
    return this->pin;
  }

  byte getBtn() {
    return this->btn;
  }

  void ctrl() {
    if (this->on)
      digitalWrite(pin, HIGH);
    else
      digitalWrite(pin, LOW);
  }
};

#endif