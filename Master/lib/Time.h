#include <stdint.h>

#ifndef Time_h
#define Time_h

#include <RTClib.h>
#include "Glob.h"

const uint8_t ClockMaxConnectAttempts = 10;

class Time {
private:
  RTC_DS3231 rtc;
  bool isConnected = false;
  bool tick = false;
  bool daytime = false;
  bool overwrite = false;
  int8_t hour = -1;


public:

  Time()
    : rtc() {}

  DateTime now() {
    return rtc.now();
  }

  bool tickClock() {
    this->tick = !this->tick;
    return this->tick;
  }

  int getTemp() {
    return (int)rtc.getTemperature();
  }

  //
  // Check is connetcted
  bool isConn() {
    return this->isConnected;
  }

  void begin() {
    Serial.print(F("Starting clock ... "));

    Wire.setWireTimeout(80000);
    this->isConnected = rtc.begin();

    if (!this->isConnected) {
      Serial.println(F(" Couldn't find RTC"));
      return;
    } else if (rtc.lostPower()) {
      Serial.println(" RTC lost power, let's set the time!");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    } else {
      Serial.println(F(" UP and running!"));
    }
  }

  /**
     * Adjust the clock
     */
  void adjust() {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  //
  // Check if it is daytime, to run pumps only in the daytime
  bool isDaytime() {
    return this->daytime;
  }

  //
  // Function is used to check levels before weekend start, if level is lower then pumping is started.
  bool isPrepareDay() {
    const DateTime currentTime = this->now();
    int currentDay = currentTime.dayOfTheWeek();

    // Assume Saturday and Sunday are weekend days
    const int weekendStartDay = 5;  // Friday

    // Check if the current day is the day before a weekend
    return currentDay == (weekendStartDay - 1);
  }

  int8_t getHour() {
    return this->hour;
  }

  void hark() {
    this->debug();


    if (!this->isConnected) return;

    if (!overwrite && spanLg.active()) {
      this->daytime = resolveDaytime();
    }

    if (this->daytime) {
      if (spanLg.active())
        digitalWrite(pinLed, !digitalRead(pinLed));
    }
  }

private:


  bool resolveDaytime() {
    const DateTime currentTime = this->now();
    int currentHour = this->hour = currentTime.hour();

    int currentMonth = currentTime.month();

    // Check for seasons based on the month
    bool isWinter = (currentMonth == 12 || (currentMonth >= 1 && currentMonth <= 2));
    bool isSummer = (currentMonth >= 6 && currentMonth <= 8);

    // Check for daytime based on the season
    if (isWinter) {
      // If it's winter, consider hours from 9 AM to 4 PM as daytime
      return (currentHour >= 9 && currentHour < 16);
    } else if (isSummer) {
      // If it's summer, consider hours from 10 AM to 8 PM as daytime
      return (currentHour >= 10 && currentHour < 20);
    } else {
      // For other seasons (fall and spring), consider hours from 9 AM to 5 PM as daytime
      return (currentHour >= 9 && currentHour < 18);
    }
  }

  void debug() {
    int8_t num = 0;

    String cmdName = F("day");

    if (cmd.set(cmdName, num)) {
      this->daytime = (bool)num;
      overwrite = true;
    }

    if (cmd.show(cmdName))
      cmd.print(F("Daytime is "), this->daytime);

    //
    // Adjust clock
    cmdName = F("clock");
    if (cmd.set(cmdName)) {
      this->adjust();
      cmd.print(F("Clock adjusted"));
    }
  }
};


#endif
