#ifndef SensorProto_h
#define SensorProto_h

#include <stdint.h>

/**
 * Pure well-UART / level-average helpers shared by Read and host unit tests.
 * LEVEL_READS must match Glob.h LevelSensorReads (4).
 */
namespace sensorProto {

constexpr uint8_t WELL_UART_START = 0xFF;
constexpr uint8_t DEFAULT_VERIFY_CORRECTION = 1;
constexpr uint8_t LEVEL_READS = 4;

enum class WellFrameStatus : uint8_t { BadStart, BadChecksum, Ok };

/**
 * Parse a JSN-SR04T UART frame (start, top, low, checksum).
 * On Ok, distanceCm is millimetres/10 → centimetres.
 * On BadStart / BadChecksum, distanceCm is set to 0.
 */
inline WellFrameStatus
parseWellUartFrame(uint8_t start, uint8_t dataTop, uint8_t dataLow,
                   uint8_t checksum, uint16_t &distanceCm,
                   uint8_t verifyCorr = DEFAULT_VERIFY_CORRECTION) {
  distanceCm = 0;
  if (start != WELL_UART_START)
    return WellFrameStatus::BadStart;
  // dataTop + dataLow vs checksum + verifyCorr (sensor quirk; Glob verifyCorrection).
  if (static_cast<uint16_t>(dataTop) + dataLow !=
      static_cast<uint16_t>(checksum) + verifyCorr)
    return WellFrameStatus::BadChecksum;
  distanceCm = static_cast<uint16_t>(((dataTop << 8) + dataLow) * 0.1);
  return WellFrameStatus::Ok;
}

/** Rolling average of LEVEL_READS samples (well UART or Slave RX byte). */
struct LevelAverage {
  uint8_t index = 0;
  uint8_t readings[LEVEL_READS]{};
  uint32_t average = 0;
  uint8_t error = 0;
  bool done = false;

  void push(int newValue) {
    readings[index] = static_cast<uint8_t>(newValue);
    index = static_cast<uint8_t>((index + 1) % LEVEL_READS);

    average = 0;
    for (uint8_t i = 0; i < LEVEL_READS; ++i)
      average += readings[i];
    average /= LEVEL_READS;

    done = true;
    error = 0;
  }
};

} // namespace sensorProto

#endif
