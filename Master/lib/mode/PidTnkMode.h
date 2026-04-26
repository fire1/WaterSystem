#ifndef PidTnkMode_h
#define PidTnkMode_h
#include "../Mode.h"


class PidTnkMode : public Mode {
private:
    // ===== Tank Specifications =====
    // Tank dimensions in cm: length × width × height
    static constexpr float WELL_TANK_LENGTH = 100.0;      // 1 meter
    static constexpr float WELL_TANK_WIDTH = 150.0;       // 1.5 meters
    static constexpr float WELL_TANK_HEIGHT = 100.0;      // 1 meter

    static constexpr float MAIN_TANK_LENGTH = 100.0;      // 1 meter
    static constexpr float MAIN_TANK_WIDTH = 120.0;       // 1.2 meters
    static constexpr float MAIN_TANK_HEIGHT = 100.0;      // 1 meter

    // Calculate actual liters per cm of height (volume / height)
    static constexpr float WELL_TANK_LITERS_PER_CM =
        (WELL_TANK_LENGTH * WELL_TANK_WIDTH) / 1000.0;    // Result: 15 L/cm

    static constexpr float MAIN_TANK_LITERS_PER_CM =
        (MAIN_TANK_LENGTH * MAIN_TANK_WIDTH) / 1000.0;    // Result: 12 L/cm

    // Ultrasonic sensor calibration: 100 = empty, ~20 = full (80 unit range for 100cm)
    static constexpr uint8_t SENSOR_EMPTY = 100;
    static constexpr uint8_t SENSOR_FULL = 20;
    static constexpr uint8_t SENSOR_RANGE = SENSOR_EMPTY - SENSOR_FULL;  // 80 units

    /*
     * PID Control (for hitting 3cm target):
     *  cppP_GAIN = 2.0           // How fast to react to drift errors
     *  I_GAIN = 0.3              // How to smooth out oscillations
     *  DRIFT_DEADBAND = 0.03     // Tolerance zone (±3%)
     */

    // ===== Tuning Parameters: PID Control =====
    static constexpr float P_GAIN = 2.0;
    static constexpr float I_GAIN = 0.3;
    static constexpr float DRIFT_DEADBAND = 0.03;

    // Efficiency weighting
    static constexpr float EFFICIENCY_ALPHA = 0.6;
    static constexpr float BEST_EFF_WEIGHT = 0.75;

    // Recovery thresholds
    static constexpr float EXTREME_DRIFT = 1.5;
    static constexpr float CRITICAL_EFF_DROP = 0.85;
    static constexpr float BREAKTIME_EMERGENCY = 480;

    // ===== Tuning Parameters: Tank Management =====
    static constexpr uint8_t MAIN_TANK_CRITICAL = 70;     // Critical level (cm)
    static constexpr uint8_t MAIN_TANK_SAFE = 80;         // Safe operating level (cm)
    static constexpr uint8_t MAIN_TANK_TARGET_MIN = 25;   // Minimum buffer (cm)
    static constexpr uint8_t WELL_TANK_MIN_BUFFER = 25;   // Leave margin in well tank (cm)

    static constexpr float AGGRESSIVE_MODE_REDUCTION = 0.7;  // Reduce breaktime to 70% when critical
    static constexpr float CONSUMPTION_LOOKAHEAD_HOURS = 12;  // Plan for 12-hour consumption

    // ===== Adaptive State Machine =====
    enum OperatingState {
        SEARCH,      // Normal optimization
        RECOVERY,    // Well depleted; extend rest
        LONG_REST,   // Extreme depletion; maximum rest
        AGGRESSIVE   // Main tank critical; maximize filling
    };

    // ===== Tank Tracking Structures =====
    struct TankLevel {
        uint8_t sensorValue;        // Raw sensor reading (100=empty, 20=full)
        float actualHeight;         // Calculated actual height in cm
        uint32_t timestamp;         // Milliseconds when measured
    };

    struct TankState {
        TankLevel wellCurrent;
        TankLevel wellPrevious;
        TankLevel mainCurrent;
        TankLevel mainPrevious;
        float wellTrendCmPerHour;   // Rate of change in well tank
        float mainTrendCmPerHour;   // Rate of change in main tank
        float mainConsumptionLiters; // Total liters consumed in monitoring period
    };

