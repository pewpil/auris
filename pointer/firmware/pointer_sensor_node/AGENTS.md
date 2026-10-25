# Deployable pointer firmware — AGENTS.md

Standing instructions for `pointer/firmware/pointer_sensor_node/`. Read with [`../../../AGENTS.md`](../../../AGENTS.md) for the repository-wide sections and with [`../../AGENTS.md`](../../AGENTS.md) (the pointer specification) for the hardware intent. This directory holds **the firmware that runs on the pointer itself**, not a bring-up sketch.

> **Scope:** [§2](../../../AGENTS.md) (document every function and class), [§3](../../../AGENTS.md) (the AI never commits), [§4](../../../AGENTS.md) (evidence classes), [§5](../../../AGENTS.md) (never artificially wrap prose), and [§6](../../../AGENTS.md) (side notes) apply here unchanged. The sibling test sketches (`imu_test/`, `node_test/`) remain the bring-up record; this directory is the deployed firmware.

## Status: built and host-tested; final harness not yet wired

The firmware is written, compiles for the NodeMCU-32S, and its pure core passes host-side simulation. It has run on the **old bench harness** (the same ESP32, CJMCU-531, GY-521 and button used by `node_test/`), but the researcher has not yet made the final pointer connections, so nothing here is a claim about the final assembly. The distinction matters for [§4](../../../AGENTS.md#4-verification-policy--no-hardware-bench-phase): the fusion and packet maths are `[SIM]`-verified on a desktop, the compiled size and rates are `[BUD]` observations, and everything read off the breadboard is development activity rather than thesis evidence.

The sibling test sketches — [`imu_test/`](../imu_test/), [`node_test/`](../node_test/), and [`tof_test_l1x/`](../tof_test_l1x/) — remain the bring-up record. This directory is the deployed firmware; its circuit table below is the authoritative wiring copy.

- **One binary, three power contexts.** The same `.bin` runs from a laptop USB port, a USB power bank, or a LiPo feeding a 5 V regulator. The firmware never waits for a serial host and never assumes a network is present; the USB stream and the Wi-Fi link are both optional at runtime.
- **Deployed behaviour is the `node_test` contract plus the actual pointer job:** MPU6050 and VL53L1X on one bus, fused orientation, non-blocking ranging, button-gated UDP to the compute unit, NVS calibration, USB provisioning and debug.
- **The "laser" is the VL53L1X's invisible IR ranging beam.** There is no separate laser output pin; the button gates the pointer's output (and therefore the rendered sound), while the ToF ranges continuously so the first pressed sample is fresh.

## What the firmware does

At boot it resets the ToF through XSHUT, gates on the `0xEACC` model identity, brings up the MPU6050 from raw registers, verifies its configuration, loads calibration from NVS, seeds a Mahony quaternion filter, starts a non-blocking Wi-Fi join, and reports every step on USB. It then samples the IMU at 200 Hz, fuses every sample, polls the ToF at 50 Hz for its ~20 Hz measurements, debounces the button, and — **only while the button is held** — sends a versioned packet to the compute unit at 100 Hz over UDP. Releasing sends one final `btn=0` packet and stops; the compute unit also times out so a lost release cannot leave sound running.

The USB stream is always on at 25 Hz, in the same schema as the datagram, so the same binary works on a bench laptop with `arduino-cli monitor`. A small `@` console over the same USB port provides `@CAL`, `@STATUS`, `@DEST`, `@DEFAULTS` and `@REBOOT`. Calibration, the boot counter and the unicast override live in NVS and survive a reflash (but not a full `erase_flash`).

Fault recovery is automatic: an all-zero MPU burst rewrites the configuration registers; a ToF missing for 600 ms is hard-reset through XSHUT and re-initialised; a dropped Wi-Fi association is retried every 5 s. Sampling and fusion continue through every link outage.

## Parts

| Part | What it is | Note |
|---|---|---|
| ESP32 NodeMCU-32S | 38-pin ESP32-WROOM-32 board, CP2102 USB-UART | Carries the firmware and the 3V3 regulator for the sensors |
| CJMCU-531 | **VL53L1X** time-of-flight breakout, sold mislabeled "VL53L0X" | Identity `EA CC 10 FF` at 16-bit index `0x010F`; see [`../tof_test_l1x/AGENTS.md`](../tof_test_l1x/AGENTS.md) for the mislabel record |
| GY-521 | MPU6050 breakout, silkscreen `ITG/MPU` and `HW-123` | Clone/variant; `WHO_AM_I` answers `0x68`; its raw accelerometer has a large zero-g offset, so `@CAL` is required |
| Tactile push button, 4-leg | The pointer's only control | `INPUT_PULLUP`; idle HIGH (`btn=0`), pressed LOW (`btn=1`) |
| 2 × 4.7 kΩ | I²C pull-ups to 3V3 | The GY-521 carries its own onboard pair in parallel, giving ≈2.3 kΩ per line |
| 2 × 100 nF ceramic | Decoupling, one at each sensor module | Specified for both; the current bench has the ToF one fitted, and the IMU position should be added for the final build |
| 100 µF, 50 V electrolytic *(optional)* | Bulk decoupling for the sensor rail | Recommended now that Wi-Fi is enabled; across the `+`/`−` rails or at the GY-521 |
| No laser diode | The "laser" is the VL53L1X IR beam | The parent's §1.5 calls it imaginary/invisible; there is no GPIO for it |

## Connections

**This table is the authoritative wiring copy for the deployed pointer.** The `.ino` header deliberately defers to it rather than repeating a second copy that can drift. The sibling test sketches have their own copies for their own benches. The GPIO number is the authority; the `r1`/`r2` position only tells you where to look. `r1` is the top row numbered 1–19; `r2` is the bottom row numbered 60–42 across the same columns, so `r1 #9` and `r2 #50` sit side by side and are unrelated numbers.

In words: the 3V3 rail feeds both sensor modules' `VCC` and both pull-up resistors; the ground rail returns both modules, the button's second leg, and the ESP32's ground. SDA and SCL run from GPIO21 and GPIO22 to both modules, with one 4.7 kΩ pull-up on each line to 3V3. The ToF's enable comes from GPIO27, the button from GPIO18, and the IMU's address select `AD0` is tied to ground. Each module gets its own 100 nF across `VCC`/`GND`, and the optional 100 µF spans the rails for the radio's current spikes. There is no other connection between the two sensors and no laser output.

| From | To | Function |
|---|---|---|
| `r1 #19` — **3V3** | breadboard **+** rail, then CJMCU-531 `VCC` and GY-521 `VCC` | 3.3 V sensor supply from the board's regulator |
| `r2 #42` — **GND** | breadboard **−** rail, then CJMCU-531 `GND`, GY-521 `GND`, button leg B | Common ground (`r1 #2`, `r1 #6`, `r2 #48` are the same net) |
| `r2 #47` — **GPIO21** | CJMCU-531 `SDA` and GY-521 `SDA` | I²C data, shared by both sensors |
| `r2 #44` — **GPIO22** | CJMCU-531 `SCL` and GY-521 `SCL` | I²C clock, shared by both sensors |
| `r1 #9` — **GPIO27** | CJMCU-531 `XSHUT` | ToF enable; driven LOW for a reset and HIGH for the run, never floated |
| `r2 #50` — **GPIO18** | button leg A | Input with the internal pull-up; idle HIGH, pressed LOW |
| GY-521 `AD0` | the **−** rail | Ties the IMU to `0x68`; floating moves it to `0x69`, which nothing probes |
| CJMCU-531 `GPIO1`, GY-521 `XDA`, `XCL`, `INT` | *no connection* | Interrupt and auxiliary pins, unused |
| 4.7 kΩ #1 | between the SDA net and the **+** rail | SDA pull-up |
| 4.7 kΩ #2 | between the SCL net and the **+** rail | SCL pull-up |
| 100 nF #1 | across CJMCU-531 `VCC` and `GND` | Decoupling at the ToF module |
| 100 nF #2 | across GY-521 `VCC` and `GND` | Decoupling at the IMU module; specified for the final build |
| 100 µF electrolytic *(optional)* | across the `+` and `−` rails, or GY-521 `VCC`/`GND` | Bulk decoupling; + leg to VCC, stripe to GND |
| Onboard LED | GPIO2, no wire | Status patterns only; not a user-facing interface |

**Do not use the 5 V input at `r1 #1` as a ground.** All four board `GND` positions are the same net; pick one and keep to it. Never route a wire to the flash pins (`GPIO6`–`GPIO11`, labelled `clk`, `sdd`, `sd1`, `sd2`, `sd3`).

### Power sources

The firmware is power-source agnostic; the board sees 5 V at its USB input or 5 V at the `5V` pin and regulates 3.3 V for itself and the sensors. The three supported arrangements are:

| Source | Path into the board | Notes |
|---|---|---|
| Laptop / desktop | USB port | Power **and** data: USB serial debug, `@` console, and the same binary flashing path. This is the provisioning and bench arrangement. |
| USB power bank | USB port | Power only; no host needed. The firmware never waits for USB data. |
| LiPo + regulator | 5 V boost/charger module, then the board's USB port **or** the `5V` pin (`r1 #1`, labelled `su`) | Use a module with a regulated 5 V output. Do **not** connect a LiPo directly to the 5 V pin or the 3V3 rail. Prefer the USB path; if feeding the 5 V pin, leave the USB cable disconnected unless the board's back-feed diode has been checked. |

**The 3V3 rail is an output, not a battery input.** It is the ESP32 board's regulated 3.3 V output and also feeds both sensors. A fresh LiPo is 4.2 V, so connecting its positive terminal to `3V3` (or to the breadboard `+` rail) applies 3.7–4.2 V directly to the ESP32 and both sensor modules, above their absolute maximum. The battery's **negative** terminal may go to the common ground rail, but its **positive** must pass through a charger/booster producing a regulated 5 V for the board's USB port or `5V` pin. A USB power bank needs no rail wiring at all — plug it into the board's USB port.

**Never feed the `5V` pin and a USB cable at the same time without verifying the board's back-feed protection.** A LiPo wired straight to `3V3` or `5V` is out of specification. When the supply sags, the ESP32 reports a brownout reset reason, which the firmware carries in every packet and prints on the console.

### The checks that matter

**XSHUT must be driven, never floated.** The ToF is held in reset for 10 ms, released, and given 100 ms to settle before any traffic; the firmware drives it HIGH for the whole run. This matters doubly on a shared bus: a floating XSHUT has historically made every device on the bus unreadable. A marginal XSHUT jumper is a prime suspect when the ToF resets intermittently.

**AD0 must sit on the ground rail.** A floating AD0 answers at `0x69`; the firmware probes `0x68` and retries forever. The boot log names the addresses that answered.

**The button's two legs must be on opposite sides of the switch contact.** On a four-leg tactile switch the two legs on one side are internally joined and permanently shorted, so wiring across one side holds the input closed: `btn` stays 1 with nothing pressed. One leg from each side makes the press real. A missing leg leaves the internal pull-up in charge: `btn` stays 0.

**Both external pull-ups stay fitted.** The GY-521's onboard pair adds in parallel; ≈2.3 kΩ per line is the parent's explicitly allowed case at 400 kHz.

**Decouple at the modules.** One 100 nF belongs across each module's own VCC/GND, not stretched across the board. The 100 µF is optional but recommended with Wi-Fi enabled; the radio's current spikes are the only pulsed load on the rail.

**The sensor supply is the board's 3.3 V rail.** The CJMCU-531 regulates its own core supply from that rail; the GY-521 has its own LDO. Do not feed either module's VCC from 5 V.

**The LED is not a user interface.** It is for the researcher during provisioning: fast blink for a sensor fault, slow blink while the link is down or disabled, solid while the button is held, off when nominal.

## Pin map in the firmware

| Signal | GPIO | `config.h` constant | Address / note |
|---|---|---|---|
| I²C SDA | 21 (`r2 #47`) | `PIN_I2C_SDA` | shared |
| I²C SCL | 22 (`r2 #44`) | `PIN_I2C_SCL` | 400 kHz |
| ToF XSHUT | 27 (`r1 #9`) | `PIN_TOF_XSHUT` | active-high enable |
| Button | 18 (`r2 #50`) | `PIN_BUTTON` | `INPUT_PULLUP` |
| Onboard LED | 2 | `PIN_LED` | no external wire |
| MPU6050 | — | `ADDR_MPU6050` | `0x68`, `AD0` grounded |
| VL53L1X | — | `ADDR_VL53L1X` | `0x29`, fixed by silicon |

## How it runs

**Boot.** Bus at 400 kHz; NVS opened; boot id incremented and reset reason captured; Wi-Fi join started without blocking; sensors brought up with per-device retries (the only intentionally blocking loop, and it runs before the device is usable); ToF long mode with a 50 ms budget and period; MPU6050 woken and configured for ±500 deg/s, ±4 g and a ~94 Hz DLPF; calibration loaded; first good IMU sample seeds the fusion; the USB header, `#status` and `#ready` lines are printed.

**Sampling and fusion.** The MPU6050 is read at 200 Hz (14-byte burst from `0x3B`) and a Mahony filter integrates bias-corrected body rates while pulling roll and pitch toward gravity. The quaternion is **body-to-world**; Euler angles are intrinsic ZYX in degrees. There is no magnetometer, so yaw is relative and drifts with whatever gyro bias remains after calibration. The ToF is polled through `dataReady()` and never blocks the IMU loop; a measurement is 50 ms budget / 50 ms period, so ~20 Hz.

**Button and link.** The button is debounced for 15 ms. On a press the next packet is forced immediately; packets then leave at 100 Hz while held. On release one final `btn=0` packet is sent and the stream stops. The ToF keeps ranging while idle so the first pressed sample is fresh. The pointer's `btn` column is in the packet and the USB row; the compute unit should gate its rendering on it.

**Packet.** Versioned CSV beginning `P1,` with sequence, monotonic extended `t_us`, boot id, button, body-to-world quaternion, ZYX Euler angles, calibrated gyro (deg/s), calibrated accel (g), die temperature, range (mm), range status, range age, flags and reset reason. Sequence numbers advance only when a datagram is sent, so gaps measure link loss. `t_us` is extended across the 32-bit `micros()` wrap, so it stays monotonic for study-length sessions. Timestamps are since boot, not a wall clock; the compute unit must pair them with its own receive times.

**Flags byte.** Bit 0 calibration valid, bit 1 IMU fresh, bit 2 ToF fresh, bit 3 link up. `range_mm = -1`, `range_status = 255`, `range_age_ms = -1` mean no measurement yet; they are never a plausible zero.

**Recovery.** All-zero IMU bursts are the signature of a part that has reset into sleep; the firmware rewrites the four configuration registers after 20 of them and counts the event. A ToF missing for 600 ms gets the full XSHUT reset and re-initialisation, with the reinit count exposed. A dropped Wi-Fi association is retried every 5 s; sends fail over to nothing and resume when the link returns. The boot/reset reason, boot id and counters make these events visible in the stream and in `@STATUS`.

**Power behaviour.** No sleep mode is enabled: the station stays associated and the radio awake so the first press is answered without wake latency. The USB stream is harmless when no host is connected; it is written at 25 Hz and drains through the CP2102/UART whether or not anything reads it.

**Desktop interoperability.** The same `.bin` runs tethered or untethered. Plugged into a laptop, it prints the 25 Hz packet stream over USB and accepts the `@` console; on the same LAN, the laptop can run the compute unit's receiver and get the identical rows over UDP. Nothing is recompiled or reconfigured between the bench and the field.

## Calibration

**Run `@CAL` with the pointer flat, sensor +Z up, and still.** The command samples 400 bursts at 200 Hz (2 s), rejects the run if the gyro peak-to-peak exceeds 8 deg/s or the accelerometer peak-to-peak exceeds 0.35 g, computes the gyro bias as the mean rate and the accelerometer offset as the flat mean minus `(0, 0, 1) g`, then stores both in NVS. The filter is re-seeded from the corrected mean.

Calibration is not optional on this hardware: the GY-521 clone's raw accelerometer has a zero-g offset far outside the MPU6050 datasheet (the earlier bench fit put roughly +0.35 g on the sensor's Z axis), so an uncalibrated board's gravity magnitude and fused tilt are wrong. `cal=0` in the status and the missing `0x01` flag bit say the device is uncalibrated. The calibration is per-device and survives a reflash; `@DEFAULTS` clears it.

