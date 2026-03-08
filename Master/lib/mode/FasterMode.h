#ifndef FASTER_MODE_H
#define FASTER_MODE_H

#include "../Mode.h"

class FasterMode : public Mode {

public:
  FasterMode() {}

  // Title displayed on LCD
  const __FlashStringHelper *title() override { return F("Fastest "); }

  RunWell well(Read *read) override {
    // In this mode, we ignore levels and drift correction.
    // We simply run the pump for a fixed short duration every cycle.

    uint8_t currentRuntime = 4; // Run for 3 minutes
    unsigned long breakTimeInterval =
        41 - currentRuntime; // Break for the rest of the day

    // Debug output for monitoring
    if (spanLg.active()) {
      dbg(F("[Fast] Fixed cycle - Work: "));
      dbg(currentRuntime);
      dbg(F("m | Break: "));
      dbg(breakTimeInterval);
      dbgLn(F("m"));
    }

    // Call the base pump logic
    return RunWell{currentRuntime, breakTimeInterval};
  }
};
