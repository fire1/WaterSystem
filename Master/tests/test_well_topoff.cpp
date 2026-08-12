#include "../lib/WellTopOff.h"
#include "TestHarness.h"

TEST(full_level_arms_four_runs) {
  wellTopOff::State topoff;

  topoff.observeFull(20, 20);

  EXPECT_TRUE(topoff.latched);
  EXPECT_EQ(topoff.remaining, 4);
  EXPECT_TRUE(topoff.allowsRun());
}

TEST(below_full_does_not_arm) {
  wellTopOff::State topoff;

  topoff.observeFull(21, 20);

  EXPECT_FALSE(topoff.latched);
  EXPECT_EQ(topoff.remaining, 0);
  EXPECT_FALSE(topoff.allowsRun());
}

TEST(exactly_four_completed_runs_are_allowed) {
  wellTopOff::State topoff;
  topoff.observeFull(19, 20);

  for (int i = 0; i < 4; ++i) {
    EXPECT_TRUE(topoff.allowsRun());
    topoff.completeRun();
  }

  EXPECT_EQ(topoff.remaining, 0);
  EXPECT_FALSE(topoff.allowsRun());
  EXPECT_TRUE(topoff.latched);
}

TEST(full_reading_does_not_rearm_after_limit) {
  wellTopOff::State topoff;
  topoff.observeFull(20, 20);

  for (int i = 0; i < 4; ++i)
    topoff.completeRun();

  topoff.observeFull(19, 20);

  EXPECT_EQ(topoff.remaining, 0);
  EXPECT_FALSE(topoff.allowsRun());
}

TEST(main_start_resets_and_allows_next_cycle) {
  wellTopOff::State topoff;
  topoff.observeFull(20, 20);
  topoff.completeRun();

  topoff.resetOnMainStart();

  EXPECT_FALSE(topoff.latched);
  EXPECT_EQ(topoff.remaining, 0);

  topoff.observeFull(19, 20);
  EXPECT_EQ(topoff.remaining, 4);
}

int main() { return run_test_harness(); }
