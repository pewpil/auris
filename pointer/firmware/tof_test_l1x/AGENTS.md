# ToF bring-up test — VL53L1X — AGENTS.md

Standing instructions for `pointer/firmware/tof_test_l1x/`. Read with [`../../AGENTS.md`](../../AGENTS.md), which carries the repository-wide sections, and with [`../AGENTS.md`](../AGENTS.md), which carries the wiring specification and firmware contract for the pointer as a whole.

> **Scope:** the repository-wide sections apply here unchanged — [§2](../../AGENTS.md) on documenting every function, [§3](../../AGENTS.md) on never committing, [§4](../../AGENTS.md) on evidence classes, [§5](../../AGENTS.md) on unwrapped prose, and [§6](../../AGENTS.md) on side notes. Nothing here overrides the parent [pointer/AGENTS.md](../AGENTS.md); where this directory departs from it, that is recorded below rather than left to be discovered.

## Status: a bring-up sketch, and the test of the part actually on the bench

**This is a bring-up sketch, not the pointer's firmware.** It exists to answer one question — does the ESP32 drive the VL53L1X correctly over I²C, from hardware reset through a continuous long-mode range stream — and nothing more. Do not treat it as an implementation of [the firmware contract](../AGENTS.md#firmware-contract), and do not grow it into one. The real firmware has not been written.

**Bench verdict (this session): the part ranges.** Identity confirmed repeatedly by the sketch's own log (`EA CC 10 FF` at 16-bit index `0x010F`, model `0xEACC` = VL53L1X), driver `init()` and long mode accepted (49904 µs budget), and the continuous stream ran for four-plus minutes with the stats recording around 2600 reads and **zero timeouts**, valid ranges tracking the environment from 159 mm out to 433 mm as targets moved in front of the lens.

An earlier session brought this module up as a supposed VL53L0X and, in doing so, discovered what it really is: identity reads returned `EA CC 10 FF` at two-byte register index `0x010F`, with every read advancing the part's internal pointer — the behaviour of a **VL53L1X** on a CJMCU-531 board, which is how it was bought and how several vendor listings still sell it, mislabeled as a "VL53L0X". That VL53L0X test was removed once the mislabel was found — the bench holds no VL53L0X, so it had nothing left to test — and the record of what it found lives on in this directory. This sketch drives the part as what it is, with the correct driver, because the VL53L0X driver cannot speak this part's register protocol. The test's scope is unchanged — **no button, nothing to the compute unit, USB serial only** — and those exclusions are the test's definition, not accidents to be "fixed".

