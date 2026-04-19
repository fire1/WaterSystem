# WaterSystem Project - Master Sketch

## Overview
This project is an automated water management system that extracts water from a well into a **well tank** and then pumps it into a **main tank**. The system consists of a **Master** Arduino (this sketch) which orchestrates the entire operation and a **Slave** Arduino (located in the main tank) which acts as a remote sensor node.
## System Architecture

### Master (this sketch)
The Master is the central controller responsible for:
- Reading the well tank level (local sensor).
- Communicating with the Slave to get the main tank level.
- Controlling two pumps:
  - **Well Pump (`ctrlWell`)**: An **airlift water pump** operating in **"slug state"** (water mixed with air). This causes significant variability in extraction rates. On average, 12 minutes of pumping yields a ~3cm rise in the well tank.
  - **Main Pump (`ctrlMain`)**: Transfers water from the well tank to the main tank.
- Managing the User Interface (LCD, buttons, buzzer).
- Implementing automation rules via a **Modular Mode System**.
- Timekeeping and temperature monitoring via a DS3231 RTC.

### Water Tanks & Sensors
- **Sensors**: Ultrasonic sensors (parking sensor type).
- **Calibration**: 100cm reading = Empty / 20cm reading = Full.
- **Well Tank**: 100cm (W) x 150cm (L) x 100cm (H).
  - Volume: `~15 Liters per cm of height`.
- **Main Tank**: 100cm (W) x 120cm (L) x 100cm (H).
  - Volume: `~12 Liters per cm of height`.

### Modular Mode System (`lib/Mode.h` & `lib/mode/`)
...

The system now uses a polymorphic approach to well pumping. The `Mode` base class defines the interface, and various specialized modes implement the logic for calculating pump runtime and breaktime:
- **Basic Timing Modes**: `HourlyMode`, `Hours3Mode`, `Hours4Mode`, `Hours6Mode`, `EvrDayMode`.
- **Optimization Modes**:
  - `PidRunMode`: Uses PID-like control and a state machine (SEARCH, RECOVERY, LONG_REST) to hit a target water rise (3cm).
  - `PidTnkMode`: Extends PID logic with tank volume awareness and consumption tracking.
- **Specialized Modes**:
  - `WinterMode`: Includes freeze protection and thermal mass conservation.
  - `D1FillMode`: Optimized for daily filling with drift correction.
  - `CleansMode`: A maintenance/idle mode with long intervals.
  - `FasterMode`: A high-intensity pumping mode.

## Key Logic and Rules (`Rule.h`)
- **Mode Management**: The `Rule` class now manages the `activeMode`, switching between them based on user selection or system state.
- **Main Pumping**: 
  - Triggered based on main tank levels (Full, Half, Void).
  - Safety checks ensure the well tank has enough water before transferring.
- **Safety Protections**:
  - **Cold Protection**: Prevents well pump operation in extreme cold (unless in `WinterMode` freeze protection).
  - **Overtime Protection**: Integrated into the mode execution and monitored by `Rule.h`.
  - **Dry Run Protection**: Critical levels in the source tank automatically stop the pumps.

## File Structure Highlights
- `Master.ino`: Main entry point and loop.
- `lib/Mode.h`: Base class for all pumping modes.
- `lib/mode/`: Directory containing specific mode implementations.
- `lib/Glob.h`: Global definitions, pin assignments, and constants.
- `lib/Read.h`: Logic for reading local and remote sensors.
- `lib/Rule.h`: Orchestrates the active mode and handles system-wide safety.
- `lib/Pump.h`: Pump control abstraction.
- `lib/Menu.h` & `lib/Draw.h`: UI and LCD handling.
