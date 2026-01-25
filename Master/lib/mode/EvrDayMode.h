#ifndef EvrDayMode_h
#define EvrDayMode_h

#include "../Mode.h"
#include "../Pump.h"
#include "../Read.h"

// Fixed timing: 12 minutes work, 48 minutes break = 1 hour cycle
#define HOURLY_RUNTIME 12
#define HOURLY_BREAK 1440 

class EvrDayMode : public Mode {
public:

    EvrDayMode() {}

    // Title displayed on LCD
    const __FlashStringHelper *title() override { return F("Every Day"); }

    RunWell well(Read* read) override {

        // In this mode, we ignore levels and drift correction.
        // We simply cycle the pump every hour.
        
        uint8_t currentRuntime = HOURLY_RUNTIME;
        unsigned long breakTimeInterval = HOURLY_BREAK;

        // Debug output for monitoring
        if (spanLg.active()) {
            dbg(F("[DAILY] Fixed cycle - Work: ")); dbg(currentRuntime);
            dbg(F("m | Break: ")); dbg(breakTimeInterval);
            dbgLn(F("m"));
        }

        // Call the base pump logic
        return RunWell{currentRuntime, breakTimeInterval};
    }
};

#endif