#include "HardwareSerial.h"
#include <stdint.h>
#include "Arduino.h"

#ifndef Rule_h
#define Rule_h

#include "Glob.h"

class Rule {
private:

    unsigned long wellTimer = 0;

    Read *read;
    Data *modeWell;
    Data *modeMain;
    Buzz *buzz;
    Time *time;

    struct Temperature {
        int summary = 0;
        int index = 0;
        int mean;
    } TempRead;

    float heat = 0;
    uint8_t fan = 0;

    float TempBeta = 0.0;    // initial parameters [K]
    float TempRinf = 0.0;    // initial parameters [ohm]

    AsyncDelay beatLed;
    uint16_t beatLedLast = 0;
    bool isDaytime = false;
    bool isAlarmOn = false;
    uint32_t timePrepareTurnOn;
    unsigned long timerNextAction = 0;
public:
    Rule(Read *rd, Time *tm, Buzz *tn, Data *mdW, Data *mdM)
            : read(rd), time(tm), buzz(tn), modeWell(mdW), modeMain(mdM), beatLed(500, AsyncDelay::MILLIS) {
    }

    void begin() {

        pinMode(pinLedBeat, OUTPUT);
        pinMode(pinWellPump, OUTPUT);
        pinMode(pinMainPump, OUTPUT);
        pinMode(pinTmpRss, INPUT);
        pinMode(pinFanRss, OUTPUT);

        analogWrite(pinFanRss, 255);

        this->timePrepareTurnOn = LevelRefreshTimeWork * LevelSensorReads - 50;

        TempBeta = (log(TempRT1 / TempRT2)) / ((1 / TempT1) - (1 / TempT2));
        TempRinf = TempR0 * exp(-TempBeta / TempT0);
    }



    void hark() {
        this->handleWellMode();
        this->handleMainMode();


        if (spanSm.isActive())
            this->readTemp();

        this->handleHeat();
    }

    int getHeat() {
        return this->heat;
    }

    void setHeat(int heat) {
        this->heat = heat;
        this->handleHeat();
    }

    void setFan(uint8_t pwm) {
        this->fan = pwm;
    }


    uint8_t getFanSpeed() {
        return this->fan;
    }

    unsigned long getActionTimer() {
        if (timerNextAction > 0)
            return timerNextAction - millis();
        return 0;
    }

private:

    unsigned long calcMinutes(unsigned int minutes) {
        return minutes * 60 * 1000UL;  // UL ensures the result is treated as an unsigned long
    }

    bool checkDaytime() {
        //
        // Wrapping time class locally
        if (time->isConn()) {
            //
            // Check for daytime each minutes
            if (spanMx.isActive())
                this->isDaytime = time->isDaytime();  //pass state for daytime locally

        } else {
            this->isDaytime = true;  // skip daytime check since there is no clock
        }

        return this->isDaytime;
    }

    //
    // Defines work amplitude for Pump1
    void pumpWell(uint8_t workMin, uint16_t stopMin) {

#ifdef CHECK_DAYTIME
        if (!this->checkDaytime()) {
            this->timerNextAction=0;
          //
          // Stop the system
          if (spanMx.isActive())  // every second display warning
            Serial.println(F("Warning: STOP /well/ It is not daytime!"));
          return;
        }
#endif

        //
        // Stop when is full well tank
        if (ctrlWell.isOn() && LevelSensorBothMax >= read->getWellLevel()) {
            Serial.println(F("Warning: STOP Well tank is full!"));
            dbg(read->getWellLevel());
            dbg(F("cm / "));
            dbg(LevelSensorBothMax);
            dbg(F("cm "));
            dbgLn();
            ctrlWell.setOn(false);
            read->stopWorkRead();
            return;
        }

        unsigned long msTimeToOff = this->calcMinutes(workMin);
        unsigned long msTimeToOn = this->calcMinutes(stopMin);

        //
        // Turn pump OFF by timeout of mode
        if (ctrlWell.isOn() && (millis() - wellTimer >= msTimeToOff)) {
            ctrlWell.setOn(false);
            wellTimer = millis();
            this->timerNextAction = wellTimer + msTimeToOn;
            dbg(F("CTRL well /"));
            dbg(stopMin);
            dbg(F("min/ pump is Off "));
            dbg(F(" ~next: "));
            Serial.println(this->timerNextAction);
            dbgLn();


            read->stopWorkRead();
            //
            // Set next

        }
        //
        // Prepare, read levels before start
        if (!ctrlWell.isOn() && !ctrlWell.isOn() && (millis() - wellTimer >= (msTimeToOn - timePrepareTurnOn))) {
            if (spanLg.isActive()) {
                read->startWorkRead();
                buzz->alarm();

                dbg(F("Prepare levels /Well/ "));
                dbgLn();
            }
        }
        //
        // Turn pump on
        if (!ctrlWell.isOn() && !ctrlWell.isOn() && (millis() - wellTimer >= msTimeToOn)) {
            this->isAlarmOn = false;
            wellTimer = millis();
            this->timerNextAction = wellTimer + msTimeToOff;


            dbg(F("CTRL well /"));
            dbg(workMin);
            dbg(F("min/ pump  is On "));
            dbg(F(" ~next: "));
            Serial.println(this->timerNextAction);
            dbgLn();


            ctrlWell.setOn(true);


        }
    }