    struct ConsumptionTracker {
        float consumption12h;       // Liters consumed in last 12 hours
        float consumption24h;       // Liters consumed in last 24 hours
        uint32_t lastUpdateTime;    // Timestamp of last calculation
        float avgConsumptionPerHour; // Rolling average consumption rate
        uint16_t samples;           // Number of measurements
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

    // ===== State Variables =====
    // Original PID optimization state
    PumpPlan current;
    PumpPlan best;
    EfficiencyTracker effTracker;
    ErrorAccumulator controller;
    OperatingState state;
    uint16_t cycleCount;
    uint16_t recoveryCounter;
    uint8_t driftExtremeCount;

    // Tank monitoring state
    TankState tanks;
    ConsumptionTracker consumption;
    uint32_t lastTankUpdateTime;
    uint8_t mainTankCriticalCounter;  // Consecutive cycles below critical

public:
    PidTnkMode() {
        // Initialize PID state
        current = {10, 180, 0.0};
        best = current;
        effTracker = {0.0, 0.0, 0};
        controller = {0.0, 0.0};
        state = SEARCH;
        cycleCount = 0;
        recoveryCounter = 0;
        driftExtremeCount = 0;

        // Initialize tank state
        tanks = {};
        consumption = {0.0, 0.0, 0, 0.0, 0};
        lastTankUpdateTime = millis();
        mainTankCriticalCounter = 0;
    }

    const __FlashStringHelper *title() override {
        return F("Auto+Tank");
    }

    RunWell well(Read *read) override {
        // Protection: Ensure valid history
        if (!hasValidHistory()) {
            return RunWell{current.runtime, current.breaktime};
        }

        // ===== Step 1: Update Tank Readings =====
        updateTankLevels(read);

        // ===== Step 2: Calculate Consumption and Trends =====
        updateConsumptionMetrics();

        // ===== Step 3: Fetch Well Performance Data =====
        float drift = constrain(this->calculateLastCorrection(), 0.5, 1.5);
        uint8_t rise = fetchRise(TARGET_RISE_CM);
        uint16_t totalCycle = current.runtime + current.breaktime;
        float measuredEff = (float)rise / (float)totalCycle;

        // Logging
        if (spanLg.active()) {
            logDiagnostics(drift, measuredEff);
        }

        // ===== Step 4: Update Efficiency Tracker =====
        updateEfficiencyTracker(measuredEff);

        if (effTracker.smoothed > best.efficiency) {
            float blendedEff = best.efficiency * (1.0 - BEST_EFF_WEIGHT)
                             + effTracker.smoothed * BEST_EFF_WEIGHT;
            best.efficiency = blendedEff;
            best.runtime = current.runtime;
            best.breaktime = current.breaktime;
        }

        // ===== Step 5: Determine Operating State (with Tank Awareness) =====
        updateOperatingState(drift, measuredEff);

        // ===== Step 6: Calculate Demand-Based Pump Target =====
        float demandTarget = calculateDemandTarget();

        // ===== Step 7: Optimize Runtime (PID + Demand) =====
        optimizeRuntime(drift, demandTarget);

        // ===== Step 8: Optimize Breaktime (State + Consumption Aware) =====
        optimizeBreaktime(drift, measuredEff, demandTarget);

        // ===== Step 9: Constrain and Return =====
        current.runtime = constrain(current.runtime, 5, 12);
        current.breaktime = constrain(current.breaktime, 45, 480);

        cycleCount++;
        return RunWell{current.runtime, current.breaktime};
    }

private:
    // ===== Tank Level Conversion =====
    // Convert sensor reading to actual height in cm
    float sensorToHeight(uint8_t sensorValue, bool isWellTank) {
        // Sensor reads 100 (empty) to 20 (full)
        // Inverted: higher sensor value = lower water level
        int16_t sensorDiff = SENSOR_EMPTY - sensorValue;
        if (sensorDiff < 0) sensorDiff = 0;
        if (sensorDiff > SENSOR_RANGE) sensorDiff = SENSOR_RANGE;

        // Convert to actual height (0 to 100 cm)
        float actualHeight = (float)sensorDiff * (100.0 / SENSOR_RANGE);
        return actualHeight;
    }

