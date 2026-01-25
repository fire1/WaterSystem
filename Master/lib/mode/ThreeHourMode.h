#ifndef ThreeHourMode_h
#define ThreeHourMode_h

#include "../Mode.h"
#include "../Pump.h"
#include "../Read.h"

// Fixed timing: 12 minutes work, 168 minutes break = 3 hours cycle
#define THREE_HOUR_RUNTIME 12
#define THREE_HOUR_BREAK 168

class ThreeHourMode : public Mode {
public:
    ThreeHourMode() {}

    // Title displayed on LCD
    const __FlashStringHelper *title() override { return F("3-Hour"); }

    RunWell well() override {
    

        // Static mode: fixed intervals regardless of sensor data
        uint8_t currentRuntime = THREE_HOUR_RUNTIME;
        unsigned long breakTimeInterval = THREE_HOUR_BREAK;

        // Debug output
        if (spanLg.active()) {
            dbg(F("[3-HOUR] Fixed cycle - Work: ")); dbg(currentRuntime);
            dbg(F("m | Break: ")); dbg(breakTimeInterval);
            dbgLn(F("m"));
        }

        // Execute pumping
        return RunWell{currentRuntime, breakTimeInterval};
    }
};

#endif