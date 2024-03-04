#include <Arduino.h>

//
// Definition setup

#define DEBUG  // Comment it to disable debugging
//#define DAYTIME_CHECK // Comment it to disable daytime check for running pumps
//#define WELL_MEASURE_DEFAULT // Uses trigger/echo to get distance (not recommended)
#define WELL_MEASURE_UART_47K // Uses Serial UART to communicate with the sensor
//#define ENABLE_CMD_INPUT // Enables Serial input listener for commands
//#define ENABLE_CLOCK // Enables DS3231 clock usage


//
// Private libs
#include "lib/Glob.h"
#include "lib/Init.h"


void setup() {

    //0
    // Setup the normal serial link to the PC
    Serial.begin(9600);
    Serial.println(F("Starting Water system /MASTER/"));
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

    cmd.hark();  // input commands from serial
    buzz.hark();

    rule.hark();

    draw.menu(&menu);

    read.hark();

    //
    // Overwrites values from Serial
#ifdef ENABLE_CMD_INPUT
    cmd.read(&read);
#endif

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