    // ===== Tank State Updates =====
    void updateTankLevels(Read *read) {
        uint32_t now = millis();

        // Store previous readings
        tanks.wellPrevious = tanks.wellCurrent;
        tanks.mainPrevious = tanks.mainCurrent;

        // Get current sensor readings
        uint8_t wellSensor = read->getWellLevel();
        uint8_t mainSensor = read->getMainLevel();

        // Convert to actual heights
        tanks.wellCurrent = {
            wellSensor,
            sensorToHeight(wellSensor, true),
            now
        };

        tanks.mainCurrent = {
            mainSensor,
            sensorToHeight(mainSensor, false),
            now
        };

        // Calculate trends (cm/hour)
        uint32_t timeDeltaMs = now - lastTankUpdateTime;
        if (timeDeltaMs > 60000) {  // Only calculate if > 1 minute has passed
            float timeDeltaHours = timeDeltaMs / 3600000.0;

            tanks.wellTrendCmPerHour =
                (tanks.wellCurrent.actualHeight - tanks.wellPrevious.actualHeight) / timeDeltaHours;

            tanks.mainTrendCmPerHour =
                (tanks.mainCurrent.actualHeight - tanks.mainPrevious.actualHeight) / timeDeltaHours;

            // Calculate consumption in liters
            float mainLevelDelta = tanks.mainPrevious.actualHeight - tanks.mainCurrent.actualHeight;
            tanks.mainConsumptionLiters = mainLevelDelta * MAIN_TANK_LITERS_PER_CM;

            lastTankUpdateTime = now;
        }
    }

    // ===== Consumption Tracking =====
    void updateConsumptionMetrics() {
        // Track consumption over rolling windows
        if (consumption.samples == 0) {
            consumption.avgConsumptionPerHour = 0.0;
        } else {
            // Exponential moving average of consumption rate
            float currentRate = tanks.mainTrendCmPerHour * -1.0;  // Negative trend = consumption
            if (currentRate < 0) currentRate = 0;  // Only count actual consumption

            consumption.avgConsumptionPerHour = 0.5 * currentRate
                                              + 0.5 * consumption.avgConsumptionPerHour;
        }

        // Calculate 12h and 24h consumption projections
        consumption.consumption12h = consumption.avgConsumptionPerHour * 12.0 * MAIN_TANK_LITERS_PER_CM;
        consumption.consumption24h = consumption.avgConsumptionPerHour * 24.0 * MAIN_TANK_LITERS_PER_CM;
        consumption.samples++;
    }

    // ===== Demand-Based Pump Target Calculation =====
    // Determine how much water should be in well tank based on consumption
    float calculateDemandTarget() {
        float targetWellHeight = WELL_TANK_MIN_BUFFER;

        // Add buffer for lookahead consumption
        float consumptionInCm = consumption.avgConsumptionPerHour * CONSUMPTION_LOOKAHEAD_HOURS;
        targetWellHeight += consumptionInCm;

        // Safety margins
        if (tanks.mainCurrent.actualHeight < MAIN_TANK_CRITICAL) {
            // Add extra 20cm when main tank is critical
            targetWellHeight += 20.0;
        } else if (tanks.mainCurrent.actualHeight < MAIN_TANK_SAFE) {
            // Add extra 10cm when main tank is below safe level
            targetWellHeight += 10.0;
        }

        // Clamp to reasonable values
        targetWellHeight = constrain(targetWellHeight, 25.0, 90.0);

        return targetWellHeight;
    }

