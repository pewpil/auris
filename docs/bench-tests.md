# Cane — P2 bench protocol (breadboard)

**No-soldering rule.** P2 attaches every component **non-permanently** — breadboards, dupont jumpers, zip ties, velcro, tape, friction mounts — and involves **no soldering of any kind**. This is as much a purchasing constraint as a build rule: most modules ship with headers unsoldered, so order everything with **headers pre-soldered** (or substitute a pre-soldered board); anything that arrives unsoldered is set aside for the P3 build, never soldered during P2. Full functionality of the whole system must be proven here before P3 solders anything. P3 then builds the aid itself — the evaluated device is soldered once and housed; it is not a disposable prototype, and rework there is limited to fixes, not redesign.

The pointer-tracking hardware ([README §2.5](../README.md#25-pointer-tracking-stack-vision-uwb-imu) — two wide-FOV camera modules, the UWB module pair, the pointer tracking marker (printed ArUco/AprilTag); [README §3.1](../README.md#31-pointer--electronics)/[§3.2](../README.md#32-wearable--electronics)) joins the P2 purchase list under the same pre-soldered rule; T7–T8 below validate the tracking tiers on the breadboards, and the module and board-level implementation choices are pinned at this gate ([README §9](../README.md#9-open-items)). The tracking hardware is part of the final design — the gates decide *how* it is built, never *whether*.

## Bring-up safety

- Check battery-cell polarity twice before each insertion; never park a bare cell on metal.
- First power-up of each device through a multimeter inline (mA range) or a current-limited USB source.
- Grounds common per device: MCU GND, sensor GND, amp GND, battery negative all on one rail.
- P2 bench sessions run from **bench USB** (a 5 V / 5 A-class USB-PD adapter for the Raspberry Pi 5 side), not the battery rail, until the power test T0 passes; battery wiring is validated on the bench (T0) before any untethered use. Watch the Pi's undervoltage/throttle flags on current-limited sources — that is bench-source behavior, not a device failure.
- Power off before any wiring change; hot-plugging I²C/I²S is how modules die. On the Raspberry Pi 5, **safe shutdown before power-off always** — a pulled plug on a live SD image risks corruption. The SD card + OS image (direct-ALSA audio config, SCHED_FIFO render thread, cpufreq governor pinned) is bench infrastructure alongside the wiring ([README §3.2](../README.md#32-wearable--electronics)).

## Wiring maps

> **Maps drawn against the selected boards (2026-09-25: Raspberry Pi 5 wearable, ESP32-S3 pointer — [README §3](../README.md#3-hardware)).** The per-device pin assignments — the Pi 5's 40-pin header (I²C for the IMU, SPI + IRQ/reset for the DW3000 anchor, PCM pins to the amplifier pair), the ESP32-S3 GPIO map, and the per-device power paths — are recorded here before the bench phase starts. The constraints they must satisfy: on the ESP32-S3, avoid strapping pins and keep the debug UART unshared; on the Pi 5, avoid boot-time-function GPIOs and keep the debug UART (the Linux console) off the header; distinct I²C addresses for the IMU and the ToF sensor on the shared bus (0x4A / 0x29); each MAX98357A amplifier with its own L/R channel-select strapping; note the Pi 5's current-limited 3.3 V header rail — the sensors take their rail per the map; and leave the battery paths (charge/protection → cells → 1S on the pointer, 2S + balance/protection + 5 V/5 A buck on the wearable) common-grounded with the rest of each device.

**Interface pressures — resolved and remaining (P2 decisions, risk 8).** The camera capture strategy is **consumed by selection**: the Raspberry Pi 5's two native CSI connectors take the Camera Module 3 Wide pair directly (no mux, no alternate-frame capture, no camera co-processor). Remaining at the gates: the UWB modules' SPI + IRQ/reset wiring per the map (T8), the **DW3000 driver bring-up on the Pi** (an added T8 sub-test before any ranging run), and the detection-rate/co-residence confirmation at T7 — on the A76 the expectation is full-rate detection, with detect-then-track as the compute-side fallback.

## Test matrix

Acceptance derives from the §4.2 budget (motion-to-sound ≤ 100 ms). Run order = dependency order; log every session to CSV.

### T0 — Power rails

- Setup: charge/protection path + battery cells per device (1S on the pointer, 2S + balance/protection + 5 V/5 A buck on the wearable), multimeter inline.
- Procedure: measure rail voltage under idle and full-load (radio TX + amp playing, and — once the tracking extras are fitted — the tracking tier at full duty: both cameras streaming + UWB ranging); 30 min soak. Camera streaming is the largest new draw: the battery budget is re-checked here with tracking active.
- Acceptance: the sensor rail stable within ±3% under load — the Pi 5 side additionally at its 5 V rail with **no undervoltage/throttle flags** read from the PMIC after the soak; charge-board protection trips on short test; no thermal runaway — the housing-temp check by touch after soak is a **logged metric** (`temp_c`), first-class on the wearable under the tracking-duty soak.
- Metric logged: `v_rail`, `i_load`, `t_soak`, `temp_c`.

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

### T3 — Device-to-device link

- Setup: pointer ↔ wearable firmware ping-pong, 12–28 B payload (d + quaternion + seq), 10–30 Hz.
- Procedure: (a) 10 min run logging one-way latency (seq timestamp at receiver) and loss; (b) **wireless medium**: repeat at 2, 5, 10 m including body-blocked (person between devices); **wired medium**: cable flex/bend cycling and connector retention throughout the run — range is fixed by the cable.
- Acceptance: one-way ≤ 10 ms at p99; loss < 1% at ≤ 10 m indoor with body between devices (wireless medium) / no lost packets over the session (wired medium).
- Metric logged: `lat_ms`, `loss_pct`, `range_m`.

### T4 — Renderer load

- Setup: wearable breadboard (Raspberry Pi 5) with both amplifiers fed by the 40-pin header's PCM pins; full renderer (generic-HRTF azimuth, carrier-pitch elevation cue, `g(D)` gain) at 48 kHz / 128-sample buffers through the hand-configured ALSA path (SCHED_FIFO render thread, governor pinned).
- Procedure: 10 min run; `clock_gettime` around the render callback per buffer; count ALSA XRUNs; sweep azimuth sectors + distance classes to cover the table.
- Acceptance: render ≤ 5 ms per buffer at p99; underrun count = 0 over 10 min.
- Metric logged: `render_ms`, `underruns`.

### T5 — End-to-end latency & placement sanity

- Setup: both devices running the full pipeline; obstacle at known pose.
- Procedure: (a) software timestamps from ToF sample to rendered buffer queue, 10 min of button sweeps; (b) sweep the pointer across a wide obstacle and verify the sound's azimuth tracks the hit; aim above/below head level and verify the elevation cue flips at head height; walk toward the obstacle with the button held and verify loudness grows monotonically.
- Acceptance: motion-to-sound ≤ 100 ms p99; placement tracks azimuth sweep without jumps; elevation cue flips at head height; loudness monotonic in `D`.
- Metric logged: `e2e_ms`, `placement_ok`, `notes`.

### T6 — Offset calibration repeatability

- Setup: flat grip fixture; ruler/calipers.
- Procedure: measure the head-to-pointer offset components `o` 5×; enter into config; run `geometry` and check `D` against tape-measured head-to-hit distance at 3 poses.
- Acceptance: repeatability ± 2 cm per component; `D` error ≤ ± 6 cm at 1–3 m with nominal grip.
- Metric logged: `o_component`, `D_err_cm`.

### T7 — Vision pointer tracking (tracking tier, [README §2.5](../README.md#25-pointer-tracking-stack-vision-uwb-imu))

- Setup: two wide-FOV camera modules on a head-form fixture (one each side, spaced like the wearable's strap stations); **printed ArUco/AprilTag marker** (high-contrast; dictionary and physical size pinned at this gate) on the pointer shell ([README §3.8](../README.md#38-pointer-tracking-hardware--notes--contingencies)); tracking processes **co-resident with the full renderer** (the T4 configuration plus tracking — vision on separate cores at priority below the audio path).
- Procedure: (a) place the pointer at tape-measured known poses across the forward hemisphere (0.5–3 m; azimuth sweep; aimed above/below head level) — log position error and tracking rate per pose; (b) record where the tag leaves each camera's FOV envelope — the tier-switch boundary; (c) 10 min co-residence run at trigger-gated duty (button held throughout, [README §1.3](../README.md#13-interaction-rule)): `:clock_gettime` per render buffer and ALSA XRUN count, as in T4, with tracking running; (d) **low-light/contrast check**: with tracking running, dim the room stepwise toward evening-lighting levels — record the minimum illumination at which the tag still detects reliably (the tag is passive and needs scene light; watch exposure and motion blur at 15–30 Hz; a printed tag emits nothing, so there is no flash-sync interference to reject).
- Acceptance: position error ≤ ±5 cm at 0.5–2 m and ≤ ±10% beyond (provisional — finalize at the P2 gate); cadence ≥ 15 Hz; FOV envelope documented per camera; **the renderer is unharmed by co-residence** — ≤ 5 ms/buffer p99 and 0 underruns over the 10 min run (the risk-8 gate: if full-rate detection fails it, the detect-then-track scheme (detect every Nth frame) is forced here, before P3 — the capture-side branches (co-processor / alternate capture) are consumed by the Pi 5's dual-CSI selection, [README §3.8](../README.md#38-pointer-tracking-hardware--notes--contingencies)).
- Metric logged: `p_err_cm`, `track_rate_hz`, `fov_envelope_deg`, `render_ms`, `underruns`. The marker's PnP orientation is **not consumed and not gated** — the tracking tier is position-only ([README §2.5](../README.md#25-pointer-tracking-stack-vision-uwb-imu)); pointer orientation always comes from its IMU.

### T8 — UWB ranging & tier fallback (tracking tier, [README §2.5](../README.md#25-pointer-tracking-stack-vision-uwb-imu))

- Setup: UWB module pair on both breadboards; **driver bring-up first on the Pi side** — the DW3000 SPI driver/stack is validated on the Raspberry Pi 5 (kernel or userspace) before any ranging run; tape-measured baselines; logging captures the active tier for every sound update.
- Procedure: (a) range error vs tape at 0.5–4 m line-of-sight; (b) body-blocked/NLOS profile (person between devices; pointer held behind the body) — characterization, the fallback tier absorbs degradation; (c) tier-switch run: sweep the pointer out of camera view into UWB-only and back, repeatedly, including behind-body holds; log the active tier per update and the placement continuity across switches.
- Acceptance: range error ≤ ±10 cm at ≤ 3 m LOS, update rate ≥ 10 Hz; NLOS profile documented (characterization, not a gate); **no placement jump beyond the T6 `D`-error envelope at any tier switch** — tier changes must be inaudible.
- Metric logged: `r_err_cm`, `uwb_rate_hz`, `tier`, `placement_jump_cm`.

## Logging format

One CSV per session: `session,test_id,timestamp_ms,metric,value,unit,notes` — drift curves (T1) and latency distributions (T3–T5) feed P4 calibration and thesis §3/§4 artifacts; the T7–T8 tier records (per-update active tier + pose error) feed the §2.5 realism claims.

## P3 parity re-run

After soldering, re-run T0–T8 on the soldered assemblies; acceptance identical, except the T7 FOV envelope is re-characterized for the housing-mounted cameras (housing geometry moves the lenses). Parity is the P3 exit criterion (§6) — **service workmanship, housing geometry, and jack wiring are the new variables**, so the breadboard numbers are the baseline to match, and the re-run is preceded by the in-house incoming inspection ([`assembly.md` §5 Incoming inspection](assembly.md#5-incoming-inspection-in-house-gate-before-any-power-up): visual solder-joint check against the assembly drawings, per-net continuity, current-limited first power-up): the soldered build is executed by an external local hand-solder service from the in-house design package (README §3.9, decision 2026-09-26), so nothing returned from the service is powered before inspection passes.
