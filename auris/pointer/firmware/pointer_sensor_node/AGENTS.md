# Deployable pointer firmware — AGENTS.mdhlj

Standing instructions for `pointer/firmware/pointer_sensor_node/`. Read with [`../../../AGENTS.md`](../../../AGENTS.md) for the repository-wide sections and with [`../../AGENTS.md`](../../AGENTS.md) (the pointer specification) for the hardware intent. This directory holds **the firmware that runs on the pointer itself**, not a bring-up sketch.

> **Scope:** [§2](../../../AGENTS.md) (document every function and class), [§3](../../../AGENTS.md) (the AI never commits), [§4](../../../AGENTS.md) (evidence classes), [§5](../../../AGENTS.md) (never artificially wrap prose), and [§6](../../../AGENTS.md) (side notes) apply here unchanged. The sibling test sketches (`imu_test/`, `node_test/`) remain the bring-up record; this directory is the deployed firmware.

## Status: built and host-tested; final harness not yet wired

The firmware is written, compiles for the 30-pin ESP32 DevKit V1, and its pure core passes host-side simulation. It has run on the previous 38-pin bench board in two builds; the harness now carries both 100 nF capacitors and the optional 100 µF, but the final enclosure, power source, and the button's idle/pressed check are still open, so nothing here is a claim about the final assembly. The distinction matters for [§4](../../../AGENTS.md#4-verification-policy--no-hardware-bench-phase): the fusion and packet maths are `[SIM]`-verified on a desktop, the compiled size and rates are `[BUD]` observations, and everything read off the breadboard is development activity rather than thesis evidence.

The sibling test sketches — [`imu_test/`](../imu_test/), [`node_test/`](../node_test/), [`tof_test_l1x/`](../tof_test_l1x/), and [`button_test/`](../button_test/) — remain the bring-up record. This directory is the deployed firmware; its circuit table below is the authoritative wiring copy.

- **One binary, three power contexts.** The same `.bin` runs from a laptop USB port, a USB power bank, or a LiPo feeding a 5 V regulator. The firmware never waits for a serial host and never assumes a network is present; the USB stream and the ESP-NOW link are both optional at runtime.
- **Deployed behaviour is the `node_test` contract plus the actual pointer job:** MPU6050 and VL53L1X on one bus, fused orientation, non-blocking ranging, button-gated ESP-NOW to the compute unit, NVS calibration, USB provisioning and debug. **The current bench build is continuous-link:** `LINK_BUTTON_GATED` is false, so packets flow at 100 Hz without a press; the gated path is retained for deployment.
- **The "laser" is the VL53L1X's invisible IR ranging beam.** There is no separate laser output pin; the button gates the pointer's output (and therefore the rendered sound), while the ToF ranges continuously so the first pressed sample is fresh.

## What the firmware does

At boot it resets the ToF through XSHUT, gates on the `0xEACC` model identity, brings up the MPU6050 from raw registers, verifies its configuration, loads calibration from NVS, seeds a Mahony quaternion filter, starts the ESP-NOW radio without joining a network, and reports every step on USB. It then samples the IMU at 200 Hz, fuses every sample, polls the ToF at 50 Hz for its ~20 Hz measurements, and debounces the button. The deployed configuration sends a versioned packet to the compute unit at 100 Hz over ESP-NOW **only while the button is held**, with one final `btn=0` packet on release; the compute unit also times out so a lost release cannot leave sound running. The current bench build sets `LINK_BUTTON_GATED` to false, so the same packets leave **continuously whenever the link is up**; the `btn` field still reports the live input but gates nothing. Flipping that one constant restores the deployed behaviour.

The USB stream is always on at 25 Hz as a compact fixed-width table of orientation, acceleration, range and health, narrow enough for one 80-column line, so the same binary works on a bench laptop with `arduino-cli monitor`. A small `@` console over the same USB port provides `@CAL`, `@STATUS`, `@DEFAULTS` and `@REBOOT`. Calibration and the boot counter live in NVS and survive a reflash (but not a full `erase_flash`).

Fault recovery is automatic: an all-zero MPU burst rewrites the configuration registers; a ToF missing for 600 ms is hard-reset through XSHUT and re-initialised; a radio or peer registration failure is retried every second. Sampling and fusion continue through every link outage.

## Parts

