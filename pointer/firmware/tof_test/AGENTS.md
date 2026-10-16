# ToF bring-up test — AGENTS.mda

Standing instructions for `pointer/firmware/tof_test/`. Read with [`../../AGENTS.md`](../../AGENTS.md), which carries the wiring specification and firmware contract for the pointer as a whole, and with the sibling [`../imu_test/AGENTS.md`](../imu_test/AGENTS.md), which this sketch deliberately mirrors.

> **Scope:** the repository-wide sections apply here unchanged — [§2](../../AGENTS.md) on documenting every function, [§3](../../AGENTS.md) on never committing, [§4](../../AGENTS.md) on evidence classes, [§5](../../AGENTS.md) on unwrapped prose, and [§6](../../AGENTS.md) on side notes. Nothing here overrides the parent [pointer/AGENTS.md](../AGENTS.md); where this directory departs from it, that is recorded below rather than left to be discovered.

## Status: flashed, bus healthy, the sensor module itself is at fault

**This is a bring-up sketch, not the pointer's firmware.** It exists to answer one question — does the VL53L0X come up, answer on the shared I²C bus, and report sane ranges when `XSHUT` is actively driven rather than left floating. It compiles cleanly, has been flashed, and is **running**. The wiring fault that blocked it was found and corrected, and the bus is now verifiably healthy. What remains cannot be fixed in software: **the module acknowledges its address, obeys `XSHUT`, and then returns garbage for every register read.** See [where it stands](#where-it-stands-the-module-answers-its-address-but-returns-no-data).

Two things follow from that.

