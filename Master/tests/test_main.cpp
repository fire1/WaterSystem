/**
 * Host unit tests for main-tank scheduling, stability, and leak watch (Main.h).
 *
 * Build & run (from Master/tests):
 *   make main_tests && ./main_tests
 */
#include "../lib/Main.h"
#include "TestHarness.h"

using namespace mainTank;

static ClockNow clockAt(uint8_t hour, uint8_t day = 2, uint8_t month = 7,
                        uint16_t year = 2026) {
  return {year, month, day, hour};
}

TEST(scheduled_fires_at_hour_21_with_good_levels) {
  CheckpointState state{};
  const Levels levels{50, 30};
  EXPECT_TRUE(shouldStartMain(Intent::Default, true, clockAt(21), levels,
                                state, 1));
}

TEST(scheduled_does_not_fire_at_hour_20) {
  CheckpointState state{};
  const Levels levels{50, 30};
  EXPECT_FALSE(shouldStartMain(Intent::Default, true, clockAt(20), levels,
                               state, 1));
  EXPECT_FALSE(sameCalendarDay(state, clockAt(21)));
}

TEST(scheduled_does_not_fire_at_hour_22) {
  CheckpointState state{};
  const Levels levels{50, 30};
  EXPECT_FALSE(shouldStartMain(Intent::Default, true, clockAt(22), levels,
                               state, 1));
}

TEST(scheduled_skips_rest_of_day_after_failed_21_check) {
  CheckpointState state{};
  const Levels bad{40, 30};
  EXPECT_FALSE(shouldStartMain(Intent::Default, true, clockAt(21), bad, state,
                               1));
  EXPECT_TRUE(sameCalendarDay(state, clockAt(21)));

  const Levels good{50, 30};
  EXPECT_FALSE(shouldStartMain(Intent::Default, true, clockAt(22), good, state,
                               1));
}

TEST(scheduled_does_not_double_fire_same_day) {
  CheckpointState state{};
  const Levels levels{50, 30};
  EXPECT_TRUE(shouldStartMain(Intent::Default, true, clockAt(21), levels, state,
                              1));
  EXPECT_FALSE(shouldStartMain(Intent::Default, true, clockAt(21), levels,
                               state, 1));
}

TEST(scheduled_fires_again_on_next_day) {
  CheckpointState state{};
  const Levels levels{50, 30};
  EXPECT_TRUE(shouldStartMain(Intent::Default, true, clockAt(21, 2), levels,
                              state, 1));
  EXPECT_TRUE(shouldStartMain(Intent::Default, true, clockAt(21, 3), levels,
                              state, 1));
}

TEST(force_ignores_hour_gate) {
  CheckpointState state{};
  const Levels levels{42, 30};
  EXPECT_TRUE(shouldStartMain(Intent::Force, false, clockAt(14), levels, state,
                              1));
}

TEST(force_uses_40cm_main_threshold) {
  EXPECT_TRUE(levelsOkOverride({41, 30}));
  EXPECT_FALSE(levelsOkOverride({40, 30}));
}

TEST(scheduled_full_mode_uses_45cm_main_threshold) {
  EXPECT_TRUE(levelsOkScheduled({46, 30}, 1));
  EXPECT_FALSE(levelsOkScheduled({45, 30}, 1));
}

TEST(scheduled_requires_rtc) {
  CheckpointState state{};
  const Levels levels{50, 30};
  EXPECT_FALSE(shouldStartMain(Intent::Default, false, clockAt(21), levels,
                               state, 1));
}

TEST(force_works_without_rtc) {
  CheckpointState state{};
  const Levels levels{42, 30};
  EXPECT_TRUE(shouldStartMain(Intent::Force, false, clockAt(21), levels, state,
                              1));
}

TEST(block_never_starts) {
  CheckpointState state{};
  const Levels levels{50, 30};
  EXPECT_FALSE(shouldStartMain(Intent::Block, true, clockAt(21), levels, state,
                               1));
}

TEST(none_mode_never_starts) {
  CheckpointState state{};
  const Levels levels{50, 30};
  EXPECT_FALSE(shouldStartMain(Intent::Default, true, clockAt(21), levels,
                              state, 0));
}

TEST(sensor_not_ready_blocks_start) {
  CheckpointState state{};
  const Levels levels{18, 30};
  EXPECT_FALSE(shouldStartMain(Intent::Default, true, clockAt(21), levels,
                               state, 1));
  EXPECT_FALSE(shouldStartMain(Intent::Force, true, clockAt(14), levels, state,
                               1));
}

