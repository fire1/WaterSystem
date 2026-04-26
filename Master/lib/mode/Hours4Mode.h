#ifndef Hours4Mode_h
#define Hours4Mode_h

#include "../Mode.h"
#include "../Pump.h"
#include "../Read.h"

// Fixed timing: 10 minutes work, 230 minutes break = 4 hours cycle
#define FOUR_HOUR_RUNTIME 12
#define FOUR_HOUR_BREAK 230

class Hours4Mode : public Mode {
public:
    Hours4Mode() {}

    // Title displayed on LCD
    const __FlashStringHelper *title() override { return F("4-Hour"); }

    RunWell well(Read* read) override {


        // Static mode: fixed intervals regardless of sensor data
        uint8_t currentRuntime = FOUR_HOUR_RUNTIME;
        unsigned long breakTimeInterval = FOUR_HOUR_BREAK;

        // Debug output
        if (spanLg.active()) {
            dbg(F("[4-HOUR] Fixed cycle - Work: ")); dbg(currentRuntime);
            dbg(F("m | Break: ")); dbg(breakTimeInterval);
            dbgLn(F("m"));
        }

        // Execute pumping
        return RunWell{currentRuntime, breakTimeInterval};
    }
};

#endif
