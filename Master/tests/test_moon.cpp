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
