#include <stdint.h>

#ifndef Time_h
#define Time_h

#include <EEPROM.h>
#include <RTClib.h>
#include "Glob.h"
#include "Moon.h"

const uint8_t ClockMaxConnectAttempts = 10;
/** EEPROM: last applied Bulgaria DST state for the DS3231 wall clock. */
static constexpr uint8_t E_ADDR_DST_FLAG = 4;
/** EEPROM: DST logic scheme (bumped when first-boot migrate semantics change). */
static constexpr uint8_t E_ADDR_DST_SCHEME = 5;
/** Scheme 1: first install / broken "seed calendar, no ±1h" → one opposite seed + step. */
static constexpr uint8_t DST_SCHEME_VER = 1;

class Time {
private:
  RTC_DS3231 rtc;
  bool isInit = true;
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

  float getTemp() {
    return rtc.getTemperature();
  }

  //
  // Check is connetcted
  bool isConn() {
    return this->isConnected;
  }

  void begin() {
    Serial.print(F("Connecting to the clock ... "));

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

    // Align wall clock to EU DST if EEPROM flag disagrees (e.g. after
    // power-off across the Oct/Mar change).
    this->applyDstCorrection();
  }

  /**
     * Adjust the clock
     */
  void adjust() {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // Compile-time stamp is local; mark DST flag aligned to calendar.
    const DateTime n = rtc.now();
    const bool dst = moon::isBulgariaDst(n.year(), n.month(), n.day());
    EEPROM.update(E_ADDR_DST_FLAG, moon::dstFlagFromCalendar(dst));
    EEPROM.update(E_ADDR_DST_SCHEME, DST_SCHEME_VER);
  }

  /** Nudge wall clock by whole hours and sync DST flag to the calendar. */
  void skewHours(int8_t hours) {
    if (!this->isConnected || hours == 0)
      return;
    const DateTime n = rtc.now();
    if (hours > 0)
      rtc.adjust(n + TimeSpan(0, hours, 0, 0));
    else
      rtc.adjust(n - TimeSpan(0, -hours, 0, 0));
    const DateTime after = rtc.now();
    const bool dst =
        moon::isBulgariaDst(after.year(), after.month(), after.day());
    EEPROM.update(E_ADDR_DST_FLAG, moon::dstFlagFromCalendar(dst));
    EEPROM.update(E_ADDR_DST_SCHEME, DST_SCHEME_VER);
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

  void loop() {
    this->debug();


    if (!this->isConnected) return;

    if (spanLg.active())
      this->applyDstCorrection();

    //
    // Resolve daytime at start of sketch
    if (!overwrite && spanLg.active() || isInit) {
      this->daytime = resolveDaytime();
    }

    if (this->daytime) {
      if (spanLg.active())
        digitalWrite(pinLed, !digitalRead(pinLed));
    }
    //
    // Close init run
    isInit = false;
  }

private:

  /**
   * Keep DS3231 on Bulgaria civil time (EET/EEST).
   * RTC has no timezone — we step ±1 h when the calendar DST state changes.
   *
   * First install (scheme unset / old): DS3231 usually still holds the previous
   * season's offset. Seed the *opposite* of the calendar flag so the missed
   * ±1 h runs once, then mark scheme current. Later boots only step when the
   * stored flag disagrees with the calendar.
   */
  void applyDstCorrection() {
    if (!this->isConnected)
      return;

    const DateTime n = rtc.now();
    const bool calendarDst =
        moon::isBulgariaDst(n.year(), n.month(), n.day());
    uint8_t stored = EEPROM.read(E_ADDR_DST_FLAG);
    const bool needMigrate =
        EEPROM.read(E_ADDR_DST_SCHEME) != DST_SCHEME_VER;

    if (needMigrate || stored == moon::DST_FLAG_UNSET) {
      // Opposite of calendar → SpringForward / FallBack on this same pass.
      stored = moon::dstMigrateSeedFlag(calendarDst);
      EEPROM.update(E_ADDR_DST_FLAG, stored);
      EEPROM.update(E_ADDR_DST_SCHEME, DST_SCHEME_VER);
      Serial.print(F("[CLK] DST migrate seed "));
      Serial.println(stored == moon::DST_FLAG_SUMMER ? F("EEST") : F("EET"));
    }

    const moon::DstClockAction action =
        moon::dstClockAction(calendarDst, stored);
    if (action == moon::DstClockAction::None)
      return;

    if (action == moon::DstClockAction::SpringForward) {
      rtc.adjust(n + TimeSpan(0, 1, 0, 0));
      Serial.println(F("[CLK] DST spring forward +1h (EEST)"));
    } else {
      rtc.adjust(n - TimeSpan(0, 1, 0, 0));
      Serial.println(F("[CLK] DST fall back -1h (EET)"));
    }
    EEPROM.update(E_ADDR_DST_FLAG, moon::dstFlagFromCalendar(calendarDst));
    EEPROM.update(E_ADDR_DST_SCHEME, DST_SCHEME_VER);
  }

  /**
   * Resolves is a daytime
   *  based on the season.
   * */
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
      return (currentHour >= 9 && currentHour <= 16);
    } else if (isSummer) {
      //
      // Full work daytime
      if (this->getTemp() < 34)
        // If it's summer, consider hours from 8 AM to 9 PM as daytime
        return (currentHour >= 8 && currentHour <= 21);
      //
      // Add a lunch break when is hot
      else
        // Return true if the current hour is between 8 and 21, excluding 12 to 15
        return (currentHour >= 8 && currentHour < 12) || (currentHour > 15 && currentHour <= 21);

    } else {
      // For other seasons (fall and spring), consider hours from 9 AM to 6 PM as daytime
      return (currentHour >= 9 && currentHour <= 18);
    }
  }

  //
  // Handles the debug data of the class.
  void debug() {
    int8_t num = 0;

    String cmdName = F("day");

    if (cmd.set(cmdName, num, F("Overwrites the daytime state."))) {
      this->daytime = (bool)num;
      overwrite = true;
    }

    if (cmd.show(cmdName, F("Show is a daytime.")))
      cmd.print(F("Daytime is "), this->daytime);

    //
    // Adjust clock
    cmdName = F("clock");
    if (cmd.set(cmdName, num, F("Adjusts the clock time."))) {
      this->adjust();
      cmd.print(F("Clock adjusted"), F("OK"));
    }

    int8_t skew = 0;
    if (cmd.set(F("clock:skew"), skew,
                F("Nudge RTC by whole hours (e.g. -1 / +1)."))) {
      this->skewHours(skew);
      cmd.print(F("Clock skewed h"), skew);
    }

    if (cmd.show(F("clock:dst"), F("Show RTC DST flag / calendar."))) {
      const DateTime n = this->now();
      const bool cal = moon::isBulgariaDst(n.year(), n.month(), n.day());
      const uint8_t fl = EEPROM.read(E_ADDR_DST_FLAG);
      Serial.print(F("Calendar "));
      Serial.print(cal ? F("EEST") : F("EET"));
      Serial.print(F(" flag="));
      if (fl == moon::DST_FLAG_UNSET)
        Serial.print(F("unset"));
      else
        Serial.print(fl == moon::DST_FLAG_SUMMER ? F("EEST") : F("EET"));
      Serial.print(F(" scheme="));
      Serial.println(EEPROM.read(E_ADDR_DST_SCHEME));
    }
  }
};


#endif
