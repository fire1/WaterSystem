#ifndef Time_h
#define Time_h

DS3231 rtc;

//
// Shortcut
DateTime rtcNow() {
  return RTClib::now();
}

//
// Check if it's daytime
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


class Time {
private:
    unsigned long previousMillis;
    unsigned long interval;

public:
    Time(unsigned long interval) : interval(interval) {
        previousMillis = millis();
    }

    bool elapsed() {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;
            return true;
        }
        return false;
    }
};

#endif
