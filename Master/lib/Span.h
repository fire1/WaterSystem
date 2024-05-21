#ifndef Span_h
#define Span_h

#include <Arduino.h>

/**
 * @class Time span class
 */
class Span {

private:
    bool isActive = false;
    uint32_t length;
    unsigned long previousMillis = 0;


public:

    Span(uint32_t len)
            : length(len) {}

    bool active() {
        return isActive;
    }

    void tick() {
        unsigned long currentMillis = millis();  // Get the current time
        // Check if enough time has passed for the span to be active
        if (currentMillis - previousMillis >= length) {
            isActive = true;
            previousMillis = currentMillis;
        } else {
            isActive = false;
        }
    }
};

#endif
