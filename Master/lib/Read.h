#include <stdint.h>
#ifndef Read_h
#define Read_h

#include "Glob.h"


volatile unsigned long startTime;
volatile unsigned long endTime;
volatile bool echoReceived = false;

void echoInterrupt() {
  if (digitalRead(pinWellEcho) == HIGH) {
    // Rising edge detected
    startTime = micros();  // Record the time when the rising edge is detected
  } else {
    // Falling edge detected
    endTime = micros();   // Record the time when the falling edge is detected
    echoReceived = true;  // Set flag indicating that the echo pulse has been received
  }
}

class Read {

private:

  SoftwareSerial com;
  AsyncDelay timerIdle;
  AsyncDelay timerWork;

  struct LevelSensorAverage {
    uint8_t index = 0;
    uint32_t average = 0;
    bool done = false;
  };

  //
  // Average from sensors
  LevelSensorAverage
    sensorWell,
    sensorMain;

  bool isWorkRead = false;
  bool isReading = false;

  //
  // Final average valuse from sensors
  uint8_t well;
  uint8_t main;

public:
  Read()
    : com(pinMainRx, -1) {}

  void begin() {
    //
    // UAR begins
    com.begin(BaudSlaveRx);
    //
    //
    timerIdle.start(LevelRefreshTimeIdle, AsyncDelay::MILLIS);
    //
    // Well tank sensor pins
    pinMode(pinWellEcho, INPUT);
    pinMode(pinWellSend, OUTPUT);
    digitalWrite(pinWellSend, LOW);
    //
    // Main tank Slave pins
    pinMode(pinMainPower, OUTPUT);
    digitalWrite(pinMainPower, LOW);

    //  attachInterrupt(digitalPinToInterrupt(pinWellEcho), echoInterrupt, CHANGE);
  }


  void hark() {


    // if (!sensorWell.done) this->readWell();

    if (!sensorMain.done) this->readMain();


    this->readAtIdle();  // Monitoring
    this->readAtWork();  // Pumping
  }

  void test() {
    digitalWrite(pinLed, HIGH);
    digitalWrite(pinMainPower, HIGH);

    uint8_t val = 0;
    while (com.available() > 0)
      val = com.readString().toInt();

    if (val > 0) {

      Serial.println(val);
    }
  }
  //Serial.println();
  //if (com.available() > 0) {
  //  Serial.write(com.read());
  //  Serial.println();
  //  Serial.flush();
  // }


  //
  // Ouput to pass  information from this methods
  //
  uint8_t
  getWellLevel() {
    return this->well;
  }

  uint8_t getMainLevel() {
    return this->main;
  }

  //
  // Sorter period for sensors
  void startWorkRead() {

    if (!this->isWorkRead) {
      this->resetLevels();
      this->startShortReadTimer();
      Serial.println("Start Work read");
      this->isWorkRead = true;
    }
  }

  void stopWorkRead() {
    if (!ctrlWell.isOn() && !ctrlMain.isOn())
      this->isWorkRead = false;
  }

  bool isWork() {
    return this->isWorkRead;
  }

  void resetLevels() {
    sensorWell.done = false;
    sensorMain.done = false;
    this->isReading = true;
  }
  //
  // Overwrites value
  void setWell(uint8_t value) {
    this->well = value;
  }
  //
  // Overwrites value
  void setMain(uint8_t value) {
    this->main = value;
  }

private:

  void startShortReadTimer() {
    timerWork.start(LevelRefreshTimeWork, AsyncDelay::MILLIS);
    this->isReading = true;
  }

  //
  // Read well tank
  void readWell() {

    if (this->sensorWell.index < LevelSensorReads) {
      digitalWrite(pinWellSend, LOW);
      delayMicroseconds(2);
      digitalWrite(pinWellSend, HIGH);
      delayMicroseconds(10);
      digitalWrite(pinWellSend, LOW);
      this->isReading = true;
      unsigned long duration = pulseIn(pinWellEcho, HIGH, 7100);  // read pulse with timeout for 7700~130cm / 7000 ~120cm
      /*
      dbg(sensorWell.index);
      dbg(F(" / "));
      dbgLn(duration);
      */
      // Check for timeout
      if (duration == 0) {
        //     Serial.println("Timeout error: Sensor WELL reading exceeds range");
        // Handle timeout (e.g., set distance to maximum or other logic)
        return;
      }

      float distance = (duration * .0343) / 2;


      // Store distance reading
      sensorWell.average += distance;
      sensorWell.index++;  // Increment index for next reading

    } else {
      this->well = sensorWell.average / sensorWell.index;
      sensorWell.index = 0;
      sensorWell.average = 0;
      sensorWell.done = true;
      this->isReading = false;
      dbg(F("Well tank average value: "));
      dbg(this->well);
      dbgLn();
    }
  }

  //
  // Read Main tank using slave MCU
  //
  // OneWire setup (Power/Serial)
  // The OneWire Serial is desined to provide a power and
  //  communication from the serial slave sensor.
  // https://forum.arduino.cc/t/jsn-sr04t-2-0/456255/10
  void readMain() {
    if (sensorMain.index < LevelSensorReads) {

      if (digitalRead(pinMainPower)) {
        
        uint8_t read = 0;
        digitalWrite(pinLed, HIGH);
        while (com.available() > 0) {
          read = com.readString().toInt();
        }
        // Check for available data and read value
        if (read > 0) {

          // Store value in the sensorMain struct
          sensorMain.average = read;
          sensorMain.index = LevelSensorReads;  // Increment index for next reading

          dbg(F("RX: "));
          dbgLn(read);  // Print current value for debugging
          digitalWrite(pinLed, LOW);
        }
        this->isReading = true;
      } else {
        this->isReading = true;
        digitalWrite(pinMainPower, HIGH);
      }
    } else {
      this->main = sensorMain.average;
      sensorMain.index = 0;
      sensorMain.average = 0;
      sensorMain.done = true;
      dbg(F("Main tank average value: "));
      this->isReading = false;
      dbg(this->main);
      dbgLn();
      if (!this->isWorkRead)
        digitalWrite(pinMainPower, LOW);
    }
  }
  //
  // Monitors tank levels
  void readAtIdle() {
    //
    // When we idle pumps just will check levels
    if (timerIdle.isExpired()) {
      this->resetLevels();
      timerIdle.repeat();
    }
  }
  //
  // When pumps are running we will read in short periods
  void readAtWork() {
    //
    // In case something is runned manually
    if ((ctrlMain.isOn() || ctrlWell.isOn()) && !this->isWorkRead) {
      this->isWorkRead = true;
      this->isReading = true;
      this->startShortReadTimer();
    }

    if (this->isWorkRead && timerWork.isExpired()) {
      this->resetLevels();
      this->isReading = true;
      timerWork.repeat();
    }

    if (spanMx.isActive() && !this->isWorkRead && digitalRead(pinMainPower))
      digitalWrite(pinMainPower, LOW);
  }
};

#endif