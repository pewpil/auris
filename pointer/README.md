# Pointer sensor node — ESP32 IMU

A prototype that reads motion data from an **MPU6050** using an **ESP32 NodeMCU-32S**. The firmware emits one CSV line per sample over the USB serial port.

**Status:** this is the sensor-readout milestone only. There is no fusion, filtering, calibration, or orientation estimate — the accelerometer, gyroscope, die temperature, and button state are passed through as read. The Wi-Fi UDP link and the Python VPython viewer that earlier drafts of this document described are **not implemented**; the title has been retracted accordingly rather than left promising them.

**The VL53L0X time-of-flight sensor is not currently wired.** It was specified in the original design and remains part of the pointer's definition in [AGENTS.md §1.5](../AGENTS.md), so its absence is a gap in the prototype rather than a design decision. See [Why the ToF was removed](#why-the-tof-was-removed) below — with the ToF attached it was corrupting the shared I²C bus, and the IMU has never read reliably in that configuration.

## Hardware

- ESP32 NodeMCU-32S (ESP32-D0WD-V3)
- MPU6050 (e.g., GY-521) IMU breakout
- Optional momentary push button
- Jumper wires and USB **data** cable
- Linux host with [arduino-cli](https://arduino.github.io/arduino-cli/latest/) (on Arch: `sudo pacman -S arduino-cli`)

## Wiring

**Disconnect USB power before wiring.** The connections below use the ESP32's **3V3** rail, not 5V. Verify that your particular breakout module supports 3.3 V supply and I²C logic.

### MPU6050

| MPU6050 pin | ESP32 pin | Notes |
|---|---|---|
| VCC | 3V3 | Power |
| GND | GND | Common ground |
| SDA | GPIO 21 | I²C data |
| SCL | GPIO 22 | I²C clock |
| XDA | Not connected | Auxiliary I²C data |
| XCL | Not connected | Auxiliary I²C clock |
| AD0 / ADO | GND (recommended) | Selects address `0x68` |
| INT (sometimes labeled NT) | Not connected | Optional interrupt |

### Optional button

| Button terminal | ESP32 pin | Notes |
|---|---|---|
| One contact | GPIO 18 | Configured `INPUT_PULLUP` |
| Other contact | GND | Closes the pin to ground when held |

A pressed button then reads `LOW`. On four-leg tactile switches, make sure the chosen legs are on opposite sides of the switch's electrical contact.

### Connections at a glance

```text
ESP32 3V3     ── MPU6050 VCC
ESP32 GND     ── MPU6050 GND ── button GND
ESP32 GPIO21  ── MPU6050 SDA
ESP32 GPIO22  ── MPU6050 SCL
ESP32 GPIO18  ── button contact
MPU6050 AD0   ── GND (recommended)
```

## Firmware

The sketch lives at [`firmware/pointer_sensor_node/`](firmware/pointer_sensor_node) and is defined as an arduino-cli sketch project, so the board and platform version are pinned in `sketch.yaml`. It has **no third-party dependencies**: the MPU6050 is read straight from its registers over `Wire`, which needs only a `WHO_AM_I` probe, four configuration writes, and one 14-byte burst per sample.

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

Note that a monitor attached after the chip has already booted sees only the CSV stream; the boot log is emitted once, inside `setup()`. That is why the firmware repeats its sensor probe periodically instead of relying on the boot banner alone.

### Output schema

A single header line is printed at boot, then one line per sample at roughly 50 Hz:

```text
t_ms,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,temp_c,btn
```

- `t_ms` — milliseconds since boot, stamped at the top of the loop.
- `ax_g`, `ay_g`, `az_g` — accelerometer, g, gravity included.
- `gx_dps`, `gy_dps`, `gz_dps` — gyroscope, degrees per second.
- `temp_c` — die temperature, degrees Celsius.
- `btn` — `1` while the button is held, `0` otherwise.

A failed transfer prints `nan` in every IMU column rather than a stale value, so a dropped read is visible as missing data rather than as a plausible-looking zero.

### Periodic probe

Every 5 seconds a diagnostic block is interleaved with the CSV, reporting the sensor's acknowledgement, its `WHO_AM_I`, and the raw burst bytes:

```text
probe t=20067 MPU6050(0x68)=ACK WHO_AM_I=readable sample=ok
      WHO_AM_I=0x68
      raw: 03 2E FE 6C 58 22 F9 11 FE E1 FE 09 FF AB
```

The raw line is what settles whether the part is genuinely reporting. A device that acknowledges but returns all zeros is **not** healthy: the accelerometer must show roughly 16384 counts on whichever axis gravity points along, and the temperature register is never exactly zero. Set `DIAG_PERIODIC_PROBE` to `false` in the sketch for a stream of nothing but CSV.

## Measured behaviour

Captured from the flashed board with the MPU6050 wired as above, stationary, 737 consecutive samples:

| Quantity | Observed | Expected at rest |
|---|---|---|
| Loop rate | 52.6 Hz (19.0 ms mean) | ~50 Hz |
| Dropped transfers | **0 of 737** | 0 |
| Accelerometer | (0.05, −0.03, 1.38) g | one axis ≈ 1.00 g |
| Gyroscope | (−2.15, −3.82, −0.69) °/s | ≈ 0 °/s |
| Temperature | 31.29 °C, range 31.24–31.35 | ambient-ish, stable |
| Button | 0 throughout | 0 |

**Two offsets need correcting before any orientation work.** The accelerometer magnitude reads 1.38 g rather than 1.00 g, and the gyroscope carries a steady bias of roughly (−2.2, −3.8, −0.7) °/s. Both are ordinary uncalibrated-part behaviour on a GY-521, and both would corrupt the orientation estimate that [AGENTS.md §1.6](../AGENTS.md) makes central to the system. Neither is corrected in firmware yet — the values above are raw, as read.

The accelerometer offset is worth checking against the datasheet before assuming it is simply bias: a 0.38 g error is large even for a cheap part, so confirm the module really is at rest and really is a `0x68` MPU6050 before calibrating away a number that might indicate something else.

## Why the ToF was removed

With the VL53L0X connected on the same bus at `0x29`, the IMU never produced a usable sample: readings alternated between dropped transfers (`nan`) and transfers that returned **all zeros**, with the temperature register reading exactly zero. With the ToF disconnected, the same firmware and the same wiring otherwise produced **737 of 737 valid samples**.

The ToF module was therefore loading or corrupting the shared bus rather than simply being absent. The usual suspects, in the order worth checking:

1. **`XSHUT` left floating.** A VL53L0X held in shutdown can hold the bus low. Drive `XSHUT` from a GPIO and hold it high in `setup()`, or add the pull-up the breakout lacks.
2. **Back-powering through the bus.** With the ToF unpowered but still wired, its I/O lines can clamp the bus. Power it or isolate it completely.
3. **A short or bad solder joint** on the ToF breakout's SDA or SCL.

Before reconnecting it, confirm the MPU6050 stays healthy with the ToF attached but `XSHUT` driven high — that isolates the shutdown-hold theory from a simple wiring fault. Wiring instructions for the ToF, kept for when it returns, are in the git history of this document.

## Troubleshooting

- **`MPU6050: no ACK at 0x68`** — check that `AD0` is tied to GND. Floating `AD0` puts the part at `0x69`, which the firmware does not probe for. Also check the 3V3 rail and the shared ground.
- **`WHO_AM_I` reads `0x70` or `0x71`** — that is an MPU6500 or MPU9250 fitted on the GY-521, and the firmware accepts it.
- **Raw burst is all zeros while `0x68` ACKs** — the part is acknowledging but nothing real is behind it. Check power and the SDA/SCL solder joints; this is not a scale-factor problem.
- **`btn` reads `1` permanently** — GPIO 18 is being held low. Check the button wiring and that the pin is not shorted to ground.
- **Nothing acknowledged at all** — the bus is not powered or not wired; check the 3V3 rail and the shared ground before suspecting the sensor.