#ifndef Moon4Mode_h
#define Moon4Mode_h

#include "../Mode.h"
#include "../Moon.h"
#include "../Pump.h"
#include "../Read.h"

class Moon4Mode : public Mode {
public:
  Moon4Mode() {}

  const __FlashStringHelper *title() override { return F("Moon"); }

  RunWell well(Read *read) override {
    (void)read;

    const bool rtcOk = getTime() != nullptr && getTime()->isConn();
    bool moonUp = false;
    float altitude = -90.f;

    if (rtcOk) {
      const DateTime now = getTime()->now();
      altitude = moon::moonAltitudeDeg(now.year(), now.month(), now.day(),
                                       now.hour(), now.minute(), SITE_LAT_DEG,
                                       SITE_LON_DEG);
      moonUp = altitude > MOON_HORIZON_MARGIN_DEG;
    }

    const moon::WellSchedule sch =
        moon::scheduleForMoon(rtcOk, moonUp);

    if (spanLg.active()) {
      dbg(F("[MOON] alt="));
      dbg(altitude);
      dbg(F(" cycle="));
      dbg(sch.breaktime == moon::MOON_BREAK_2H ? F("2h") : F("4h"));
      dbg(F(" work="));
      dbg(sch.runtime);
      dbg(F("m break="));
      dbg(sch.breaktime);
      dbgLn(F("m"));
    }

    return RunWell{sch.runtime, sch.breaktime};
  }
};

#endif
