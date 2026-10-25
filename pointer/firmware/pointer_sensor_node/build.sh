#!/usr/bin/env bash
# Build the deployable Auris pointer firmware.
#
# Outputs the application image and its companions into dist/. For a fresh
# board, `arduino-cli upload --profile pointer-sensor-node --port /dev/ttyUSB0`
# writes the bootloader, partition table and application in one step. A
# production line that flashes bare ESP32 modules can merge the three files with
# esptool.py (bootloader at 0x1000, partitions at 0x8000, app at 0x10000 on this
# board definition).
#
# Inputs:  the sketch and src/ in this directory; wifi_secrets.h optionally.
# Outputs: dist/pointer_sensor_node.ino.bin and companion images.
set -euo pipefail
cd "$(dirname "$0")"

if [[ ! -f wifi_secrets.h ]]; then
  echo "warning: wifi_secrets.h is absent; the build will be USB-only" >&2
fi

mkdir -p dist
arduino-cli compile --profile pointer-sensor-node --output-dir dist
ls -lh dist/*.bin
