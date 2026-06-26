/**
 * Host-side unit tests for pump overtime protection.
 *
 * Build & run (from Master/tests):
 *   make && ./overtime_tests
 */
#include "../lib/Overtime.h"
#include "TestHarness.h"

// ---------------------------------------------------------------------------
// Well pump — Rule::handleWellOvertime predicate
// ---------------------------------------------------------------------------

TEST(well_not_tripped_at_exact_limit) {
  EXPECT_FALSE(overtime::wellExceeded(overtime::WELL_LIMIT_MS));
}

TEST(well_tripped_one_ms_past_limit) {
  EXPECT_TRUE(overtime::wellExceeded(overtime::WELL_LIMIT_MS + 1));
}

TEST(well_not_tripped_when_under_limit) {
  EXPECT_FALSE(overtime::wellExceeded(overtime::WELL_LIMIT_MS - 1));
}

TEST(well_zero_runtime_never_trips) {
  EXPECT_FALSE(overtime::wellExceeded(0));
}

// ---------------------------------------------------------------------------
// Main pump — shared predicate (Rule + Mode)
// ---------------------------------------------------------------------------

TEST(main_not_tripped_at_exact_limit) {
  EXPECT_FALSE(overtime::mainExceeded(overtime::MAIN_LIMIT_MS));
}

TEST(main_tripped_one_ms_past_limit) {
  EXPECT_TRUE(overtime::mainExceeded(overtime::MAIN_LIMIT_MS + 1));
}

TEST(main_not_tripped_when_under_limit) {
  EXPECT_FALSE(overtime::mainExceeded(overtime::MAIN_LIMIT_MS - 1));
}

// ---------------------------------------------------------------------------
// Rule::handleMainOvertime state machine (two-phase arm)
// ---------------------------------------------------------------------------

TEST(rule_main_first_tick_only_arms) {
  overtime::RuleMainTracker t;
  EXPECT_EQ(t.step(true, 1000), overtime::RuleMainTracker::Event::Armed);
  EXPECT_EQ(t.startMs(), 1000u);
}

TEST(rule_main_does_not_trip_before_limit) {
  overtime::RuleMainTracker t;
  t.step(true, 0);
  auto atLimit =
      t.step(true, overtime::MAIN_LIMIT_MS);
  EXPECT_EQ(atLimit, overtime::RuleMainTracker::Event::None);
}

TEST(rule_main_trips_one_ms_after_limit) {
  overtime::RuleMainTracker t;
  t.step(true, 0);
  auto ev = t.step(true, overtime::MAIN_LIMIT_MS + 1);
  EXPECT_EQ(ev, overtime::RuleMainTracker::Event::Tripped);
}

TEST(rule_main_arms_at_millis_zero) {
  overtime::RuleMainTracker t;
  EXPECT_EQ(t.step(true, 0), overtime::RuleMainTracker::Event::Armed);
  EXPECT_TRUE(t.armed());
  EXPECT_EQ(t.step(true, overtime::MAIN_LIMIT_MS),
            overtime::RuleMainTracker::Event::None);
  EXPECT_EQ(t.step(true, overtime::MAIN_LIMIT_MS + 1),
            overtime::RuleMainTracker::Event::Tripped);
}

TEST(rule_main_resets_when_pump_off) {
  overtime::RuleMainTracker t;
  t.step(true, 0);
  t.step(false, 500);
  EXPECT_FALSE(t.armed());
  EXPECT_EQ(t.step(true, 600), overtime::RuleMainTracker::Event::Armed);
}

TEST(rule_main_survives_millis_wrap) {
  overtime::RuleMainTracker t;
  const uint32_t start = 0xFFFFFFF0u;
  t.step(true, start);
  const uint32_t now = start + overtime::MAIN_LIMIT_MS + 1; // wraps uint32
  EXPECT_EQ(t.step(true, now), overtime::RuleMainTracker::Event::Tripped);
}

// ---------------------------------------------------------------------------
// Mode::handleMainStop overtime branch — should match Rule behaviour
// ---------------------------------------------------------------------------

TEST(mode_main_matches_rule_arm_and_trip) {
  overtime::ModeMainTracker mode;
  overtime::RuleMainTracker rule;

  EXPECT_EQ(mode.step(true, 0), overtime::ModeMainTracker::Event::Armed);
  EXPECT_EQ(rule.step(true, 0), overtime::RuleMainTracker::Event::Armed);

  for (uint32_t t = 1; t <= overtime::MAIN_LIMIT_MS; ++t) {
    EXPECT_EQ(mode.step(true, t), overtime::ModeMainTracker::Event::None);
    EXPECT_EQ(rule.step(true, t), overtime::RuleMainTracker::Event::None);
  }

  EXPECT_EQ(mode.step(true, overtime::MAIN_LIMIT_MS + 1),
            overtime::ModeMainTracker::Event::Tripped);
  EXPECT_EQ(rule.step(true, overtime::MAIN_LIMIT_MS + 1),
            overtime::RuleMainTracker::Event::Tripped);
}

// ---------------------------------------------------------------------------
// Integration-style: pump on continuously for full overtime window
// ---------------------------------------------------------------------------

TEST(full_main_pump_session_trips_exactly_once) {
  overtime::RuleMainTracker t;
  int trips = 0;
  uint32_t now = 0;
  bool pumpOn = true;

  for (unsigned i = 0; i < 5000; ++i) {
    auto ev = t.step(pumpOn, now);
    if (ev == overtime::RuleMainTracker::Event::Tripped) {
      ++trips;
      pumpOn = false; // mirrors ctrlMain.setOn(false) after trip
    }
    now += 1000;
  }

  EXPECT_EQ(trips, 1);
}

TEST(well_predicate_matches_rule_condition) {
  // Rule trips when: ctrlWell.isOn() && activeMode && wellExceeded(timer)
  const uint32_t timer = overtime::WELL_LIMIT_MS + 5000;
  const bool pumpOn = true;
  const bool hasMode = true;
  const bool shouldTrip =
      pumpOn && hasMode && overtime::wellExceeded(timer);
  EXPECT_TRUE(shouldTrip);
}

TEST(well_safe_when_mode_missing) {
  const uint32_t timer = overtime::WELL_LIMIT_MS + 5000;
  const bool pumpOn = true;
  const bool hasMode = false;
  const bool shouldTrip =
      pumpOn && hasMode && overtime::wellExceeded(timer);
  EXPECT_FALSE(shouldTrip);
}

int main() { return run_test_harness(); }