The flat reference is the **sensor's** +Z axis, not the eventual enclosure's; the mounting rotation from the sensor frame to the pointer's mechanical frame is a compute-unit constant that has not been measured yet.

## Packet schema

```
P1,seq,t_us,boot,btn,qw,qx,qy,qz,roll_deg,pitch_deg,yaw_deg,gx_dps,gy_dps,gz_dps,ax_g,ay_g,az_g,temp_C,range_mm,range_status,range_age_ms,flags,reset
```

| Field | Unit / meaning |
|---|---|
| `seq` | UDP datagram sequence, increments only on a send; a gap is a lost datagram |
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

The USB row is the same line, printed with `Serial.println`; a `#`-prefixed header repeats every five seconds. The serial row carries the last sequence number actually sent, not a fabricated one.

## USB console

Commands are sent as a line beginning `@`; replies begin `#`. The console and the 25 Hz stream share the port, so a parser must ignore non-`P1` lines.

| Command | Effect |
|---|---|
| `@CAL` | Flat/Z-up/still calibration, saved to NVS |
| `@STATUS` | Boot id, reset reason, calibration, sensor/link flags, counters, IP/destination/RSSI |
| `@DEST <ip>` | Store a unicast destination and switch to it |
| `@DEST broadcast` | Clear the override and use the subnet's directed broadcast |
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

