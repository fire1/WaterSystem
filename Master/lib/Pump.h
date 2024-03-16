#include "Arduino.h"

#ifndef Pump_h
#define Pump_h

class Pump {
private:
    bool on = false;
    byte pin;
    byte btn;
    byte led;


    void handleLed() {
        if (this->on) digitalWrite(led, LOW); else digitalWrite(led, HIGH);
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
        this->handleLed();
    }

    void toggle() {
        this->on = !this->on;
        this->handleLed();
    }

    byte getPin() {
        return this->pin;
    }

    byte getBtn() {
        return this->btn;
    }

    void ctrl() {
        if (this->on) digitalWrite(pin, HIGH); else digitalWrite(pin, LOW);
    }

};

#endif