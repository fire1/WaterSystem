# WaterSystem — Master Firmware

## Overview

Automated water management for a **well → well tank → main tank** pipeline. An **Arduino Mega 2560** (Master) controls two SSR-driven pumps, reads tank levels from two ultrasonic sensors, and runs user-selectable pumping strategies with layered safety.

The well uses an **airlift pump** with variable slug flow, so adaptive modes tune runtime/rest to hit a target rise (~3 cm per session) without dry-running or overfilling.

**Remote repo layout** (sibling sketches under `WaterSystem/`):
- `Master/` — this sketch (Mega)
- `Slave/` — ATmega8 node at the main tank (`Slave.ino`, `Lib.h`)

---

## Physical Architecture

```
  [Well] --airlift--> [Well Tank] --main pump--> [Main Tank]
         well pump              (long cable)
                                    |
                            [ATmega8 Slave + JSN-SR04T]
```

| Node | MCU | Role |
|------|-----|------|
| Master | ATmega2560 | Pumps, UI, well sensor (local UART), Slave polling, RTC, SSR cooling |
| Slave | ATmega8 | Main-tank ultrasonic read + single-byte UART reply over shared power line |

### Tank geometry & calibration

Ultrasonic distance increases as water level drops (parking-sensor style).

| Tank | Footprint (L×W) | Height | Full (cm) | Empty (cm) | ~Volume/cm | Full volume |
|------|-----------------|--------|-----------|------------|------------|-------------|
| Well (Tank1) | 1.0 m × 1.5 m | 1.0 m | 20 | 110 | ~15 L | ~1500 L |
| Main (Tank2) | 1.0 m × 1.2 m | 1.0 m | 20 | 105 | ~12 L | ~1200 L |

Water depth from tank bottom (cm) ≈ `(emptyReading − sensorReading) × height / (emptyReading − fullReading)`.

Safe margins: `LevelSensorWellMax` / `LevelSensorMainMax` = 20 cm; well dry-run stop at 90 cm (`LevelSensorStopWell`). Main pump intake sits **15 cm** above the well floor (`WELL_PUMP_BOTTOM_MARGIN_CM`) — that volume is not drawable.

---

## Communication

### 1. Well sensor — JSN-SR04T-2.0 (local UART)

- **Hardware**: 47 kΩ resistor on R19 pad enables UART mode (`WELL_MEASURE_UART_47K` in `Glob.h`). Alternative trigger/echo mode exists but is disabled (blocks main loop).
- **Interface**: `Serial3` @ **9600 baud** — TX pin 14, RX pin 15.
- **Protocol** (`Read.h` → `onReadWellSensorDistance`):
  1. Master sends `startUartCommand` (`0x01`).
  2. Sensor replies: `0xFF`, `dataTop`, `dataLow`, checksum.
  3. Checksum: `(dataTop + dataLow) == (dataSum + verifyCorrection)` (correction = 1).
  4. Distance (cm) = `((dataTop << 8) + dataLow) * 0.1`.
- **Filtering**: rolling average of `LevelSensorReads` (4) samples before `sensorWell.done`.

### 2. Main sensor — ATmega8 Slave (power + UART on two wires)

Remote node at the main tank; Master powers the Slave and reads one distance byte per poll. Code comments refer to this as **“OneWire Serial”** (power and UART on a shared pair — not Dallas 1-Wire).

**Master side** (`Read.h` → `readMain`):

| Signal | Pin | Notes |
|--------|-----|-------|
| Slave power (GND switch) | `pinMainPower` = 8 | HIGH = power Slave |
| RX from Slave | `pinMainRx` = 10 | `SoftwareSerial` RX-only |
| Baud | `BaudSlaveRx` = 4800 | |

**Poll sequence**:
1. Assert `pinMainPower` HIGH.
2. `SoftwareSerial.listen()` and wait for one byte (distance in cm).
3. Push into 4-sample moving average; mark `sensorMain.done`.
4. Auto power-down after `TimeoutPowerSlave` (2× work refresh interval).

**Slave side** (`Slave/Slave.ino`):

