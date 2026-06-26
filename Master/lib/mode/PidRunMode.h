#ifndef PidRunMode_h
#define PidRunMode_h
#include "../Mode.h"
#include "../AirliftOpt.h"

class PidRunMode : public Mode {
private:
    // ===== Tuning Parameters =====
    static constexpr float P_GAIN = 0.8f;
    static constexpr float I_GAIN = 0.15f;
    static constexpr float DRIFT_DEADBAND = 0.05f;

    static constexpr float EFFICIENCY_ALPHA = 0.2f;
    static constexpr float BEST_EFF_WEIGHT = 0.5f;

    static constexpr float EXTREME_DRIFT = 1.6f;
    static constexpr float CRITICAL_EFF_DROP = 0.75f;
    static constexpr float BREAKTIME_EMERGENCY = 480;

    enum OperatingState {
        SEARCH,
        RECOVERY,
        LONG_REST
    };

    struct PumpPlan {
        uint8_t runtime;
        uint16_t breaktime;
        float efficiency;
    };

    struct EfficiencyTracker {
        float smoothed;
        float trend;
        uint16_t samples;
    };

    struct ErrorAccumulator {
        float integral;
        float lastDrift;
    };

    PumpPlan current;
    PumpPlan best;
    EfficiencyTracker effTracker;
    ErrorAccumulator controller;
    OperatingState state;
    uint16_t cycleCount;
    uint16_t recoveryCounter;
    uint8_t driftExtremeCount;
    bool deferBreakBoost;

public:
    PidRunMode() {
        current = {10, 180, 0.0};
        best = current;
        effTracker = {0.0, 0.0, 0};
        controller = {0.0, 0.0};
        state = SEARCH;
        cycleCount = 0;
        recoveryCounter = 0;
        driftExtremeCount = 0;
        deferBreakBoost = false;
    }

    const __FlashStringHelper *title() override {
        return F("Auto Run");
    }

    RunWell well(Read *read) override {
        if (!hasValidHistory()) {
            return RunWell{current.runtime, current.breaktime};
        }

        float drift = constrain(this->fetchWeightedCorrection(), 0.5f, 1.5f);
        uint8_t rise = fetchRise(TARGET_RISE_CM);
        float measuredEff =
            airlift::riseEfficiency(rise, current.runtime);

        if (spanLg.active()) {
            dbg(F("[OPT] Cycle: ")); dbg(cycleCount);
            dbg(F(" | State: ")); dbg(getStateName());
            dbg(F(" | Drift: ")); dbg(drift);
            dbg(F(" | RiseEff: ")); dbg(measuredEff);
            dbg(F(" | SmthEff: ")); dbgLn(effTracker.smoothed);
        }

        updateEfficiencyTracker(measuredEff);

        if (effTracker.smoothed > best.efficiency) {
            float blendedEff = best.efficiency * (1.0f - BEST_EFF_WEIGHT)
                             + effTracker.smoothed * BEST_EFF_WEIGHT;
            best.efficiency = blendedEff;
            best.runtime = current.runtime;
            best.breaktime = current.breaktime;
        }

        updateOperatingState(drift, measuredEff);
        optimizeRuntime(drift);
        optimizeBreaktime(drift, measuredEff);

        current.runtime = constrain(current.runtime, airlift::RUNTIME_MIN,
                                    airlift::RUNTIME_HARD_MAX);
        current.breaktime = constrain(current.breaktime, 45, 480);

        cycleCount++;
        deferBreakBoost = false;

        return RunWell{current.runtime, current.breaktime};
    }

private:
    void updateEfficiencyTracker(float measuredEff) {
        if (effTracker.samples == 0) {
            effTracker.smoothed = measuredEff;
            effTracker.trend = 0.0f;
        } else {
            float prevSmoothed = effTracker.smoothed;
            effTracker.smoothed = EFFICIENCY_ALPHA * measuredEff
                                + (1.0f - EFFICIENCY_ALPHA) * effTracker.smoothed;
            effTracker.trend = effTracker.smoothed - prevSmoothed;
        }
        effTracker.samples++;
    }

    void updateOperatingState(float drift, float measuredEff) {
        if (drift > EXTREME_DRIFT) {
            driftExtremeCount++;
            if (driftExtremeCount >= 2) {
                state = LONG_REST;
                recoveryCounter = 0;
                controller.integral = 0.f;
            }
        } else {
            driftExtremeCount = 0;
        }

        if (state == LONG_REST && drift < 1.2f) {
            state = RECOVERY;
            recoveryCounter = 0;
        }

        if (state == SEARCH && measuredEff < (best.efficiency * CRITICAL_EFF_DROP)) {
            state = RECOVERY;
            recoveryCounter = 0;
            controller.integral = 0.f;
        }

        if (state == RECOVERY && measuredEff >= (best.efficiency * 0.95f)) {
            state = SEARCH;
        }
    }

    void optimizeRuntime(float drift) {
        if (current.runtime < airlift::RUNTIME_SOFT_MAX) {
            controller.integral = constrain(
                controller.integral + (drift - 1.0f) * I_GAIN, -1.0f, 1.0f);
        } else {
            controller.integral = 0.f;
        }

        airlift::RuntimeAdjustResult adj = airlift::adjustRuntime(
            drift, current.runtime, effTracker.trend, controller.integral,
            P_GAIN, I_GAIN, DRIFT_DEADBAND);

        current.runtime = adj.runtime;
        deferBreakBoost = adj.deferToRest;
        controller.lastDrift = drift;
    }

    void optimizeBreaktime(float drift, float measuredEff) {
        if (deferBreakBoost || (drift > 1.05f && current.runtime >= airlift::RUNTIME_SOFT_MAX)) {
            current.breaktime = constrain(current.breaktime + 20, 45, BREAKTIME_EMERGENCY);
        }

        switch (state) {
            case LONG_REST:
                current.breaktime = constrain(current.breaktime + 45, 45, BREAKTIME_EMERGENCY);
                break;

            case RECOVERY:
                if (measuredEff < (best.efficiency * 0.90f)) {
                    current.breaktime = constrain(current.breaktime + 20, 45, 360);
                    recoveryCounter++;
                } else if (measuredEff >= (best.efficiency * 0.95f)) {
                    current.breaktime = constrain(current.breaktime - 10, 45, 360);
                }
                break;

            case SEARCH: {
                float effRatio = measuredEff / (best.efficiency + 0.0001f);

                if (effRatio < 0.90f || deferBreakBoost) {
                    current.breaktime = constrain(current.breaktime + 15, 45, 480);
                } else if (effRatio > 1.05f) {
                    current.breaktime = constrain(current.breaktime - 8, 45, 300);
                } else if (effTracker.trend < -0.001f) {
                    current.breaktime = constrain(current.breaktime + 5, 45, 480);
                }
                break;
            }
        }
    }

    const char* getStateName() const {
        switch (state) {
            case SEARCH: return "SRCH";
            case RECOVERY: return "RECV";
            case LONG_REST: return "REST";
            default: return "UNKN";
        }
    }
};

#endif

