

#include "../lib/Glob.h"
#include "ModeInterface.h"

#define WORK_LEN 10
struct WorkWaterPumped{
    uint8_t time;
    uint8_t size;
};

#define DEFAULT_WELL_CYCLE_VOLUME 3 // Centimeters per single well pump run

class EasyMode : public ModeInterface  {
 
    private:
    Read* read;
    WorkWaterPumped workBuffer[WORK_LEN];
    uint8_t workIndex = 0;


    void handleWorkBuffer(uint8_t runtime, uint8_t volume){
        workBuffer[workIndex].time = runtime;
        workBuffer[workIndex].size = volume;
        workIndex++;
        if(workIndex >= WORK_LEN){
            workIndex = 0;
        }
    }

    public:
    EasyMode(){

    }

    void init(Read* rd) override {
        read = rd;
    }

    //
    // As an example we will work with well...
    void exec(uint8_t runtime, uint8_t level) override {
        uint8_t volume = this->getWellVolume(level);
        this->handleWorkBuffer(runtime,volume);
        int8_t neededCycles = volume / DEFAULT_WELL_CYCLE_VOLUME; 
        
        
        uint16_t interval = (this->workHours * 60) / neededCycles; // in minutes
        
        // "runtime" needs to be adjusted /a bit/ from average value
        read->pumpWell(runtime, interval);

        //
        // Need to get start/stop level of the well tank in order to calculate 
        //  well cycle volume properly.


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
// ...existing code...
}