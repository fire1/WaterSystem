#ifndef Hours6Mode_h
#define Hours6Mode_h

#include "../Mode.h"
#include "../Pump.h"
#include "../Read.h"

// Fixed timing: 10 minutes work, 350 minutes break = 6 hours cycle
#define SIX_HOUR_RUNTIME 12
#define SIX_HOUR_BREAK 350

class Hours6Mode : public Mode {
public:
    Hours6Mode() {}

    // Title displayed on LCD
    const __FlashStringHelper *title() override { return F("6-Hour"); }

    RunWell well(Read* read) override {


        // Static mode: fixed intervals regardless of sensor data
        uint8_t currentRuntime = SIX_HOUR_RUNTIME;
        unsigned long breakTimeInterval = SIX_HOUR_BREAK;

        // Debug output
        if (spanLg.active()) {
            dbg(F("[6-HOUR] Fixed cycle - Work: ")); dbg(currentRuntime);
            dbg(F("m | Break: ")); dbg(breakTimeInterval);
            dbgLn(F("m"));
        }

        // Execute pumping
        return RunWell{currentRuntime, breakTimeInterval};
    }
};

#endif
