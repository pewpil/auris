#!/usr/bin/env bash
# Purpose: Start Auris pointer and head IMU inputs, pointer ToF, and the navigation dashboard.
# Inputs: Optional AURIS_SYSTEM_DIR, AURIS_PYTHON, AURIS_SERIAL_PORT, AURIS_BAUD, AURIS_PORT, and AURIS_CAMERA environment variables.
# Outputs: Live sensor snapshots and the dashboard over HTTP; sensor and startup logs go to stdout/stderr.
set -euo pipefail

PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
DASHBOARD_DIR="${AURIS_DASHBOARD_DIR:-$PROJECT_ROOT/dashboard}"
BAUD="${AURIS_BAUD:-115200}"
PORT="${AURIS_PORT:-8081}"

if [[ -n "${AURIS_SYSTEM_DIR:-}" ]]; then
  SYSTEM_DIR="$AURIS_SYSTEM_DIR"
elif [[ -f "$PROJECT_ROOT/cu/audition/__main__.py" ]]; then
  SYSTEM_DIR="$PROJECT_ROOT/cu"
elif [[ -f "$PROJECT_ROOT/../system/audition/__main__.py" ]]; then
  SYSTEM_DIR="$(cd -- "$PROJECT_ROOT/../system" && pwd)"
else
  printf 'Auris compute-unit source is missing. Set AURIS_SYSTEM_DIR to the directory containing audition/.\n' >&2
  exit 2
fi

PYTHON="${AURIS_PYTHON:-$SYSTEM_DIR/.venv/bin/python}"
SERIAL_PORT="${AURIS_SERIAL_PORT:-/dev/serial/by-id/usb-Silicon_Labs_CP2102_USB_to_UART_Bridge_Controller_0001-if00-port0}"

if [[ ! -x "$PYTHON" ]]; then
  printf 'Auris Python environment is missing or not executable: %s\n' "$PYTHON" >&2
  exit 2
fi

if [[ ! -e "$SERIAL_PORT" ]]; then
  printf 'Pointer serial device is missing: %s\n' "$SERIAL_PORT" >&2
  printf 'Set AURIS_SERIAL_PORT to the active serial device path.\n' >&2
  exit 2
fi

if [[ ! -f "$DASHBOARD_DIR/index.html" ]]; then
  printf 'Sensor dashboard is missing: %s/index.html\n' "$DASHBOARD_DIR" >&2
  exit 2
fi

# Camera configuration remains in the compute-unit backend; opt in after it is configured.
CAMERA_ARGS=(--no-camera)
if [[ "${AURIS_CAMERA:-0}" == "1" ]]; then
  CAMERA_ARGS=()
fi

cd "$SYSTEM_DIR"
exec "$PYTHON" -m audition \
  --pointer-source serial \
  --serial-port "$SERIAL_PORT" \
  --head-source serial \
  --baud "$BAUD" \
  "${CAMERA_ARGS[@]}" \
  --align none \
  --audio null \
  --viz "$PORT" \
  --viz-root "$DASHBOARD_DIR"
