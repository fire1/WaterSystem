/**
 * Host unit tests for Slave distance / LED / averaging logic.
 *
 * Build & run (from Slave/tests):
 *   make test
 */
#include "../lib/SlaveLogic.h"
#include "TestHarness.h"

using namespace slaveLogic;

TEST(pulse_zero_is_zero_cm) {
  EXPECT_EQ(distanceFromPulseUs(0), 0);
}

TEST(pulse_converts_to_cm) {
  // duration such that (d * 0.0343) / 2 == 50 → d = 50 * 2 / 0.0343 ≈ 2915
  const unsigned long us = 2915;
  const uint16_t cm = distanceFromPulseUs(us);
  EXPECT_TRUE(cm >= 49 && cm <= 51);
}

TEST(led_map_full_is_zero_leds) {
  EXPECT_EQ(mapDistanceToLeds(20), 0);
}

TEST(led_map_empty_is_ten_leds) {
  EXPECT_EQ(mapDistanceToLeds(100), 10);
}

TEST(led_map_midpoint) {
  EXPECT_EQ(mapDistanceToLeds(60), 5);
}

TEST(led_map_clamps_below_min) {
  EXPECT_EQ(mapDistanceToLeds(0), 0);
  EXPECT_EQ(mapDistanceToLeds(10), 0);
}

TEST(led_map_clamps_above_max) {
  EXPECT_EQ(mapDistanceToLeds(120), 10);
  EXPECT_EQ(mapDistanceToLeds(255), 10);
}

TEST(average_does_not_emit_before_limit) {
  SampleAverage avg;
  avg.sampleLimit = 60;
  int out = -1;
  for (int i = 0; i < 59; ++i)
    EXPECT_FALSE(avg.push(40.0, out));
  EXPECT_EQ(out, -1);
  EXPECT_EQ(avg.count, 59);
}

TEST(average_emits_after_exactly_60_samples) {
  SampleAverage avg;
  avg.sampleLimit = 60;
  int out = 0;
  for (int i = 0; i < 59; ++i)
    EXPECT_FALSE(avg.push(40.0, out));
  EXPECT_TRUE(avg.push(40.0, out));
  EXPECT_EQ(out, 40);
  EXPECT_EQ(avg.count, 0);
}

TEST(average_integer_mean_of_window) {
  SampleAverage avg;
  avg.sampleLimit = 4;
  int out = 0;
  EXPECT_FALSE(avg.push(10.0, out));
  EXPECT_FALSE(avg.push(20.0, out));
  EXPECT_FALSE(avg.push(30.0, out));
  EXPECT_TRUE(avg.push(40.0, out));
  EXPECT_EQ(out, 25);
}

int main() { return run_test_harness(); }
