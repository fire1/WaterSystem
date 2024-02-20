
#ifndef Time_h
#define Time_h

#include <RTClib.h>
#include "Glob.h"

class Time {
private:
  RTC_DS3231 rtc;
  bool isConnected = false;
  bool tick = false;
  bool daytime = false;




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
    if (!rtc.begin()) {
      Serial.println(F("Couldn't find RTC"));
      Serial.flush();

    } else {
      rtc.adjust(DateTime(__DATE__, __TIME__));  // adjust data / time
      DateTime dt = now();
      this->isConnected = true;
      Serial.print(F("Adjusted clock to: "));
      Serial.print(dt.hour(), DEC);
      Serial.print(F(":"));
      Serial.print(dt.minute(), DEC);
      Serial.println();
    }
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

  void hark() {
    if (spanMx.isActive()) {
      this->daytime = resolveDaytime();
    }

    if (this->daytime) {
      if (spanLg.isActive())
        digitalWrite(pinLed, !digitalRead(pinLed));
    }
  }

private:
  bool resolveDaytime() {
    const DateTime currentTime = this->now();
    int currentHour = currentTime.hour();
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
};


#endif
