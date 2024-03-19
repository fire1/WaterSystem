//
// Created by fire1 on 2024-03-18.
//

#ifndef Heat_h
#define Heat_h

#include "Glob.h"

//
// NTC Sensor setup
// source link https://www.instructables.com/NTC-Temperature-Sensor-With-Arduino/
const float TempVin = 5.0;     // [V]
const float TempRt = 10000;    // Resistor t [ohm]
const float TempR0 = 5000;    // value of rct in T0 [ohm] / original 1000
const float TempT0 = 298.15;   // use T0 in Kelvin [K]
// use the datasheet to get this data.
const float TempT1 = 273.15;      // [K] in datasheet 0º C
const float TempT2 = 373.15;      // [K] in datasheet 100° C
const float TempRT1 = 35563;   // [ohms]  resistance in T1
const float TempRT2 = 549;    // [ohms]   resistance in T2

class Heat {
private:

    Buzz *buzz;
    int8_t heat = 0;
    uint8_t fan = 0;

    bool isAlarmOn = false;
    bool isReading = true;
    const float beta = (log(TempRT1 / TempRT2)) / ((1 / TempT1) - (1 / TempT2));
    const float rInf = TempR0 * exp(-beta / TempT0);

    const uint8_t edgeWorkingTemp = stopMaxTemp - 10;

    struct Temperature {
        int summary = 0;
        int index = 0;
        int mean;
    } TempRead;

public:
    Heat(Buzz *bz) : buzz(bz) {

    }

    /**
     * Handle the display
     * @param dr
     */
    void warn(DrawInterface *dr) {
        if (this->isAlarmOn)
            dr->warn(WarnMenu_Heat, false);
    }

    void begin() {
        pinMode(pinTmpRss, INPUT_PULLUP);
        pinMode(pinFanRss, OUTPUT);
        analogWrite(pinFanRss, 255);
        delay(200);
    }

    void hark() {
        if (this->isReading && spanSm.isActive())
            this->read();


        this->handle();
    }

    //
    // Public access info
    //

    int8_t getTemperature() {
        return this->heat;
    }

    void setHeat(int8_t value) {
        this->heat = value;
        this->isReading = false;
        this->handle();
    }


    void setFan(uint8_t pwm) {
        this->fan = pwm;
    }


    uint8_t getFanSpeed() {
        return this->fan;
    }

private:
    /**
     * Read the sensor
     */
    void read() {
        if (TempRead.index <= TempSampleReads) {
            TempRead.summary += analogRead(pinTmpRss);
            TempRead.index++;
        }

        if (TempRead.index < TempSampleReads) return;


        TempRead.mean = TempRead.summary / TempRead.index;
        TempRead.index = 1;
        TempRead.summary = TempRead.mean;


        this->heat = this->calculate(TempRead.mean);

        //Serial.println(this->heat);
    }

    /**
     * Calculate temperature from readings
     * @param value
     * @return
     */
    int8_t calculate(uint8_t value) {
        float Vout = TempVin * ((float) (value) / 1024.0); // calc for ntc
        float Rout = (TempRt * Vout / (TempVin - Vout));

        float TempK = (beta / log(Rout / rInf)); // calc for temperature
        return (int8_t) TempK - 273.15;
    }

    /**
     * Handle temperature and control the fan
     */
    void handle() {
        if (this->heat >= stopMaxTemp) {
            if (spanLg.isActive())
                Serial.println(F("Warning: Overeating temperature for  SSR!"));

            ctrlMain.setOn(false);
            ctrlWell.setOn(false);
        }

        this->isAlarmOn = (this->heat >= this->edgeWorkingTemp);

        if (this->isAlarmOn && spanMd.isActive()) {
            buzz->alarm();
        }
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