| Part                                   | What it is                                                     | Note                                                                                                                                  |
| -------------------------------------- | -------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------- |
| ESP32 DevKit V1 (30-pin)               | 30-pin ESP32-WROOM-32 board, CP2102 USB-UART, silkscreen labels | Carries the firmware and the 3V3 regulator for the sensors; the board's printed pin labels are the authority                          |
| CJMCU-531                              | **VL53L1X** time-of-flight breakout, sold mislabeled "VL53L0X" | Identity `EA CC 10 FF` at 16-bit index `0x010F`; see [`../tof_test_l1x/AGENTS.md`](../tof_test_l1x/AGENTS.md) for the mislabel record |
| GY-521                                 | MPU6050 breakout, silkscreen `ITG/MPU` and `HW-123`            | Clone/variant; `WHO_AM_I` answers `0x68`; its raw accelerometer has a large zero-g offset, so `@CAL` is required                      |
| Tactile push button, 4-leg             | The pointer's only control                                     | `INPUT_PULLUP`; idle HIGH (`btn=0`), pressed LOW (`btn=1`)                                                                            |
| 2 × 4.7 kΩ                             | I²C pull-ups to 3V3                                            | The GY-521 carries its own onboard pair in parallel, giving ≈2.3 kΩ per line                                                          |
| 2 × 100 nF ceramic                     | Decoupling, one at each sensor module                          | Specified for both; the current bench has the ToF one fitted, and the IMU position should be added for the final build                |
| 100 µF, 50 V electrolytic *(optional)* | Bulk decoupling for the sensor rail                            | Recommended now that the radio is enabled; across the `+`/`−` rails or at the GY-521                                                      |
| No laser diode                         | The "laser" is the VL53L1X IR beam                             | The parent's §1.5 calls it imaginary/invisible; there is no GPIO for it                                                               |

## Connections

**This table is the authoritative wiring copy for the deployed pointer.** The `.ino` header deliberately defers to it rather than repeating a second copy that can drift. The sibling test sketches have their own copies for their own benches. The pointer board prints its pin names on the silkscreen, so the labels are the authority; there is no row/column numbering to translate. The bottom row runs `3V3, GND, D15, D2, D4, RX2, TX2, D5, D18, D19, D21, RX0, TX0, D22, D23`; the top row is `VN, GND, D13, D12, D14, D27, D26, D25, D33, D32, D35, D34, VN, VP, EN` as printed. The used pins and their neighbours are called out in the table.

In words: the 3V3 rail feeds both sensor modules' `VCC` and both pull-up resistors; the ground rail returns both modules, the button's second leg, and the ESP32's ground. SDA and SCL run from D21 (GPIO21) and D22 (GPIO22) to both modules, with one 4.7 kΩ pull-up on each line to 3V3. The ToF's enable comes from D27 (GPIO27), the button from D18 (GPIO18), and the IMU's address select `AD0` is tied to ground. Each module gets its own 100 nF across `VCC`/`GND`, and the optional 100 µF spans the rails for the radio's current spikes. There is no other connection between the two sensors and no laser output.

| From                                          | To                                                                      | Function                                                                  |
| --------------------------------------------- | ----------------------------------------------------------------------- | ------------------------------------------------------------------------- |
| `3V3` (bottom row, next to `GND`)             | breadboard **+** rail, then CJMCU-531 `VCC` and GY-521 `VCC`            | 3.3 V sensor supply from the board's regulator                            |
| `GND` (bottom row, next to `3V3`)             | breadboard **−** rail, then CJMCU-531 `GND`, GY-521 `GND`, button leg B | Common ground; the top row also exposes a `GND`                          |
| `D21` — **GPIO21** (between `D19` and `RX0`)  | CJMCU-531 `SDA` and GY-521 `SDA`                                        | I²C data, shared by both sensors                                          |
| `D22` — **GPIO22** (between `TX0` and `D23`)  | CJMCU-531 `SCL` and GY-521 `SCL`                                        | I²C clock, shared by both sensors                                         |
| `D27` — **GPIO27** (top, between `D14`/`D26`) | CJMCU-531 `XSHUT`                                                       | ToF enable; driven LOW for a reset and HIGH for the run, never floated    |
| `D18` — **GPIO18** (between `D5` and `D19`)   | button leg A                                                            | Input with the internal pull-up; idle HIGH, pressed LOW                   |
| GY-521 `AD0`                                  | the **−** rail                                                          | Ties the IMU to `0x68`; floating moves it to `0x69`, which nothing probes |
| CJMCU-531 `GPIO1`, GY-521 `XDA`, `XCL`, `INT` | *no connection*                                                         | Interrupt and auxiliary pins, unused                                      |
| 4.7 kΩ #1                                     | between the SDA net and the **+** rail                                  | SDA pull-up                                                               |
| 4.7 kΩ #2                                     | between the SCL net and the **+** rail                                  | SCL pull-up                                                               |
| 100 nF #1                                     | across CJMCU-531 `VCC` and `GND`                                        | Decoupling at the ToF module                                              |
| 100 nF #2                                     | across GY-521 `VCC` and `GND`                                           | Decoupling at the IMU module; specified for the final build               |
| 100 µF electrolytic *(optional)*              | across the `+` and `−` rails, or GY-521 `VCC`/`GND`                     | Bulk decoupling; + leg to VCC, stripe to GND                              |
| Onboard LED                                   | `D2` (GPIO2, between `D15` and `D4`), no wire                           | Status patterns only; not a user-facing interface                         |