The profile **`pointer-sensor-node`** is the one the parent [`pointer/AGENTS.md`](../../AGENTS.md#build-flash-monitor) already documents; before this directory existed it did not resolve. The VL53L1X driver is pinned in `sketch.yaml`; a machine that has never installed it needs `arduino-cli lib install VL53L1X` once.

`wifi_secrets.h` is untracked and ignored by `pointer/firmware/.gitignore`; copy it from `../imu_test/`. Without it the sketch builds and runs USB-only, saying so at boot. The build produces `dist/pointer_sensor_node.ino.bin` plus the bootloader/partition companions; `arduino-cli upload` writes all of them for a fresh board. A production line that flashes bare modules can merge them with `esptool.py` (bootloader `0x1000`, partitions `0x8000`, app `0x10000` for this board definition).

Provisioning before deployment: flash the board, connect USB, run `@CAL` on a flat, still surface, optionally set `@DEST`, then disconnect and power from the chosen source. NVS keeps the calibration; a full `erase_flash` would not.

## Verification status

**`[SIM]` — host tests.** `test/test_fusion.cpp` builds with plain `g++` and checks Euler/quaternion round trips, identity seeding, static 30° roll convergence, 90° gyro yaw integration, quaternion norm, gravity-estimate agreement, the 90° pitch rotation, the flat-offset calculation, and the 24-field packet serialisation including the no-range sentinel. Run it with:

```bash
g++ -std=c++17 -Wall -Wextra -O2 test/test_fusion.cpp src/fusion.cpp src/packet.cpp -o /tmp/pointer_host_test && /tmp/pointer_host_test
```

**`[BUD]` — build size.** 954,736 bytes of program storage (72% of 1,310,720), 49,272 bytes of static RAM (15%), from arduino-cli 3.3.12 for `esp32:esp32:nodemcu-32s`.

**Bench development observation (old harness, before the final wiring).** The same binary streamed 623 well-formed `P1` rows at 25 Hz with valid ToF ranges (33–42 mm, status 0), live IMU flags, and the Wi-Fi station joined at `192.168.1.5`; `@STATUS` and `@REBOOT` worked; the gate counters recorded `link_sent=5237`, `link_fail=0` while the old button input was held. That run also had `cal=0` (yaw drifting at the uncalibrated gyro bias) and printed the boot `#status` before the first IMU sample and before association, so its `imu=0 link=0` fields are a reporting-order artefact, not a fault.

**Not yet verified.** The final pointer wiring, `@CAL` on the final mount, UDP reception at the compute unit (send-side counters are not proof of arrival), runtime on a power bank and on a LiPo regulator, and the sensor-frame-to-mechanical-frame mounting rotation. These are development checks, not §4 evidence.

## Known limitations

- **Yaw is unobservable.** No magnetometer; yaw integrates residual gyro bias and its *difference* between pointer and head drifts too. The study design has to live with this or add an absolute reference; the firmware reports the quaternion it can defend and does not pretend otherwise.
- **The GY-521 clone needs calibration.** Its raw accelerometer offset is far outside the MPU6050 datasheet; without `@CAL`, tilt and gravity magnitude are wrong. The offset is per-device, so it belongs in NVS, not in code.
- **The ToF has shown intermittent resets on the bench.** Status-5 rows with `0xFFFF` sentinel values and runs where measurements stopped until a reset were observed in `node_test/`; the firmware's XSHUT recovery is a mitigation, not a cure. Reseat XSHUT and the module supply, and fit the optional 100 µF, before trusting the link.
- **The UDP link is fire-and-forget.** No retransmission, acknowledgement, or clock synchronisation; a lost datagram is a lost sample (sequence gaps expose it). Broadcast may be filtered by AP isolation; `@DEST` is the escape hatch. The ESP32 radio is 2.4 GHz only.
- **The button is a gate, not a laser.** There is no separate emitter; the pointer's "laser" is the ToF IR beam. The button is debounced in firmware but is still a raw electromechanical input.
- **Calibration requires a flat, still placement.** There is no self-calibration in the field; provisioning needs USB and a level surface. This is deliberate for VI users, who should never enter a setup mode by gripping the device at power-on.
- **The boot `#status` line is taken before the first IMU sample and before Wi-Fi association**, so `imu=0 link=0` there is normal; use `@STATUS` or the stream for live state.
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
- UDP reception at the compute unit and the runtime on a power bank and on a LiPo regulator are unverified.
- The parent [`pointer/AGENTS.md`](../../AGENTS.md) still needs its part name (VL53L0X → VL53L1X) and current-state paragraphs brought in line with this directory.