    // ===== Demand-Aware State Management =====
    void updateOperatingState(float drift, float measuredEff) {
        // Check for critical main tank level
        if (tanks.mainCurrent.actualHeight < MAIN_TANK_CRITICAL) {
            mainTankCriticalCounter++;
            if (mainTankCriticalCounter >= 1) {
                state = AGGRESSIVE;
            }
        } else {
            mainTankCriticalCounter = 0;
        }

        // Emergency long-rest triggered by extreme drift
        if (drift > EXTREME_DRIFT) {
            driftExtremeCount++;
            if (driftExtremeCount >= 2 && state != AGGRESSIVE) {
                state = LONG_REST;
                recoveryCounter = 0;
            }
        } else {
            driftExtremeCount = 0;
        }

        // Exit LONG_REST when drift normalizes (unless main tank is critical)
        if (state == LONG_REST && drift < 1.2 &&
            tanks.mainCurrent.actualHeight >= MAIN_TANK_CRITICAL) {
            state = RECOVERY;
            recoveryCounter = 0;
        }

        // Exit AGGRESSIVE when main tank recovers above safe level
        if (state == AGGRESSIVE && tanks.mainCurrent.actualHeight >= MAIN_TANK_SAFE) {
            state = SEARCH;
            mainTankCriticalCounter = 0;
        }

        // Transition to RECOVERY when efficiency drops significantly
        if (state == SEARCH && measuredEff < (best.efficiency * CRITICAL_EFF_DROP) &&
            tanks.mainCurrent.actualHeight >= MAIN_TANK_CRITICAL) {
            state = RECOVERY;
            recoveryCounter = 0;
        }

        // Exit RECOVERY when efficiency improves
        if (state == RECOVERY && measuredEff >= (best.efficiency * 0.95)) {
            state = SEARCH;
        }
    }

    // ===== Enhanced Runtime Optimization =====
    // Goal: Achieve TARGET_RISE_CM (3cm) while maintaining high work efficiency
    // Passing 8-10 mins in slug flow causes efficiency to drop significantly.
    void optimizeRuntime(float drift, float demandTarget) {
        // Accumulate integral error
        controller.integral = constrain(
            controller.integral + (drift - 1.0) * I_GAIN,
            -2.0, 2.0
        );

        // Apply hysteresis
        float adjustedDrift = drift;
        if (fabs(drift - 1.0) < DRIFT_DEADBAND) {
            adjustedDrift = 1.0;
        }

        // Base PID correction for 3cm target
        float correction = (adjustedDrift - 1.0) * P_GAIN + controller.integral;

        // Asymmetric gain: reluctant to increase runtime, eager to decrease to sweet spot
        float gain = (correction > 0) ? 0.05 : 0.15; 
        float adjustedRuntime = current.runtime * (1.0 + correction * gain);

        // Demand-aware adjustment: if well tank is below target, increase runtime slightly
        // But still respect the efficiency principle
        float wellDeficit = demandTarget - tanks.wellCurrent.actualHeight;
        if (wellDeficit > 5.0) {
            // Well tank below demand target: boost runtime (max +20% boost)
            float boostFactor = min(1.20, 1.0 + (wellDeficit / 60.0));
            adjustedRuntime *= boostFactor;
        }

        // Efficiency bias: gently pull runtime towards 8-10 minute sweet spot
        if (adjustedRuntime > 10.0) {
            adjustedRuntime -= 0.1; // Passive decay back to efficiency
        }

        current.runtime = round(adjustedRuntime);
        controller.lastDrift = drift;
    }

    // ===== Demand-Aware Breaktime Optimization =====
    void optimizeBreaktime(float drift, float measuredEff, float demandTarget) {
        switch (state) {
            case AGGRESSIVE: {
                // Critical main tank: minimize rest to pump faster
                // Reduce breaktime to 70% of current to accelerate filling
                current.breaktime = constrain(
                    (uint16_t)(current.breaktime * AGGRESSIVE_MODE_REDUCTION),
                    45, 180  // Cap at 180 min max during aggressive mode
                );
                break;
            }

            case LONG_REST: {
                // Emergency recovery mode
                current.breaktime = constrain(current.breaktime + 45, 45, BREAKTIME_EMERGENCY);
                break;
            }

            case RECOVERY: {
                // Gradual recovery with demand awareness
                if (measuredEff < (best.efficiency * 0.90)) {
                    // Efficiency low: extend rest
                    current.breaktime = constrain(current.breaktime + 20, 45, 360);
                    recoveryCounter++;
                } else if (measuredEff >= (best.efficiency * 0.95)) {
                    // Efficiency recovering: reduce rest
                    current.breaktime = constrain(current.breaktime - 10, 45, 360);
                }

                // But don't rest too long if well tank is below demand target
                if (tanks.wellCurrent.actualHeight < demandTarget) {
                    current.breaktime = constrain(current.breaktime - 15, 45, 360);
                }
                break;
            }

            case SEARCH: {
                // Normal optimization with demand awareness
                float effRatio = measuredEff / (best.efficiency + 0.0001);

                // Calculate demand pressure
                float wellDeficit = demandTarget - tanks.wellCurrent.actualHeight;

                if (wellDeficit > 15.0) {
                    // Well tank significantly below target: reduce breaktime aggressively
                    current.breaktime = constrain(current.breaktime - 15, 45, 300);
                } else if (wellDeficit > 5.0) {
                    // Well tank moderately below target: reduce breaktime gently
                    current.breaktime = constrain(current.breaktime - 8, 45, 300);
                } else if (wellDeficit < -10.0) {
                    // Well tank above target (buffer built): can extend rest
                    current.breaktime = constrain(current.breaktime + 10, 45, 480);
                } else if (effRatio < 0.90) {
                    // Efficiency dropped: extend rest to recover well
                    current.breaktime = constrain(current.breaktime + 15, 45, 480);
                } else if (effRatio > 1.05) {
                    // Efficiency good: try to reduce rest
                    current.breaktime = constrain(current.breaktime - 8, 45, 300);
                } else {
                    // In sweet spot: trend-aware micro-adjustments
                    if (effTracker.trend < -0.001) {
                        current.breaktime = constrain(current.breaktime + 5, 45, 480);
                    }
                }
                break;
            }
        }
    }

