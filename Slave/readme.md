## Slave sketch

This is the Slave sketch source code.
The slave is used to transfer serial communication to the master over long distances.

**MCU:** ATmega8 (Arduino NG / Uno-style pinout) — FQBN `arduino:avr:atmegang:cpu=atmega8`.

### CLion

Open this `Slave/` folder as the CLion project root. `CMakeLists.txt` wires autocomplete (`Slave_index`) and `arduino-cli` targets (`firmware`, `arduino-compile`, `arduino-upload`). Optional env: `ARDUINO15`, `ARDUINO_LIBRARIES`, `ARDUINO_CLI`, `ARDUINO_PORT`. See `Master/AGENTS.md` → **CLion / CMake**.

Note: `arduino-cli` compile for `atmegang:cpu=atmega8` currently fails with the stock AVR `SoftwareSerial` (ATmega8 lacks PCMSK pin-change registers). Autocomplete still uses `__AVR_ATmega8__` + `standard` variant.

### Host unit tests

Pure logic (distance from pulse, LED bar mapping, 60-sample average) lives in `lib/SlaveLogic.h` and is verified on the host — no board required:

```bash
cd Slave/tests && make test

# Or from repo root (Master + Slave):
make test
```
