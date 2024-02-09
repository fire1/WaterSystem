//
// Slave code to send information to the master
// Chip used to send data is Atmega8.
//
#include <SoftwareSerial.h>

#define DEBUG 1

#if DEBUG == 1
#define dbg(x) Serial.print(x)
#define dbgLn(x) Serial.println(x)
#else
#define dbg(x)
#define dbgLn(x)
#endif






const uint8_t numLeds = 10;       // Number of LEDs
const uint8_t maxDistance = 100;  // Maximum distance in centimeters
const uint8_t minDistance = 20;   // Minimum distance in centimeters

// Map function to map distance values to the range of LEDs
uint8_t mapDistanceToLEDs(uint8_t distance) {
  return map(distance, minDistance, maxDistance, 0, numLeds);
}

// Pin numbers for the LED bar array
const int ledPins[numLeds] = { 14, 8, 7, 6, 5, 9, 10, 11, 12, 13 };

//
// Lightup the ledbar
void lightUp(int data, unsigned int upTime = 25) {
  //
  // Turn off all LEDs
  for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }
  //
  // Turn on LEDs based on the mapped distance value
  uint8_t ledsToTurnOn = mapDistanceToLEDs(data);
  Serial.print("LD ");
  Serial.println(ledsToTurnOn);
  for (int i = 0; i < ledsToTurnOn; i++) {
    digitalWrite(ledPins[i], HIGH);
    delay(upTime);  // Adjust the delay time as needed
    digitalWrite(ledPins[i], LOW);
  }

  //
  // Turn on the last LED and leave it ON
  digitalWrite(ledPins[ledsToTurnOn], HIGH);
}

const byte pinEch = 2;
const byte pinTrg = 3;
//
// Read sensor distance
uint16_t readSensor() {
  digitalWrite(pinTrg, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrg, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrg, LOW);

  float duration = pulseIn(pinEch, HIGH);
  float distance = (duration * .0343) / 2;
  // Serial.print("Distance: ");
  // Serial.println(distance);

  delay(50);
  return distance;
}

const byte pinTx = 4;
SoftwareSerial com(-1, pinTx, true);

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
float index = 0, distances;
int data = 0;
void loop() {
  // float read = getSurfaceDistance();
  float read = readSensor();

  //  dbg(F("rd: "));
  //  Serial.println(read);

  distances += read;
  index++;

  if (index > 15) {

    data = distances / index;
    index = 1;
    distances = data;

    lightUp(data);

    dbg(F("\t\t TX: "));
    dbg(data);
    dbgLn();

    com.print(data, DEC);
  }
}
