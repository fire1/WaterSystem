//
// Slave code to send information to the master
// Chip used to send data is Atmega8.
//
#include <Arduino.h>
#include <SoftwareSerial.h>

#define DEBUG 1  // Enables Serial output

const uint8_t numLeds = 10;       // Number of LEDs
const uint8_t maxDistance = 100;  // Maximum distance in centimeters
const uint8_t minDistance = 20;   // Minimum distance in centimeters

// Pin numbers for the LED bar array
const int ledPins[numLeds] = { 14, 8, 7, 6, 5, 9, 10, 11, 12, 13 };


const byte pinEch = 2;
const byte pinTrg = 3;


const byte pinTx = 4;
SoftwareSerial com(-1, pinTx, true);

#include "Lib.h"

void setup() {
  //
  // Prapare LED pins
  for (int i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  pinMode(pinEch, INPUT);
  pinMode(pinTrg, OUTPUT);
  pinMode(pinTx, OUTPUT);
  //
  // Do not place pinMode for Serial pins
  //
  com.begin(4800);
  //
  // Start the PC serial connection
  Serial.begin(9600);
  dbgLn(F("\n Slave System"));
}

//
// Distance in cm
int index = 0;
double distances;
int data = 0;
void loop() {
  // float read = getSurfaceDistance();
  float read = readSensor();

  //  dbg(F("rd: "));
  //  Serial.println(read);

  distances += read;
  index++;

  if (index > 60) {

    data = distances / index;
    index = 0;
    distances = data;

    lightUp(data);

    dbg(F("\t\t TX: "));
    dbg(data);
    dbgLn();
    //
    // Send byte data to Master
    com.write(data);
    delay(1200);
  }
}
