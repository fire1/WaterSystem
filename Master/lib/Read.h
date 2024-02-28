#include "Arduino.h"
#include <stdint.h>
#ifndef Read_h
#define Read_h

#include "Glob.h"


volatile unsigned long startTime;
volatile unsigned long endTime;
volatile bool echoReceived = false;


class Read {

private:

  SoftwareSerial com;
  AsyncDelay timerIdle;
  AsyncDelay timerWork;

  struct LevelSensorAverage {
    uint8_t index = 0;
    uint8_t readings[LevelSensorReads];
    uint32_t average = 0;
    uint8_t error = 0;
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

#ifdef WELL_MEASURE_DEFAULT
    //
    // Well tank sensor default mode
    pinMode(pinWellEcho, INPUT);
    pinMode(pinWellSend, OUTPUT);
    digitalWrite(pinWellSend, LOW);
#endif

#ifdef WELL_MEASURE_UART_47K
    Serial3.begin(9600);
    pinMode(14, OUTPUT);
    pinMode(15, INPUT);

#endif

    //
    // Main tank Slave pins
    pinMode(pinMainPower, OUTPUT);
    digitalWrite(pinMainPower, LOW);

    //  attachInterrupt(digitalPinToInterrupt(pinWellEcho), echoInterrupt, CHANGE);
  }


  void hark() {


    if (!sensorWell.done && sensorWell.error < DisableSensorError) this->readWell();

    if (!sensorMain.done && sensorMain.error < DisableSensorError) this->readMain();


    this->readAtIdle();  // Monitoring
    this->readAtWork();  // Pumping
  }

  void test() {
    digitalWrite(pinLed, HIGH);
    digitalWrite(pinMainPower, HIGH);

    uint8_t val = 0;
    while (com.available() > 0)
      val = com.read();

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
    sensorWell.error = 0;
    sensorMain.done = false;
    sensorMain.error = 0;

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


    if (!sensorWell.done) {

    //  dbg(F("Well read "));
      uint8_t distance = 0;
#ifdef WELL_MEASURE_DEFAULT

      digitalWrite(pinWellSend, LOW);
      delayMicroseconds(2);
      digitalWrite(pinWellSend, HIGH);
      delayMicroseconds(10);
      digitalWrite(pinWellSend, LOW);
      this->isReading = true;
      unsigned long duration = pulseIn(pinWellEcho, HIGH, 7100);  // read pulse with timeout for 7700~130cm / 7000 ~120cm
      // Check for timeout
      if (duration == 0) {
        sensorWell.error++;
        return;
      }

      distance = (duration * .0343) / 2;
     // dbg(F("Default "));
#endif

#ifdef WELL_MEASURE_UART_47K

      byte frame[3];
      if (this->isReading)
        if (Serial3.available()) {
          this->isReading = false;
          byte startByte, dataHigh, dataLow, dataSum = 0;
          startByte = Serial3.read();
          if (startByte != 255) return;

          dbg(F(" /reciving/ "));

          Serial3.readBytes(frame, 3);
          dataHigh = frame[0];
          dataLow = frame[1];
          dataSum = frame[2];
          //Serial3.flush();
          //
          // Verify recived data
          if ((dataHigh + dataLow) != (dataSum + verifyCorrection)) {
            sensorWell.error++;
            return;
          } else {
            distance = ((dataHigh << 8) + dataLow) * 0.1;
            Serial3.flush();
          }
          digitalWrite(pinLed, LOW);
        }

      if (!this->isReading) {
        Serial3.setTimeout(100);
        Serial3.write(startUartCommand);
        this->isReading = true;
        dbgLn(F(" /sending UART/ "));
      }

      //dbg(F("UART "));
#endif

      dbg(distance);
      if (distance == 0) {
        sensorWell.error++;
        return;
      }

      pushAverage(sensorWell, distance);
      this->well = sensorWell.average;
      dbg(F(" AVR "));
      dbgLn(this->well);
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
    if (!sensorMain.done) {
      if (digitalRead(pinMainPower)) {

        uint8_t distance = 0;
        digitalWrite(pinLed, HIGH);
        if (com.available() > 0) {
          distance = com.read();
          com.stopListening();
        }
        //
        // Check for available data and read value
        if (distance > 0) {
          pushAverage(sensorMain, distance);
          this->main = sensorMain.average;
          dbg(F("Main read UAR "));
          dbg(distance);
          dbg(F(" AVR "));
          dbgLn(this->main);
          digitalWrite(pinLed, LOW);
          digitalWrite(pinMainPower, LOW);
        } else sensorMain.error++;
      } else {
        digitalWrite(pinMainPower, HIGH);
        com.listen();
      }
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

  //
  // Pushes new value to average buffer
  void pushAverage(LevelSensorAverage &sensor, int newValue) {
    // Subtract the oldest reading from the total
    sensor.average -= sensor.readings[sensor.index];
    // Store the new reading
    sensor.readings[sensor.index] = newValue;
    // Move to the next position in the array
    sensor.index = (sensor.index + 1) % LevelSensorReads;

    // Update the total by summing all readings
    sensor.average = 0;
    for (int i = 0; i < LevelSensorReads; ++i) {
      sensor.average += sensor.readings[i];
    }

    // Calculate the average
    sensor.average /= LevelSensorReads;

    // Mark data as done and clear any problems
    sensor.done = true;
    sensor.error = 0;
  }
};

#endif