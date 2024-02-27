
#if DEBUG == 1
#define dbg(x) Serial.print(x)
#define dbgLn(x) Serial.println(x)
#else
#define dbg(x)
#define dbgLn(x)
#endif

//
// Map function to map distance values to the range of LEDs
uint8_t mapDistanceToLEDs(uint8_t distance) {
  return map(distance, minDistance, maxDistance, 0, numLeds);
}



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

  delay(20);
  return distance;
}