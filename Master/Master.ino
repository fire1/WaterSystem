#include <Arduino.h>

#include "lib/Glob.h"
#include "lib/Init.h"


/* Note:
 *  The source code for this sketch is based on the "dependencies-injection" type.
 *  This code structure will improve a customization of the final needs of the sketch.
 */


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
    time.begin();

    //time.adjust();
}

void loop() {
    //
    // Debugging
    cmd.listen();

    time.hark();
    //
    // Listeners
    buzz.hark();
    rule.hark();
    read.hark();
    heat.hark();

    //
    // Display menu
    if (spanMd.active())
        draw.menu(&menu);

    //
    // Warnings
    heat.warn(&draw);
    rule.warn(&draw);

    ctrlWell.ctrl();
    ctrlMain.ctrl();

    spanSm.tick();
    spanMd.tick();
    spanLg.tick();
    spanMx.tick();

}

// end of the file...

