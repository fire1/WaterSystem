// Slave Test Code
// This code starts by listening for a transmission
// And it will only respond after it recieves a message
// If the message is larger than one char, change the recieve
// condition to match the new mec:\Users\fire1\OneDrive\Documents\Arduino\WhaterSystem\Master\lib\Debug.hssage characteristic
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include <AsyncDelay.h>
#include <Wire.h>
#include <DS3231.h>

#define DEBUG


//
// Communication pins
const byte pinRx = 10;         // Recive data pin for bank 2
const byte pinB2 = 8;          // Turn on power for Bank 2
const byte pinTx = -1;         // Just disable Tx pin
const byte pinLed = 13;        // LED blinks
const uint16_t rxBaud = 4800;  // This is the communication speed over serial
const byte pinTone = 9;

//
// Private libs
#include "lib/Glob.h"

//
// Define tanks
DefineData(
  tankNames,
  "None",
  "Full",
  "High",
  "Half",
  "Lows");
Data tk1(5, tankNames, 0);
Data tk2(5, tankNames, 1);


//
// Define mode
DefineData(
  modeNames,
  "None",
  "Easy",
  "Fast",
  "Now!");
Data md(4, modeNames, 2);

//
// Menu UI instance
Menu mn(tk1, tk2, md);

//
// Initialize managment driver
Rule rl(rxBaud, md, tk1, tk2);

//
// Draw driver
Draw ui;

//
// Comands
Cmd cd;

void setup() {
  //tone(pinTone, 2000);
  //
  // Setup the normal serial link to the PC
  Serial.begin(9600);
  dbgLn(F("Starting Water system MASTER..."));
  //
  // I2C start
  Wire.begin();
  //
  // LED to indicate when recieving
  pinMode(pinLed, OUTPUT);
  pinMode(pinB2, OUTPUT);
  pinMode(pinTone, OUTPUT);
  //
  // Setup the menu
  ui.begin();
  //
  // Prepare managment
  rl.begin();

  playMelody(Melodies[MELODY_BOOT]);
}

void loop(){
  rl.test();
}

void _loop() {
  cd.hark();
  rl.hark();
  ui.draw(&mn);
}
