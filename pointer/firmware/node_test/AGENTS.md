# Sensor-node bring-up test — AGENTS.md

Standing instructions for `pointer/firmware/node_test/`. Read with [`../../AGENTS.md`](../../AGENTS.md), which carries the repository-wide sections, and with [`../AGENTS.md`](../AGENTS.md), which carries the wiring specification and firmware contract for the pointer as a whole.

> **Scope:** the repository-wide sections apply here unchanged — [§2](../../AGENTS.md) on documenting every function, [§3](../../AGENTS.md) on never committing, [§4](../../AGENTS.md) on evidence classes, [§5](../../AGENTS.md) on unwrapped prose, and [§6](../../AGENTS.md) on side notes. Nothing here overrides the parent [pointer/AGENTS.md](../AGENTS.md); where this directory departs from it, that is recorded below rather than left to be discovered.

## Status: the sensor-node prototype — both sensors and the button, together

**This is a bring-up sketch, not the pointer's firmware** — but it is the closest thing on the bench to it so far. It exists to prove the pointer's sensor node *as a whole*: the VL53L1X (CJMCU-531) and the MPU6050 (GY-521) sharing one I²C bus at 400 kHz, plus the laser button on GPIO 18, with one combined tabular stream borne on USB serial and, while the button is held, broadcast over Wi-Fi UDP to the compute unit. The single-sensor halves were proved by their siblings — [`imu_test/`](../imu_test/) for the IMU alone, [`tof_test_l1x/`](../tof_test_l1x/) for the ToF alone, the latter also having discovered that the module bought as a "VL53L0X" is a **VL53L1X**. What this test adds is the coexistence question the siblings could not touch: both sensors plus the button, on one bus, through one harness.

**Bench verdict (this session): the node runs as one.** The bus scan showed `0x29` and `0x68` with no stray addresses, `WHO_AM_I` answered `0x68`, the ToF's `0xEACC` answered at `0x010F`, and a sixteen-second capture recorded all 168 rows with `imu` `ok`, zero ToF timeouts, every range valid (33–53 mm against a hand at the lens), a confident gravity vector whose magnitude is ~0.97 g, and the button column live with one press counted.

**Repeat runs (two further captures after rewiring):** both identities and the bus scan repeated (`0x29`, `0x68`), every row again carried `imu` `ok` with zero ToF timeouts, and the range tracked targets from 4 mm out to 1580 mm. Those runs exposed this part's zero-g offset (quantified under [Known limitations](#known-limitations)) and left the button question open — `btn` read 1 on every row of every capture so far, which is either a held button or two legs on one side of the switch.

Power comes from the laptop's USB into the board's own port — no separate supply. The cu link is the same Wi-Fi UDP path [`imu_test/`](../imu_test/) established: rows are broadcast on port 5005 only while the button is held, and the USB stream continues either way. A wired link exists in the contract for latency; this wireless path is the session's development link, not the finished one.