TEST(half_mode_thresholds_at_21) {
  EXPECT_TRUE(levelsOkScheduled({53, 50}, 2));
  EXPECT_FALSE(levelsOkScheduled({52, 50}, 2));
  EXPECT_FALSE(levelsOkScheduled({53, 55}, 2));
}

TEST(void_mode_thresholds_at_21) {
  EXPECT_TRUE(levelsOkScheduled({79, 25}, 3));
  EXPECT_FALSE(levelsOkScheduled({78, 25}, 3));
  EXPECT_FALSE(levelsOkScheduled({79, 30}, 3));
}

TEST(stability_rejects_single_spike) {
  Stability s{};
  s.push(50);
  s.push(51);
  s.push(52);
  s.push(90);
  EXPECT_FALSE(s.isStable());
  EXPECT_EQ((int)s.median(), 51);
}

TEST(stability_confirms_slow_samples) {
  Stability s{};
  s.push(50);
  s.push(51);
  s.push(50);
  s.push(52);
  EXPECT_TRUE(s.isStable());
  EXPECT_EQ((int)s.median(), 50);
}

TEST(stability_resets_on_demand) {
  Stability s{};
  s.push(50);
  s.push(51);
  s.push(52);
  s.push(53);
  s.reset();
  EXPECT_FALSE(s.isStable());
}

TEST(leak_detects_steady_night_drain) {
  LeakWatch watch{};
  const unsigned long sample = 60000;
  const unsigned long t0 = 1000;
  watch.tick(40, false, t0, 2, sample);
  watch.tick(41, false, t0 + sample, 2, sample);
  watch.tick(42, false, t0 + sample * 2, 2, sample);
  watch.tick(43, false, t0 + sample * 3, 2, sample);
  EXPECT_TRUE(watch.isAlarm());
}

TEST(leak_ignores_bursty_day_use) {
  LeakWatch watch{};
  const unsigned long sample = 60000;
  const unsigned long t0 = 0;
  watch.tick(55, false, t0, 14, sample);
  watch.tick(60, false, t0 + sample, 14, sample);
  watch.tick(60, false, t0 + sample * 2, 14, sample);
  watch.tick(61, false, t0 + sample * 3, 14, sample);
  watch.tick(62, false, t0 + sample * 4, 14, sample);
  EXPECT_FALSE(watch.isAlarm());
}

TEST(leak_day_requires_level_above_50cm) {
  LeakWatch watch{};
  const unsigned long sample = 60000;
  const unsigned long t0 = 0;
  watch.tick(48, false, t0, 14, sample);
  watch.tick(49, false, t0 + sample, 14, sample);
  watch.tick(50, false, t0 + sample * 2, 14, sample);
  watch.tick(51, false, t0 + sample * 3, 14, sample);
  watch.tick(52, false, t0 + sample * 4, 14, sample);
  EXPECT_FALSE(watch.isAlarm());
}

TEST(leak_detects_steady_day_drain_above_50cm) {
  LeakWatch watch{};
  const unsigned long sample = 60000;
  const unsigned long t0 = 0;
  watch.tick(52, false, t0, 14, sample);
  watch.tick(53, false, t0 + sample, 14, sample);
  watch.tick(54, false, t0 + sample * 2, 14, sample);
  watch.tick(55, false, t0 + sample * 3, 14, sample);
  watch.tick(56, false, t0 + sample * 4, 14, sample);
  EXPECT_TRUE(watch.isAlarm());
}

TEST(leak_resets_while_main_pump_on) {
  LeakWatch watch{};
  const unsigned long sample = 60000;
  const unsigned long t0 = 0;
  watch.tick(40, false, t0, 2, sample);
  watch.tick(41, false, t0 + sample, 2, sample);
  watch.tick(80, true, t0 + sample * 2, 2, sample);
  watch.tick(81, false, t0 + sample * 3, 2, sample);
  watch.tick(82, false, t0 + sample * 4, 2, sample);
  watch.tick(83, false, t0 + sample * 5, 2, sample);
  EXPECT_FALSE(watch.isAlarm());
}

TEST(leak_ignores_variable_rates) {
  LeakWatch watch{};
  const unsigned long sample = 60000;
  const unsigned long t0 = 0;
  watch.tick(40, false, t0, 3, sample);
  watch.tick(41, false, t0 + sample, 3, sample);
  watch.tick(44, false, t0 + sample * 2, 3, sample);
  watch.tick(45, false, t0 + sample * 3, 3, sample);
  watch.tick(47, false, t0 + sample * 4, 3, sample);
  EXPECT_FALSE(watch.isAlarm());
}

int main() { return run_test_harness(); }
