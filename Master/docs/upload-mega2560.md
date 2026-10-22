# Upload Master firmware (Arduino Mega 2560)

Build and flash the `Master/` sketch with **arduino-cli**. Board FQBN:

```text
arduino:avr:mega:cpu=atmega2560
```

## Prerequisites

1. Install [arduino-cli](https://arduino.github.io/arduino-cli/).
2. Install the AVR core and sketch libraries (fresh OS):

```bash
cd Master
./docs/install-deps.sh
```

That installs `arduino:avr` plus **AsyncDelay**, **RTClib** (and **Adafruit BusIO**), and **CmdSerial**. LiquidCrystal / SoftwareSerial / Wire ship with the AVR core.

3. Connect the Mega over USB. On Linux you may need dialout membership:

```bash
sudo usermod -aG dialout "$USER"   # then log out / in
```

## Find the serial port

```bash
arduino-cli board list
```

Typical ports:

| OS | Port examples |
|----|----------------|
| Linux | `/dev/ttyACM0`, `/dev/ttyUSB0` |
| macOS | `/dev/cu.usbmodem*` |
| Windows | `COM3`, `COM4`, … |

If the board does not appear, unplug/replug USB and run `board list` again.

## Compile

From the **WaterSystem** repo (parent of `Master/`):

```bash
cd /path/to/WaterSystem

arduino-cli compile \
  -b arduino:avr:mega:cpu=atmega2560 \
  --library "$HOME/Arduino/libraries" \
  Master
```

Omit `--library` if your libraries already live in the default sketchbook path that arduino-cli uses.

## Upload

Replace the port with the one from `board list`:

```bash
export ARDUINO_PORT=/dev/ttyACM0   # Linux example

arduino-cli compile \
  -b arduino:avr:mega:cpu=atmega2560 \
  --library "$HOME/Arduino/libraries" \
  Master

arduino-cli upload \
  -b arduino:avr:mega:cpu=atmega2560 \
  -p "$ARDUINO_PORT" \
  Master
```

One-liner (compile + upload):

```bash
arduino-cli compile -b arduino:avr:mega:cpu=atmega2560 \
  --library "$HOME/Arduino/libraries" Master \
  && arduino-cli upload -b arduino:avr:mega:cpu=atmega2560 \
  -p /dev/ttyACM0 Master
```

Close any Serial Monitor / IDE session that holds the port before uploading.

## CMake / CLion (optional)

Open **`Master/`** as the CLion project root. Targets wrap the same CLI:

```bash
export ARDUINO_PORT=/dev/ttyACM0
cd Master
cmake -B cmake-build-debug
cmake --build cmake-build-debug --target arduino-compile   # build only
cmake --build cmake-build-debug --target arduino-upload    # build + flash
```

| Env var | Purpose |
|---------|---------|
| `ARDUINO_PORT` | Upload port (required for `arduino-upload`) |
| `ARDUINO_CLI` | Path to `arduino-cli` if not on `PATH` |
| `ARDUINO_LIBRARIES` / `ARDUINO_SKETCHBOOK` | Sketchbook libraries dir |

## Serial monitor after upload

```bash
arduino-cli monitor -p /dev/ttyACM0 -c baudrate=9600
```

Master debug / `ENABLE_CMD` uses **9600** baud on USB Serial.

## Troubleshooting

| Symptom | What to try |
|---------|-------------|
| `Permission denied` on `/dev/tty*` | Add user to `dialout`; unplug/replug; check `ls -l /dev/ttyACM*` |
| Upload times out / not in sync | Press reset as upload starts; try another USB cable/port; confirm FQBN is Mega 2560 (not Uno) |
| Missing headers (`AsyncDelay.h`, …) | Install libs into sketchbook; pass `--library …` |
| Wrong board flashed | Confirm `arduino:avr:mega:cpu=atmega2560` — Master is **Mega 2560**, not ATmega8 Slave |
