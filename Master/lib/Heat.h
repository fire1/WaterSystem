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
const float TempRT2 = 550;    // [ohms]   resistance in T2

class Heat {
private:

    Buzz *buzz;
    float heat = 0;
    uint8_t fan = 0;

    bool isAlarmOn = false;
    bool isReading = true;
    bool isHandle = true;
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
            dr->warn(MenuWarn_Heat, false);
    }

    void begin() {
        //
        // Pin 2 timer 3 setup the speed
        TCCR3B = TCCR3B & B11111000 | B00000100;   //  122.55 Hz
        pinMode(pinTmpRss, INPUT);
        pinMode(pinFanSsr, OUTPUT);
        analogWrite(pinFanSsr, 255);
        delay(400);
    }

    void hark() {
        this->debug();
        if (spanSm.active()) {
            if (this->isReading) this->read();
            if (this->isHandle) this->handle();
        }


    }

    //
    // Public access info
    //

    float getTemperature() {
        return this->heat;
    }

    void setHeat(float value) {
        this->heat = value;
        this->handle();
    }


    void setFan(uint8_t pwm) {
        this->fan = pwm;
        this->handle();
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

        if (cmd.show(F("mean"),F("Shows mean temperature for SSR.")))
            cmd.print(F("Mean temp"), TempRead.mean);

        //this->heat = map(TempRead.mean, 553, 493, 27, 33);
        // raw = C*
        // 553 = 27
        // 493 = 32

        // 325 - max

        this->heat = calculate_mf52(TempRead.mean);

        //this->heat = this->calculate(TempRead.mean); // <--- use this
        //this->heat = this->calc(TempRead.mean);

        //Serial.println(this->heat);
    }

    /**
     * Debug info for heat class
     */
    void debug() {

        //
        // Debug fan speed
        if (cmd.set(F("cool"), this->fan,F("Overwrite the fan speed."))) {
            this->isHandle = false;
            analogWrite(pinFanSsr, this->fan);
        }
        //
        // Debug temperature
        if (cmd.set(F("heat"), this->heat, F("Overwrites SSR temperature."))) this->isReading = false;

        //
        // Show internal values for cooling fan / SSR heat.
        if (cmd.show(F("cool"),  F("Shows fan speed PWM."))) cmd.print(F("Cool:"), this->fan);
        if (cmd.show(F("heat"),  F("Shows SSR temperature."))) cmd.print(F("Heat:"), this->heat);
    }

    /**
     * Calculate temperature from readings
     * @param value
     * @return
     */
    int8_t calculate(int value) {
        float Vout = TempVin * ((float) (value) / 1023.0); // calc for ntc
        float Rout = (TempRt * Vout / (TempVin - Vout));

        float TempK = (beta / log(Rout / rInf)); // calc for temperature
        return (int8_t) TempK - 273.15;
    }


    const int Rc = 5000; //value of resistance
    const int Vcc = 5; // voltage
    // Correction factor for temperature calculation, get from the datasheet.
    const float A = 1.11492089e-3;
    const float B = 2.372075385e-4;
    const float C = 6.954079529e-8;
    const float K = 2.5; //dissipation factor in mW/C

    float calculate_mf52(int raw){

        float V =  raw / 1024 * Vcc;

          float R = (Rc * V ) / (Vcc - V);


          float logR  = log(R);
          float R_th = 1.0 / (A + B * logR + C * logR * logR * logR );

          float kelvin = R_th - V*V/(K * R)*1000;
          float celsius = kelvin - 273.15;

          //printf("2/ Temperature: %.2f°C\n", celsius -10);
          return celsius -10;
    }

    /**
     * Handle temperature and control the fan
     */
    void handle() {
        if (this->heat >= stopMaxTemp) {
            if (spanLg.active())
                Serial.println(F("Warning: Overeating temperature for  SSR!"));

            ctrlMain.setOn(false);
            ctrlWell.setOn(false);
        }

        this->isAlarmOn = (this->heat >= this->edgeWorkingTemp);

        if (this->isAlarmOn && spanMd.active()) {
            buzz->alarm();
        }
        int pwm;

        if (this->heat > 40 && this->heat < 75) {
            pwm = map(this->heat, 40, 75, 5, 50);  // map temp over pwm with thresholds
        } else
            pwm = map(this->heat, 75, edgeWorkingTemp, 50, 255);  // map temp over pwm with thresholds
        //ww
        // Set end points
        if (pwm < 5) pwm = 0;
        if (pwm > 254) pwm = 254;

        this->fan = (uint8_t) pwm;

        if (pwm == 0) {
            digitalWrite(pinFanSsr, LOW);
            return;
        }
        analogWrite(pinFanSsr, this->fan);

    }
};

#endif
