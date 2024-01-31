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
const byte pinRx = 10;
const byte pinTx = 11;
const byte pinLed = 13;
const uint16_t rxBaud = 4800;  // This is the communication speed over serial

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
Rule rl(rxBaud, pinLed, md, tk1, tk2);

//
// Draw driver
Draw ui;

//
// Comands
Cmd cd;

void setup() {
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
  //
  // Setup the menu
  ui.begin();
  //
  // Prepare managment
  rl.begin();
}

void loop() {
  cd.hark();
  rl.hark();
  ui.draw(&mn);
}
