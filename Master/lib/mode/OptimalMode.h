#ifndef OptimalMode_h
#define OptimalMode_h

#include "../Mode.h"

class OptimalMode : public Mode {
private:
    struct PumpPlan {
        uint8_t runtime;    // work minutes
        uint16_t breaktime; // pause minutes
        float efficiency;   // actual cm per total cycle minute
    };

    PumpPlan current;
    PumpPlan best;
    uint8_t searchPhase = 0; 

public:
    OptimalMode() {
        current = {12, 180, 0.0};
        best = current;
    }

    const __FlashStringHelper *title() override { return F("Auto Adjst"); }

    RunWell well(Read *read) override {
        // 1. Initial run protection
        if (!hasValidHistory()) {
            return RunWell{current.runtime, current.breaktime};
        }

        // 2. Use your correction logic to see the error
        // drift > 1.0 means we are under-performing (well is tired)
        // drift < 1.0 means we are over-performing (well is strong)
        float drift = constrain(this->calculateLastCorrection(), 0.5, 1.5);
        
        // Calculate real rise based on history
        uint8_t rise = fetchRise(TARGET_RISE_CM);
        
        // Calculate Efficiency: (Real CM) / (Work + Pause minutes)
        uint16_t totalCycle = current.runtime + current.breaktime;
        float measuredEff = (float)rise / (float)totalCycle;

        if (spanLg.active()) {
            dbg(F("[OPT] Drift: ")); dbg(drift);
            dbg(F(" | Eff: ")); dbgLn(measuredEff);
        }

        // 3. Update Best known efficiency
        if (measuredEff > best.efficiency) {
            best = current;
            best.efficiency = measuredEff;
        }

        // 4. Optimization Engine using Drift as a guide
        if (searchPhase == 0) {
            // Adjust Runtime based on Drift
            // If drift is 1.2, we need ~20% more time to hit TARGET_RISE_CM
            if (drift > 1.05) {
                if (current.runtime < 16) current.runtime++;
            } else if (drift < 0.95) {
                if (current.runtime > 7) current.runtime--;
            }
            searchPhase = 1;
        } 
        else {
            // Adjust Breaktime based on Efficiency
            // If current efficiency is dropping compared to BEST, increase rest
            if (measuredEff < (best.efficiency * 0.95)) {
                current.breaktime = constrain(current.breaktime + 15, 60, 480);
            } else {
                // If we are doing well, try to push the limits by reducing rest
                current.breaktime = constrain(current.breaktime - 10, 60, 480);
            }
            searchPhase = 0;
        }

        return RunWell{current.runtime, current.breaktime};
    }
};
#endif