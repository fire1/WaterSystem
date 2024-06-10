
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
    AsyncDelay powerMain;

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
    bool isWellReadSent;

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
        if (millis() < 200) return;

        this->debug();

        if (spanMx.active()) {
            sensorWell.error = 0;
            sensorMain.error = 0;
        }


        if (!sensorWell.done /*&& sensorWell.error < DisableSensorError*/) this->readWell();

        if (!sensorMain.done /*&& sensorMain.error < DisableSensorError*/) {
            powerMain.start(TimeoutPowerSlave, AsyncDelay::MILLIS);  // Set power downtime for main sensor
            this->readMain();
        }


        this->readAtIdle();  // Monitoring
        this->readAtWork();  // Pumping


        if (powerMain.isExpired() && digitalRead(pinMainPower)) {
            digitalWrite(pinMainPower, LOW);
            dbgLn("Turning Off Main sensor power.");
        }
        /*
            if (spanLg.active()) {
                if (sensorWell.error >= DisableSensorError)
                    Serial.println(F("Warning: Unable to read well sensor!"));
                if (sensorMain.error >= DisableSensorError)
                    Serial.println(F("Warning: Unable to read main sensor!"));
            }
            */
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


    //
    // Output to pass  information from this methods
    //
    uint8_t getWellLevel() {
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
        if (!isWorkRead) sensorWell.error = 0;
        sensorMain.done = false;
        if (!isWorkRead) sensorMain.error = 0;
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

    void expireWorkTimer() {
        timerWork.expire();
    }

private:

    //
    // When pumps are on or displays tank levels, this timer will be executed.
    void startShortReadTimer() {
        if (this->main < 22)
            // When main tanks is reaching almost full is good to switch to faster read,
            //  since the pumping is faster than well.
            timerWork.start(LevelRefreshTimeWork / 2, AsyncDelay::MILLIS);
        else
            // A common time to read tank levels faster.
            timerWork.start(LevelRefreshTimeWork, AsyncDelay::MILLIS);
    }

    //
    // Read a well tank.
    void readWell() {
        if (!sensorWell.done) {
            // Read well sensor has two variants of reading distance.
            // The default type for the module is not recommended since disturbs main loop.
            // Soldering 47k Ohm resistor for R19 on the module is HIGHLY recommended,
            // this will enable UART of the module.
            uint16_t distance;
            if (onReadWellSensorDistance(distance)) {
                if (distance == 0) {
                    sensorWell.error++;
                    return;
                }
                dbg(distance);
                pushAverage(sensorWell, distance);
                this->well = sensorWell.average;
                dbg(F(" AVR "));
                dbgLn(this->well);
            }
        }
    }

    //
    // Read Main tank using Slave MCU
    //
    // OneWire setup (Power/Serial)
    // The OneWire Serial is desined to provide a power and
    //  communication from the serial Slave sensor.
    // Https://forum.arduino.cc/t/jsn-sr04t-2-0/456255/10
    void readMain() {
        if (sensorMain.done) return;
        if (digitalRead(pinMainPower) && com.isListening()) {
            if (com.available() > 0) {
                uint8_t distance = 0;
                digitalWrite(pinLed, HIGH);
                distance = com.read();
                com.stopListening();
                //
                // Check for available data and read value
                if (distance > 0) {
                    pushAverage(sensorMain, distance);
                    this->main = sensorMain.average;
                    dbg(F("Main read UAR "));
                    dbg(distance);
                    dbg(F(" AVR "));
                    dbgLn(this->main);
                } else {
                    sensorMain.error++;
                }
                digitalWrite(pinLed, LOW);
            }
        } else {
            if (!com.isListening()) com.listen();
            digitalWrite(pinMainPower, HIGH);
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
        // In case something is run manually
        if ((ctrlMain.isOn() || ctrlWell.isOn()) && !this->isWorkRead) {
            this->isWorkRead = true;
            this->startShortReadTimer();
        }


        if (this->isWorkRead && timerWork.isExpired()) {
            this->resetLevels();
            timerWork.repeat();
        }
    }

    /**
     * Sets average for a sensor.
     * @param sensor
     * @param newValue
     */
    void pushAverage(LevelSensorAverage &sensor, int newValue) {
        // Subtract the oldest reading from the total
        sensor.average -= sensor.readings[sensor.index];
        // Store the new reading
        sensor.readings[sensor.index] = newValue;
        // Move to the next position in the array
        sensor.index = (sensor.index + 1) % LevelSensorReads;

        // Calculate the average
        sensor.average = 0;
        for (int i = 0; i < LevelSensorReads; ++i) {
            sensor.average += sensor.readings[i];
        }
        sensor.average /= LevelSensorReads;

        // Mark data as done and clear any problems
        sensor.done = true;
        sensor.error = 0;
    }

    //
    // Using UART Serial, resistor 47K soldiered on the module
#ifdef WELL_MEASURE_UART_47K

    /**
     * Parse distance from the sensor /RECOMMENDED/
     * @param distance
     * @return
     */
    bool onReadWellSensorDistance(uint16_t &distance) {
        distance = 0;
        if (this->isWellReadSent && Serial3.available()) {
            digitalWrite(pinLed, HIGH);
            byte startByte, dataTop, dataLow, dataSum = 0;
            startByte = Serial3.read();
            if (startByte != 255) return true;

            dbg(F(" /UART/ Receiving "));

            byte readFrames[3];
            Serial3.readBytes(readFrames, 3);
            digitalWrite(pinLed, LOW);

            dataTop = readFrames[0];
            dataLow = readFrames[1];
            dataSum = readFrames[2];

            //
            // Verify received data by comparing two chunks of the received data.
            if ((dataTop + dataLow) != (dataSum + verifyCorrection)) {
                //
                // Nothing to do, distance is already 0
            } else
                distance = ((dataTop << 8) + dataLow) * 0.1;

            digitalWrite(pinLed, LOW);
            return true;  // finish the reading
        } else if (spanLg.active()) {
            Serial3.setTimeout(50);
            Serial3.write(startUartCommand);
            this->isWellReadSent = true;
            // dbgLn(F(" /UART/ Sending  "));
        }
        return false;
    }

#endif
    //
    // Using pulseIn method, but not recommended since using delay
#ifdef WELL_MEASURE_DEFAULT
    /**
     * Parse distance from the sensor /BACKUP/
     * @param distance
     * @return
     */
    bool onReadWellSensorDistance(uint16_t &distance) {
      digitalWrite(pinWellSend, LOW);
      delayMicroseconds(2);
      digitalWrite(pinWellSend, HIGH);
      delayMicroseconds(10);
      digitalWrite(pinWellSend, LOW);
      //
      // read pulse with timeout for 7700~130cm / 7000 ~120cm
      unsigned long duration = pulseIn(pinWellEcho, HIGH, 7100);

      // Check for timeout accure
      if (duration == 0)
        distance = 0;
      else
        distance = (duration * .0343) / 2;
      return true;
    }

#endif

    /**
     * Handles debug
     */
    void debug() {
        cmd.set(F("well"), this->well, F("Overwrite well tank level."));
        cmd.set(F("main"), this->main, F("Overwrite main tank level."));

        if (cmd.show(F("well"),F("Show well tank level.")))
            cmd.print(F("Well level is:"), this->well);

        if (cmd.show(F("main"),F("Show main tank level.")))
            cmd.print(F("Main level is:"), this->well);
    }
};

#endif