- **The pointer's contract is unmet by design.** The contract requires both sensors on one bus, laser-button input, and data onward. This sketch does none of those.
- **Its output is a development observation, not evidence.** In the terms of [§4.1](../../AGENTS.md#41-evidence-classes), ranges off this breadboard are development activity rather than `[SIM]`, `[BUD]`, `[PROP]`, or `[CITE]` claims, and promoting anything into the thesis requires the §4 fallback order on its own terms.

## The part: CJMCU-531, whose silicon is a VL53L1X

- **Vendors mislabel it.** The module was bought as a VL53L0X; several listings sell this same board labelled that way. When checking a seller's page against this bench, the label "VL53L0X" on a CJMCU-531 is not evidence of silicon.
- **Identification is unambiguous.** The VL53L1X answers at the same fixed `0x29` the VL53L0X uses, with **two-byte register indexing** and a model id of **`0xEACC` at 16-bit index `0x010F`**. This session's read returned `EA CC 10 FF` — only the first pair gates; the trailing `10 FF` is printed but its meaning was never pinned down.
- **The one-byte-index VL53L0X protocol produces pointer-driven garbage on this part**: its register indexing is two bytes wide and it advances an internal pointer on every access, so VL53L0X-shaped reads come back sequence-shifted. Every "identity read" mismatch the earlier session chased was the part answering honestly, in its own language.

## Connections

This is the wiring for this test. The [parent's connection list](../AGENTS.md#connection-list) describes the finished build including the MPU6050 and the button; **this sketch is ToF-only**. The two header rows are numbered differently — `r1` runs 1–19 top, `r2` runs 60–42 bottom across the same columns — so the GPIO number is the authority and the position only tells you where to look.

| From                  | To                                     | Function                                                            |
| --------------------- | -------------------------------------- | -------------------------------------------------------------------- |
| `r1 #19` — `3V3`      | VL53L1X `VCC`                          | 3.3 V power from the ESP32's regulated rail                           |
| `r2 #42` — `GND`      | VL53L1X `GND`                          | Common ground                                                         |
| `r2 #47` — **GPIO21** | VL53L1X `SDA`                          | I²C data                                                              |
| `r2 #44` — **GPIO22** | VL53L1X `SCL`                          | I²C clock                                                             |
| `r1 #9` — **GPIO27**  | VL53L1X `XSHUT`                        | ToF enable, actively driven — **not left floating**                   |
| VL53L1X `GPIO1`       | *no connection*                        | Interrupt output, unused                                              |
| MPU6050               | **not connected at all**               | No IMU in this test                                                   |
| button                | **not connected**                      | Not part of this test; the sketch never touches GPIO 18              |
| `4.7 kΩ` #1           | between the SDA net and the **+** rail | SDA pull-up                                                           |
| `4.7 kΩ` #2           | between the SCL net and the **+** rail | SCL pull-up                                                           |
| 100 nF ceramic                    | across `VL53L1X.VCC` and `VL53L1X.GND` | Decoupling at the module — the canonical part, now fitted |

**Ground:** `r2 #42`, or `r1 #2` / `r1 #6` / `r2 #48` — any of them is the same net. Do not use the 5 V input at `r1 #1` as a ground.

These are the same five wires (VCC, GND, SDA, SCL, XSHUT) this module was already brought up on across the earlier session — probed, cycled, and scanned through nine bring-up attempts on exactly this wiring — so a rebuilt harness should be rebuilt to match.

### The checks that matter here

**XSHUT is driven, never floated.** Per [the parent's XSHUT rule](../AGENTS.md#xshut-must-be-driven-not-floated): the wire is `r1 #9` (GPIO27), the firmware holds it LOW for 10 ms to force hardware standby, releases it HIGH, waits **100 ms** before any traffic, and keeps it actively driven HIGH for the whole run.

**Supply: this module regulates itself; a different unit may not.** The parent's "[VL53L0X.VREF must be 2.8 V](../AGENTS.md#verify-before-first-power-up)" check exists because cheap L0X clones can tie a 2.8 V part's supply straight to `VCC`. The same class of hazard applies to any 2.8 V-limbed part. This specific CJMCU-531 carries its own on-board regulator, and the evidence is this bench's own record: it ACKed steadily from the 3V3 rail through the whole earlier session. For *this* module the check is already satisfied; if a replacement module ever sits here, confirm its supply arrangement before power-up rather than assuming this one's.

**Both 4.7 kΩ pull-ups.** No GY-521 is present to carry them, and the module supplies none of its own; the pair, the shared bus, and the fixed `0x29` address complete the electrical picture of [the parent's I²C section](../AGENTS.md#i2c-bus), which runs the bus at 400 kHz.

**The decoupling is the canonical 100 nF ceramic, fitted across the module's `VCC` and `GND` rows.** Ceramic is non-polarized — either orientation works — and its low ESR is exactly what the parent spec'd this passive for. (For one session the bench ran on a 100 µF / 50 V aluminum electrolytic substituted for lack of the ceramic on hand: a valid bulk-cap substitute, but polarized — + leg to `VCC`, stripe to `GND` — and noisier at high frequency. The captures from that period stand as recorded.)

**Both header copies must agree.** The same wiring appears in the comment at the top of `tof_test_l1x.ino`; if one is edited, edit the other. The pin numbers here are the authoritative copy.

## What the sketch does

`tof_test_l1x.ino` is an Arduino sketch whose only third-party dependency is **Pololu's VL53L1X 1.3.1**. Its `init()` gates on the same `0xEACC` the raw probe below checks, and the part's long init sequence (patch-time behaviour, distance-mode setup) lives in that driver — the same reasoning the parent's contract applies to the VL53L0X's driver, applied to the part that is actually here.

Bring-up runs as: hardware reset through XSHUT (LOW 10 ms → released HIGH → 100 ms settle), then an I²C scan printing every ACKing address, then a raw identity read at 16-bit index `0x010F` **using the part's own two-byte indexing** (gating on the first two bytes being `EA CC`), then driver `init()`, then configuration: **long distance mode** (the part's farthest band, out to 4 m), a **50 ms measurement budget** (long mode's documented minimum is 33 ms; 50 ms leaves margin), and a 500 ms read timeout. `setup()` retries the whole attempt **every second** so refitting a wire is picked up without reflashing.

Once up, it streams continuous-mode ranges — nominal **20 Hz** at a 50 ms inter-measurement period, observed on the bench at **~10.6 Hz** (rows every ~94 ms; see [Known limitations](#known-limitations)) — to USB serial at 115200 baud. **The stream is tabular, a deliberate departure from [the parent contract's](../AGENTS.md#firmware-contract) CSV rule for the real firmware**: every field is padded to a fixed width and closed with a tab, so the monitor lines the stream up as a table and a field that overflows its width still lands on the next tab stop. The column names, as printed in the header:

```
t_ms    range_mm status  sig_MCPS amb_MCPS read
```

- `t_ms` — milliseconds since boot (the board's own monotonic counter).
- `range_mm` — distance along the beam in millimetres; long mode reads from roughly 40 mm out to 4000 mm, target-dependent.
- `status` — the measurement's own verdict as the driver's enum: `0` is `RangeValid`; `1` SigmaFail, `2` SignalFail, `3` RangeValidMinRangeClipped, `4` OutOfBoundsFail, `5` HardwareFail, `6` RangeValidNoWrapCheckFail, `7` WrapTargetFail, `9` XtalkSignalFail, `10` SynchronizationInt, `13` MinRangeFail, `255` none. `setup()` prints the legend at boot, and a nonzero verdict emits `nan` for the range and the count rates — the part's verdict is data, not silence.
- `sig_MCPS` and `amb_MCPS` — peak signal and ambient count rates in million counts per second; together they say whether the target is even visible, and distinguish a dark target from a bright one.
- `read` — `ok` when a new measurement arrived, `timeout` after 500 ms without one. A timeout row emits `nan`, never a plausible zero, per the parent contract's rule.

Every five seconds a probe reprints the raw identity bytes, the active configuration, and running stats — `ok`/`timeout` counts, the min/max valid range seen, and the last sample's range status by name — and then the column header, because a monitor attaching after boot misses the boot banner, and attaching afterwards is the normal case (the parent's rule).

### Reading the log when it fails

Each failure names its check:

- **`nothing ACKs on the bus`** — XSHUT wire, SDA/SCL wires, 3V3, common ground, or the two 4.7 kΩ. The scan that precedes it prints whatever does answer; `0x68` on that list means the GY-521's harness is still attached somewhere.
- **`0x010F did not read 0xEACC`** — the bytes printed are what to quote back to the vendor; several listings sell this exact board mislabelled as a VL53L0X.
- **`driver init() was rejected`** — the part stopped ACKing mid-sequence; refit the jumpers, and the one-second retry re-drives XSHUT from scratch.

## How this differs from the pointer's specified wiring

| Parent specifies | This sketch | Why |
|---|---|---|
| ToF is a **VL53L0X** breakout ([the parent's parts table](../AGENTS.md#components)) | The bench part is a **VL53L1X** (CJMCU-531, sold mislabelled) | A recorded, unresolved discrepancy: the parent's table still pins the L0X, and changing it is a decision at the parent's level, not here |
| Firmware contract sanctions the Pololu **VL53L0X** driver as the one library exception | Uses the Pololu **VL53L1X** 1.3.1 driver | The same long-init reasoning applies to the L1X; the contract line stays unedited until the part is pinned |
| MPU6050 shared on the I²C bus at `0x68` | **Absent** — the sketch never addresses it | ToF-only bring-up |
| Button on `r2 #50` (GPIO18) | **Absent** — GPIO 18 untouched | Session scope: no button |
| Data onward to the compute unit | **None** — USB serial only | Session scope: nothing to the cu |
| Both 4.7 kΩ pull-ups fitted externally | **Both fitted** | No GY-521 to supply them |

The bus-traffic conclusion stands: this sketch proves the ToF half of the bus and still nothing about the IMU; only the finished build's shared-bus wiring is evidence for two sensors at once.

## Known limitations

- **One question answered.** Identity, init, and continuous ranging. Accuracy, calibration drift, sampling under motion, and coexistence with the IMU are out of scope and belong to the real firmware.
- **The gates are two, and neither is deeper than the driver's own.** The raw probe and `init()` both check the model id `0xEACC` and stop there. A variant part differing only in the trailing bytes would pass — the full four bytes are printed for exactly that reason.
- **Long mode's band is target-dependent.** Beyond 4 m (`OutOfBoundsFail`) or closer than ~40 mm, the verdict arrives as a nonzero `range_status`, never as a silenced column.
- **The observed cadence is ~10.6 Hz (rows every ~94 ms), not the nominal 20 Hz.** Each blocking read consumes exactly one frame per two measurement periods, a behaviour consistent with phase-locking to the driver's data-ready window at every other frame; the root cause was not chased, and the rate is plentiful for a bring-up stream. The real firmware should treat the read/period pairing as a design item rather than inherit this sketch's cadence.
- **`WrapTargetFail` (status 7) is normal long-mode near-field behaviour.** It appeared at ~20% of frames against a near target in one capture and ~64% in another — the part intermittently reports a wrapped phase on close targets in long mode, interleaved with `RangeValid` frames, and never times out. The status column names it instead of hiding it, which is the parent contract working as written; the mode choice itself is a real-firmware decision.
- **Raw `cat` captures of this stream corrupt in capture-start bursts, and the shape varies.** Three raw captures on record each opened with tens of thousands of byte-identical host-side repetitions — whole rows in the first two (`t=1003`, then `t=247523`), and a 12-byte row fragment (`"\t2.53     ok"`, 1034 copies ≈ 55% of a 22 KB capture) in the third — always at wire rate, with the chip's own timeline unbroken and the chip provably not the source: sending 2413 copies at 115200 baud requires ~6 s of TX-blocking, which would have advanced the stamped `millis()`. This is the flaky-serial-capture class of this bench, now in its third recorded manifestation. For viewing use `arduino-cli monitor`; any archival capture must be integrity-checked (`t_ms` monotonicity, row shape) before it is cited as evidence, and the trustworthiest per-capture facts are the chip's own five-second status probes.
- **The firmware contract mismatch is open.** Until the parent's parts table and contract name the L1X, treat the contract's library exception as L0X-shaped; this directory documents the bench, not the contract.
- **The stream is a development stream, not the §4.2 event stream.** No sequence numbers, no paired timestamps; nothing here is intended to become telemetry evidence.

## Build, flash, and monitor

arduino-cli with the `esp32:esp32` core and the Pololu VL53L1X 1.3.1 library pinned in `sketch.yaml`. **This sketch's profile is `tof-test-l1x`.** The parent's build section documents `--profile pointer-sensor-node`, and that profile does not exist in this repository; copying it into this directory will fail. From this directory:

```bash
arduino-cli compile --profile tof-test-l1x
arduino-cli upload  --profile tof-test-l1x --port /dev/ttyUSB0
arduino-cli monitor --port /dev/ttyUSB0 --config baudrate=115200
```

The library is pinned by name and version in the profile (`- VL53L1X (1.3.1)`), so a machine that has never installed it only needs `arduino-cli lib install VL53L1X` once before the first compile. Everything else in the parent's [build section](../AGENTS.md#build-flash-monitor) carries over unchanged: the `uucp` group membership on Arch (`dialout` elsewhere), passing `--port` explicitly rather than pinning a `default_port`, and the periodic-probe rule for monitors that attach after boot.

**Expect a working test to look like this:** a bring-up burst ending in `range config: long mode, 50000 us budget (accepted), 500 ms read timeout`, then the status legend, the tab-aligned column headers, and rows at about 10 per second (the observed bench cadence) — a wall in front of the sensor reads its true distance with `status` `0`, moving a target moves `range_mm` smoothly, and the five-second stats line shows `timeout=` holding still. Both capture sessions on record look exactly like this apart from the duplication bursts at capture start, which the byte analysis disowns from the chip ([Known limitations](#known-limitations)). Anything else reads against [the failure list](#reading-the-log-when-it-fails) above.