    //
    // Controls pump1
    void handleWellMode() {
        // if (spanMd.isActive()) {
        //   uint8_t targetLevel = this->getTargetMode(this->modeWell, LevelSensorWellMin);
        //   Serial.println(targetLevel);
        // }

        //
        // Stop when Main is full
        if (ctrlMain.isOn() && LevelSensorBothMax >= read->getMainLevel()) {
            ctrlMain.setOn(false);

            dbgLn(F("CTRL /Main/ turn off  /TOP FULL/"));
            read->stopWorkRead();
        }

        if (ctrlMain.isOn() && 85 <= read->getWellLevel()) {
            ctrlMain.setOn(false);

            dbgLn(F("CTRL /Main/ turn off /Well empty/"));
            read->stopWorkRead();
        }


        switch (modeWell->value()) {
            default:
            case 0:
                // Noting
                beatWell(0);  // Disables the led heartbeat
                break;

            case 1:
                // Easy
                beatWell(1500);
                pumpWellSchedule(ScheduleWellOnMainEasy);

                break;

            case 2:
                // Fast
                beatWell(800);
                pumpWellSchedule(ScheduleWellOnMainFast);
                break;

            case 3:
                // Now!
                beatWell(400);
                pumpWell(15, 20);
                break;
        }
    }

    void pumpWellSchedule(const PumpSchedule &schedule) {
        uint8_t main = read->getMainLevel();
        uint16_t stop = 180;

        if (main > 18) {
            for (int i = 0; i < schedule.intervals; ++i) {
                if (schedule.levels[i] > main) {
                    stop = schedule.stops[i];
                    break;  // Exit loop once stop is found
                }
            }
        }

        pumpWell(schedule.workMin, stop);
    }

    /**
     * Just turns on the pump to main
     */
    void pumpMain() {

#ifdef CHECK_DAYTIME
        if (!this->checkDaytime()) {
          //
          // Stop the system
          if (spanMx.isActive())  // every second display warning
            Serial.println(F("Warning: STOP /main/ It is not daytime!"));
          return;
        }
#endif

        uint8_t main = read->getMainLevel();

        if (!ctrlMain.isOn() && !ctrlWell.isOn()) {

            read->startWorkRead();

            dbg(F("CTRL /Main/ at level "));
            dbg(main);
            dbg(F("cm turn ON"));
            dbgLn();

            ctrlMain.setOn(true);
        }
    }

    void handleMainMode() {
        uint8_t levelMain = read->getMainLevel();
        uint8_t levelWell = read->getWellLevel();

        //
        // Stop this function when sensor is not avialable
        if (levelMain == 0)
            return;

        // Mapping values from 20 to 95, like 20 is Full and 95 empty
        switch (modeMain->value()) {
            default:
            case 0:  // Noting
                break;
            case 1:  // Full
                if (levelMain > 32 && levelWell < 60)
                    return pumpMain();
            case 2:  // Half
                if (levelMain > 47 && levelWell < 40)
                    return pumpMain();
            case 3:  // Void
                if (levelMain > 80 && levelWell < 30)
                    return pumpMain();
        }
    }


    uint8_t getTargetMode(Data *mode, const uint8_t levelMin) {
        return map(mode->value(), 0, mode->length(), LevelSensorBothMax, levelMin);
    }

    void beatWell(int ms) {

        if (ms == 0) {
            //
            // Turn led off
            digitalWrite(pinLedBeat, HIGH);
            return;
        }

        if (ms != beatLedLast) {
            beatLed.start(ms, AsyncDelay::MILLIS);
            beatLedLast = ms;
        }
        if (beatLed.isExpired()) {
            digitalWrite(pinLedBeat, !digitalRead(pinLedBeat));  // Toggle LED state
            if (ms == beatLedLast && ms != 0) {
                beatLed.repeat();
            }
        }
    }

    void readTemp() {
        if (TempRead.index <= TempSampleReads) {
            TempRead.summary += analogRead(pinTmpRss);
            TempRead.index++;
        }

        if (TempRead.index < TempSampleReads) return;


        TempRead.mean = TempRead.summary / TempRead.index;
        TempRead.index = 1;
        TempRead.summary = TempRead.mean;


        float Vout = TempVin * ((float) (TempRead.mean) / 1024.0); // calc for ntc
        float Rout = (TempRt * Vout / (TempVin - Vout));

        float TempK = (TempBeta / log(Rout / TempRinf)); // calc for temperature
        this->heat = TempK - 273.15;

        //Serial.println(this->heat);
    }


    //
    // Handles the overheating protection
    void handleHeat() {
        if (this->heat >= stopMaxTemp) {
            if (spanLg.isActive())
                Serial.println(F("Warning: Overeating temperature for  SSR!"));

            ctrlMain.setOn(false);
            ctrlWell.setOn(false);
        }

        const uint8_t edgeWorkingTemp = stopMaxTemp - 3;

        if (this->heat >= edgeWorkingTemp && spanMd.isActive())
            buzz->alarm();
        int pwm;

        if (this->heat > 30 && this->heat < 75) {
            pwm = map(this->heat, 30, 75, 2, 15);  // map temp over pwm with thresholds
        } else
            pwm = map(this->heat, 75, edgeWorkingTemp, 50, 254);  // map temp over pwm with thresholds
        //
        // Set end points
        if (pwm < 2) pwm = 0;
        if (pwm > 254) pwm = 254;


        this->fan = (uint8_t) pwm;

        if (spanSm.isActive()) {
            if (pwm == 0) {
                digitalWrite(pinFanRss, LOW);
                return;
            }
            analogWrite(pinFanRss, this->fan);
        }
    }
};


#endif
