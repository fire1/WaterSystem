#ifndef Time_h
#define Time_h

DS3231 rtc;

//
// Shortcut
DateTime rtcNow() {
  return RTClib::now();
}

//
// Check if it is daytime, to run pumps only in the daytime
bool isDaytime() {
  const DateTime currentTime = rtcNow();
  int currentHour = currentTime.hour();
  int currentMonth = currentTime.month();

  // Check for seasons based on the month
  bool isWinter = (currentMonth == 12 || (currentMonth >= 1 && currentMonth <= 2));
  bool isSummer = (currentMonth >= 6 && currentMonth <= 8);

  // Check for daytime based on the season
  if (isWinter) {
    // If it's winter, consider hours from 9 AM to 3 PM as daytime
    return (currentHour >= 9 && currentHour < 15);
  } else if (isSummer) {
    // If it's summer, consider hours from 10 AM to 8 PM as daytime
    return (currentHour >= 10 && currentHour < 20);
  } else {
    // For other seasons (fall and spring), consider hours from 9 AM to 5 PM as daytime
    return (currentHour >= 9 && currentHour < 17);
  }
}

//
// Function is used to check levels before weekend start, if level is lower then pumping is started.
bool isPrepareDay() {
  const DateTime currentTime = rtcNow();
  int currentDay = currentTime.dayOfTheWeek();

  // Assume Saturday and Sunday are weekend days
  const int weekendStartDay = 5;  // Friday

  // Check if the current day is the day before a weekend
  return currentDay == (weekendStartDay - 1);
}



#endif