- **The [parent's firmware contract](../AGENTS.md#firmware-contract) is now largely honoured by construction** — see [the conformance map](#how-this-relates-to-the-parent-contract) below. What still separates this sketch from the real firmware is stated there and in [Known limitations](#known-limitations), not discovered by whoever reads next.
- **Its output is a development observation, not evidence.** In the terms of [§4.1](../../AGENTS.md#41-evidence-classes), numbers off this breadboard are development activity rather than `[SIM]`, `[BUD]`, `[PROP]`, or `[CITE]` claims, and promoting anything into the thesis requires the §4 fallback order on its own terms.

## Parts

| Part | What it is | Note |
|---|---|---|
| ESP32 NodeMCU-32S | 38-pin breadboard MCU, CP2102 USB-UART | Laptoppowers it; the port also carries the serial link ([the parent's pins table](../AGENTS.md#esp32-header-pins) for anything new) |
| CJMCU-531 | **VL53L1X** ToF breakout (sold mislabeled "VL53L0X") | Identity `0xEACC` at 16-bit index `0x010F`; see [`tof_test_l1x/AGENTS.md`](../tof_test_l1x/AGENTS.md) for the full mislabel story |
| GY-521 | MPU6050 IMU breakout | `WHO_AM_I` expected `0x68` with `AD0` grounded |
| Tactile push button, 4-leg | Laser trigger stand-in | `INPUT_PULLUP`; idle HIGH, pressed LOW |
| 2 × 4.7 kΩ | I²C pull-ups, external | The GY-521 carries its own onboard pair in parallel |
| 100 nF ceramic | ToF module decoupling | Across the CJMCU-531's VCC/GND, non-polarized |
| 100 µF, 50 V electrolytic | *optional* GY-521 decoupling | The earlier phase's substitute cap; a bulk cap, acceptable — the run is also healthy without it |

## Connections

The [parent's connection list](../AGENTS.md#connection-list) describes exactly this build plus its completed-build details; **the wiring here is that list, with the CJMCU-531's part named correctly and the session's scope exclusions applied (nothing to the cu, no laser)**. The two header rows are numbered differently — `r1` runs 1–19 top, `r2` runs 60–42 bottom across the same columns — so the GPIO number is the authority and the position only tells you where to look.

| From                                          | To                                          | Function                                               |
| --------------------------------------------- | ------------------------------------------- | ------------------------------------------------------ |
| `r1 #19` — `3V3`                              | CJMCU-531 `VCC`, GY-521 `VCC`               | 3.3 V power from the ESP32's regulated rail            |
| `r2 #42` — `GND`                              | CJMCU-531 `GND`, GY-521 `GND`, button leg B | Common ground (r1 #2 / r1 #6 / r2 #48 are equivalent)  |
| `r2 #47` — **GPIO21**                         | CJMCU-531 `SDA`, GY-521 `SDA`               | I²C data — shared by both sensors                      |
| `r2 #44` — **GPIO22**                         | CJMCU-531 `SCL`, GY-521 `SCL`               | I²C clock — shared by both sensors                     |
| `r1 #9` — **GPIO27**                          | CJMCU-531 `XSHUT`                           | ToF enable, actively driven — **not left floating**    |
| `r2 #50` — **GPIO18**                         | button leg A                                | Input, internal pull-up; idle HIGH, pressed LOW        |
| GY-521 `AD0`                                  | the **−** rail                              | Ties the IMU to `0x68`; **floating puts it at `0x69`** |
| CJMCU-531 `GPIO1`, GY-521 `XDA`, `XCL`, `INT` | *no connection*                             | Interrupt/auxiliary pins, unused                       |
| `4.7 kΩ` #1                                   | between the SDA net and the **+** rail      | SDA pull-up                                            |
| `4.7 kΩ` #2                                   | between the SCL net and the **+** rail      | SCL pull-up                                            |
| 100 nF ceramic                                | across CJMCU-531 `VCC` and `GND`            | Decoupling at the module (canonical passive)           |
| 100 µF electrolytic *(optional)*              | across GY-521 `VCC` and `GND`               | Decoupling at the module (+ leg to VCC, stripe to GND) |

**Do not use the 5 V input at `r1 #1` as a ground.**

### The checks that matter here

**XSHUT is driven, never floated.** Per [the parent's XSHUT rule](../AGENTS.md#xshut-must-be-driven-not-floated): held LOW for 10 ms, released HIGH, settles 100 ms before any traffic, and driven HIGH for the whole run. This is doubly important now: with *two* devices on the shared bus, a ToF left floating is what makes the whole bus unreadable, and the IMU would be the first casualty.

**The GY-521's `AD0` must sit on the ground rail.** Nothing probes `0x69`; a floating AD0 answers there and every IMU-shaped attempt at `0x68` NAKs. The scan output exposes it: `0x69` on the list instead of `0x68` is exactly this wiring error.

**The button's two chosen legs must be on opposite sides of the switch contact.** On a four-leg tactile switch the two legs on one side are internally joined and permanently shorted together, so a button wired across a single side holds the input closed — the stream shows `btn` stuck at 1 with nothing pressed. One leg from each side is what makes a press a press; a leg on the wrong row instead leaves only the internal pull-up and `btn` stuck at 0.

**Both pull-ups, or rather six counting the GY-521's own.** The two externals stay fitted; the GY-521's onboard pair adds in parallel for roughly 2.3 kΩ per line, comfortably within spec at 400 kHz over breadboard runs — the parent's I²C section states this as the allowed case, and the dual-ended sharing is the electrical picture this test exists to confirm.

**Supply is unchecked territory only outside the already-proven modules.** The CJMCU-531 regulates its own supply (proven across three sessions); the GY-521 was proven by [`imu_test/`](../imu_test/); the laptop's USB feeds the NodeMCU-32S's regulator, whose headroom the parent's power-budget note already covers. Nothing new to confirm — power up as wire-by-wire, not all at once.

**Both header copies must agree.** The same wiring appears in the comment at the top of `node_test.ino`; if one is edited, edit the other. The pin numbers here are the authoritative copy.

## What the sketch does

`node_test.ino` drives everything from raw registers except the VL53L1X, whose long init sequence lives in **Pololu's VL53L1X 1.3.1** driver — the parent contract's one library exception, carried over from [`tof_test_l1x/`](../tof_test_l1x/).

**MPU6050 sequence per the parent contract:** a `WHO_AM_I` probe at `0x75` (expected `0x68`, printed whatever it is — known quiet revisions answer differently, and the record keeps the actual byte), then four acknowledged configuration writes — `PWR_MGMT_1 = 0x01` (wake, clock from gyro PLL), `CONFIG = 0x03` (DLPF ≈ 44 Hz), `GYRO_CONFIG = 0x00` (±250 deg/s), `ACCEL_CONFIG = 0x00` (±2 g) — then one 14-byte big-endian burst from `0x3B` per loop, straight into the columns: accelerometer in **g**, gyroscope in **deg/s**, die temperature in **degC**.

**The button** sits on GPIO 18, `INPUT_PULLUP`: the emitted column is `1` while pressed and `0` otherwise; the five-second probe counts press edges (`LOW`-going transitions) since boot. No debounce — the column is the wiring's proof, and the real firmware makes its own filtering decisions. While it is held, each combined row is also broadcast to the compute unit; releasing stops the broadcast and the USB record carries on, announcing both edges on the serial log.

**The link** mirrors [`imu_test/`](../imu_test/): credentials live in an untracked `wifi_secrets.h`, the station joins at boot, and datagrams go to the subnet's directed broadcast address on port 5005 unless `LINK_UNICAST_HOST` overrides it. `linkSend()` is fire-and-forget and runs only after the row has been printed, so a broken link costs the compute unit rows, never the local record; the five-second probe reports `link: up`/`down`, the destination, and sent/failed counts. Without the secrets file the sketch builds and streams USB-only.

**The ToF** is exactly [`tof_test_l1x`'s](../tof_test_l1x/AGENTS.md) configuration: XSHUT reset, long distance mode, 50 ms budget, 500 ms read timeout, the raw identity probe gating on `0xEACC`.

Each loop reads the IMU burst, the ToF one measurement, and the button level, then emits one combined row:

```
t_ms    btn   ax_g   ay_g   az_g   gx_dps  gy_dps  gz_dps  temp_C imu   range_mm status sig_MCPS amb_MCPS tof
```

- `t_ms` — milliseconds since boot.
- `btn` — 1 while pressed, 0 otherwise.
- `ax_g` / `ay_g` / `az_g` — acceleration in g, +/−2 g full scale.
- `gx_dps` / `gy_dps` / `gz_dps` — angular rate in deg/s, ±250 deg/s full scale.
- `temp_C` — the MPU6050's die temperature in degC.
- `imu` — the burst's outcome: `ok`, `short` (partial reply), or `nack` (unanswered address).
- `range_mm` — ToF distance along the beam, millimetres.
- `status` — the ToF measurement's verdict as the `RangeStatus` enum number (legend printed at boot; `7` `WrapTargetFail` remains normal long-mode near-field behaviour).
- `sig_MCPS` / `amb_MCPS` — the ToF's peak-signal and ambient rates in million counts per second.
- `tof` — the ToF read's outcome: `ok` or `timeout` after 500 ms without a fresh measurement.

**No-signal-is-signal rule, per the parent contract:** each sensor's fallout is confined to its own columns — a `tof` timeout emits `nan` for the range columns and leaves the IMU columns carrying that row's true burst; an IMU `short`/`nack` emits `nan` for the IMU columns and leaves the range columns fresh — and neither failure ever masquerades as a plausible zero.

**Every five seconds a probe** reprints both identities back, the ToF config, and the stats — rows/tof-ok/timeout, valid range min/max, imu ok/short/nack, button press counts — and then the column header, so a monitor attaching after boot orients immediately (the parent's periodic-probe rule). `setup()` retries the whole attempt every second so refitting a wire is picked up without reflashing.

Cadence: the ToF's blocking read still paces the loop at the two-perioded ~94 ms stride the [`tof_test_l1x` captures](../tof_test_l1x/AGENTS.md) measured, so every column lands at about 10.6 Hz and the totals in the probe agree.

### Reading the log when it fails

- **`nothing ACKs on the bus`** — the shared bus basics: XSHUT, SDA/SCL, 3V3, common ground, or the external pull-ups. The scan that precedes it prints whatever does answer.
- **`expecting ... and an extra 0x69 would mean the AD0 is floating`** — the IMU's address select wandered onto the wrong rail.
- **`0x010F did not read 0xEACC`** — ToF absent or wrong silicon; the bytes printed are what to quote to the vendor.
- **`driver init() was rejected` / `ToF's 010F...`** — the part stopped ACKing mid-sequence; refit the ToF jumpers.
- **`nothing answered WHO_AM_I at 0x68`** — the IMU never answered; the scan above names whether it sits somewhere else (0x69 = floating AD0) or nowhere (VCC/GND/SDA/SCL).
- **`WHO_AM_I: 0xXX -- NOT 0x68`** — brought up anyway; a known quiet-revision/clone pattern. The byte belongs in the record, either for the vendor quote-back or for a later firmware decision.
- **`MPU6050 config failed at ...`** / an `imu` column that stays `nack` — the IMU stopped ACKing after the probe; refit its jumpers.
- **A `btn` column stuck at 1** — either the button is being held, or its two legs sit on the same side of the switch (internally joined, permanently closing GPIO 18 to ground); nothing else pulls that input low. **A `btn` column stuck at 0** — a leg missed its wire or landed on the wrong row, leaving only the internal pull-up.
- **A `tof` column that stays `timeout`** — the ToF stopped producing measurements; refit XSHUT first, then the bus.
- **`wifi: no wifi_secrets.h found`** — the sketch is running USB-only; copy the untracked header from [`imu_test/`](../imu_test/) into this directory and reflash.
- **`wifi: FAILED to join`** — the module is 2.4 GHz only, so a 5 GHz SSID is invisible to it; the scan printed with the failure names every network it can see. A refused join is the password.
- **`link: up` but `sent` stays 0 while the button is held** — the button is not actually reading pressed (`btn` stays 0), or the access point filters client-to-client broadcast (AP isolation); set `LINK_UNICAST_HOST` to the compute unit's address as the escape hatch, and confirm the receiver is running on port 5005.
- **`link: udp.begin failed`** — the local UDP port is unavailable; reboot the board before suspecting anything else.

## How this relates to the parent contract

The point of this test is to *become* the sensor-harness half of the [firmware contract](../AGENTS.md#firmware-contract), so it is worth stating the ledger plainly rather than implying it:

| Contract says | This sketch | Verdict |
|---|---|---|
| Read the MPU6050 directly over `Wire` (WHO_AM_I, four config writes, 14-byte burst) | Exactly that sequence, no IMU library | **Honoured** |
| The VL53L0X driver is the one library exception | Pololu VL53L1X 1.3.1 for the part that is actually here | **Honoured in spirit**; see the open parts-table note below |
| Emit CSV with units named in the headers | Tabular fixed-width tab-separated columns, units in the [column names](#what-the-sketch-does) — a deliberate departure recorded for monitor readability | **Divergence, recorded** |
| Never emit a plausible-looking zero for a failed read | Per-sensor `nan` + an explicit per-sensor status column | **Honoured** |
| Drive `XSHUT` through the startup sequence, gate all ToF traffic behind it | The 10 ms low / ≥100 ms settle / driven-high pattern on every attempt | **Honoured** |
| Report presence at boot so a miswired sensor is named outright | Bus scan + both identity probes + targeted failure text | **Honoured** |
| Acceleration in g, angular rate in deg/s, temperature in degC, range in mm | The four columns, those units | **Honoured** |
| Data onward to the compute unit | While the button is held, each row is broadcast over Wi-Fi UDP to port 5005, mirroring `imu_test`; released, sending stops | **Honoured (button-gated)**; the wireless path is the session's development link |

The open decision from [`tof_test_l1x/AGENTS.md`](../tof_test_l1x/AGENTS.md) carries forward: the parent's parts table still pins a VL53L0X; every sketch in this directory documents the bench, not the table.

## Known limitations

Not defects — the distance between a bring-up sketch and a firmware, recorded so the sketch is not mistaken for more than it is.

- **One question answered.** Coexistence on one bus, both identities, the button, and a combined stream. Calibration, drift, sampling behaviour under motion, fusion, tiering, and anything beyond single-zone ranging belong to the real firmware.
- **Cadence is inherited, not tuned.** The ~94 ms two-period ToF read stride paces everything, so the IMU lands at ~10.6 Hz too. Plenty for a bring-up stream; the read/period pairing stays a design item for the real firmware.
- **The button is raw.** Level-sampled once per loop (~94 ms) with no debounce: fast taps shorter than a loop may vanish, and bouncing shows as extra press counts. The press count is a wiring proof, not a feature.
- **Full-scale gains are the bring-up defaults.** ±2 g and ±250 deg/s clip in real motion — deliberate, to keep LSB noise readable on the bench; the real firmware picks gains per its motion budget.
- **The raw stream carries the part's zero-g offset.** A constant offset fitted across the three captured orientations comes out at about `+0.35 g` on the chip's Z axis and near zero on X/Y — this GY-521's raw accelerometer therefore reads a gravity magnitude anywhere from 0.68 g to 0.97 g depending on tilt, and about 1.35 g flat with Z up. That is well outside the MPU6050's datasheet zero-g offset, so the breakout's die (or a clone) is the suspect rather than the wiring. [`imu_test`](../imu_test/) measures and subtracts per-axis offsets at startup for exactly this reason; `node_test` deliberately streams raw registers, so no calibration is applied here.
- **`WHO_AM_I` mismatches are tolerated by design and loudly recorded.** A resumed clone that answers, say, `0x98` streams garbage in the IMU columns rather than nothing; the bytes in the log are the arbiter.
- **The raw-`cat` capture flake class carries over.** This bench's flaky-serial capture path has shown three manifestations (whole-row duplicate bursts, fragment floods, mid-row fragment repetitions) with the chip provably not the source; for viewing use `arduino-cli monitor`, and archival captures pass integrity checks (`t_ms` monotonicity, row shape) before being cited. The chip's own periodic status probes are the trustworthiest facts per capture.
- **The stream is a development stream, not the §4.2 event stream.** No sequence numbers, no paired timestamps, no per-update active-source record; nothing here is intended to become telemetry evidence.
- **The UDP link is fire-and-forget.** No sequence numbers, retransmission, or acknowledgement: a datagram that does not arrive is not retried, because on this link a late sample is worse than a missing one. Wi-Fi is only used while the button is held, and the local print happens first, so the link adds no dependency to the sensor loop or the USB record. The compute unit must be running its receiver on port 5005, and client-to-client broadcast must survive the access point (see the failure list for the unicast escape hatch).
- **The single library remains the contract's exception.** `VL53L1X.h` only; nothing else beyond `Wire.h`.

## Build, flash, and monitor

arduino-cli with the `esp32:esp32` core and the Pololu VL53L1X 1.3.1 library pinned in `sketch.yaml`. **This sketch's profile is `node-test`.** The parent's `--profile pointer-sensor-node` does not exist in this repository; copying it in verbatim will fail. From this directory:

```bash
arduino-cli compile --profile node-test
arduino-cli upload  --profile node-test --port /dev/ttyUSB0
arduino-cli monitor --port /dev/ttyUSB0 --config baudrate=115200
```

The library is pinned by name and version in the profile (`- VL53L1X (1.3.1)`), so a machine that has never installed it needs `arduino-cli lib install VL53L1X` once. The Wi-Fi link needs `wifi_secrets.h` in this directory — copy it from [`imu_test/`](../imu_test/); it is ignored by `pointer/firmware/.gitignore` and must never be tracked. Without it the sketch compiles and streams USB-only, saying so at boot. Everything else in the parent's [build section](../AGENTS.md#build-flash-monitor) carries over: the `uucp` group membership on Arch (`dialout` elsewhere), `--port` explicit on every command, and the periodic-probe rule for monitors that attach after boot.

**Expect a working test to look like this:** the bring-up burst ending in `button: GPIO18 INPUT_PULLUP, idle reads 1`, the Wi-Fi join and `link: up` lines, the legend, the header, then rows at about 10 per second with the IMU columns centred near (0, 0, +1) g and gyros near zero when flat; a wall across the room reads its true distance with `status` `0`; waving a hand moves `range_mm` down through a `min` in the low hundreds; holding the button prints `button DOWN - streaming to the compute unit`, shows `btn` at `1`, climbs the stats' `presses=`, and climbs `link ... sent=` while the compute unit receives the rows — releasing prints `button UP` and stops the send; tilting the board moves the `gx_dps`/`gy_dps`/`gz_dps` columns until the board sits still again. A still-flat board's `az_g` hovers around +1.0 with 0.02–0.05 g of jitter. Anything else reads against [the failure list](#reading-the-log-when-it-fails) above.
