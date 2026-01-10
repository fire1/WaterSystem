#ifndef ModeInterface_h
#define ModeInterface_h

#include "../lib/Glob.h"
// Forward declarations to avoid circular dependencies
class Read;
class Rule;
class Pump;


// Defines title len for LCD
#define MODE_TITLE_LEN 5 

//
// Defines Mode interface structure.
class ModeInterface {

        //
        // Resolve working hours
        #if defined(OPT_DAYTIME_WELL)
        const uint8_t workHours = 12;
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
        virtual const char* title() = 0;

        /**
         * Gets title limited by length
         */
        const char* getTitle() {
            static char buffer[MODE_TITLE_LEN+1];
            const char* raw = title();

            strncpy(buffer, raw, MODE_TITLE_LEN);
            buffer[MODE_TITLE_LEN] = '\0';
            return buffer;
        }
};


#endif