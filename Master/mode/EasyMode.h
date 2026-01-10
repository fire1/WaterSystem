#ifndef EasyMode_h
#define EasyMode_h

#include "ModeInterface.h"
#include "../lib/Read.h"
#include "../lib/Pump.h"

#define DEFAULT_WELL_CYCLE_VOLUME 3 // cm per single well pump run

#define WORK_LEN 10
struct WorkWaterPumped{
    uint8_t time;
    uint8_t size;
};

class EasyMode : public ModeInterface {
private:
    Read* read = nullptr;
    Rule* rule = nullptr;
    Pump* pump = nullptr;
    
    WorkWaterPumped workBuffer[WORK_LEN];
    uint8_t workIndex = 0;

    unsigned long pumpOffTime = 0;
    uint8_t runtimeMinutes = WellPumpDefaultRuntime; // from Glob.h


        void handleWorkBuffer(uint8_t runtime, uint8_t volume){
        workBuffer[workIndex].time = runtime;
        workBuffer[workIndex].size = volume;
        workIndex++;
        if(workIndex >= WORK_LEN){
            workIndex = 0;
        }
    }

public:
    EasyMode() {}

    const char* title() override {
        return "Easy";
    }

    void init(Read* rd, Rule* rl, Pump* p) override {
        read = rd;
        rule = rl;
        pump = p;
    }

    void exec() override {
        if (!read || !pump) return;

        // Turn pump off when runtime expired
        if (pump->isOn()) {
            if (pumpOffTime && millis() >= pumpOffTime) {
                pump->setOn(false);
            }
            return;
        }

        uint8_t level = read->getWellLevel();

        // Not enough water
        if (level < LevelSensorBareMax(LevelSensorWellMax)) return;

        // Compute simple volume & cycles
        uint8_t volume = this->getWellVolume(level);
        int8_t neededCycles = volume / DEFAULT_WELL_CYCLE_VOLUME;
        if (neededCycles <= 0) neededCycles = 1;

        // For simplicity run a single cycle of runtimeMinutes
        pump->setOn(true);
        pumpOffTime = millis() + ((unsigned long)runtimeMinutes) * 60UL * 1000UL;
    }


    
uint8_t getOptimalRuntimeAndCycles(float desiredWater, uint8_t& outCycles) {
    float bestDiff = 99999;
    uint8_t bestRuntime = 12;
    uint8_t bestCycles = 1;

    for (uint8_t runtime = 5; runtime <= 12; runtime++) {
        float avgFlow = getAvgFlowForRuntime(runtime); // need this function
        for (uint8_t cycles = 1; cycles <= 10; cycles++) {
            float totalWater = avgFlow * runtime * cycles;
            float diff = abs(desiredWater - totalWater);
            if (diff < bestDiff) {
                bestDiff = diff;
                bestRuntime = runtime;
                bestCycles = cycles;
            }
        }
    }
    outCycles = bestCycles;
    return bestRuntime;
}

float getAvgFlowForRuntime(uint8_t runtime) {
    // Simple average flow rates based on empirical data
    switch (runtime) {
        case 5: return 0.8;  // liters per minute
        case 6: return 0.9;
        case 7: return 1.0;
        case 8: return 1.1;
        case 9: return 1.2;
        case 10: return 1.3;
        case 11: return 1.4;
        case 12: return 1.5;
        default: return 1.0;

}
}
};

#endif
