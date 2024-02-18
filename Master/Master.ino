


//
// Communication pins
const byte pinRx = 10;         // Recive data pin for bank 2
const byte pinB2 = 8;          // Turn on power for Bank 2
const byte pinTx = -1;         // Just disable Tx pin
const byte pinLed = 13;        // LED blinks
const uint16_t rxBaud = 4800;  // This is the communication speed over serial
const byte pinTone = 9;

#define DEBUG
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
  // I2C start
  Wire.begin();
  //
  // LED to indicate when recieving
  pinMode(pinLed, OUTPUT);

  menu.begin();
  //
  // Setup the menu
  draw.begin();
  //
  // Prepare managment
  rule.begin();
  //
  // Setup sounds
  buzz.begin();
  //
  // Establishe time connection
  time.begin();
}

void loop() {

  cmd.hark();  // input commands from serial

  buzz.hark();
  rule.hark();
  draw.draw(&menu);

  spanSm.tick();
  spanMd.tick();
  spanLg.tick();
  spanMinite.tick();
}

void loop_() {  // test sensors
  //rl.hark();
  /*
  Serial.print(F(" Main: "));
  Serial.print(rl.getMainLevel());

  Serial.print(F(" Well: "));
  Serial.print(rl.getWellLevel());

  Serial.println();
  //rl.testMain();
  // rl.testWell();

  delay(300);*/
}