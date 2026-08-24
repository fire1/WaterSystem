/**
 * Host unit tests for moon position and Moon3Mode schedule selection.
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
// Schedule selection (Moon3Mode logic)
// ---------------------------------------------------------------------------

TEST(schedule_moon4_rtc_off_uses_4h) {
  auto s = moon::scheduleForTideMoon4(false, 0.f, 0.f, 0.f);
  EXPECT_EQ(s.runtime, moon::MOON_RUNTIME);
  EXPECT_EQ(s.breaktime, moon::MOON_BREAK_4H);
}

TEST(schedule_moon4_tide_high_uses_3h) {
  auto s = moon::scheduleForTideMoon4(true, 0.f, 0.f, 0.f);
  EXPECT_EQ(s.breaktime, moon::MOON_BREAK_3H);
  EXPECT_EQ((int)s.runtime + (int)s.breaktime, 180);
}

TEST(schedule_moon4_mid_gap_targets_half_period) {
  const float w = moon::tideWindowHours(0.f);
  // Lagged HA at low-tide midpoint (|HA|≈6)
  auto s = moon::scheduleForTideMoon4(true, 6.f, 0.f, 0.f);
  const uint16_t expected =
      moon::clampBreakMinutes(moon::haDeltaToMinutes((12.f - w) - 6.f));
  EXPECT_EQ(s.breaktime, expected);
}

TEST(schedule_moon4_approaching_mid_waits_for_midpoint) {
  // |HA|=4 → 2h of HA until mid at 6
  auto s = moon::scheduleForTideMoon4(true, 4.f, 0.f, 0.f);
  const uint16_t expected = moon::clampBreakMinutes(moon::haDeltaToMinutes(2.f));
  EXPECT_EQ(s.breaktime, expected);
}

TEST(schedule_moon4_3h_cycle_totals_180_minutes) {
  auto s = moon::scheduleForTideMoon4(true, 0.f, 0.f, 0.f);
  EXPECT_EQ((int)s.runtime + (int)s.breaktime, 180);
}

TEST(schedule_moon4_4h_fallback_totals_242_minutes) {
  auto s = moon::scheduleForTideMoon4(false, 0.f, 0.f, 0.f);
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

TEST(schedule_tide_rtc_off_uses_4h_break) {
  auto s = moon::scheduleForTide(false, 0.f, 0.f, 0.f);
  EXPECT_EQ(s.runtime, moon::MOON_RUNTIME);
  EXPECT_EQ(s.breaktime, moon::MOON_BREAK_4H);
}

TEST(schedule_tide_low_before_transit_waits_for_peak_window) {
  const float w = moon::tideWindowHours(0.f);
  auto s = moon::scheduleForTide(true, -4.f, 0.f, 0.f);
  const uint16_t expected =
      moon::clampBreakMinutes(moon::haDeltaToMinutes(4.f - w));
  EXPECT_EQ(s.breaktime, expected);
}

TEST(schedule_tide_low_after_transit_waits_for_nadir_peak_window) {
  const float w = moon::tideWindowHours(0.f);
  auto s = moon::scheduleForTide(true, 4.f, 0.f, 0.f);
  const uint16_t expected =
      moon::clampBreakMinutes(moon::haDeltaToMinutes((12.f - w) - 4.f));
  EXPECT_EQ(s.breaktime, expected);
}

TEST(schedule_tide_mid_gap_skips_extra_low_run) {
  const float w = moon::tideWindowHours(0.f);
  auto s = moon::scheduleForTide(true, 6.f, 0.f, 0.f);
  const uint16_t expected =
      moon::clampBreakMinutes(moon::haDeltaToMinutes((12.f - w) - 6.f));
  EXPECT_EQ(s.breaktime, expected);
}

TEST(schedule_tide_high_uses_3h_cycle) {
  auto s = moon::scheduleForTide(true, 0.f, 0.f, 0.f);
  EXPECT_EQ(s.breaktime, moon::MOON_BREAK_3H);
  EXPECT_EQ((int)s.runtime + (int)s.breaktime, 180);
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
