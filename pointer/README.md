# Pointer sensor node — ESP32 ToF + IMU

A prototype that reads distance from a **VL53L0X** time-of-flight sensor and motion data from an **MPU6050** using an **ESP32 NodeMCU-32S**. The firmware emits one CSV line per sample over the USB serial port.

**Status:** this is the sensor-readout milestone only. There is no fusion, filtering, or orientation estimate — the accelerometer, gyroscope, die temperature, range, and button state are passed through as read. The Wi-Fi UDP link and the Python VPython viewer that earlier drafts of this document described are **not implemented**; the title has been retracted accordingly rather than left promising them.

## Hardware

- ESP32 NodeMCU-32S (ESP32)
- VL53L0X ToF distance sensor breakout
- MPU6050 (e.g., GY-521) IMU breakout
- Optional momentary push button
- Jumper wires and USB **data** cable
- Linux host with [arduino-cli](https://arduino.github.io/arduino-cli/latest/) (on Arch: `sudo pacman -S arduino-cli`)

## Wiring

**Disconnect USB power before wiring.** The connections below use the ESP32's **3V3** rail, not 5V. Verify that your particular breakout modules support 3.3 V supply and I²C logic.

### MPU6050

| MPU6050 pin | ESP32 pin | Notes |
|---|---|---|
| VCC | 3V3 | Power |
| GND | GND | Common ground |
| SDA | GPIO 21 | Shared I²C data |
| SCL | GPIO 22 | Shared I²C clock |
| XDA | Not connected | Auxiliary I²C data |
| XCL | Not connected | Auxiliary I²C clock |
| AD0 / ADO | GND (recommended) | Selects address `0x68` |
| INT (sometimes labeled NT) | Not connected | Optional interrupt |

### VL53L0X

| VL53L0X pin | ESP32 pin | Notes |
|---|---|---|
| VCC | 3V3 | Power |
| GND | GND | Common ground |
| SDA | GPIO 21 | Shared I²C data |
| SCL | GPIO 22 | Shared I²C clock |
| GPIO1 | Not connected | Optional interrupt |
| XSHUT | Not connected* | Shutdown control |

*Leave `XSHUT` unconnected only when the breakout board provides the pull-up required to keep the sensor enabled; check the module documentation.

### Optional button

| Button terminal | ESP32 pin | Notes |
|---|---|---|
| One contact | GPIO 18 | Configured `INPUT_PULLUP` |
| Other contact | GND | Closes the pin to ground when held |

A pressed button then reads `LOW`. On four-leg tactile switches, make sure the chosen legs are on opposite sides of the switch's electrical contact.

### Shared connections at a glance

```text
ESP32 3V3     ── MPU6050 VCC ── VL53L0X VCC
ESP32 GND     ── MPU6050 GND ── VL53L0X GND ── button GND
ESP32 GPIO21  ── MPU6050 SDA ── VL53L0X SDA
ESP32 GPIO22  ── MPU6050 SCL ── VL53L0X SCL
ESP32 GPIO18  ── button contact
MPU6050 AD0   ── GND (recommended)
```

The sensors can share the same I²C bus because their default addresses differ: **MPU6050 `0x68`** (AD0 low), **VL53L0X `0x29`**.

## Firmware

The sketch lives at [`firmware/pointer_sensor_node/`](firmware/pointer_sensor_node) and is defined as an arduino-cli sketch project, so the board, port, platform version, and library version are all pinned in `sketch.yaml`.

The only third-party dependency is Pololu's **VL53L0X** library, whose driver supplies the sensor's multi-stage init sequence. The MPU6050 is read straight from its registers over `Wire`, since that part needs no more than a `WHO_AM_I` probe, four configuration writes, and one 14-byte burst per sample.

### Build, flash, monitor

The serial port is owned by `root:uucp`, so the user account must be in the `uucp` group (`dialout` on other distributions) before the port can be opened:

```bash
sudo usermod -aG uucp "$USER"   # then log out and back in
```

With that done, from `pointer/firmware/pointer_sensor_node/`:

```bash
arduino-cli compile --profile pointer-sensor-node
arduino-cli upload  --profile pointer-sensor-node --port /dev/ttyUSB0
arduino-cli monitor --port /dev/ttyUSB0 --config baudrate=115200
```

The profile pins the board as `esp32:esp32:nodemcu-32s`, the variant Espressif ships for this board, so no `-b` flag is needed. It deliberately pins **no** port: arduino-cli aborts a compile outright when a `default_port` in `sketch.yaml` names a device that is not currently plugged in, and the CP2102 renumbers between `/dev/ttyUSB0` and `/dev/ttyUSB1` across replugs regardless. Substitute whichever the port actually enumerates as.

### Status

The firmware compiles clean against `esp32:esp32@3.3.12` with `VL53L0X@1.3.1` under `--warnings all` — 299,580 bytes of flash (22%) and 23,524 bytes of RAM (7%), no warnings. Whether both sensors answer on the real bus is a separate question, answered by the boot log rather than by the build.

### Output schema

A single header line is printed at boot, then one line per sample at roughly 50 Hz:

```text
t_ms,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,temp_c,dist_mm,tof_status,btn
```

- `t_ms` — milliseconds since boot, stamped at the top of the loop.
- `ax_g`, `ay_g`, `az_g` — accelerometer, g, gravity included.
- `gx_dps`, `gy_dps`, `gz_dps` — gyroscope, degrees per second.
- `temp_c` — die temperature, degrees Celsius.
- `dist_mm` — range in millimetres; `0` whenever `tof_status` is not `0`.
- `tof_status` — `0` valid, `1` signal fail (8190), `2` out of range (8191), `3` no fresh reading within the 200 ms driver timeout, `4` sensor unavailable. Kept as its own column so a failed reading can never be mistaken for a distance.
- `btn` — `1` while the button is held, `0` otherwise.

The ToF is ranged on a 33 ms budget with a 50 ms period, giving about 20 fresh readings per second, while the loop emits a line every 20 ms. Consecutive lines therefore legitimately repeat a range, and a driver timeout costs one skipped cycle rather than a corrupted line. A failed IMU transfer prints `nan` in every IMU column rather than a stale value.

### Troubleshooting

The firmware scans the whole I²C bus at boot and prints every address that acknowledges, so a silent sensor is named in the log instead of being guessed at later. The Pololu driver also prints its own calibration chatter during init, so expect extra lines above the CSV header; only lines after the header are CSV.

- **`VL53L0X: no ACK at 0x29`** — the most likely cause is `XSHUT`. A part whose `XSHUT` is held low stays in shutdown and never acknowledges at all. Either add the pull-up the breakout lacks, or drive `XSHUT` from a GPIO and hold it high in `setup()`.
- **`MPU6050: no ACK at 0x68`** — check that `AD0` really is tied to GND. Floating `AD0` puts the part at `0x69`, which the firmware does not probe for.
- **`WHO_AM_I` reads `0x70` or `0x71`** — that is an MPU6500 or MPU9250 fitted on the GY-521, and the firmware accepts it.
- **`VL53L0X: answered at 0x29 but failed to initialise`** — the part is present but did not complete its calibration. The firmware asks for 2.8 V I/O mode, which is correct for the many breakouts that feed `VREF` from an on-board 2.8 V regulator. If yours instead ties `VREF` straight to `VCC`, set `TOF_IO_2V8` to `false` in the sketch so the driver selects 1.8 V mode.
- **Nothing acknowledged at all** — the bus is not powered or not wired; check the 3V3 rail and the shared ground before suspecting the sensors.
