#ifndef Moon3Mode_h
#define Moon3Mode_h

#include "../Mode.h"
#include "../Moon.h"
#include "../Pump.h"
#include "../Read.h"

class Moon3Mode : public Mode {
public:
  Moon3Mode() {}

  const __FlashStringHelper *title() override { return F("3h + Moon"); }

  RunWell well(Read *read) override {
    (void)read;

    const bool rtcOk = getTime() != nullptr && getTime()->isConn();
    bool tideHigh = false;
    float ha = 0.f;
    float phase = 0.f;
    float windowH = moon::TIDE_WINDOW_BASE_HOURS;

    if (rtcOk) {
      const DateTime now = getTime()->now();
      const auto h = moon::computeHorizon(now.year(), now.month(), now.day(),
                                          now.hour(), now.minute(), SITE_LAT_DEG,
                                          SITE_LON_DEG);
      ha = h.hourAngleHours;
      phase = h.phaseFraction;
      windowH = moon::tideWindowHours(phase);
      tideHigh = moon::isLunarTideHighAt(ha, phase, SITE_TIDE_LAG_HOURS);
    }

    const moon::WellSchedule sch =
        moon::scheduleForTideMoon4(rtcOk, ha, phase, SITE_TIDE_LAG_HOURS);

    if (spanLg.active()) {
      dbg(F("[MOON] ha="));
      dbg(ha);
      dbg(F(" lagHa="));
      dbg(moon::applyTideLag(ha, SITE_TIDE_LAG_HOURS));
      dbg(F(" win="));
      dbg(windowH);
      dbg(F("h spring="));
      dbg(moon::springTideFactor(phase));
      dbg(F(" peak="));
      dbg(tideHigh ? F("yes") : F("no"));
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