**Do not use the `VIN`/5 V pin as a ground.** The pointer board exposes `GND` on both rows; use the bottom-row `GND` next to `3V3` for the rail. The 30-pin board does not break out the SPI-flash pins, unlike the 38-pin bridge.

### Power sources

The firmware is power-source agnostic; the board sees 5 V at its USB input or 5 V at the `5V` pin and regulates 3.3 V for itself and the sensors. The three supported arrangements are:

| Source | Path into the board | Notes |
|---|---|---|
| Laptop / desktop | USB port | Power **and** data: USB serial debug, `@` console, and the same binary flashing path. This is the provisioning and bench arrangement. |
| USB power bank | USB port | Power only; no host needed. The firmware never waits for USB data. |
| LiPo + regulator | 5 V boost/charger module, then the board's USB port **or** the `VIN`/5 V pin | Use a module with a regulated 5 V output. Do **not** connect a LiPo directly to the 5 V pin or the 3V3 rail. Prefer the USB path; if feeding the 5 V pin, leave the USB cable disconnected unless the board's back-feed diode has been checked. |

**The 3V3 rail is an output, not a battery input.** It is the ESP32 board's regulated 3.3 V output and also feeds both sensors. A fresh LiPo is 4.2 V, so connecting its positive terminal to `3V3` (or to the breadboard `+` rail) applies 3.7–4.2 V directly to the ESP32 and both sensor modules, above their absolute maximum. The battery's **negative** terminal may go to the common ground rail, but its **positive** must pass through a charger/booster producing a regulated 5 V for the board's USB port or `5V` pin. A USB power bank needs no rail wiring at all — plug it into the board's USB port.

**Never feed the `5V` pin and a USB cable at the same time without verifying the board's back-feed protection.** A LiPo wired straight to `3V3` or `5V` is out of specification. When the supply sags, the ESP32 reports a brownout reset reason, which the firmware carries in every packet and prints on the console.

### The checks that matter

**XSHUT must be driven, never floated.** The ToF is held in reset for 10 ms, released, and given 100 ms to settle before any traffic; the firmware drives it HIGH for the whole run. This matters doubly on a shared bus: a floating XSHUT has historically made every device on the bus unreadable. A marginal XSHUT jumper is a prime suspect when the ToF resets intermittently.

**AD0 must sit on the ground rail.** A floating AD0 answers at `0x69`; the firmware probes `0x68` and retries forever. The boot log names the addresses that answered.

**The button's two legs must be on opposite sides of the switch contact.** On a four-leg tactile switch the two legs on one side are internally joined and permanently shorted, so wiring across one side holds the input closed: `btn` stays 1 with nothing pressed. One leg from each side makes the press real. A missing leg leaves the internal pull-up in charge: `btn` stays 0.

**Both external pull-ups stay fitted.** The GY-521's onboard pair adds in parallel; ≈2.3 kΩ per line is the parent's explicitly allowed case at 400 kHz.

**Decouple at the modules.** One 100 nF belongs across each module's own VCC/GND, not stretched across the board. The 100 µF is optional but recommended with the radio enabled; the radio's current spikes are the only pulsed load on the rail.

**The sensor supply is the board's 3.3 V rail.** The CJMCU-531 regulates its own core supply from that rail; the GY-521 has its own LDO. Do not feed either module's VCC from 5 V.