- JSN-SR04T via trigger/echo on pins 2/3 (`readSensor()` in `Lib.h`).
- Averages **60** readings, then `com.write(data)` — one byte, distance in cm.
- TX on pin 4 via **inverted** `SoftwareSerial` (`SoftwareSerial(-1, pinTx, true)`) @ 4800 baud.
- 10-LED bar graph shows level locally; 1.2 s delay after each TX.

**Why Slave?** Long cable run (**60 m+** power/serial pair) to main tank; dedicated MCU keeps timing stable and offloads pulse timing from the Mega. UART readings can be **unstable** (false “empty” spikes for minutes); never use a single raw sample for pump-start decisions — see **Main tank handler** below.

---

## Software Architecture

### Dependency injection

`Master.ino` includes only `Glob.h` and `Init.h`. `Init.h` wires global singletons:

```
read, time, buzz, heat, rule, menu, draw
modes[] → Cleans, Faster, Hourly, Hours3/4/6, EvrDay, D1Fill, PidRun, PidTnk, Winter
modeWellTank (EEPROM idx 2) — well pumping strategy
modeMainTank (EEPROM idx 1) — main transfer: None / Full / Half / Void
```

`Rule` owns mode selection; each `Mode` implements `well()` (schedule) and uses base `pumpWell()` / `pumpMain()` for actuation.

### Main loop (`Master.ino`)

| Order | Component | Purpose |
|-------|-----------|---------|
| 1 | `cmd.listen()` | Serial debug commands |
| 2 | `time.loop()` | RTC tick, daytime |
| 3 | `buzz.loop()` | Buzzer patterns |
| 4 | `rule.loop()` | Mode exec + safety (after `RULE_START_WAIT` 1.5 s) |
| 5 | `read.loop()` | Sensor polling (idle 30 min / work 12 s) |
| 6 | `heat.loop()` | SSR NTC temperature + fan PWM |
| 7 | `draw.menu()` | LCD refresh (`spanMd` 250 ms) |
| 8 | `heat.warn` / `rule.warn` | Overlay warnings |
| 9 | `ctrlWell/Main.ctrl()` | Debounced SSR pin updates |
| 10 | `spanSm/Md/Lg/Mx.tick()` | Cooperative “every N ms” flags |

### Timing spans (`Span.h`)

| Span | Period | Typical use |
|------|--------|-------------|
| `spanSm` | 149 ms | Heat sampling |
| `spanMd` | 250 ms | LCD, sensor power-down |
| `spanLg` | ~7.6 s | Warnings, idle level refresh, mode heartbeat |
| `spanMx` | ~250 s | Clear sensor error counters |

### Sensor read modes (`Read.h`)

- **Idle**: every 30 min (`LevelRefreshTimeIdle`), triggers burst of work reads.
- **Work**: 12 s (6 s when main &lt; 22 cm) while pumps run or display active.
- `startWorkRead()` / `stopWorkRead()` coordinate faster sampling during pumping.

---

## Pumping Modes (`lib/mode/`)

All modes extend `Mode` (`lib/Mode.h`). Well logic returns `RunWell { runtime, breaktime }` in minutes; base class handles timers, buzzer prep, and `WellPoint` buffer for rise/correction tracking.

| Index | Class | Title | Summary |
|-------|-------|-------|---------|
| 0 | `CleansMode` | Cleans | Maintenance / idle |
| 1 | `FasterMode` | Faster | Aggressive `ScheduleWellFast` |
| 2 | `HourlyMode` | Hourly | Fixed hourly interval |
| 3 | `Hours3Mode` | 3 Hours | Every 3 h |
| 4 | `Hours4Mode` | 4 Hours | Every 4 h |
| 5 | `Hours6Mode` | 6 Hours | Every 6 h |
| 6 | `EvrDayMode` | Evr Day | Every N days |
| 7 | `D1FillMode` | D1 Fill | Daily fill optimization |
| 8 | `PidRunMode` | Auto Run | PID-like adaptive (drift correction) |
| 9 | `PidTnkMode` | Auto Tnk | PidRun + tank geometry / consumption |
| 10 | `WinterMode` | Winter | Freeze protection using well water temp |
| 11 | `Moon4Mode` | Moon | 4 h cycle; **2 h** when moon above horizon (RTC + `Moon.h`) |

