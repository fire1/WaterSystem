# WaterSystem

Automated well → well-tank → main-tank water management.

```
              _|=|__________
             /  /            \                                       [~~~~]
            /  /              \                                      [____]
           /__/________________\                                     //
            ||  || /--\ ||  ||                                      //
            ||[]|| | .| ||[]||                                     //
         () ||__||_|__|_||__||     ()          ___    __ [~~~~~~] //   ___
        ( )|-|-|-|====|-|-|-|||-|  ( )        (_(O)--^|| [______]//`--(O)_)
       ^^^^^^^^^^^====^^^^^^^^^^^^^^^^^^^^^^^YYYY^^^^^||^^^^^^^^^^^^^^YYYY^^^^^^/
                                                      ||
                                                     {~~}
                                                     {__}
```

An **Arduino Mega 2560** (Master) runs pumps, UI, RTC scheduling, and safety rules. An **ATmega8** (Slave) at the main tank reads the ultrasonic sensor and sends level bytes over a long two-wire power/serial link (~60 m+).

```
  [Well] --airlift--> [Well Tank] --main pump--> [Main Tank]
         well pump              (long cable)
                                    |
                            [ATmega8 Slave + JSN-SR04T]
```

## Features

- Selectable well pumping modes (hourly / multi-hour, adaptive airlift PID-style, moon/tide, winter, …)
- Scheduled main-tank transfer with spike-resistant level stability and leak watch
- Mutual exclusion, overtime, dry-run / overfill, cold, and SSR thermal protection
- 16×2 LCD UI, manual pump controls, serial debug commands

Deep firmware notes (pins, modes, protocols): **[Master/AGENTS.md](Master/AGENTS.md)**.

## Repository layout

| Path | Role |
|------|------|
| [`Master/`](Master/) | Mega 2560 sketch — open this folder in CLion |
| [`Slave/`](Slave/) | ATmega8 sketch — open this folder in CLion |
| [`docs/`](docs/) | Wiring notes (long-range UART, SSR, …) |
| [`cmake/`](cmake/) | Shared CLion / Arduino helpers |

Branches: **`main`** is the official release line; **`dev`** is ongoing work.

## Build & IDE

**Arduino CLI** (firmware):

```bash
arduino-cli compile -b arduino:avr:mega:cpu=atmega2560 Master
# Slave: arduino:avr:atmegang:cpu=atmega8  (see Slave/readme.md)
```

**CLion:** open `Master/` or `Slave/` as the project root (not the repo root). CMake wires autocomplete and `arduino-cli` targets (`arduino-compile`, `arduino-upload`). Optional env: `ARDUINO15`, `ARDUINO_LIBRARIES`, `ARDUINO_PORT`. Details in [Master/AGENTS.md](Master/AGENTS.md#clion--cmake).

Libraries used on Master include LiquidCrystal, AsyncDelay, RTClib, SoftwareSerial, CmdSerial.

## Tests

Host unit tests run on the PC with `g++` — **no Arduino board or toolchain required**. Pure logic lives in headers; sketches stay on-device.

### Where things live

| Path | What it covers |
|------|----------------|
| [`Master/tests/`](Master/tests/) | Master host tests + shared [`TestHarness.h`](Master/tests/TestHarness.h) |
| [`Master/lib/Overtime.h`](Master/lib/Overtime.h) | Pump overtime limits / Rule arming |
| [`Master/lib/Moon.h`](Master/lib/Moon.h) | Moon altitude, tide windows, DST helpers |
| [`Master/lib/AirliftOpt.h`](Master/lib/AirliftOpt.h) | Airlift runtime/rest tuning |
| [`Master/lib/Main.h`](Master/lib/Main.h) | Main transfer schedule, level stability, leak watch |
| [`Master/lib/WellTopOff.h`](Master/lib/WellTopOff.h) | Extra well runs after tank full |
| [`Master/lib/SensorProto.h`](Master/lib/SensorProto.h) | Well UART frame parse + 4-sample average (Slave RX path) |
| [`Slave/tests/`](Slave/tests/) | Slave host tests |
| [`Slave/lib/SlaveLogic.h`](Slave/lib/SlaveLogic.h) | Pulse→cm, LED bar map, 60-sample TX average |

### How to run

```bash
# Everything (Master + Slave) from repo root
make test

# Master only
cd Master/tests && make test

# Slave only
cd Slave/tests && make test

# Single Master suite (examples)
cd Master/tests && make sensor_tests && ./sensor_tests
cd Master/tests && make moon_tests && ./moon_tests
```

`make clean` in `Master/tests` or `Slave/tests` removes the built `*_tests` binaries. CLion: open `Master/` or `Slave/` and build the `host_tests` target.

## UI demo

[Wokwi UI demo](https://wokwi.com/projects/392574312711891969) — inject levels with `well` / `main` values `20`–`95`; type `help` in the serial monitor for commands.

## Hardware (summary)

- Master: Arduino Mega 2560 — pumps (SSR), well JSN-SR04T (UART), Slave poll, LCD, RTC (DS3231), buzzer, SSR fan/NTC
- Slave: ATmega8 — main-tank JSN-SR04T (trigger/echo), inverted UART TX @ 4800, local LED bar
- Two tanks with ultrasonic level sensors; airlift well pump + main transfer pump

## License

MIT — see [LICENSE](LICENSE).