**The LED is not a user interface.** It is for the researcher during provisioning: fast blink for a sensor fault, slow blink while the link is down or disabled, solid while the button is held in the gated mode, off when nominal. In the current continuous-link mode the button is ignored for the LED, so nominal means off.

## Pin map in the firmware

| Signal | Label / GPIO | `config.h` constant | Address / note |
|---|---|---|---|
| I²C SDA | `D21` / 21, bottom row between `D19` and `RX0` | `PIN_I2C_SDA` | shared |
| I²C SCL | `D22` / 22, bottom row between `TX0` and `D23` | `PIN_I2C_SCL` | 400 kHz |
| ToF XSHUT | `D27` / 27, top row between `D14` and `D26` | `PIN_TOF_XSHUT` | active-high enable |
| Button | `D18` / 18, bottom row between `D5` and `D19` | `PIN_BUTTON` | `INPUT_PULLUP` |
| Onboard LED | `D2` / 2, bottom row between `D15` and `D4` | `PIN_LED` | no external wire |
| MPU6050 | — | `ADDR_MPU6050` | `0x68`, `AD0` grounded |
| VL53L1X | — | `ADDR_VL53L1X` | `0x29`, fixed by silicon |

## How it runs

**Boot.** Bus at 400 kHz; the button is configured `INPUT_PULLUP` before the first read (without this the pin floats and falsely reads pressed); NVS opened; boot id incremented and reset reason captured; ESP-NOW radio started without joining a network; sensors brought up with per-device retries (the only intentionally blocking loop, and it runs before the device is usable); ToF long mode with a 50 ms budget and period; MPU6050 woken and configured for ±500 deg/s, ±4 g and a ~94 Hz DLPF; calibration loaded; first good IMU sample seeds the fusion; the USB header, `#status` and `#ready` lines are printed.

**Sampling and fusion.** The MPU6050 is read at 200 Hz (14-byte burst from `0x3B`) and a Mahony filter integrates bias-corrected body rates while pulling roll and pitch toward gravity. The quaternion is **body-to-world**; Euler angles are intrinsic ZYX in degrees. There is no magnetometer, so yaw is relative and drifts with whatever gyro bias remains after calibration. The ToF is polled through `dataReady()` and never blocks the IMU loop; a measurement is 50 ms budget / 50 ms period, so ~20 Hz.

**Button and link.** The button is debounced for 15 ms. In the deployed mode (`LINK_BUTTON_GATED == true`) a press forces the next packet immediately, packets leave at 100 Hz while held, and a release sends one final `btn=0` packet before the stream stops; the ToF keeps ranging while idle so the first pressed sample is fresh. In the current bench mode (`false`) packets leave continuously at 100 Hz whenever the link is up, and the button is only reported. The pointer's `btn` field is in the packet and the USB row; the compute unit should gate its rendering on it when the link is gated.

**Packet.** Versioned CSV beginning `P1,` with sequence, monotonic extended `t_us`, boot id, button, body-to-world quaternion, ZYX Euler angles, calibrated gyro (deg/s), calibrated accel (g), die temperature, range (mm), range status, range age, flags and reset reason. Sequence numbers advance only when a datagram is sent, so gaps measure link loss. `t_us` is extended across the 32-bit `micros()` wrap, so it stays monotonic for study-length sessions. Timestamps are since boot, not a wall clock; the compute unit must pair them with its own receive times.

**Flags byte.** Bit 0 calibration valid, bit 1 IMU fresh, bit 2 ToF fresh, bit 3 link up. `range_mm = -1`, `range_status = 255`, `range_age_ms = -1` mean no measurement yet; they are never a plausible zero.

**Recovery.** All-zero IMU bursts are the signature of a part that has reset into sleep; the firmware rewrites the four configuration registers after 20 of them and counts the event. A ToF missing for 600 ms gets the full XSHUT reset and re-initialisation, with the reinit count exposed. A failed ESP-NOW initialisation or peer registration is retried every second; sends fail over to nothing and resume when the link returns. The boot/reset reason, boot id and counters make these events visible in the stream and in `@STATUS`.

**Power behaviour.** No sleep mode is enabled: the station stays associated and the radio awake so the first press is answered without wake latency. The USB stream is harmless when no host is connected; it is written at 25 Hz and drains through the CP2102/UART whether or not anything reads it.

