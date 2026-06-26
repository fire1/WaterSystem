#include "../lib/AirliftOpt.h"
#include "TestHarness.h"

TEST(effective_work_subtracts_startup_minute) {
  EXPECT_EQ((int)airlift::effectiveWorkMinutes(10), 9);
  EXPECT_EQ((int)airlift::effectiveWorkMinutes(1), 1);
}

TEST(rise_efficiency_uses_effective_minutes) {
  float eff = airlift::riseEfficiency(3, 10);
  EXPECT_TRUE(eff > 0.29f && eff < 0.35f); // 3/9
}

TEST(drift_compensation_reduces_over_correction) {
  float raw = airlift::driftAfterStartupCompensation(1.3f, 10);
  EXPECT_TRUE(raw < 1.3f);
  EXPECT_TRUE(raw > 1.0f);
}

TEST(at_soft_max_under_target_defers_to_rest) {
  auto r = airlift::adjustRuntime(1.25f, 10, 0.f, 0.f, 0.8f, 0.15f, 0.05f);
  EXPECT_TRUE(r.deferToRest);
  EXPECT_TRUE(r.runtime <= 10);
}

TEST(depleting_well_defers_to_rest_even_if_runtime_low) {
  auto r = airlift::adjustRuntime(1.15f, 8, -0.01f, 0.f, 0.8f, 0.15f, 0.05f);
  EXPECT_TRUE(r.deferToRest);
}

TEST(short_runtime_can_increase_slightly_when_under_target) {
  auto r = airlift::adjustRuntime(1.3f, 7, 0.01f, 0.f, 0.8f, 0.15f, 0.05f);
  EXPECT_FALSE(r.deferToRest);
  EXPECT_TRUE(r.runtime >= 7);
  EXPECT_TRUE(r.runtime <= airlift::RUNTIME_SOFT_MAX);
}

TEST(long_runtime_creeps_down_when_over_target) {
  auto r = airlift::adjustRuntime(0.8f, 10, 0.f, 0.f, 0.8f, 0.15f, 0.05f);
  EXPECT_FALSE(r.deferToRest);
  EXPECT_TRUE(r.runtime <= 10);
}

int main() { return run_test_harness(); }
