#ifndef AirliftOpt_h
#define AirliftOpt_h

/**
 * Airlift well pump optimization helpers (host-testable).
 *
 * Physical model:
 * - ~1 min at start builds compressor pressure before water moves (dead time).
 * - Runs past ~10 min deplete the well; longer runs hurt efficiency, not help.
 * - Under-performance should prefer longer REST, not longer RUN.
 */

#include <math.h>
#include <stdint.h>

namespace airlift {

constexpr uint8_t STARTUP_DEAD_MIN = 1;
constexpr uint8_t RUNTIME_MIN = 5;
constexpr uint8_t RUNTIME_SOFT_MAX = 10; // prefer not to exceed (well depletion)
constexpr uint8_t RUNTIME_HARD_MAX = 12;

/** Minutes that can actually move water (exclude compressor spin-up). */
inline float effectiveWorkMinutes(uint8_t runtime) {
  if (runtime <= STARTUP_DEAD_MIN)
    return 1.f;
  return (float)(runtime - STARTUP_DEAD_MIN);
}

/** cm rise per effective pumping minute (not per calendar minute on SSR). */
inline float riseEfficiency(uint8_t riseCm, uint8_t runtime) {
  return (float)riseCm / effectiveWorkMinutes(runtime);
}

/**
 * Reduce apparent "slowness" drift caused by startup dead time.
 * Example: drift=1.3 at 10 min run → adjusted ~1.17 (less aggressive runtime bump).
 */
inline float driftAfterStartupCompensation(float drift, uint8_t runtime) {
  if (drift <= 1.f || runtime <= STARTUP_DEAD_MIN)
    return drift;
  const float frac = effectiveWorkMinutes(runtime) / (float)runtime;
  return 1.f + (drift - 1.f) * frac;
}

struct RuntimeAdjustResult {
  uint8_t runtime;
  bool deferToRest; // caller should lengthen break, not runtime
};

/**
 * PID-like runtime step for PidRunMode.
 * Never increases runtime when at/above soft max or well is depleting (effTrend < 0).
 */
inline RuntimeAdjustResult adjustRuntime(float drift, uint8_t currentRuntime,
                                         float effTrend, float integral,
                                         float pGain, float /*iGain*/,
                                         float deadband) {
  RuntimeAdjustResult out{currentRuntime, false};

  const bool depleting = effTrend < -0.0005f;
  const bool underTarget = drift > 1.f + deadband;

  if (underTarget && (currentRuntime >= RUNTIME_SOFT_MAX || depleting)) {
    out.deferToRest = true;
    if (currentRuntime > 8)
      out.runtime = currentRuntime - 1;
    return out;
  }

  float adjDrift = drift;
  if (fabsf(drift - 1.f) < deadband)
    adjDrift = 1.f;
  else if (adjDrift > 1.f)
    adjDrift = driftAfterStartupCompensation(adjDrift, currentRuntime);

  float correction = (adjDrift - 1.f) * pGain + integral;
  const float gain = (correction > 0.f) ? 0.04f : 0.15f;
  float next = (float)currentRuntime * (1.f + correction * gain);

  if (correction > 0.f && next > (float)RUNTIME_SOFT_MAX)
    next = (float)RUNTIME_SOFT_MAX;

  if (next < (float)RUNTIME_MIN)
    next = (float)RUNTIME_MIN;
  if (next > (float)RUNTIME_HARD_MAX)
    next = (float)RUNTIME_HARD_MAX;

  out.runtime = (uint8_t)(next + 0.5f);
  return out;
}

} // namespace airlift

#endif