**Desktop interoperability.** The same `.bin` runs tethered or untethered. Plugged into a laptop, it prints the 25 Hz compact table over USB and accepts the `@` console; with the compute unit's ESP-NOW bridge in range, the receiver gets the `P1` rows over the radio link. Nothing is recompiled or reconfigured between the bench and the field.

## Calibration

**Run `@CAL` with the pointer flat, sensor +Z up, and still.** The command samples 400 bursts at 200 Hz (2 s), rejects the run if the gyro peak-to-peak exceeds 8 deg/s or the accelerometer peak-to-peak exceeds 0.35 g, computes the gyro bias as the mean rate and the accelerometer offset as the flat mean minus `(0, 0, 1) g`, then stores both in NVS. The filter is re-seeded from the corrected mean.

Calibration is not optional on this hardware: the GY-521 clone's raw accelerometer has a zero-g offset far outside the MPU6050 datasheet (the earlier bench fit put roughly +0.35 g on the sensor's Z axis), so an uncalibrated board's gravity magnitude and fused tilt are wrong. `cal=0` in the status and the missing `0x01` flag bit say the device is uncalibrated. The calibration is per-device and survives a reflash; `@DEFAULTS` clears it.

The flat reference is the **sensor's** +Z axis, not the eventual enclosure's; the mounting rotation from the sensor frame to the pointer's mechanical frame is a compute-unit constant that has not been measured yet.

## Packet and USB schemas

The ESP-NOW payload is the versioned `P1` CSV line; the USB stream is a compact fixed-width table for a human monitor. They are deliberately different formats: the link wants a stable machine schema, the monitor wants a line that fits one terminal row. The table omits the quaternion, gyro, temperature, sequence, boot id and reset reason to stay under 80 columns; those remain in the `P1` packet and are reachable through `@STATUS`.

```
P1,seq,t_us,boot,btn,qw,qx,qy,qz,roll_deg,pitch_deg,yaw_deg,gx_dps,gy_dps,gz_dps,ax_g,ay_g,az_g,temp_C,range_mm,range_status,range_age_ms,flags,reset
```

| Field | Unit / meaning |
|---|---|
| `seq` | Link packet sequence, increments only on a send; a gap is a lost packet |
| `t_us` | microseconds since boot, extended across the 32-bit wrap |
| `boot` | boot counter from NVS |
| `btn` | 1 while held, 0 otherwise |
| `qw,qx,qy,qz` | body-to-world quaternion |
| `roll_deg,pitch_deg,yaw_deg` | intrinsic ZYX Euler angles |
| `gx_dps,gy_dps,gz_dps` | calibrated body rate in deg/s |
| `ax_g,ay_g,az_g` | calibrated acceleration in g |
| `temp_C` | MPU6050 die temperature |
| `range_mm` | ToF distance along the beam, -1 if none |
| `range_status` | Pololu RangeStatus (0 valid, 7 wrap/near-field, 255 none) |
| `range_age_ms` | age of the last measurement, -1 if none |
| `flags` | bit field described above, printed as `0x..` |
| `reset` | `esp_reset_reason()` at boot (1 power-on, 3 software, 9 brownout, …) |

The USB table has these columns, in order: `t_ms btn roll pitch yaw ax ay az range st age flags`. `t_ms` is `t_us / 1000`; `roll/pitch/yaw` are degrees with one decimal; `ax/ay/az` are g with two decimals; `range` is millimetres (`-1` when none); `st` is the range status (`255` when none); `age` is the range age in milliseconds; `flags` is the same bit field as the packet. It is space-separated, fixed-width, and about 79 characters, so it fits one 80-column line. The header is repeated every five seconds.

## USB console

Commands are sent as a line beginning `@`; replies begin `#`. The console and the 25 Hz table share the port, so a parser must ignore non-data lines; the `P1` CSV schema is the ESP-NOW link only.

| Command | Effect |
|---|---|
| `@CAL` | Flat/Z-up/still calibration, saved to NVS |
| `@STATUS` | Boot id, reset reason, calibration, sensor/link flags, counters, IP/destination/RSSI |
| `@DEFAULTS` | Erase calibration and destination from NVS |
| `@REBOOT` | Restart the ESP32 |

## Build, flash, and provision

From this directory:

