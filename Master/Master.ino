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
    cmd.hark(&read, &draw, &heat);  // input commands from serial
#endif
    //
    // Listeners
    buzz.hark();
    rule.hark();
    read.hark();
    heat.hark();

    //
    // Display menu
    draw.menu(&menu);

    //
    // Warnings
    heat.warn(&draw);
    rule.warn(&draw);

    if (spanSm.isActive()) {
        ctrlWell.ctrl();
        ctrlMain.ctrl();
    }

    spanSm.tick();
    spanMd.tick();
    spanLg.tick();
    spanMx.tick();


}

//
// Rest of the junk, kept as tests
void loop_() {
    //read.test();
    if (spanSm.isActive()) rule.hark();


    spanSm.tick();
}