- **The pointer's contract is unmet by design.** The contract in [the parent](../AGENTS.md#firmware-contract) requires ToF range, `XSHUT` sequencing, and both sensors on one bus. This sketch does the first two and deliberately abandons the third, so satisfying it here is not progress toward the contract — it is a detour around it.
- **Its output is a development observation, not evidence.** Readings taken off this breadboard are, in the terms of [§4.1](../../AGENTS.md#41-evidence-classes), development activity rather than a `[SIM]`, `[BUD]`, `[PROP]`, or `[CITE]` claim. Promoting anything from this stream into the thesis still requires the §4 fallback order to be satisfied on its own terms.

## Connections

This is what goes on the breadboard. The [parent's connection list](../AGENTS.md#connection-list) describes the finished build with both sensors; **this sketch is ToF-only**, so the parent list does not describe the present wiring and reading it as though it does will mislead anyone checking continuity.

The two header rows are numbered differently — `r1` is the top row numbered 1–19, `r2` is the bottom row numbered 60–42 across the same columns — so a column carries two unrelated numbers. The GPIO number is the authority; the position only tells you where to look.

| From | To | Function |
|---|---|---|
| `r1 #19` — `3V3` | VL53L0X `VCC` | 3.3 V power |
| `r2 #42` — `GND` | VL53L0X `GND` | Common ground |
| `r2 #47` — **GPIO21** | VL53L0X `SDA` | I²C data |
| `r2 #44` — **GPIO22** | VL53L0X `SCL` | I²C clock |
| `r1 #9` — **GPIO27** | VL53L0X `XSHUT` | **Actively driven — the whole point of this test** |
| `r2 #50` — **GPIO18** | button leg A | Button input, internal pull-up |
| `r2 #42` — `GND` | button leg B | Closes GPIO 18 to ground when held |
| `3V3` → `4.7 kΩ` → `SDA` | — | **SDA pull-up, mandatory** |
| `3V3` → `4.7 kΩ` → `SCL` | — | **SCL pull-up, mandatory** |
| VL53L0X `GPIO1` | *no connection* | Interrupt output, unused |
| MPU6050 | **not connected at all** | No IMU in this test — see below |

**Ground:** `r2 #42`, or `r1 #2` / `r1 #6` / `r2 #48` — any of them is the same net. Do not use the 5 V input at `r1 #1` as a ground.

**The two pull-ups are mandatory here, and unlike in the IMU test.** The GY-521 carries its own 4.7 kΩ on both lines; the VL53L0X breakout typically supplies **none**. With the GY-521 deliberately absent, nothing else is holding SDA and SCL high, so a bus fitted without these resistors is dead — and it fails silently, as a part that simply never acknowledges. The [parent](../AGENTS.md#i2c-bus) calls for both resistors in the finished build as well; this test is the case where skipping them is definitely wrong rather than merely untidy.

**`VREF` must sit at 2.8 V before the board is powered.** This is the parent's ["Verify before first power-up"](../AGENTS.md#verify-before-first-power-up) item 1, and it is the one check here that can destroy the module rather than merely waste an afternoon. Most VL53L0X breakouts feed `VREF` from an on-board regulator; cheap clones sometimes tie it straight to `VCC`, and powering such a module from the 3V3 rail overdrives it. **Measure it with a meter, USB cable disconnected, before connecting power.** The firmware cannot check this — it is a precondition on the wiring, not a behaviour of the code — and `TOF_IO_2V8` only selects the I/O voltage mode once the part is already powered and answering.

### Why the IMU is deliberately absent

The modules are being brought up one at a time, and this is the half that broke the other half last time. The previous prototype had both sensors on one bus with `XSHUT` floating, and the result was not that the ToF misbehaved — it was that **the IMU stopped producing valid samples at all**, reading back as dropped transfers or all-zero register dumps, and recovering completely only once the ToF was removed. That failure is inseparable from the ToF's presence, so it cannot be studied while the ToF is the thing under test.

Testing the ToF alone therefore says nothing about whether both sensors coexist. That combination is a separate test, and the one whose outcome the parent singles out as the most important correction in the build. See [the XSHUT section](#xshut-is-driven-not-floated) for what this sketch can and cannot settle.

## Where it stands: the module answers its address but returns no data

**The sketch is flashed and running, and the bus is now healthy. The wiring was faulted and has been corrected. What remains is not a wiring fault: every layer of the stack has been tested and the sensor is the thing that fails.**

The probe repeats all of it every five seconds, so none of it depends on catching the boot banner:

```
probe  dev(0x29)=ACK ctl(0x62)=NAK XSHUT=high model=0x00 rev=0x00 idle SDA=HIGH SCL=HIGH init=bad-model-id link=up raw=0 dist=0 status=4
idlebits SDA=FFFF SCL=FFFF (stable high)
cmp C0: mine=FF ws=0  driver=00 ws=0  rstart=00 ws=0
scan: 29
xshuttest: shutdown ACK=no  released ACK=yes  -> pin reaches the module
```

### What each test proves, and what it ruled out

Every layer was checked deliberately rather than assumed, because the obvious explanations kept turning out to be wrong.

| Test | Result | What it establishes |
|---|---|---|
| `ctl(0x62)` | `NAK` | **The bus is healthy.** An unused address correctly refusing to answer rules out a shorted or floating data line, which an earlier fault had been |
| `scan:` | `29` only | **The module is attached at the right address**, and nothing else is on the bus. No stray device is answering in its place |
| `idlebits` | `SDA=FFFF SCL=FFFF` | **Both pull-ups work.** Sixteen samples, all high — a line with no pull-up on it wanders |
| `xshuttest` | shutdown `no`, released `yes` | **The `XSHUT` wire reaches the module and its polarity is right.** Asserting it removes the part from the bus; releasing it brings it back |
| `cmp C0` | `mine=FF`, `driver=00` | **Two independent readers get different wrong answers from the same register.** The vendor driver never reads 0xEE, and the sketch never does either |
| `status=4` | unavailable | `init()` failed at its very first check, so no ranging was ever attempted and no range has ever been measured |

**The failure is confined to the data phase.** The part acknowledges its own address, obeys `XSHUT`, and keeps its lines pulled up correctly — and then returns a different byte every time a register is read. Across runs the same register `0xC0` has returned `0x00`, `0x01`, `0x03`, `0x08`, `0x09`, `0x0B`, `0x21`, `0x92` and `0xFF`. Nothing in a working identification register changes like that.

**Software was eliminated, not assumed.** The bus clock was dropped from 400 kHz to 100 kHz in case it was a rise-time problem; register reads were attempted both with a STOP and with a repeated START as ST specifies; and the sketch's own read was compared against the Pololu driver's read on the same register. All three read the same wrong thing, which means the bytes are genuinely arriving from the module and are genuinely wrong. **No code change will fix this.**

**The verdict is the module.** A `CJMCU-531` — pinout `VCC GND SCL SDA GPIO1 XSHUT`, no `VREF` pin — that powers, addresses, and shuts down correctly but cannot complete a register transfer points at the part itself: a failed internal 2.8 V rail, or damage from the earlier prototype session where `XSHUT` floated and this module was implicated in the IMU failure. **Swap it for a second module and re-run unchanged.** If the replacement reports `model=0xEE` and `init=OK`, that confirms the diagnosis and no firmware work was needed.

### Three diagnostic mistakes worth recording

Each one produced a confident answer that the hardware immediately refuted. They are recorded because the failure mode is the same every time: **a test that cannot fail is not a test.**

**A single idle-line reading was presented as proof the pull-ups worked.** It was not. Sampling the pin once, 200 µs after releasing it, cannot distinguish a line held high by a resistor from a line floating on parasitic capacitance. It read `HIGH` while the bus was genuinely broken. Replacing it with sixteen samples over several milliseconds is what made the result trustworthy — and it is also what showed the pull-ups *were* fine, killing a hypothesis that had felt very likely.

**A garbage register read was read as a discovery.** One capture returned `0xEE` at `0xC1`, the exact byte the driver wants at `0xC0`, one address away — a convincing explanation for the whole failure. Re-reading the same address four times gave `0x00 0x00 0x00 0x01`, and `0xC1` was `0x00` in every subsequent capture. **The byte was noise that happened to land on the expected value.** A near-miss at exactly the right value is the most seductive possible false positive, and the only defence is sampling until the value is stable.

**A format-string mismatch made the firmware report a number that could not exist.** The probe printed `raw=1061192117`; the field is a `uint16_t`, so no such reading was possible. There were 14 format conversions against 15 arguments, which shifted every label after `link=` — the init token appeared under the link label, and a `char*` was printed with `%u`, yielding a heap pointer that looked like a sensor value. **Anything reading as a measurement should be sanity-checked against what the variable can hold.** The arguments and conversions are now counted and matched, and a `uint16_t` that prints above 65535 is a bug report, not a range.
### Two firmware bugs this run exposed

Both were found by the hardware rather than by reading the code, and both are fixed.

**The loop was unbounded when the sensor was down, and overran the serial port.** `readRange()` returns immediately when `tofReady` is false, so with no pacing anywhere in `loop()` the sketch free-ran at the rate the CPU could format rows — far above what 115200 baud carries. The symptom was not an error but a **mangled stream**: timestamps frozen at four consecutive values, lines truncated to fragments like `2254`, and the boot banner lost entirely inside the noise. `loop()` now waits `TOF_PERIOD_MS` whenever the sensor is down. The pacing claim in [the line-rate section](#the-line-rate-is-the-ranging-rate-not-the-loop-rate) is true only when the sensor is up; that dependency was implicit and is now stated in the code as well.

**Diagnostics that print once are diagnostics that get missed.** The identity registers and the failure reason were printed only during `setup()`, and this board could not be reset over the RTS line from a script — so a monitor attaching later saw nothing but rows and no explanation at all. Both are now held and **re-printed on every probe**, which is why the state above is knowable at all without a reset. This is the concrete form of the parent's advice to prefer repeating diagnostics over catching the one moment.

### A trap when reading this sketch's output from the host

**A serial reader killed mid-stream leaves its unread bytes in the kernel tty buffer, and the next reader starts by flushing that backlog.** The result looks exactly like the board repeating itself: thousands of byte-identical rows arriving in one burst. A 10-second capture appeared to show 4349 rows carrying only 197 distinct timestamps, which is physically impossible for honest firmware — 4349 rows would mean 4349 distinct `millis()` values. Reading single rows a second apart instead showed timestamps advancing by ~918 ms, confirming the stream was honest and the apparent duplication was stale data. **Drain the port before concluding anything from a bulk capture**, and prefer reading small chunks when diagnosing.

---

## `XSHUT` is driven, not floated

The sketch asserts `XSHUT` deliberately rather than hoping the sensor comes up on its own: configure GPIO 27 as an output, drive it **low** for 20 ms to force a hardware reset, release it **high**, then wait **200 ms** before issuing a single I²C transaction. The part needs its oscillator to start and its boot sequence to run; addressing it too early is the classic "it never responds" failure. The parent calls anything under about 100 ms unverified, and 200 ms sits comfortably above that.

If the sensor does not acknowledge at `0x29` even after that sequence, the sketch says so and **stays in a repeating error report rather than streaming zeros** — because a column of zeros looks exactly like a working sensor, and that mistake is the same shape as the one that cost the most time during the IMU bring-up. The probe reports `XSHUT=high` on every cycle precisely so the driven state is verifiable from the running stream and not only from the banner.

**This part of the wiring is confirmed correct.** The live probe shows `XSHUT=high`, so GPIO 27 is driving the line as intended. The failure is on SDA and has nothing to do with `XSHUT`.

## What the sketch does

`tof_test.ino` drives the VL53L0X through the Pololu driver, streams CSV on the USB serial port at 115200 baud, and — while the button is held — broadcasts the same rows to the compute unit over UDP.

```
t_ms,raw,dist_mm,status
```

The button is **not** a data column, exactly as in the IMU test: it gates the wireless link. Held, samples are transmitted; released, transmission stops. The local USB stream runs either way, because it is the operator's own record and should not depend on the radio. This mirrors [§1.5](../../AGENTS.md), where the button gates the laser.

Each row is formatted **once** and written to both sinks, so the USB record and the copy the compute unit receives cannot disagree about what was measured.

### The `status` column, and why `raw` sits next to it

The parent's firmware contract requires an explicit status column rather than folding failure into the value, and for this sensor that matters more than for the IMU — a failed range has **four** distinct failure modes and they are not interchangeable:

| `status` | Meaning | `dist_mm` |
|---|---|---|
| `0` | Valid measurement | the range |
| `1` | Signal fail — target too dark or too far to reflect enough back | `0` |
| `2` | Out of range — no target within the sensor's reach | `0` |
| `3` | Timeout — no fresh measurement arrived in time | `0` |
| `4` | Unavailable — the sensor never came up, so nothing was attempted | `0` |

Only status `0` carries a distance; every other case prints `0` **and** a non-zero status, so a failed reading is never mistaken for a target at zero range.

**The `raw` column exists because one thing could not be settled without hardware in hand.** Whether the driver returns a bare millimetre count or leaves any status bits encoded in the same word cannot be determined from the source alone, and the two possibilities are *indistinguishable in the decoded column*: an encoding that shifted the range would still print a plausible millimetre figure and would be indistinguishable from a calibration error until much later. Printing the driver's return value verbatim means any such mismatch is obvious on the very first sample. If `raw` and `dist_mm` ever differ while `status` is `0`, that is the bug, and it will have been caught immediately rather than after hours of range measurements.

The three ways the driver reports failure are all distinct and all handled: its I2C timeout returns `65535`, and the sensor's own signal-fail and out-of-range sentinels return `8190` and `8191`.

### The driver's timeout must be set explicitly

The Pololu driver initialises its I2C timeout to **zero**, and its expiry check is written as `io_timeout > 0 && ...` — so a zero timeout does not mean "no limit", it means **the check never fires**. On a sensor that stops reporting, `readRangeContinuousMillimeters()` then spins forever, and the board presents as a *frozen stream* rather than as an error: no timeout message, no error code, just a stream that stops. The sketch therefore calls `setTimeout(200)` before initialising, and classifies a raised timeout flag as status `3` rather than as a distance.

### The line rate is the ranging rate, not the loop rate

Unlike the IMU test, this sketch has no `LOOP_PERIOD_MS` controlling its output rate. Each iteration blocks inside `readRangeContinuousMillimeters()` until the sensor has a measurement, so the loop self-paces to the ranging period: `TOF_PERIOD_MS = 50` gives **about 20 rows per second**, and no amount of loop tightening will exceed that.

**The 50 ms floor is confirmed on hardware.** Reading single rows a second apart shows timestamps advancing in steps of **51 ms** (measured: 145757 → 146675 over one second of wall time). That is the `delay()` path, since the sensor is not up; the blocking path inside `readRange()` will impose the same floor once ranging starts, but **that has not been measured**, because no measurement has ever been taken.

Lowering the period lowers latency per sample but also lowers accuracy, because the measurement timing budget and the period have to agree — the budget cannot exceed the period. Both are named constants at the top of the sketch, documented in place, and chosen at 33 ms / 50 ms as an accuracy-favouring starting point rather than a latency-favouring one. If the study later needs the range faster, that is a constant to change, not a redesign.

## How this differs from the pointer's specified wiring

| Parent specifies | This sketch | Why |
|---|---|---|
| MPU6050 at `0x68`, shared I²C bus | **Absent** — the sketch never addresses it | One module at a time; the ToF broke the IMU last round |
| Five pins: 21, 22, 18, 27, plus both sensors | **Four pins: 21, 22, 18, 27** | Follows from the IMU being absent |
| Both 4.7 kΩ pull-ups fitted externally | **Both fitted — and mandatory** | The GY-521, which supplies its own, is not present |
| Range in millimetres with an explicit status column | **Same, plus a raw passthrough column** | Encoding unverified without hardware; see above |
| Bus at **400 kHz** | **100 kHz** | Breadboard jumpers; slower is the safe default until a known-good module proves the faster rate |
| No control-address probe specified | **Added** — probes an unused address beside the sensor's | An address ACK means nothing on a bus whose SDA is shorted low |

The 100 kHz figure is a **provisional deviation** from the parent's specified 400 kHz, and it is not a fix for anything — it was tried as a rise-time hypothesis and made no difference, because the fault turned out to be the module. It is left slower because that is the setting under which the address phase was observed to behave correctly, and a replacement module should be tested at 100 kHz first. **Raise it back to 400 kHz once a good module reads `0xEE`,** and treat a failure at 400 kHz as a fresh symptom rather than a return of this one.

Everything else matches the parent exactly, which is deliberate: the point of the deviations above is to isolate the ToF, not to redesign the wiring. The control-address probe is the one addition that is not about isolation — it is the diagnostic that named the present fault, and it should be carried into the real firmware.

## Known limitations, and what the real firmware still owes

None of these is a defect in the sketch. They are the distance between a bring-up test and a firmware, recorded so the sketch is not mistaken for more than it is.

- **No range has ever been measured.** The sketch is flashed and its pacing and diagnostics are confirmed against a real board, but the sensor has never passed `init()`, so every status code, sentinel value, and timing figure here is still taken from the Pololu driver and the ST datasheet rather than from observation. The 51 ms output floor *is* measured; the ranging path is not.
- **The ToF does not know where the laser is pointing.** A VL53L0X returns the distance to whatever is inside its field of view — which includes the wall the user is facing, the ceiling, and their own hand — not to whatever the laser happens to be hitting. Aiming the sensor down a corridor and having it report the floor is expected behaviour, not a fault. Pairing the range with the laser's own direction is a system-level problem the compute unit solves, and this sketch has no laser to pair with.
- **No line-of-sight or transparency reporting.** The part returns a number, not a confidence, and a dark or specular target can return a perfectly confident wrong answer. `status` catches the cases the sensor volunteers and is silent about the ones it does not.
- **The link is unauthenticated.** The receiver binds `0.0.0.0`, so any host on the network can inject rows that get printed. This matches the existing MJPEG preview server, which makes the same assumption and documents it at its bind site. Acceptable on a bench, not on an open network; `--bind 127.0.0.1` restricts it.
- **There is no clock alignment.** `t_ms` is the pointer's own monotonic counter with no epoch, so it orders samples but cannot be compared against the compute unit's clock. §4.2 asks for sequence numbers and paired tx/rx timestamps; a bare CSV row carries neither, so this stream is a development stream and not yet the §4.2 event stream.
- **The 2.8 V condition is checked by meter, never by code.** Nothing in the firmware can detect an over-driven `VREF` before it causes harm. This is the one wiring precondition here with a destructive failure mode, and it is why the parent's item 1 exists.

## The receiving end on the compute unit

Unchanged from the IMU test, and for the same reasons. `cu/aruco_pose/pointer_link.py` lives in the **separate `cu` submodule** — which has its own remote and history, so a commit there does not move this repository's gitlink.

```bash
cd <auris-repo>/cu
PYTHONPATH=. .venv/bin/python -m aruco_pose.pointer_link --port 5005
```

It needs **no new dependency** — standard library only — so `requirements.txt` and `setup.sh` are unchanged. Run on the actual Pi as `ssh capstone 'cd ~/Projects/system && python3 -m aruco_pose.pointer_link --port 5005'`.

**The column count differs from the IMU test's**, which matters: the receiver is deliberately tolerant and prints whatever it is sent rather than parsing to a fixed schema, so it will accept four columns without complaint. It is a bring-up receiver, not a parser.

## Credentials

`wifi_secrets.h` is **not tracked in git** and is **symlinked** from `../imu_test/wifi_secrets.h`, so the two sketches cannot drift apart and there is exactly one copy of the password on disk.

Two requirements bite here, both discovered the hard way on the IMU test and both documented in [the sibling's Connections section](../imu_test/AGENTS.md#connections):

- **The network must be 2.4 GHz.** The ESP32-WROOM-32 radio cannot receive 5 GHz at all. A 5 GHz SSID does not appear in a scan and will never join, failing as `WL_DISCONNECTED` — the same generic code as a wrong password, which gives nothing away. On a failed join the sketch scans and prints every network it *can* see, which settles the question rather than leaving it to inference.
- **Editing credentials requires clearing the build cache.** arduino-cli's sketch hash covers the `.ino` and `sketch.yaml` only, **not auxiliary headers**, so a changed `wifi_secrets.h` leaves the previous strings baked into the cached object and the firmware announces the old SSID. Run `rm -rf build ~/.cache/arduino/sketches` before recompiling.

This second point is not hypothetical for *this* sketch: it was compiled once at 408 KB before the symlink existed, with `wifi_secrets.h` therefore absent and the entire Wi-Fi path compiled out via `#if __has_include`. It looked like a working build. After the symlink was added it compiled to 921 KB — the difference between "no wireless link exists" and "wireless link exists", decided entirely by a file the cache could not see.

## Build, flash, and monitor

arduino-cli with the `esp32:esp32` core pinned in `sketch.yaml`. **This sketch's profile is `tof-test`**, and it is the one place the third-party VL53L0X driver is used — the legitimate exception named in [the parent's contract](../AGENTS.md#firmware-contract), because that part's init sequence is long and must be correct.

```bash
arduino-cli compile --profile tof-test
arduino-cli upload  --profile tof-test --port /dev/ttyUSB0
arduino-cli monitor --port /dev/ttyUSB0 --config baudrate=115200
```

The parent's build section documents `--profile pointer-sensor-node`, which does not exist in this repository. Copying those commands in verbatim will fail; use the ones above, from this directory.

**A monitor attaching after the chip has already booted sees only the sample stream**, because the banner is emitted once inside `setup()` — and here that matters unusually much, since the Wi-Fi join alone can take up to 20 seconds and a scan on failure takes several more. The sketch therefore re-probes the sensor every 5 seconds and prints the acknowledgement state, a control-address probe, the `XSHUT` level, the identity registers, the init result, and the last raw reading — so a monitor that missed the banner still has a complete diagnosis. Prefer that over trying to catch the one moment.

**Do not rely on a script-driven reset over RTS.** Asserting RTS for 150 ms and for 600 ms both left this board running, with uptime still climbing afterwards. `arduino-cli upload` resets it reliably, so the practical recipe when a banner is needed is to re-upload. Designing diagnostics to repeat removes the need entirely, which is the better answer and is why the probe carries everything.

## Before powering up

The parent's full list applies, but three of its six items are load-bearing for this sketch specifically:

1. **`VREF` is at 2.8 V.** Measure it with the USB cable disconnected. Destructive if wrong, undetectable by the firmware.
2. **Both pull-ups reach 3V3 and the correct data line.** With the GY-521 absent there is no second source of them, and a resistor bridging SDA to SCL — the classic breadboard error — shows up only as a completely dead bus.
3. **No wire is on a flash pin.** Pins 4, 5, 6, 10, 11, 12, labelled `cmd`, `sd3`, `sd2`, `sd1`, `sdd`, `clk`. They sit at the ends of the rows where wires naturally run, and their labels give no hint of the hazard.

The ESP32 powers the sensor through its own 3V3 regulator, so there is no separate supply to sequence — but see item 1 first.