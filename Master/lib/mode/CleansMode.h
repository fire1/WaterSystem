#ifndef CleansMode_h
#define CleansMode_h

#include "../Mode.h"
#include "../Pump.h"
#include "../Read.h"

#define HOURLY_RUNTIME 6
#define HOURLY_BREAK 5760  

class CleansMode : public Mode {
public:

    CleansMode() {}

    // Title displayed on LCD
    const __FlashStringHelper *title() override { return F("Idle / 3d"); }

    RunWell well(Read* read) override {

        // In this mode, we ignore levels and drift correction.
        // We simply cycle the pump every hour.
        
        uint8_t currentRuntime = HOURLY_RUNTIME;
        unsigned long breakTimeInterval = HOURLY_BREAK;

        // Debug output for monitoring
        if (spanLg.active()) {
            dbg(F("[Idle] Fixed cycle - Work: ")); dbg(currentRuntime);
            dbg(F("m | Break: ")); dbg(breakTimeInterval);
            dbgLn(F("m"));
        }

        // Call the base pump logic
        return RunWell{currentRuntime, breakTimeInterval};
    }
};

#endif