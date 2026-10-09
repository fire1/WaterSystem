#ifndef SlaveLogic_h
#define SlaveLogic_h

#include <stdint.h>

namespace slaveLogic {

constexpr uint8_t DEFAULT_AVG_SAMPLES = 60;
constexpr uint8_t DEFAULT_MIN_CM = 20;
constexpr uint8_t DEFAULT_MAX_CM = 100;
constexpr uint8_t DEFAULT_NUM_LEDS = 10;

/** Ultrasonic pulse duration (µs) → distance in cm. */
inline uint16_t distanceFromPulseUs(unsigned long durationUs) {
  if (durationUs == 0)
    return 0;
  return static_cast<uint16_t>((durationUs * 0.0343) / 2.0);
}

/**
 * Map distance (cm) onto LED count [0, numLeds].
 * Input is clamped to [minCm, maxCm] before linear mapping (Arduino map).
 */
inline uint8_t mapDistanceToLeds(uint8_t distance, uint8_t minCm = DEFAULT_MIN_CM,
                                 uint8_t maxCm = DEFAULT_MAX_CM,
                                 uint8_t numLeds = DEFAULT_NUM_LEDS) {
  if (maxCm <= minCm)
    return 0;
  uint8_t clamped = distance;
  if (clamped < minCm)
    clamped = minCm;
  if (clamped > maxCm)
    clamped = maxCm;
  return static_cast<uint8_t>(
      ((clamped - minCm) * numLeds) / (maxCm - minCm));
}

/**
 * Accumulate samples; when count reaches sampleLimit, expose the integer
 * average and reset the accumulator.
 */
struct SampleAverage {
  uint16_t sampleLimit = DEFAULT_AVG_SAMPLES;
  uint16_t count = 0;
  double sum = 0;

  void reset() {
    count = 0;
    sum = 0;
  }

  /** Returns true when a full window produced an average. */
  bool push(double sample, int &averageOut) {
    sum += sample;
    ++count;
    if (count < sampleLimit)
      return false;
    averageOut = static_cast<int>(sum / count);
    reset();
    return true;
  }
};

} // namespace slaveLogic

#endif
