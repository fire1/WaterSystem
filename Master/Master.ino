

#define DEBUG  // Comment it to disable debugging
//#define DAYTIME_CHECK // Comment it to disable daytime check for running pumps

//
// Private libs
#include "lib/Glob.h"
#include "lib/Init.h"


void setup() {

  //
  // Setup the normal serial link to the PC
  Serial.begin(9600);
  Serial.println(F("Starting Water system /MASTER/"));
  delay(10);
  //
  // global LED to indicate data/sleep
  pinMode(pinLed, OUTPUT);
  //
  // I2C start
  Wire.begin();
  read.begin();
  menu.begin();
  draw.begin();
  rule.begin();
  buzz.begin();
  time.begin();
}

void loop_(){
  read.test();
}


void loop() {

  cmd.hark();  // input commands from serial
  buzz.hark();
  time.hark();

  read.hark();
  cmd.read(&read);

  rule.hark();

  draw.menu(&menu);

  spanSm.tick();
  spanMd.tick();
  spanLg.tick();
  spanMx.tick();
}