```bash
arduino-cli compile --profile pointer-sensor-node
arduino-cli upload  --profile pointer-sensor-node --port /dev/ttyUSB0
./build.sh                       # binaries into dist/
arduino-cli monitor --port /dev/ttyUSB0 --config baudrate=115200
```

The profile **`pointer-sensor-node`** is the one the parent [`pointer/AGENTS.md`](../../AGENTS.md#build-flash-monitor) already documents; the sketch now targets the pointer board, **`esp32:esp32:esp32doit-devkit-v1`** (30-pin DevKit V1), not the 38-pin NodeMCU-32S. The VL53L1X driver is pinned in `sketch.yaml`; a machine that has never installed it needs `arduino-cli lib install VL53L1X` once.

**No credentials.** The link is ESP-NOW, so the sketch has no SSID, password or `wifi_secrets.h`. It starts the Wi-Fi driver in station mode, disconnects, and pins channel 1; it never associates. The build produces `dist/pointer_sensor_node.ino.bin` plus the bootloader/partition companions; `arduino-cli upload` writes all of them for a fresh board. A production line that flashes bare modules can merge them with `esptool.py` (bootloader `0x1000`, partitions `0x8000`, app `0x10000` for this board definition).

**Link status:** the firmware now uses ESP-NOW and compiles at 931,236 bytes of program storage (71%) and 47,560 bytes of static RAM (14%). It has **not been flashed** since the swap, because the pointer board was not connected to the build machine. The bridge firmware at 460800 is also compiled but the board on the Pi still runs the previous 115200 image. Until both are flashed, the link is code-complete but not exercised.

Provisioning before deployment: flash the board, connect USB, run `@CAL` on a flat, still surface, then disconnect and power from the chosen source. NVS keeps the calibration; a full `erase_flash` would not.

## Verification status

**`[SIM]` — host tests.** `test/test_fusion.cpp` builds with plain `g++` and checks Euler/quaternion round trips, identity seeding, static 30° roll convergence, 90° gyro yaw integration, quaternion norm, gravity-estimate agreement, the 90° pitch rotation, the flat-offset calculation, the 24-field packet serialisation including the no-range sentinel, and the compact 12-field USB table fitting one 80-column line with its header. Run it with:

```bash
g++ -std=c++17 -Wall -Wextra -O2 test/test_fusion.cpp src/fusion.cpp src/packet.cpp -o /tmp/pointer_host_test && /tmp/pointer_host_test
```

**`[BUD]` — build size.** 931,236 bytes of program storage (71% of 1,310,720), 47,560 bytes of static RAM (14%), from arduino-cli 3.3.12 for `esp32:esp32:esp32doit-devkit-v1` (30-pin pointer board). The button test builds to 272,780 bytes (20%).

**Bench development observation (previous 38-pin NodeMCU-32S, before the 30-pin pointer board).** The same firmware code streamed 623 well-formed USB rows at 25 Hz with valid ToF ranges (33–42 mm, status 0), live IMU flags, and the Wi-Fi station joined at `192.168.1.5`; that build's USB row was the `P1` CSV, before the tabular USB output was introduced. `@STATUS` and `@REBOOT` worked; the gate counters recorded `link_sent=5237`, `link_fail=0` while the old button input was held. That run also had `cal=0` (yaw drifting at the uncalibrated gyro bias) and printed the boot `#status` before the first IMU sample and before association, so its `imu=0 link=0` fields are a reporting-order artefact, not a fault.

**Later 45 s run on the capacitor-updated 38-pin bench board.** 1,124 of 1,124 ToF measurements were valid at 1.50–1.53 m with zero status-5 sentinels and zero stale ages, the IMU was live throughout, and the stream held 25 Hz; the two 100 nF capacitors and the 100 µF were fitted for this run. The button read held for the whole capture (`presses=1`, `releases=0`), so idle/pressed verification is still outstanding. The send-side counters reached `link_sent=7071`, `link_fail=0`; that build used the Wi-Fi UDP transport; its receive side was never verified at the compute unit, and the ESP-NOW replacement has not been flashed yet. The USB table was verified separately: 500 rows, every row 12 fixed-width fields, header repeated; the earlier 23-column version was replaced because it wrapped in the monitor.

**Continuous bench mode on the 38-pin board (28 Oct).** With `LINK_BUTTON_GATED` false and the GND–GPIO18 short still fitted (`btn=1`, `presses=1`, `releases=0`), `link_sent` climbed 718 → 1215 over five seconds (~99 Hz) with `link_fail=0`, confirming the stream is independent of the button. This was a send-side observation; reception at the compute unit is still unverified.

**Not yet verified.** The final pointer wiring, `@CAL` on the final mount, ESP-NOW reception at the compute unit (send-side counters are not proof of arrival), runtime on a power bank and on a LiPo regulator, and the sensor-frame-to-mechanical-frame mounting rotation. These are development checks, not §4 evidence.

## Known limitations

- **Yaw is unobservable.** No magnetometer; yaw integrates residual gyro bias and its *difference* between pointer and head drifts too. The study design has to live with this or add an absolute reference; the firmware reports the quaternion it can defend and does not pretend otherwise.
- **The GY-521 clone needs calibration.** Its raw accelerometer offset is far outside the MPU6050 datasheet; without `@CAL`, tilt and gravity magnitude are wrong. The offset is per-device, so it belongs in NVS, not in code.
- **The ToF has shown intermittent resets on the bench.** Status-5 rows with `0xFFFF` sentinel values and runs where measurements stopped until a reset were observed in `node_test/`; the firmware's XSHUT recovery is a mitigation, not a cure. Reseat XSHUT and the module supply, and fit the optional 100 µF, before trusting the link.
- **The ESP-NOW link is fire-and-forget.** No retransmission, acknowledgement, or clock synchronisation; a lost packet is a lost sample (sequence gaps expose it). The peer MAC and channel are compile-time constants shared with the bridge. The ESP32 radio is 2.4 GHz only.
- **The current bench mode streams continuously.** With `LINK_BUTTON_GATED` false the radio transmits at 100 Hz for as long as the pointer is powered, which costs more battery than the gated deployment and makes the link independent of the button. Set the flag true for the real device.
- **The button is a gate, not a laser.** There is no separate emitter; the pointer's "laser" is the ToF IR beam. The button is debounced in firmware but is still a raw electromechanical input.
- **Calibration requires a flat, still placement.** There is no self-calibration in the field; provisioning needs USB and a level surface. This is deliberate for VI users, who should never enter a setup mode by gripping the device at power-on.
- **The boot `#status` line is taken before the first IMU sample and before the radio link is ready**, so `imu=0 link=0` there is normal; use `@STATUS` or the stream for live state.
- **No battery monitoring.** The firmware does not measure the supply; a brownout appears as reset reason 9. The final power budget and runtime are open items.
- **The stream is development telemetry.** It carries sequence, timestamps, button and reset reasons, but it is not the full §4.2 event stream (render timings and tracking tiers belong to the compute unit).

## Reading the log when it fails

- **`#imu bring-up attempt N failed`** — no ACK at `0x68`; check `VCC`/`GND`/`SDA`/`SCL` and that `AD0` is grounded (`0x69` means it floats).
- **`#tof bring-up attempt N failed`** — check XSHUT on GPIO27, the module's supply, and the shared bus; the identity must read `EA CC 10 FF`.
- **`#cal missing`** — run `@CAL` flat, Z up, still.
- **`#cal unstable`** — the device moved during the two-second window; repeat on a level, still surface.
- **`#link down` / no `#link up`** — 2.4 GHz network, credentials, or AP isolation; `@STATUS` shows the IP/destination/RSSI.
- **`link_sent` stays 0 while the button is held** — `btn` is not reading 1 (check the two switch legs), or the station is down; the firmware never sends while the link is down.
- **`tof_reinit` climbing** — the ToF is resetting; reseat XSHUT and supply, fit the bulk capacitor, and check the module.
- **`imu_zero` climbing / `#imu reconfigured`** — the MPU slept or browned out; the rewrite is automatic, but repeated occurrences point at the supply.
- **`reset=9(brownout)`** — the power source sagged; check the cable, USB port, regulator, and bulk decoupling.

## Open items

- Final pointer wiring and enclosure are not assembled; the circuit above is the target, and the bench record is from the old harness.
- `@CAL` and the sensor-frame-to-mechanical-frame mounting rotation are still to be done on the final build.
- ESP-NOW reception at the compute unit and the runtime on a power bank and on a LiPo regulator are unverified.
- The parent [`pointer/AGENTS.md`](../../AGENTS.md) still needs its part name (VL53L0X → VL53L1X) and current-state paragraphs brought in line with this directory.