**Moon mode** (`Moon4Mode`): alternative to `Hours4Mode`. Uses DS3231 time and site constants (`SITE_LAT_DEG` / `SITE_LON_DEG` in `Glob.h`, default Bulgaria 42.7°N 25.5°E). When moon altitude &gt; `MOON_HORIZON_MARGIN_DEG` (3°), break is 108 min (~2 h cycle); otherwise 230 min (~4 h, same as Hours4). Requires `ENABLE_CLOCK`; falls back to 4 h if RTC is missing. Math in `lib/Moon.h` (host-testable).

**Adaptive modes** (`PidRunMode`, `PidTnkMode`): states `SEARCH` → `RECOVERY` → `LONG_REST`; target `TARGET_RISE_CM` (3). Airlift-aware tuning in `lib/AirliftOpt.h` — 1 min compressor dead time excluded from efficiency; runtime capped at **10 min** soft max; under-performance extends **rest** instead of run when well depletes.

### Main tank handler (`lib/Main.h`)

Main transfer must **not** react to immediate/raw Slave readings. The 60 m cable and Slave firmware can report false low-water spikes for minutes. `Read.h` feeds `mainTank::observeMainSample()` on each `spanLg` tick (~7.6 s); `Main.h` keeps a ring of samples, **rejects spikes** (&gt; 12 cm from median), and requires **4 agreeing samples** before `hasStableMain()` is true. Pump **start** uses `stabilizedMain()`; pump **stop** still uses raw levels for fast fail-safe.

**Scheduled transfer** (`modeMainTank` ≠ None): once per day at **22:10** (RTC required). If levels fail at the checkpoint, skip until next day.

| `modeMainTank` | Start at 22:10 (stable main + raw well) |
|----------------|----------------------------------------|
| None | off |
| Full | main **> 42 cm** (needs fill); well **≥ ~60 cm usable water** (sensor **< 43 cm**) |
| Half | main **> 52 cm**; well **< 55 cm** sensor |
| Void | main **> 78 cm**; well **< 30 cm** sensor |

**Full-mode well minimum** (`MAIN_LEVEL_WELL_MAX`): require **60 cm** drawable water above the pump intake, plus **15 cm** bottom margin the pipe cannot reach → **75 cm** total water depth from the floor (~**1125 L** in the 1.0×1.5×1.0 m well). On the sensor scale that is reading **≤ 42 cm** (`wellLevel < 43`). Lower sensor reading = more water in the tank.

**Well-mode override**: `Mode::mainTransfer()` can return `Force` (bypass 22:10 gate). `TidRunMode` forces during lunar tide peak when the well meets the same minimum and stable `main > 40`. Manual ON via UI unchanged; OFF always automatic when main full or well empty.

**Leak detection**: compares stabilized main level every **30 min** while the main pump is off. A leak shows as **steady drain** — similar cm rise per interval (not a single big drop from showering). **Night** (23:00–06:00): 3 matching intervals, any level. **Day**: 4 intervals, only when main **> 50 cm** (tunable `MAIN_LEAK_DAY_MIN_LEVEL_CM`). Bursty use resets the rate history. Alarm → LCD **MAIN TANK LEAK!** + `Buzz::leakAlarm()`.

---

## Safety — `Rule.h`

Final gatekeeper on top of mode intent and manual buttons.

1. **Mutual exclusion** — well and main pumps must never run together; both stop + alarm on conflict.
2. **Dry run / overfill** — main stops if well ≥ 90 cm or main ≤ 20 cm; well stops if main full.
3. **Overtime fallback** — if sensors stick: well 15 min (`OPT_WELL_OVERTIME`), main 30 min (`OPT_MAIN_OVERTIME`).
4. **Cold protection** — no well run below `OPT_PROTECT_COLD` (14 °C RTC) unless head still warm (&lt; 2 h since last run).
5. **Daytime** — optional `OPT_DAYTIME_WELL` / `OPT_NIGHTTIME_WELL` via RTC.
6. **SSR heat** (`Heat.h`) — NTC MF52 on A9; emergency shutdown + fan on pin 2 above `stopMaxTemp` (90 °C).
7. **Main tank leak** (`Main.h` + `Rule::handleMainLeak`) — steady-drain rate sampling; night vs day thresholds.

Startup waits `RULE_START_WAIT` (1.5 s) so sensors can settle before decisions.

---

## Hardware Pin Map (Master)

