# IMU bring-up test — AGENTS.md

Standing instructions for `pointer/firmware/imu_test/`. Read with [`../../AGENTS.md`](../../AGENTS.md), which carries the wiring specification and firmware contract for the pointer as a whole.

> **Scope:** the repository-wide sections apply here unchanged — [§2](../../AGENTS.md) on documenting every function, [§3](../../AGENTS.md) on never committing, [§4](../../AGENTS.md) on evidence classes, [§5](../../AGENTS.md) on unwrapped prose, and [§6](../../AGENTS.md) on side notes. Nothing here overrides the parent [pointer/AGENTS.md](../AGENTS.md); where this directory departs from it, that is recorded below rather than left to be discovered.

## Status: in progress, and not the pointer firmware

**This is a bring-up sketch, not the pointer's firmware.** It exists to answer two questions — does the ESP32 read the MPU6050 correctly over I²C, and can it get that data to the compute unit — and it is being worked on right now. Do not treat it as an implementation of [the firmware contract](../AGENTS.md#firmware-contract), and do not grow it into one. The real firmware has not been written; [the sketch the parent's build commands name does not exist yet](#build-flash-and-monitor).

Two things follow from that, and they are the two most likely mistakes to make here.

- **The pointer's contract is unmet by design.** The contract in [the parent](../AGENTS.md#firmware-contract) requires ToF range, `XSHUT` sequencing, and both sensors on one bus. This sketch does none of those, so satisfying it here is not progress toward the contract — it is a detour around it.
- **Its output is a development observation, not evidence.** Readings taken off this breadboard are, in the terms of [§4.1](../../AGENTS.md#41-evidence-classes), development activity rather than a `[SIM]`, `[BUD]`, `[PROP]`, or `[CITE]` claim. Promoting anything from this stream into the thesis still requires the §4 fallback order to be satisfied on its own terms.

## Connections

This is what is actually on the breadboard. The [parent's connection list](../AGENTS.md#connection-list) describes the finished build including the VL53L0X; **this sketch is IMU-only**, so the parent list does not describe the present wiring and reading it as though it does will mislead anyone checking continuity.

The two header rows are numbered differently — `r1` is the top row numbered 1–19, `r2` is the bottom row numbered 60–42 across the same columns — so a column carries two unrelated numbers. The GPIO number is the authority; the position only tells you where to look.

| From | To | Function |
|---|---|---|
| `r1 #19` — `3V3` | GY-521 `VCC` | 3.3 V power |
| `r2 #42` — `GND` | GY-521 `GND` | Common ground |
| `r2 #47` — **GPIO21** | GY-521 `SDA` | I²C data |
| `r2 #44` — **GPIO22** | GY-521 `SCL` | I²C clock |
| `r2 #42` — `GND` | GY-521 `AD0` | Selects I²C address `0x68` |
| `r2 #50` — **GPIO18** | button leg A | Button input, internal pull-up |
| `r2 #42` — `GND` | button leg B | Closes GPIO 18 to ground when held |
| GY-521 `XDA`, `XCL`, `INT` | *no connection* | Unused |
| VL53L0X | **not connected at all** | No ToF in this test |

**Ground:** `r2 #42`, or `r1 #2` / `r1 #6` / `r2 #48` — any of them is the same net. Do not use the 5 V input at `r1 #1` as a ground.

**No external pull-ups are fitted.** The GY-521 carries its own 4.7 kΩ on SDA and SCL, so adding more only brings the pair down to roughly 2.3 kΩ for no benefit during bring-up. The [parent](../AGENTS.md#i2c-bus) specifies both 4.7 kΩ for the finished build, where the VL53L0X breakout typically supplies none.

**No pin changes are needed for the Wi-Fi link.** The radio rides the module's own antenna; only power and ground matter, and those are already connected. But the link carries two requirements that are *not* wiring and have both cost real time here.

- **The network must be 2.4 GHz.** The ESP32-WROOM-32 radio cannot receive 5 GHz at all, so a 5 GHz SSID does not appear in a scan and will never join — it fails as `WL_DISCONNECTED`, the same generic code as a wrong password, which gives nothing away. If a join fails, the sketch now scans and prints every network it *can* see, which settles the question rather than leaving it to inference.
- **Credentials go in `wifi_secrets.h`, and changing them requires clearing the build cache.** arduino-cli's sketch hash covers the `.ino` and `sketch.yaml` only, **not auxiliary headers**, so editing `wifi_secrets.h` alone leaves the old strings baked into the cached object and the firmware still announces the previous SSID. Run `rm -rf build ~/.cache/arduino/sketches` before recompiling, or the change silently does nothing.

**Both header copies must agree.** The same wiring appears in the comment at the top of `imu_test.ino`; if one is edited, edit the other. The pin numbers here are the authoritative copy.

## What the sketch does

`imu_test.ino` is a self-contained Arduino sketch with no third-party libraries. It brings up the MPU6050 over `Wire`, measures per-axis zero offsets at startup, streams CSV on the USB serial port at 115200 baud, and — while the button is held — broadcasts the same rows to the compute unit over UDP.

```
t_ms,roll_d,pitch_d,yaw_d,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,temp_c
```

Roll and pitch come from a complementary filter — gravity from the accelerometer pins them absolutely, the gyroscope carries them between updates — and yaw is integrated from the gyroscope alone. Every value carries its unit in the header, and a failed read emits `stale` rather than a number, which is the right instinct and matches the parent's rule about never emitting a plausible-looking zero.

Each row is formatted **once** and then written to both sinks, so the USB record and the copy the compute unit receives can never disagree about what was measured.

### The button gates the wireless link, not the display

The button no longer appears in the output. It does one thing: **while it is held, each sample is broadcast to the compute unit; when released, transmission stops.** The local USB serial stream keeps running either way, because it is the operator's own record and should not depend on the radio.

That mirrors [§1.5](../../AGENTS.md), where the button gates the laser — the link is gated the same way. Press and release are each reported once on the serial port, so the operator can see the gate open and close without watching the compute unit's terminal to know it is running.

### The link is wireless and broadcast

The pointer reaches the compute unit over **UDP**, on port **5005**, to the subnet's **directed broadcast address** — not to a configured peer. The receiver binds `0.0.0.0`, so neither side needs to know the other's IP address.

Broadcast is the default for a concrete reason, not for convenience: **this compute unit's address has moved three times** under DHCP, and the repository [§7.2](../../AGENTS.md#72-identifying-the-device--the-mac-is-stable-the-ip-is-not) records its MAC as the stable identity for exactly that reason. A hardcoded peer would silently stop delivering the first time the lease moved.

If the access point turns out to filter client-to-client broadcast, set `LINK_UNICAST_HOST` in the sketch to the compute unit's address instead. Broadcast is the default because it survives address changes; unicast is the escape hatch for a filtered network.

Credentials live in **`wifi_secrets.h`**, which is **not tracked in git**. Fill in the SSID and password there. The sketch compiles with or without the file — with `#if __has_include` — and falls back to USB serial only, saying so at boot. Never move the password into a tracked file.

## How this differs from the pointer's specified wiring

| Parent specifies | This sketch | Why |
|---|---|---|
| VL53L0X at `0x29`, shared I²C bus | **Absent** — the sketch never addresses it | IMU-only bring-up |
| `XSHUT` driven from `r1 #9` (GPIO27) | **GPIO27 unused** | No ToF to reset |
| Both 4.7 kΩ pull-ups fitted externally | **None** — the GY-521 carries its own | Fewer jumpers for a first run |
| Four pins: 21, 22, 18, 27 | **Three pins: 21, 22, 18** | Follows from the ToF being absent |

The `XSHUT` row is the one to take seriously. The parent calls driving it the single most important correction in the build, and the failure it fixes — a floating `XSHUT` making *every other device on the bus unreadable* — is exactly what would be invisible here, because there is no ToF to cause it. A successful run of this sketch is therefore **not** evidence that the bus is healthy with both sensors attached.

## Known limitations, and what the real firmware still owes

None of this is a defect in the sketch. It is the distance between a bring-up test and a firmware, and it is recorded so the sketch is not mistaken for more than it is.

- **The link is unauthenticated.** The receiver binds `0.0.0.0`, so any host on the network can inject rows that get printed. This matches the existing MJPEG preview server, which makes the same assumption and documents it at its bind site. Acceptable on a bench, not on an open network; `--bind 127.0.0.1` restricts it.
- **Broadcast is untested end to end.** Broadcast datagrams sent from a host are not reflected back to sockets on that same host over Wi-Fi, so the delivery path could not be proven from one machine — it needs the ESP32, which is a genuinely separate host, to send the first datagram. If nothing arrives, suspect AP client isolation before suspecting the code.
- **There is no clock alignment.** `t_ms` is the pointer's own monotonic counter with no epoch, so it orders samples but cannot be compared against the compute unit's clock. §4.2 asks for sequence numbers and paired tx/rx timestamps; a bare CSV row carries neither, so this stream is a development stream and not yet the §4.2 event stream.
- **Yaw is unobservable.** An MPU6050 has no magnetometer, so there is no absolute heading. The sketch's own header says yaw will drift, and it will — slowly, because the startup bias measurement keeps the rate down, but without limit. This is a property of the sensor, not of the filter.
- **Yaw is not wrapped.** `yaw_deg` accumulates without bound and is never reduced to a canonical range, unlike roll and pitch which gravity keeps bounded. Fine over a short session; the real firmware will need unwrapping if it reports yaw in `(-180, 180]`.
- **The filter is test-grade.** A complementary filter with a 0.5 s time constant is the right thing for proving the sensor works, and the wrong thing for the final system. The compute unit is where the real fusion belongs, since that is where the second IMU and the camera geometry live.
- **Gravity-based angles are invalid under acceleration.** `anglesFromGravity` reads specific force, not gravity, whenever the board moves. The filter smooths the transient; it does not remove the error.
- **No ToF, no `XSHUT` sequencing, no fusion output.** The contract in [the parent](../AGENTS.md#firmware-contract) is where these belong.

### Two calibration details that are easy to undo by accident

**The accelerometer bias removes 1 g from the Z average rather than folding gravity into the bias.** Averaging raw Z and subtracting it would delete gravity itself and leave the filter with no reference, so the angles jump at random. This also means the board must be held genuinely flat, Z up, for the full two seconds — an earlier run captured with the board at an angle produced a Z average nowhere near 1 g and correspondingly wrong angles. The sketch says so at boot; leave that prompt in place.

**The sensor must be woken before it will report anything.** The MPU6050 powers up in SLEEP, where `WHO_AM_I` answers perfectly but every data register reads back as zero. Writing `PWR_MGMT_1` is the only thing that makes it live. `setup()` therefore samples the burst 20 times and reports `data path live (n of 20)`, because a correct identity byte proves only that a chip is on the bus.

### One open question worth settling early

The system goal needs full 3-D orientation, including yaw, and the components table currently specifies an MPU6050 for the pointer with no magnetometer anywhere in the design. The relative yaw between pointer and head is the quantity that actually matters for placing sound, and two independently-drifting gyros do not cancel — their *difference* drifts too, just more slowly. Whether that is acceptable for the study, or whether an absolute reference is needed, is a design question rather than a firmware one, and it gets cheaper to answer now than after the real firmware assumes either answer.

## The receiving end on the compute unit

`cu/aruco_pose/pointer_link.py`, in the **separate `cu` submodule** — which has its own remote and history, so a commit there does not move this repository's gitlink. Run it with:

```bash
cd <auris-repo>/cu
PYTHONPATH=. .venv/bin/python -m aruco_pose.pointer_link --port 5005
```

It needs **no new dependency**: the receiver uses only the standard library, so `requirements.txt` and `setup.sh` are unchanged and the Pi's virtual environment does not need rebuilding.

It follows two patterns already in `cu/`: the daemon-thread `start()`/`stop()` lifecycle from `aruco_pose.preview.MJPEGPreview`, including an idempotent `stop()` that is safe to call on a receiver that was never started; and `FrameBroadcaster`'s stance that a late frame is worse than a dropped one, which is why printing is throttled by dropping rows rather than by queueing them.

The receiving counter keeps rising even when printing is throttled, so a rate-limited stream stays distinguishable from an absent one.

## Build, flash, and monitor

arduino-cli with the `esp32:esp32` core pinned in `sketch.yaml`. **This sketch's profile is `imu-test`.** The parent's `--profile pointer-sensor-node` now belongs to the deployed firmware in [`../pointer_sensor_node/`](../pointer_sensor_node/); profiles are per sketch, so copying the parent's commands into this directory verbatim will still fail. Use these instead, from this directory:

```bash
arduino-cli compile --profile imu-test
arduino-cli upload  --profile imu-test --port /dev/ttyUSB0
arduino-cli monitor --port /dev/ttyUSB0 --config baudrate=115200
```

Everything else in the parent's [build section](../AGENTS.md#build-flash-monitor) carries over unchanged: the `uucp` group membership needed before the port can be opened, passing `--port` explicitly rather than pinning a `default_port`, and the fact that a monitor attaching after boot misses the banner, which is why the sketch re-probes the sensor periodically instead.

**Hold the pointer flat and still, Z up, for the first two seconds after reset.** The calibration prompt at boot is not decoration; see the two calibration details above.
