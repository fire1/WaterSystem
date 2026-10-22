#!/usr/bin/env bash
# Install Arduino AVR core + Master sketch libraries for a fresh OS.
# Usage (from anywhere):
#   ./docs/install-deps.sh
#   bash Master/docs/install-deps.sh

set -euo pipefail

CORE="arduino:avr"
# Bundled with AVR core (not installed here): LiquidCrystal, SoftwareSerial, Wire, EEPROM
LIBS=(
  "AsyncDelay"
  "RTClib"      # pulls Adafruit BusIO automatically
  "CmdSerial"
)

die() { echo "error: $*" >&2; exit 1; }

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || die "'$1' not found — install it first"
}

lib_installed() {
  local name="$1"
  arduino-cli lib list 2>/dev/null | awk 'NR>1 {print $1}' | grep -Fxq "$name"
}

core_installed() {
  arduino-cli core list 2>/dev/null | awk 'NR>1 {print $1}' | grep -Fxq "$CORE"
}

need_cmd arduino-cli

echo "==> arduino-cli $(arduino-cli version 2>/dev/null | head -1)"
echo "==> user dir:  $(arduino-cli config get directories.user 2>/dev/null || echo '(default)')"
echo "==> data dir:  $(arduino-cli config get directories.data 2>/dev/null || echo '(default)')"
echo

echo "==> Updating package / library indexes"
arduino-cli core update-index
arduino-cli lib update-index

if core_installed; then
  echo "==> Core already installed: $CORE"
else
  echo "==> Installing core: $CORE"
  arduino-cli core install "$CORE"
fi

for lib in "${LIBS[@]}"; do
  if lib_installed "$lib"; then
    echo "==> Library already installed: $lib"
  else
    echo "==> Installing library: $lib"
    arduino-cli lib install "$lib"
  fi
done

echo
echo "==> Installed cores"
arduino-cli core list
echo
echo "==> Installed libraries"
arduino-cli lib list
echo
echo "Done. Next: see docs/upload-mega2560.md"
