/**
 * Host unit tests for well UART frame parse and level averaging (SensorProto.h).
 *
 * Build & run (from Master/tests):
 *   make sensor_tests && ./sensor_tests
 */
#include "../lib/SensorProto.h"
#include "TestHarness.h"

using namespace sensorProto;

static void encodeMm(uint16_t mm, uint8_t &top, uint8_t &low, uint8_t &sum,
                     uint8_t verifyCorrection = DEFAULT_VERIFY_CORRECTION) {
  top = static_cast<uint8_t>((mm >> 8) & 0xFF);
  low = static_cast<uint8_t>(mm & 0xFF);
  sum = static_cast<uint8_t>(top + low - verifyCorrection);
}

TEST(well_uart_valid_500mm_is_50cm) {
  uint8_t top, low, sum;
  encodeMm(500, top, low, sum);
  uint16_t cm = 0;
  EXPECT_EQ(parseWellUartFrame(WELL_UART_START, top, low, sum, cm),
            WellFrameStatus::Ok);
  EXPECT_EQ(cm, 50);
}

TEST(well_uart_valid_200mm_is_20cm) {
  uint8_t top, low, sum;
  encodeMm(200, top, low, sum);
  uint16_t cm = 0;
  EXPECT_EQ(parseWellUartFrame(WELL_UART_START, top, low, sum, cm),
            WellFrameStatus::Ok);
  EXPECT_EQ(cm, 20);
}

TEST(well_uart_bad_start_rejected) {
  uint8_t top, low, sum;
  encodeMm(500, top, low, sum);
  uint16_t cm = 99;
  EXPECT_EQ(parseWellUartFrame(0x00, top, low, sum, cm),
            WellFrameStatus::BadStart);
  EXPECT_EQ(cm, 0);
}

TEST(well_uart_bad_checksum_rejected) {
  uint8_t top, low, sum;
  encodeMm(500, top, low, sum);
  sum = static_cast<uint8_t>(sum + 1);
  uint16_t cm = 99;
  EXPECT_EQ(parseWellUartFrame(WELL_UART_START, top, low, sum, cm),
            WellFrameStatus::BadChecksum);
  EXPECT_EQ(cm, 0);
}

TEST(level_average_four_slave_bytes) {
  LevelAverage avg;
  avg.push(40);
  avg.push(42);
  avg.push(44);
  avg.push(46);
  EXPECT_TRUE(avg.done);
  EXPECT_EQ(avg.error, 0);
  EXPECT_EQ(avg.average, 43u);
}

TEST(level_average_rolls_oldest_sample) {
  LevelAverage avg;
  avg.push(10);
  avg.push(10);
  avg.push(10);
  avg.push(10);
  EXPECT_EQ(avg.average, 10u);
  avg.push(50);
  // readings: 50, 10, 10, 10 → avg 20
  EXPECT_EQ(avg.average, 20u);
  EXPECT_TRUE(avg.done);
}

TEST(level_average_zeros_still_mark_done) {
  LevelAverage avg;
  // Caller (Read) skips push on zero Slave byte; if pushed, done is set.
  avg.push(0);
  EXPECT_TRUE(avg.done);
  EXPECT_EQ(avg.error, 0);
  EXPECT_EQ(avg.average, 0u);
}

TEST(level_average_push_clears_error) {
  LevelAverage avg;
  avg.error = 3;
  avg.push(30);
  EXPECT_EQ(avg.error, 0);
  EXPECT_TRUE(avg.done);
}

int main() { return run_test_harness(); }
