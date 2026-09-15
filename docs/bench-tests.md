# Cane — P2 bench protocol (breadboard)

**No-soldering rule.** P2 attaches every component **non-permanently** — breadboards, dupont jumpers, zip ties, velcro, tape, friction mounts — and involves **no soldering of any kind**. This is as much a purchasing constraint as a build rule: the XIAO ESP32-S3 and most GY/ToF modules ship with headers unsoldered, so order everything with **headers pre-soldered** (or substitute a pre-soldered board); anything that arrives unsoldered is set aside for the P3 build, never soldered during P2. Full functionality of the whole system must be proven here before P3 solders anything. P3 then builds the aid itself — the evaluated device is soldered once and housed; it is not a disposable prototype, and rework there is limited to fixes, not redesign.

The pointer-tracking extras ([README §3.8](../README.md#38-pointer-tracking-upgrade--extras-bom) — two wide-FOV camera modules, the UWB module pair, the pointer fiducial) join the P2 purchase list under the same pre-soldered rule; T7–T8 below validate the tracking tiers on the breadboards, and the module choices are pinned at this gate ([README §9](../README.md#9-open-items)).

## Bring-up safety

- Check 18650 polarity twice before each insertion; never park a bare cell on metal.
- First power-up of each device through a multimeter inline (mA range) or a current-limited USB source.
- Grounds common per device: MCU GND, sensor GND, amp GND, battery negative all on one rail.
- P2 bench sessions run from **USB power banks or bench USB**, not the 18650 rail, until the power test T0 passes; battery wiring is validated on the bench (T0) before any untethered use.
- Power off before any wiring change; hot-plugging I²C/I²S is how modules die.

## Wiring map — pointer (ESP32-C3 SuperMini)

ESP32-C3 I²C is remappable; the pairs below avoid strapping pins (GPIO2/8/9) and the onboard LED/button pins. Verify labels against the board silk before wiring — SuperMini revisions differ.

| Signal | ESP32-C3 pin | Notes |
|---|---|---|
| VL53L1X SDA | GPIO4 | `Wire.begin(4, 5)`; bus shared with IMU |
| VL53L1X SCL | GPIO5 | 3.3 V logic; module VCC from 3V3 rail |
| MPU-9250 SDA/SCL | same bus (GPIO4/5) | addr `0x68`; VL53L1X at `0x29` — no conflict |
| MPU-9250 VCC | 3V3 rail | GY-9250 module regulator accepts it |
| Trigger button | GPIO3 | `INPUT_PULLUP`, other leg to GND |
| Reserved | GPIO0, GPIO1 | keep free (ADC/deep-sleep wake) |
| UART debug | GPIO20 (RX) / GPIO21 (TX) | flashing logs; leave unshared |
| Power | TP4056 OUT → 5 V pin, GND common | bench-verify the onboard regulator across the 3.7–4.2 V Li-ion range; fall back to USB power bank if it misbehaves |

## Wiring map — wearable (Seeed XIAO ESP32-S3)

XIAO pin map per the Seeed wiki: D4 = GPIO5 (SDA), D5 = GPIO6 (SCL), D6 = GPIO43, D7 = GPIO44, D8 = GPIO7, D9 = GPIO8, D10 = GPIO9.

| Signal | XIAO pin | Notes |
|---|---|---|
| MPU-9250 SDA | D4 (GPIO5) | default `Wire` pins on this board |
| MPU-9250 SCL | D5 (GPIO6) | module VCC from 3V3 rail |
| Amp bus BCLK | D8 (GPIO7) | both MAX98357A share this I²S bus |
| Amp bus LRC | D9 (GPIO8) | |
| Amp bus DIN | D10 (GPIO9) | |
| Amp channel select | each amp's SD pin | SD > 1.4 V = left; 0.77–1.4 V = right; 0.08–0.77 V = (L+R)/2; < 0.08 V = shutdown — strap one amp L, one amp R per the breakout's documentation at build time |
| Re-zero button | D6 (GPIO43) | `INPUT_PULLUP`, other leg to GND |
| Earphones | 3.5 mm jack, one channel per amp | wired only; never Bluetooth audio |
| Telemetry | USB-C serial (built-in) | logs over the flashing cable; D7 kept free |
| Status LED | GPIO21 (USER_LED) | link/render activity indicator |
| Power (P2 bench) | USB-C power bank on XIAO USB-C | battery path validated separately in T0 |
| Power (P3 build) | TP4056 OUT → XIAO BAT pads (3.7–4.2 V) | XIAO onboard PMIC handles regulation and charging; charge the cell via the TP4056, not simultaneously through the XIAO |

**GPIO budget check.** Used: I²C 2 + I²S 3 + button 1 + LED 1 = 7 of 11 exposed GPIOs; D0–D3 (GPIO1–4) remain free.

**Tracking-extras GPIO pressure (P2 decision).** The frozen core leaves no room for two DVP cameras plus a UWB module (SPI + IRQ) on the XIAO's exposed pins: the ESP32-S3 has one DVP interface, so two head cameras need a mux, alternate-frame capture on one port, a DevKit-class ESP32-S3 board (more GPIOs — the [§3.7](../README.md#37-component-notes) drop-in alternates), or a camera co-processor. UWB needs an SPI bus plus IRQ/reset pins. This board-level choice is exactly the risk-8 gate ([README §7](../README.md#7-risks-and-limitations)) and is settled by T7 before the P3 build.

## Test matrix

Acceptance derives from the §4.2 budget (motion-to-sound ≤ 100 ms). Run order = dependency order; log every session to CSV.

### T0 — Power rails

- Setup: TP4056 + 18650 per device, multimeter inline.
- Procedure: measure rail voltage under idle and full-load (radio TX + amp playing); 30 min soak.
- Acceptance: 3.3 V rail stable within ±3% under load; TP4056 protection trips on short test; no thermal runaway (housing-temp check by touch after soak).
- Metric logged: `v_rail`, `i_load`, `t_soak`.

### T1 — IMU orientation & drift (both devices)

- Setup: device on a flat level reference; second reference: phone inclinometer app.
- Procedure: (a) static pitch/roll at 5 poses vs. reference; (b) gyro-only relative yaw drift over 10 min after bias calibration, both devices powered simultaneously; (c) magnetometer disturbance characterization: heading error near a steel table/rebar vs. open area.
- Acceptance: static pitch/roll error ≤ 2°; pitch/roll drift ≤ 0.5° per 5 min; combined relative-yaw drift ≤ 2°/min (re-zero button bounds it in use); disturbance test is **characterization** (records bias), not a gate — mitigated by re-zero (§7 risk 2).
- Metric logged: `pose_err_deg`, `drift_deg_per_min`, `mag_bias_deg`.

### T2 — ToF accuracy & cadence

- Setup: pointer breadboard clamped on a stand; tape-measured distances; surfaces: white foam board, cardboard box, dark fabric (three reflectances).
- Procedure: 30 s continuous ranging at 0.3, 0.5, 1, 2, 3, 4 m per surface; record error, hit rate, cadence.
- Acceptance: error ≤ ±3 cm ≤ 2 m and ≤ ±5% beyond, on ≥ 2 of 3 surfaces; sustained cadence ≥ 20 Hz; invalid-read rate < 5%; behavior on no-target (drop to range-max or invalid flag) documented.
- Metric logged: `d_true`, `d_meas`, `rate_hz`, `miss_rate`.

### T3 — ESP-NOW link

- Setup: pointer ↔ wearable firmware ping-pong, 12–28 B payload (d + quaternion + seq), 10–30 Hz.
- Procedure: 10 min indoor run at 2, 5, 10 m including body-blocked (person between devices); log one-way latency (seq timestamp at receiver) and loss.
- Acceptance: one-way ≤ 10 ms at p99; loss < 1% at ≤ 10 m indoor with body between devices.
- Metric logged: `lat_ms`, `loss_pct`, `range_m`.

### T4 — Renderer load

- Setup: wearable breadboard with both amps; full renderer (generic-HRTF azimuth, carrier-pitch elevation cue, `g(D)` gain) at 48 kHz / 128-sample buffers.
- Procedure: 10 min run; `esp_timer` around the render callback per buffer; count I²S underruns; sweep azimuth sectors + distance classes to cover the table.
- Acceptance: render ≤ 5 ms per buffer at p99; underrun count = 0 over 10 min.
- Metric logged: `render_ms`, `underruns`.

### T5 — End-to-end latency & placement sanity

- Setup: both devices running the full pipeline; obstacle at known pose.
- Procedure: (a) firmware timestamps from ToF sample to I²S buffer queue, 10 min of button sweeps; (b) sweep the pointer across a wide obstacle and verify the sound's azimuth tracks the hit; aim above/below head level and verify the elevation cue flips at head height; walk toward the obstacle with the button held and verify loudness grows monotonically.
- Acceptance: motion-to-sound ≤ 100 ms p99; placement tracks azimuth sweep without jumps; elevation cue flips at head height; loudness monotonic in `D`.
- Metric logged: `e2e_ms`, `placement_ok`, `notes`.

### T6 — Offset calibration repeatability

- Setup: flat grip fixture; ruler/calipers.
- Procedure: measure the head-to-pointer offset components `o` 5×; enter into config; run `geometry` and check `D` against tape-measured head-to-hit distance at 3 poses.
- Acceptance: repeatability ± 2 cm per component; `D` error ≤ ± 6 cm at 1–3 m with nominal grip.
- Metric logged: `o_component`, `D_err_cm`.

### T7 — Vision pointer tracking (upgrade tier, [README §2.5](../README.md#25-pointer-tracking-stack-vision-uwb-imu))

- Setup: two wide-FOV camera modules on a head-form fixture (one each side, spaced like the wearable shell); fiducial pattern on the pointer shell; tracking firmware **co-resident with the full renderer** (the T4 configuration plus tracking).
- Procedure: (a) place the pointer at tape-measured known poses across the forward hemisphere (0.5–3 m; azimuth sweep; aimed above/below head level) — log position error and tracking rate per pose; (b) record where the fiducial leaves each camera's FOV envelope — the tier-switch boundary; (c) 10 min co-residence run: `esp_timer` per render buffer and I²S underrun count, as in T4, with tracking running.
- Acceptance: position error ≤ ±5 cm at 0.5–2 m and ≤ ±10% beyond (provisional — finalize at the P2 gate); cadence ≥ 15 Hz; FOV envelope documented per camera; **the renderer is unharmed by co-residence** — ≤ 5 ms/buffer p99 and 0 underruns over the 10 min run (the risk-8 gate: if it fails, the co-processor or alternate-DVP decision is forced here, before P3).
- Metric logged: `p_err_cm`, `track_rate_hz`, `fov_envelope_deg`, `render_ms`, `underruns`.

### T8 — UWB ranging & tier fallback (upgrade tier, [README §2.5](../README.md#25-pointer-tracking-stack-vision-uwb-imu))

- Setup: DW1000-class pair on both breadboards; tape-measured baselines; logging captures the active tier for every sound update.
- Procedure: (a) range error vs tape at 0.5–4 m line-of-sight; (b) body-blocked/NLOS profile (person between devices; pointer held behind the body) — characterization, the fallback tier absorbs degradation; (c) tier-switch run: sweep the pointer out of camera view into UWB-only and back, repeatedly, including behind-body holds; log the active tier per update and the placement continuity across switches.
- Acceptance: range error ≤ ±10 cm at ≤ 3 m LOS, update rate ≥ 10 Hz; NLOS profile documented (characterization, not a gate); **no placement jump beyond the T6 `D`-error envelope at any tier switch** — tier changes must be inaudible.
- Metric logged: `r_err_cm`, `uwb_rate_hz`, `tier`, `placement_jump_cm`.

## Logging format

One CSV per session: `session,test_id,timestamp_ms,metric,value,unit,notes` — drift curves (T1) and latency distributions (T3–T5) feed P4 calibration and thesis §3/§4 artifacts; the T7–T8 tier records (per-update active tier + pose error) feed the §2.5 realism claims.

## P3 parity re-run

After soldering, re-run T0–T8 on the soldered assemblies; acceptance identical, except the T7 FOV envelope is re-characterized for the housing-mounted cameras (housing geometry moves the lenses). Parity is the P3 exit criterion (§6) — solder joints, housing geometry, and jack wiring are the new variables, so the breadboard numbers are the baseline to match.
