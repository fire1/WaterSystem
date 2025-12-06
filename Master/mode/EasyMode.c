

#include "../lib/Glob.h"
#include "ModeInterface.h"

#define WORK_LEN 10
struct WorkWaterPumped{
    uint8_t time;
    uint8_t size;
};




class EasyMode : public ModeInterface  {
 
    private:
    Read* read;
    WorkWaterPumped workBuffer[WORK_LEN];
    uint8_t workIndex = 0;


    void handleWorkBuffer(){
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
    uint8_t toFill = this->getWellVolume(level);

        workBuffer[workIndex].time = runtime;
        workBuffer[workIndex].size = toFill;



        handleWorkBuffer();

    }
// ...existing code...

uint8_t getOptimalRuntimeAndCycles(float desiredWater, uint8_t& outCycles) {
    float bestDiff = 99999;
    uint8_t bestRuntime = 12;
    uint8_t bestCycles = 1;

    for (uint8_t runtime = 5; runtime <= 12; runtime++) {
        float avgFlow = getAvgFlowForRuntime(runtime); // Ваша функция
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