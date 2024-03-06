#include <Arduino.h>

//
// Private libs
#include "lib/Glob.h"
#include "lib/Init.h"


void setup() {
    //
    // Setup the normal serial link to the PC
    Serial.begin(9600);
    Serial.println();
    Serial.println(F("-------------------------------"));
    Serial.println(F("Starting Water system /MASTER/"));
    Serial.println(F("-------------------------------"));
    Serial.println();
    delay(10);
    //
    // global LED to indicate data/sleep
    pinMode(pinLed, OUTPUT);

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

#ifdef ENABLE_CLOCK
    time.hark();
#endif
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

   // handleLedOffState();

}

//
// Rest of the junk, kept as tests
void loop_() {
    read.test();
}
