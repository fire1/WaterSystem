# WaterSystem Project - Master Sketch

## Overview
This project is an automated water management system that extracts water from a well into a **well tank** and then transfers it to a **main tank**. The system is built around an Arduino Mega (Master) and an remote sensor node (Slave).

The system is designed for high reliability and efficiency, especially given the variable extraction rates of the **airlift pump** (slug state). It implements multi-layer safety protections and an adaptive optimization logic to maximize water yield while protecting the equipment.

## System Architecture

### Master (Arduino Mega)
The central controller responsible for:
- **Sensing**: Reads well tank level (local UART sensor) and communicates with the Slave for the main tank level.
- **Actuation**: Controls the Well Pump and the Main Pump via SSRs.
- **UI**: Manages a 16x2 LCD, navigation buttons, and a buzzer for notifications.
- **Logic**: Orchestrates automation modes and safety rules.
- **Cooling**: Monitors SSR temperature and controls a cooling fan.

### Remote Slave
- Located at the main tank.
- Powered and polled by the Master via a dedicated line.
- Returns distance data to the Master using SoftwareSerial.

### Water Tanks & Sensors
- **Sensors**: JSN-SR04T-2.0 or similar ultrasonic sensors (Parking sensor style).
- **Calibration**: 20cm = Full / 100-110cm = Empty.
- **Well Tank**: ~15 Liters per cm (100x150x100cm).
- **Main Tank**: ~12 Liters per cm (100x120x100cm).

---

## Modular Mode System (`lib/Mode.h` & `lib/mode/`)

The system uses a polymorphic approach to well pumping. Each mode implements its own logic for `well()` (calculating work/break time) and `main()` (transfer conditions).

### Key Modes
- **Adaptive Optimization (`PidRunMode` & `PidTnkMode`)**:
    - **Goal**: Hits a target rise (3cm) per session.
    - **Logic**: Uses a PID-like proportional controller to adjust runtime based on past performance (drift).
    - **States**: 
        - `SEARCH`: Normal optimization.
        - `RECOVERY`: Well is depleting; extends rest periods.
        - `LONG_REST`: Emergency recovery for extremely low well levels.
    - **Volume Awareness**: `PidTnkMode` adds tank geometry and consumption tracking.
- **Timed Modes**: `HourlyMode`, `Hours3Mode`, `Hours4Mode`, `Hours6Mode`, `EvrDayMode` (Fixed intervals).
- **Specialized**:
    - `WinterMode`: Includes freeze protection logic.
    - `D1FillMode`: Daily filling optimization.
    - `FasterMode`: High-intensity extraction.
    - `CleansMode`: Maintenance/idle state.

---

## Multi-Layer Safety & Rules (`Rule.h`)

The `Rule` class is the primary safety orchestrator. While `Mode` classes implement the logic for *when* to pump, the `Rule` class acts as the final gatekeeper to ensure hardware safety.

### Critical Safety Constraint: Mutual Exclusion
**The Well Pump and the Main Pump must NEVER operate at the same time.** 
The pumps are high-power machines, and simultaneous operation can lead to electrical overload or hydraulic issues. The `Rule` class is responsible for monitoring the state of both pumps and enforcing this mutual exclusion, regardless of the active `Mode`'s intent or manual overrides.

### Safety Responsibilities:
1.  **Mutual Exclusion**: Ensures only one pump is active at any given time.
2.  **Dry Run Protection**: Automatically stops the Main pump if the well tank is empty or the Well pump if the main tank is full.
3.  **Final Fallback: Overtime Protection**: 
    - This is the absolute last line of defense, designed to trigger if sensors fail or provide incorrect data (e.g., a "stuck" level reading).
    - The `Rule` class independently monitors the total continuous runtime of each pump.
    - If a pump exceeds its hard-coded time limit, it is forcibly shut down and marked as a "FAILURE," requiring investigation.
    - **Well Pump Limit**: 15 minutes (`OPT_WELL_OVERTIME`).
    - **Main Pump Limit**: 30-45 minutes (`OPT_MAIN_OVERTIME`).
4.  **Environmental Safety**:
    - **Cold Protection**: Prevents well pump operation in extreme cold unless in specific anti-freeze modes.
    - **Daytime/Nighttime Logic**: Restricts operation based on the RTC clock to comply with local noise or power constraints.
5.  **SSR Heat Management (`Heat.h`)**: Monitors Solid State Relay temperatures and triggers emergency shutdowns if thresholds are exceeded.

---

## Hardware & Communication Details

### Well Sensor (UART Mode)
- The well sensor is modified with a 47kΩ resistor (R19) to enable UART mode.
- Master communicates via `Serial3` at 9600 baud.
- Provides more reliable readings than the default pulse-echo mode.

### Main Sensor (Slave MCU)
- Master controls power to the Slave (`pinMainPower`).
- Communicates via SoftwareSerial at 4800 baud (`BaudSlaveRx`).
- Polling sequence: Power ON -> Listen -> Receive Byte -> Power OFF.

### Timekeeping
- Uses a DS3231 RTC for accurate time and temperature monitoring.
- Enables scheduled tasks like "Dayjob" (ensuring at least one pump run per day).

---

## User Interface & Menu (`Menu.h` & `Draw.h`)

The system features a structured menu for monitoring and configuration:
- **Home Screen**: Displays tank levels and pump status.
- **Mode Selection**: Allows users to switch between the different pumping strategies.
- **Manual Control**: Instant toggle for Well and Main pumps.
- **Feedback**:
    - **Buzzer**: Alarm on errors or preparation for pumping.
    - **LED Heartbeat**: Indicates system health and active mode intensity.
    - **Warning Screens**: Displays descriptive errors (e.g., "Too cold to run!", "SSR Overheat").

---

## File Structure
- `Master.ino`: Application entry point and main loop.
- `lib/Glob.h`: Global configuration, pin assignments, and constants.
- `lib/Rule.h`: The high-level orchestrator for modes and safety.
- `lib/Read.h`: Sensor data acquisition and smoothing (moving average).
- `lib/Mode.h`: Base class for all well pumping strategies.
- `lib/Heat.h`: Thermal management logic for SSR.
- `lib/Pump.h`: Low-level pump control and fault tracking.
- `lib/Data.h`: Handles EEPROM persistence for settings.
