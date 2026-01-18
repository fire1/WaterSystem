
#ifndef WinterMode_h
#define WinterMode_h

#include "ModeInterface.h"
#include "../Read.h"
#include "../Pump.h"

#define WORK_LEN 10
#define COOL_DOWN_MS 2700000UL // 45 minutes rest (45 * 60 * 1000)

struct WorkWaterPumped {
    uint8_t time;
    uint8_t size; // CM rise
};

class WinterMode : public ModeInterface {
private:
    WorkWaterPumped workBuffer[WORK_LEN];
    uint8_t workIndex = 0;

    unsigned long pumpOffTime = 0;
    unsigned long lastFinishTime = 0;
    uint8_t startLevelCapture = 0;
    
    float timeMultiplier = 1.0; 
    uint8_t currentRuntime = 12; // Start with 12 mins base

    void handleWorkBuffer(uint8_t runtime, uint8_t volume) {
        workBuffer[workIndex].time = runtime;
        workBuffer[workIndex].size = volume;
        if (++workIndex >= WORK_LEN) workIndex = 0;
    }

public:
    WinterMode() {}

    const __FlashStringHelper* title() override {
        return F("Winter");
    }

void exec() override {
        if (!read || !rule) return;

        uint8_t wellLevel = read->getWellLevel();   // Tank we are FILLING
        uint8_t mainLevel = read->getMainLevel();   // Tank we are DRAWING from
        
        // --- 1. MONITOR ACTIVE PUMPING ---
        // if (ctrlWell.isOn()) {
            if (millis() >= pumpOffTime) {
                float drift = calculateCorrection(startLevelCapture, wellLevel);
                timeMultiplier = (timeMultiplier * 0.7) + (drift * 0.3);
            }
        

        // --- 2. CALCULATE SPACE AND QUOTA ---
        // How many cm of empty space are in the Well Tank?
        int16_t emptySpaceCm = (int16_t)wellLevel - (int16_t)LevelSensorWellMax;
        if (emptySpaceCm < 0) emptySpaceCm = 0;

        // How many 3cm cycles do we need to fill the Well Tank?
        float neededCycles = (float)emptySpaceCm / (float)TARGET_RISE_CM;

        // Default pause (80 min hardware safety)
        uint16_t pauseMinutes = 80;

        if (neededCycles > 1.0) {
            // Spread the needed cycles across the available work hours
            // Pause = (Total Work Minutes) / Number of Cycles
            uint16_t spreadingPause = (uint16_t)(((float)this->workHours * 60.0) / neededCycles);
            
            if (spreadingPause > 45) {
                pauseMinutes = spreadingPause;
            }
        }

        // --- 3. START CONDITIONS ---
        // - Drawing source (Main Tank) must have water (e.g., level < 100cm)
        // - Destination (Well Tank) must have space (e.g., level > (Max + 3cm))
        if (mainLevel < 100 && emptySpaceCm >= TARGET_RISE_CM) {
            
            float calculated = 12.0 * timeMultiplier;
            if (calculated < 5) calculated = 5;
            if (calculated > 20) calculated = 20;
            
            currentRuntime = (uint8_t)calculated;
            startLevelCapture = wellLevel; // Capture current well level before filling

            // Trigger rule: Pump for X minutes, then wait Y minutes
            //rule->pumpWell(currentRuntime, pauseMinutes);
            
            pumpOffTime = millis() + ((unsigned long)currentRuntime * 60000UL);
        }
        
        if (cmd.show(F("mode"), F("Mode info"))){
            cmd.print("Time to on", this->getNextOn());
        }
        this->pumpWell(currentRuntime, pauseMinutes);
    }
};
#endif
