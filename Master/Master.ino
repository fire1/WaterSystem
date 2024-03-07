#include <Arduino.h>

//
// Private libs
#include "lib/Glob.h"
#include "lib/Init.h"


void setup() {
    //
    // Initialize Serial
    welcomeSerialMessage();
    //
    // global LED to indicate data/sleep
    pinMode(pinLed, OUTPUT);
    //
    // Initialize classes
    read.begin();
    menu.begin();
    draw.begin();
    rule.begin();
    buzz.begin();

#ifdef ENABLE_CLOCK
    time.begin();
#endif
}

void loop() {
    time.hark();
#ifdef ENABLE_CMD
    cmd.hark(&read);  // input commands from serial
#endif

    buzz.hark();
    rule.hark();
    draw.menu(&menu);
    read.hark();

    spanSm.tick();
    spanMd.tick();
    spanLg.tick();
    spanMx.tick();

}

//
// Rest of the junk, kept as tests
void loop_() {
    read.test();
}
