

#include "../lib/Glob.h"


//
// Defines Mode interface structure.
class ModeInterface {


        #if defined(OPT_DAYTIME_WELL)
        const uint8_t workHours = 12
        #else
        const uint8_t workHours = 24;
        #endif

    //
    // Shortcut/helper functions
    // 

    protected:
        uint8_t getWellVolume(uint8_t tankLevel){
            // also need to be used LevelSensorMainMax
            return tankLevel - LevelSensorWellMax; 
        }

        uint8_t getMainVolume(uint8_t tankLevel){
            // also need to be used LevelSensorMainMax
            return tankLevel - LevelSensorMainMax; 
        }

        
    public:
        virtual void init(Read* read, Rule* rule, Pump* pump) = 0;
        virtual void exec() = 0;
};