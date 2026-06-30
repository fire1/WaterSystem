/**
 * Host unit tests for moon position and Moon4Mode schedule selection.
 *
 * Build & run (from Master/tests):
 *   make moon_test && ./moon_tests
 */
#include "../lib/Moon.h"
#include "TestHarness.h"

static constexpr float LAT = 42.7f;
static constexpr float LON = 25.5f;

// ---------------------------------------------------------------------------
// Altitude sanity
// ---------------------------------------------------------------------------

TEST(altitude_within_valid_range) {
  float alt = moon::moonAltitudeDeg(2024, 6, 15, 12, 0, LAT, LON);
  EXPECT_TRUE(alt >= -90.f && alt <= 90.f);
}

TEST(full_moon_night_moon_is_above_horizon) {
  // 2024-01-25 ~full moon, 22:00 local Bulgaria
  bool up = moon::isMoonAboveHorizon(2024, 1, 25, 22, 0, LAT, LON, 3.f);
  EXPECT_TRUE(up);
}

TEST(midday_same_date_moon_often_below_horizon) {
  float alt = moon::moonAltitudeDeg(2024, 1, 25, 12, 0, LAT, LON);
  EXPECT_TRUE(alt < 3.f);
}

TEST(horizon_margin_respected) {
  float alt = moon::moonAltitudeDeg(2024, 6, 15, 23, 0, LAT, LON);
  bool up3 = moon::isMoonAboveHorizon(2024, 6, 15, 23, 0, LAT, LON, 3.f);
  if (alt > 3.f)
    EXPECT_TRUE(up3);
  else
    EXPECT_FALSE(up3);
}

// ---------------------------------------------------------------------------
// Schedule selection (Moon4Mode logic)
// ---------------------------------------------------------------------------

TEST(schedule_rtc_off_uses_4h) {
  auto s = moon::scheduleForMoon(false, true);
  EXPECT_EQ(s.runtime, moon::MOON_RUNTIME);
  EXPECT_EQ(s.breaktime, moon::MOON_BREAK_4H);
}

TEST(schedule_moon_down_uses_4h) {
  auto s = moon::scheduleForMoon(true, false);
  EXPECT_EQ(s.breaktime, moon::MOON_BREAK_4H);
}

TEST(schedule_moon_up_uses_2h) {
  auto s = moon::scheduleForMoon(true, true);
  EXPECT_EQ(s.breaktime, moon::MOON_BREAK_2H);
}

TEST(schedule_2h_cycle_totals_120_minutes) {
  auto s = moon::scheduleForMoon(true, true);
  EXPECT_EQ((int)s.runtime + (int)s.breaktime, 120);
}

TEST(schedule_4h_cycle_matches_hours4) {
  auto s = moon::scheduleForMoon(true, false);
  EXPECT_EQ((int)s.runtime + (int)s.breaktime, 242);
}

// ---------------------------------------------------------------------------
// Lunar tide (TidRunMode logic)
// ---------------------------------------------------------------------------

TEST(tide_high_near_transit) {
  const float w = moon::TIDE_WINDOW_BASE_HOURS;
  EXPECT_TRUE(moon::isLunarTideHigh(0.f, w));
  EXPECT_TRUE(moon::isLunarTideHigh(1.f, w));
  EXPECT_TRUE(moon::isLunarTideHigh(-1.2f, w));
}

TEST(tide_high_near_nadir) {
  const float w = moon::TIDE_WINDOW_BASE_HOURS;
  EXPECT_TRUE(moon::isLunarTideHigh(12.f, w));
  EXPECT_TRUE(moon::isLunarTideHigh(-11.f, w));
  EXPECT_TRUE(moon::isLunarTideHigh(10.8f, w));
}

TEST(tide_low_midway_between_peaks) {
  const float w = moon::TIDE_WINDOW_BASE_HOURS;
  EXPECT_FALSE(moon::isLunarTideHigh(6.f, w));
  EXPECT_FALSE(moon::isLunarTideHigh(-6.f, w));
  EXPECT_FALSE(moon::isLunarTideHigh(4.f, w));
}

TEST(tide_lag_shifts_peak_detection) {
  EXPECT_TRUE(moon::isLunarTideHighAt(0.f, 0.f, 0.f));
  EXPECT_TRUE(moon::isLunarTideHighAt(2.f, 0.f, 1.f));
  EXPECT_FALSE(moon::isLunarTideHighAt(2.f, 0.f, 0.f));
}

TEST(spring_tide_widens_window_at_full_moon) {
  const float neap = moon::tideWindowHours(0.25f);
  const float spring = moon::tideWindowHours(0.f);
  EXPECT_TRUE(spring > neap);
  EXPECT_TRUE(moon::springTideFactor(0.f) > 0.9f);
  EXPECT_TRUE(moon::springTideFactor(0.25f) < 0.1f);
}

TEST(m2_half_period_matches_noaa) {
  EXPECT_EQ(moon::M2_HALF_PERIOD_MIN, 373);
  EXPECT_EQ((int)moon::M2_PERIOD_MIN, 745);
}

TEST(tide_break_totals_target_six_per_day) {
  EXPECT_EQ((int)moon::MOON_RUNTIME + (int)moon::TIDE_BREAK_HIGH, 140);
  EXPECT_EQ((int)moon::MOON_RUNTIME + (int)moon::TIDE_BREAK_LOW, 417);
}

TEST(schedule_tide_rtc_off_uses_low_break) {
  auto s = moon::scheduleForTide(false, true);
  EXPECT_EQ(s.runtime, moon::MOON_RUNTIME);
  EXPECT_EQ(s.breaktime, moon::TIDE_BREAK_LOW);
}

TEST(schedule_tide_low_uses_long_break) {
  auto s = moon::scheduleForTide(true, false);
  EXPECT_EQ(s.breaktime, moon::TIDE_BREAK_LOW);
  EXPECT_EQ((int)s.runtime + (int)s.breaktime, 417);
}

TEST(schedule_tide_high_uses_short_break) {
  auto s = moon::scheduleForTide(true, true);
  EXPECT_EQ(s.breaktime, moon::TIDE_BREAK_HIGH);
  EXPECT_EQ((int)s.runtime + (int)s.breaktime, 140);
}

// ---------------------------------------------------------------------------
// Phase fraction
// ---------------------------------------------------------------------------

TEST(phase_fraction_in_unit_interval) {
  auto h = moon::computeHorizon(2024, 1, 25, 22, 0, LAT, LON);
  EXPECT_TRUE(h.phaseFraction >= 0.f && h.phaseFraction <= 1.f);
}

TEST(full_moon_phase_near_maximum) {
  auto h = moon::computeHorizon(2024, 1, 25, 22, 0, LAT, LON);
  EXPECT_TRUE(h.phaseFraction > 0.85f);
}

// ---------------------------------------------------------------------------
// DST helper
// ---------------------------------------------------------------------------

TEST(bulgaria_summer_uses_dst) {
  EXPECT_TRUE(moon::isBulgariaDst(2024, 7, 15));
}

TEST(bulgaria_winter_no_dst) {
  EXPECT_FALSE(moon::isBulgariaDst(2024, 1, 15));
}

int main() { return run_test_harness(); }
