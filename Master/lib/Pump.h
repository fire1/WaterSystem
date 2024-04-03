#include "Arduino.h"

#ifndef Pump_h
#define Pump_h

class Pump {
private:
    bool on = false;
    bool lastState = false;
    uint8_t debounce = 0;
    byte pin;
    byte btn;
    byte led;
    bool isTerminate = false;
    bool isOverwrite = false;


    void handlePins() {
        digitalWrite(pin, this->on);
        digitalWrite(led, !this->on);

        String msg = "Handle pins state: ";
        msg += this->on;
        Serial.println(msg);
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
    }

    /**
        * Toggle pump state off/on.
        *  By default access is frом a human interaction.
        * @param overwrite
        * @return
        */
    void toggle() {
        this->isOverwrite = true;
        this->on = !this->on;
        this->handlePins();
    }

    void terminate() {
        this->isTerminate = true;
    }

    bool isTerminated() {
        return this->isTerminate;
    }

    bool isOverwritten() {
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

        //
        // It is good to have some debounce of switching the machine power
        if (on == lastState) return;
        if (debounce >= 200) {
            lastState = on;
            debounce = 0;
            handlePins();
        }
        debounce++;
    }
};

#endif