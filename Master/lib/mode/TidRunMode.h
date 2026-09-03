#ifndef TidRunMode_h
#define TidRunMode_h

#include "../Main.h"
#include "../Mode.h"
#include "../Moon.h"
#include "../Pump.h"
#include "../Read.h"

// Extra lag on top of SITE_TIDE_LAG_HOURS — shifts the pump window
// ~15 min later so Moon Tides catches the rising-pressure phase of the tide.
static constexpr float TIDE_MODE_EXTRA_LAG_HOURS = 0.25f;
//
// Peak cadence (scheduleForTide): ~3 runs per high-tide window with adaptive
// rest (45–90 min, spring/neap). Two peaks per M2 (~12h 25min) → ~6 runs/day.
// Outside peaks: wait until the next transit/nadir window (no low-tide runs).

class TidRunMode : public Mode {
public:
  TidRunMode() {}

  const __FlashStringHelper *title() override { return F("Moon Tides"); }

  // TEMP: tide-peak main transfer off — monitor well-tank rise first.
  mainTank::Intent mainTransfer(Read *read) override {
    (void)read;
    return mainTank::Intent::Default;
#if 0
    const bool rtcOk = getTime() != nullptr && getTime()->isConn();
    if (!rtcOk)
      return mainTank::Intent::Default;

    const DateTime now = getTime()->now();
    const auto h = moon::computeHorizon(now.year(), now.month(), now.day(),
                                        now.hour(), now.minute(), SITE_LAT_DEG,
                                        SITE_LON_DEG);
    const bool tideHigh =
        moon::isLunarTideHighAt(h.hourAngleHours, h.phaseFraction,
                                SITE_TIDE_LAG_HOURS + TIDE_MODE_EXTRA_LAG_HOURS);
    const uint8_t levelMain = mainTank::hasStableMain()
                                  ? mainTank::stabilizedMain()
                                  : read->getMainLevel();
    const uint8_t levelWell = read->getWellLevel();

    if (tideHigh && levelWell < mainTank::MAIN_LEVEL_WELL_MAX &&
        levelMain > mainTank::MAIN_LEVEL_MAIN_OVERRIDE)
      return mainTank::Intent::Force;

    return mainTank::Intent::Default;
#endif
  }

  RunWell well(Read *read) override {
    (void)read;

    const bool rtcOk = getTime() != nullptr && getTime()->isConn();
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
    }

    constexpr float lag = SITE_TIDE_LAG_HOURS + TIDE_MODE_EXTRA_LAG_HOURS;
    const bool tideHigh =
        rtcOk && moon::isLunarTideHighAt(ha, phase, lag);
    const moon::WellSchedule sch =
        moon::scheduleForTide(rtcOk, ha, phase, lag);

    if (spanLg.active()) {
      dbg(F("[TIDE] ha="));
      dbg(ha);
      dbg(F(" lagHa="));
      dbg(moon::applyTideLag(ha, lag));
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
