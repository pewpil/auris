# Cane — P2 bench protocol (breadboard)

**No-soldering rule.** P2 attaches every component **non-permanently** — breadboards, dupont jumpers, zip ties, velcro, tape, friction mounts — and involves **no soldering of any kind**. This is as much a purchasing constraint as a build rule: most modules ship with headers unsoldered, so order everything with **headers pre-soldered** (or substitute a pre-soldered board); anything that arrives unsoldered is set aside for the P3 build, never soldered during P2. Full functionality of the whole system must be proven here before P3 solders anything. P3 then builds the aid itself — the evaluated device is soldered once and housed; it is not a disposable prototype, and rework there is limited to fixes, not redesign.

The pointer-tracking hardware ([README §2.5](../README.md#25-pointer-tracking-stack-vision-uwb-imu) — two wide-FOV camera modules, the UWB module pair, the pointer tracking marker (printed ArUco/AprilTag); [README §3.1](../README.md#31-pointer--electronics)/[§3.2](../README.md#32-wearable--electronics)) joins the P2 purchase list under the same pre-soldered rule; T7–T8 below validate the tracking tiers on the breadboards, and the module and board-level implementation choices are pinned at this gate ([README §9](../README.md#9-open-items)). The tracking hardware is part of the final design — the gates decide *how* it is built, never *whether*.

## Bring-up safety

- Check battery-cell polarity twice before each insertion; never park a bare cell on metal.
- First power-up of each device through a multimeter inline (mA range) or a current-limited USB source.
- Grounds common per device: MCU GND, sensor GND, amp GND, battery negative all on one rail.
- P2 bench sessions run from **bench USB** (the boards' own USB ports, fed from a current-limited source or the bench power banks), not the battery rail, until the power test T0 passes; battery wiring is validated on the bench (T0) before any untethered use. Watch for brownout-type resets on a weak or current-limited bench source — that is bench-source behavior, not a device failure (a microcontroller-class board resets an underpowered rail rather than running it dirty; the raspi-class boards' power-path behavior is characterized at T0 per the selected class).
- Power off before any wiring change; hot-plugging I²C/I²S is how modules die. There is no OS image to corrupt unless the selected board class runs one (the raspi-class wearable does — power-off behavior then follows its boot discipline); the rule is module safety. Firmware flashing over the boards' USB ports (the flashing/debug path — [README §3.1](../README.md#31-pointer--electronics)/[§3.2](../README.md#32-wearable--electronics)) is bench infrastructure alongside the wiring.

## Wiring maps

> **To be drawn up once components are selected.** The per-device pin assignments below the selected MCU boards, sensors, amplifiers, and power path — including the UWB SPI bus and the camera interface strategy — are recorded here before the bench phase starts. The constraints they must satisfy: avoid strapping pins on devkit-class boards and keep the debug UART unshared; share one I²C bus between the IMU and the ToF sensor on the pointer (distinct fixed addresses); give each amplifier its own channel-select strapping on the wearable; the sensor rail comes from each board's own 3.3 V source per the map (devkit-class regulators, or the raspi-class power path); and leave the battery paths (charge board → protection → cell) common-grounded with the rest of each device. Under the dual-compatible criterion (second clearing 2026-09-29), the maps are drawn only after the re-selection: until then, both an esp32-s3 devkit and a raspi-class wearable stay adoptable.

**Interface pressures the selection must resolve (P2 decisions, risk 8).** On the pointer, the UWB module needs an SPI bus plus IRQ/reset. On the wearable, the two head cameras need a capture strategy against the selected boards (camera mux / alternate-frame capture on one interface / a board with more pins or a second port / a camera co-processor); note the interface split the dual-compatible criterion carries — DVP on esp32-s3-class devkits, CSI on raspi-class boards. The T7/T8 gates settle both against the chosen boards.

## Test matrix

Acceptance derives from the §4.2 budget (motion-to-sound ≤ 100 ms). Run order = dependency order; log every session to CSV.

### T0 — Power rails

- Setup: USB-C charge/protect board + battery cell per device (1S both devices: slim Li-ion pouch on the pointer, protected 18650 on the wearable — the cell classes are carried over from the researched selection unless the compatible list revises them), multimeter inline.
- Procedure: measure rail voltage under idle and full-load (radio TX + amp playing, and — once the tracking extras are fitted — the tracking tier at full duty: both cameras capturing per the capture strategy + UWB ranging); 30 min soak. Camera streaming is the largest new draw: the battery budget is re-checked here with tracking active.
- Acceptance: the sensor rail stable within ±3% under load, with **no brownout resets logged** after the soak; charge-board protection trips on short test; no thermal runaway — the housing-temp check by touch after soak is a **logged metric** (`temp_c`), first-class on the wearable under the tracking-duty soak.
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

- Setup: wearable breadboard with the audio output path on the board (a mono I²S Class-D amplifier pair on devkit-class I²S, or the selected board class's audio path — one per ear, channel-strapped where the path allows); full renderer (generic-HRTF azimuth, carrier-pitch elevation cue, `g(D)` gain) at 48 kHz / 128-sample buffers through the selected output path.
- Procedure: 10 min run; `esp_timer_get_time()` around the render callback per buffer; count I²S underruns; sweep azimuth sectors + distance classes to cover the table.
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
- Procedure: (a) place the pointer at tape-measured known poses across the forward hemisphere (0.5–3 m; azimuth sweep; aimed above/below head level) — log position error and tracking rate per pose; (b) record where the tag leaves each camera's FOV envelope — the tier-switch boundary; (c) 10 min co-residence run at trigger-gated duty (button held throughout, [README §1.3](../README.md#13-interaction-rule)): `esp_timer_get_time()` per render buffer and I²S underrun count, as in T4, with tracking running; (d) **low-light/contrast check**: with tracking running, dim the room stepwise toward evening-lighting levels — record the minimum illumination at which the tag still detects reliably (the tag is passive and needs scene light; watch exposure and motion blur at 15–30 Hz; a printed tag emits nothing, so there is no flash-sync interference to reject).
- Acceptance: position error ≤ ±5 cm at 0.5–2 m and ≤ ±10% beyond (provisional — finalize at the P2 gate); cadence ≥ 15 Hz; FOV envelope documented per camera; **the renderer is unharmed by co-residence** — ≤ 5 ms/buffer p99 and 0 underruns over the 10 min run (the risk-8 gate: if full-rate detection fails it, the detect-then-track scheme (detect every Nth frame) is forced here, before P3 — and the capture-side decision (interface retained / mux / co-processor) is settled at this gate against the selected boards, [README §3.8](../README.md#38-pointer-tracking-hardware--notes--contingencies)).
- Metric logged: `p_err_cm`, `track_rate_hz`, `fov_envelope_deg`, `render_ms`, `underruns`. The marker's PnP orientation is **not consumed and not gated** — the tracking tier is position-only ([README §2.5](../README.md#25-pointer-tracking-stack-vision-uwb-imu)); pointer orientation always comes from its IMU.

### T8 — UWB ranging & tier fallback (tracking tier, [README §2.5](../README.md#25-pointer-tracking-stack-vision-uwb-imu))

- Setup: UWB module pair on both breadboards; **driver bring-up first on both boards** — the selected UWB module's SPI driver/stack is validated against each selected board class (tag and anchor both) before any ranging run; tape-measured baselines; logging captures the active tier for every sound update.
- Procedure: (a) range error vs tape at 0.5–4 m line-of-sight; (b) body-blocked/NLOS profile (person between devices; pointer held behind the body) — characterization, the fallback tier absorbs degradation; (c) tier-switch run: sweep the pointer out of camera view into UWB-only and back, repeatedly, including behind-body holds; log the active tier per update and the placement continuity across switches.
- Acceptance: range error ≤ ±10 cm at ≤ 3 m LOS, update rate ≥ 10 Hz; NLOS profile documented (characterization, not a gate); **no placement jump beyond the T6 `D`-error envelope at any tier switch** — tier changes must be inaudible.
- Metric logged: `r_err_cm`, `uwb_rate_hz`, `tier`, `placement_jump_cm`.

## Logging format

One CSV per session: `session,test_id,timestamp_ms,metric,value,unit,notes` — drift curves (T1) and latency distributions (T3–T5) feed P4 calibration and thesis §3/§4 artifacts; the T7–T8 tier records (per-update active tier + pose error) feed the §2.5 realism claims. The CSV is produced by an automated pipeline, not hand transcription: **firmware event stream → host capture → reduction → row check** ([README §4.1](../README.md#41-firmware-modules), `bench-log` module).

**Device side — the `bench-log` event stream (both devices).** The instrumentation outputs one NDJSON event per line over the native USB CDC port (the same cable that flashes the board): device uptime `ts_ms` plus event family — sensor frames (quaternion, ToF reading + validity flag, UWB range + active tier, tracking-cycle detections/misses), render instrumentation (per-buffer `esp_timer_get_time()` deltas — the T4/T7 `render_ms` input — I²S underrun events, and the T5 stamp pairing a ToF sample id with the rendered-buffer-queue event), and link events (`seq` + TX timestamp at send on the pointer; `seq` + RX timestamp on the wearable — the one-way latency of T3 is computed post-hoc by pairing `(seq, tx)` against `(seq, rx)`, payload unchanged from [README §2.1](../README.md#21-devices)). The trigger-button state rides every event plus a 1 Hz heartbeat so reduction derives button-held windows mechanically ([§1.3](../README.md#13-interaction-rule) gating); reset reasons (`esp_reset_reason()`) are logged so T0's brownout check reads straight from the stream. When the logger task sits below the audio path's priority — like `pointer-track` — and the render timing is captured in-task by the timer, the instrumentation cannot perturb the render gate it measures. Untethered runs (the T3 walks at 2–10 m, the T5 approach) cannot trail a cable to the head strap: the event stream batch-appends to a flash partition (LittleFS; a 10-min session is tens of KB) and dumps over USB on reconnect into the same session stream, so a session never spans two ledgers.

**Operator entry — the command channel.** The host sends three commands over the same serial: `start <test_id>` / `stop`, `mark <label>` (station boundaries, surface changes, body-block segments, soak checkpoints, dimming steps), and `set <key>=<value>` (ground truths — tape-measured `d_true`, phone-inclinometer references, meter readings, caliper offset components). Every operator reading enters the same event stream as a timestamped event, so ground truths and annotations join the device rows in one ledger — no second notebook, no transcription drift.

**Host harness — capture, runbook, reduce.** The harness lives in `bench/`: `bench/capture.py` opens both USB CDC ports, generates the session ID (`YYYYMMDD-HHMM-<test_id>`), writes the raw per-device streams (`bench_logs/<session>/pointer.ndjson`, `wearable.ndjson`, plus echoed `start`/`mark`/`set` records), and relays commands; `bench/runbooks/<test>.yaml` lists each test's per-run stations with the threshold set for its acceptance rows (tagged with the protocol revision it quotes — this file stays the source of truth) and drives the session interactively ("distance 1.0 m, surface cardboard — aim, hold the button, Enter to `set` the truth"); `bench/reduce.py` joins both devices on session-relative `timestamp_ms` (a marker sent to both ports yields each device's uptime-clock offset with sub-millisecond skew; `esp_timer` is crystal-backed, so drift over one run is negligible) and emits the CSV above, deriving every metric the matrix names that the raw stream doesn't emit directly — T1 `pose_err_deg` (entered references) and `drift_deg_per_min` (slope over the 10-min window), T2 `rate_hz`/`miss_rate`, T3 `lat_ms` (p99)/`loss_pct`/`range_m`, T4/T7 `render_ms` (p99)/`underruns`, T6/T7/T8 error metrics vs the entered truths and `placement_jump_cm` across logged tier switches. The reducer then prints a per-run **acceptance check** against the runbook thresholds, and a `--parity` mode diffs a run's summary against the stored P2 session — the [P3 parity re-run](#p3-parity-re-run) numbers fall out of the same pipeline. Provenance is preserved without breaking the schema: `timestamp_ms` is normalized to session-relative ms, and each row's `notes` field states its origin (`device` vs `operator`), so touch-checked `temp_c` and meter-read `i_load` rows are never mistaken for autonomous readings.

## P3 parity re-run

After soldering, re-run T0–T8 on the soldered assemblies; acceptance identical, except the T7 FOV envelope is re-characterized for the housing-mounted cameras (housing geometry moves the lenses). Parity is the P3 exit criterion (§6) — **service workmanship, housing geometry, and jack wiring are the new variables**, so the breadboard numbers are the baseline to match, and the re-run is preceded by the in-house incoming inspection ([`assembly.md` §5 Incoming inspection](assembly.md#5-incoming-inspection-in-house-gate-before-any-power-up): visual solder-joint check against the assembly drawings, per-net continuity, current-limited first power-up): the soldered build is executed by an external local hand-solder service from the in-house design package (README §3.9, decision 2026-09-26), so nothing returned from the service is powered before inspection passes.