    // ===== Efficiency Tracking: EMA =====
    void updateEfficiencyTracker(float measuredEff) {
        if (effTracker.samples == 0) {
            effTracker.smoothed = measuredEff;
            effTracker.trend = 0.0;
        } else {
            float prevSmoothed = effTracker.smoothed;
            effTracker.smoothed = EFFICIENCY_ALPHA * measuredEff
                                + (1.0 - EFFICIENCY_ALPHA) * effTracker.smoothed;
            effTracker.trend = effTracker.smoothed - prevSmoothed;
        }
        effTracker.samples++;
    }

    // ===== Comprehensive Logging =====
    void logDiagnostics(float drift, float measuredEff) {
        dbg(F("[OPT+T] Cycle: ")); dbg(cycleCount);
        dbg(F(" | State: ")); dbg(getStateName());
        dbg(F(" | Drift: ")); dbg(drift);
        dbg(F(" | Well: ")); dbg(tanks.wellCurrent.actualHeight );
        dbg(F("cm | Main: ")); dbg(tanks.mainCurrent.actualHeight);
        dbg(F("cm | Cons: ")); dbg(consumption.avgConsumptionPerHour);
        dbg(F(" cm/h | RT: ")); dbg(current.runtime);
        dbg(F("m BT: ")); dbgLn(current.breaktime);
    }

    // ===== Utility: State Name =====
    const char* getStateName() const {
        switch (state) {
            case SEARCH: return "SRCH";
            case RECOVERY: return "RECV";
            case LONG_REST: return "REST";
            case AGGRESSIVE: return "AGGR";
            default: return "UNKN";
        }
    }
};

#endif

/*
 * OptimalMode Enhanced with Tank Monitoring & Demand-Based Scheduling
 *
 * KEY IMPROVEMENTS:
 * 1. Tank State Tracking: Real-time monitoring of well and main tank levels
 * 2. Consumption Metrics: Tracks consumption rate and projects 12/24 hour needs
 * 3. Demand-Aware Targets: Adjusts pump targets based on main tank consumption
 * 4. AGGRESSIVE State: Emergency mode when main tank < 70cm (reduces rest to 70%)
 * 5. Deficit-Based Adjustment: If well tank below target, increases pump intensity
 * 6. Integrated PID Control: Original drift-based optimization + new demand awareness
 *
 * TANK SPECIFICATIONS:
 * - Well Tank: 1m × 1.5m × 1m = 15 L/cm height
 * - Main Tank: 1m × 1.2m × 1m = 12 L/cm height
 * - Sensor range: 100 (empty) to 20 (full) = 80 units for 100cm
 *
 * OPERATING PRINCIPLES:
 * - Monitors main tank consumption continuously
 * - Maintains well tank at level needed for 12-hour lookahead consumption
 * - Keeps minimum 25cm buffer in well tank for stability
 * - Enters AGGRESSIVE mode if main tank falls below 70cm
 * - Balances optimal well delivery with consumption demands
 * - Prevents both over-depletion and wasteful over-filling
 */