| Function | Pin |
|----------|-----|
| Well pump SSR | A10 |
| Main pump SSR | A11 |
| Well / Main manual buttons | 33 / 34 |
| Well / Main status LEDs | 31 / 32 |
| Mode heartbeat LED | 30 |
| LCD RS, E, D4–D7 | 22–27 |
| Backlight | 29 |
| Nav Back / OK / Next | 35–37 |
| Buzzer | 9 |
| Debug LED | 13 |
| Slave power / RX | 8 / 10 |
| Well UART (Serial3) | 14 TX / 15 RX |
| SSR temp NTC | A9 |
| Cooling fan SSR PWM | 2 |
| RTC (DS3231) I2C | 20 SDA / 21 SCL |

---

## UI (`Menu.h`, `Draw.h`)

- 16×2 LCD home: Tank1 (well) / Tank2 (main) bar graphs.
- Mode selection, manual pump toggles, time/heat info screens.
- Backlight auto-off after 4 min (`SuspendDisplayTime`).
- Fault indicator (char 235) on pump failure/terminate.
- Wokwi UI demo: https://wokwi.com/projects/392574312711891969

---

## Persistence & Debug

- **EEPROM** (`Data.h`): well mode @ address 2, main tank mode @ 1.
- **Serial commands** (`CmdSerial`, `ENABLE_CMD`): `well`, `main`, `mpwr`, `well:tmp`, `timer:on/off`, `well:pump`, `help`.
- **DEBUG** macro in `Glob.h` enables `dbg()` / `dbgLn()` on USB Serial @ 9600.

---

## File Structure

```
Master/
├── Master.ino          # setup/loop entry
├── lib/
│   ├── Glob.h          # pins, constants, includes
│   ├── Init.h          # object wiring (DI root)
│   ├── Read.h          # both sensors + averaging
│   ├── Rule.h          # safety + mode orchestration
│   ├── Mode.h          # base pumping state machine
│   ├── Main.h          # main transfer schedule, level stability, leak watch
│   ├── Moon.h          # moon altitude / schedule helpers
│   ├── mode/*.h        # concrete strategies
│   ├── Pump.h          # SSR + debounce
│   ├── Heat.h          # SSR thermal management
│   ├── Time.h          # DS3231 RTC
│   ├── Data.h          # EEPROM settings
│   ├── Menu.h / Draw.h # LCD UI
│   ├── Buzz.h, Span.h, Util.h, DrIn.h, Stat.h
├── CMakeLists.txt      # CLion/IDE indexing (ATmega2560), not Arduino CLI build
└── diagram.json        # Wokwi simulation wiring
```

---

## Testing

### Host unit tests (`tests/`)

Overtime protection logic is extracted to `lib/Overtime.h` and verified on the host (no board required):

```bash
cd tests && make test
```

Covers **overtime** (`Overtime.h`):
- Well/main limits (15 min / 30 min) use strict `>` — exactly at the limit does **not** trip
- `Rule::handleMainOvertime` two-phase arm (first on-tick arms, trip on later ticks)
- `Mode::handleMainStop` main overtime branch matches Rule behaviour
- `millis()` wrap-around (unsigned elapsed time)
- Well overtime skipped when `activeMode` is null

Covers **moon mode** (`Moon.h`):
- Topocentric altitude for Bulgaria test coordinates
- Full-moon night vs midday horizon checks
- Schedule selection: 2 h vs 4 h break intervals
- EU DST helper for local → UTC conversion

Covers **airlift optimization** (`AirliftOpt.h`):
- Startup dead-minute compensation
- Runtime soft cap (10 min) with defer-to-rest when under target

Covers **main tank handler** (`Main.h`):
- 22:00 daily checkpoint and per-mode thresholds
- Spike rejection and multi-sample stability before pump start
- Steady-drain leak detection (constant rate over sample windows; night vs day rules)

### On-device / simulation

- **Arduino IDE / CLI**: compile `Master.ino` for **Arduino Mega 2560**; libraries include AsyncDelay, RTClib, LiquidCrystal, SoftwareSerial, CmdSerial.
- **CMake**: `CMakeLists.txt` targets IDE code intelligence against local Arduino AVR toolchain paths.
- **Wokwi**: serial commands `well <20-95>` / `main <20-95>` to inject levels.
