# Cane — Bench test protocol (two evidence paths)

The quality gates of this project are the T0–T8 matrix of the §4.2 budget (motion-to-sound ≤ 100 ms). This document holds **two evidence paths** for those gates:

- **Path A — hardware bench** (§1): the original protocol — the whole system proven on breadboards, zero soldering, module-level gates with logged metrics. Run **if this thesis phase executes the physical bench**.
- **Path B — software/data-sheet bench** (§2): selected quality assertions are validated as substitute with the same T-gate vocabulary (SPICE, PC-pipeline simulations, cycle/MAD budgets, and datasheet/literature carry-outs — no hardware benches). Run **if hardware benching is skipped** and the wrongness of untested parts of the results is the thesis acknowledges in its limitations.

The active path is a **co-researcher decision, adviser-informed** — it rewrites nothing else in this file: Path A runs as written if and when the bench phase happens; Path B produces its own artifacts if chosen; both keep the safety and inspection lanes (below). Evidence-class tags used in Path B: **[SIM]** (simulated), **[BUD]** (cycle/analytic budget), **[CITE]** (datasheet/literature citation), **[PROP]** (error-propagation Monte-Carlo).

> **Always in force — minimal hardware bring-up.** Whenever any hardware is *first* powered (any path, any phase), the incoming inspection and current-limited first power-up of [`assembly.md`](assembly.md): visual solder-joint check, per-net continuity + no-short rails, battery-path polarity, current-limited energization, rail verification with a multimeter **before** modules attach, and one firmware-flash/logger-visibility check. Neither path discharges this.

## 1. Path A — hardware component bench tests (run "in case" the bench phase executes)

**No-soldering rule.** Any phase that attaches components physically attaches them **non-permanently** — breadboards, dupont jumpers, zip ties, velcro, tape, friction mounts — and involves **no soldering of any kind**. This is as much a purchasing constraint as a build rule: most modules ship with headers unsoldered, so order everything with **headers pre-soldered** (or substitute a pre-soldered board); anything that arrives unsoldered stays for the permanent build, never hand-soldered at the bench. The tracking hardware ([README §2.5](../README.md#25-pointer-tracking-stack-vision-uwb-imu)) joins the cart under the same rule; the gates decide *how* it is built, never *whether*.

### Bring-up safety (gate to anything energized)

- Check battery-cell polarity twice before insertion; never park a bare cell on metal.
- First power-up per device through a multimeter in-line (mA range) or a current-limited USB source.
- Grounds common per device: board ground, sensor ground, battery negative on one rail.
- Bench sessions run from **bench USB/bank ports** (feeding the pointer's USB-C charge board and the wearable's PD bank), not exposed battery wiring, until the power test T0 passes. Watch for brownout-type resets on a weak source — that is bench-source behavior, not device failure.
- Power off before any wiring change; hot-plugging I²C/I²S is how modules die. On the raspi-5 wearable the same rule extends to its OS: power-off follows its boot discipline (no cutting power to a writing filesystem).
- Firmware flashing over the boards' USB ports is bench infrastructure alongside the wiring.

### Wiring maps

> **Drawn before the first wiring change once the cart arrives.** The per-device pin/connect assignments — the pointer's shared I²C bus (IMU + ToF at distinct fixed addresses), the UWB SPI + IRQ/reset, the wearable's I²S-DAC wiring + `[HiFiBerry-class ALSA overlay]`, the two CSI cameras to the Pi's two lanes, battery/charge lanes common-ground per device — are recorded here into [`bench/runbooks/`](bench/) informed by [`hardware.md`](hardware.md)'s picks. Constraints: avoid strapping pins and keep the debug UART unshared on the pointer; independent per-ear channels on the wearable; sensor rail from each board's own regulator per the map.

**I²C + SPI + I²S are planned as one coordinated pin/overlay set (raspi-5 class).** The wearable needs the BNO085 IMU on I²C **and** the UWB anchor on SPI **and** the DAC on I²S simultaneously; on the Pi 5 those overlays contend for GPIO, so the Raspberry Pi documentation is explicit that "all other peripheral overlays that use conflicting GPIO pins must be disabled" and that any `dtparam`s enabling I²C or SPI on those pins must be commented out or inverted. The wiring map therefore resolves the three overlays (and the CSI lanes) against one GPIO budget *as a whole*, picks non-conflicting pins, and records the exact `config.txt`/overlay set — it is not three independent per-row choices. Verified at first power-up (T4 confirms the DAC overlay; T8 confirms the UWB SPI driver), with the USB-audio-dongle as the audio fallback if the I²S overlay cannot be made to coexist.

**Interface pressures settled by the selection, checked at the gate (risk 8).** On the pointer: UWB SPI + IRQ/reset. On the wearable: the two CSI lanes close the camera-mux question in hardware — the capture strategy (per-camera frame cadence at detection rate) is validated by T7 as a *software* question; the pointer's Wi-Fi joining the Pi's AP is exercised by T3.

### Test matrix

Acceptance derives from the §4.2 budget (motion-to-sound ≤ 100 ms). Run order = dependency order; log every session to CSV (§ Logging format).

**T0 — Power rails**

- Setup: pointer on its 1S pouch + TP4056-C charge board; the wearable fed by its **USB-PD rail (5 V/5 A profile)**; multimeter/in-line USB meter.
- Procedure: idle and full-load rail readings (radio TX + DAC audio on the pointer; cameras streaming + renderer + UWB on the wearable at tracking duty); 30 min soak; per the capture strategy.
- Acceptance: sensor rail is conditions-stable within ±3 % under load, **no brownout resets logged** after the soak; charge-board protection trips on short test (pointer); wearable PD-profile current within the bank's continuous rating at full duty; no thermal runaway — the housing-temp touch check after soak is a logged metric (`temp_c`).
- Logged: `v_rail`, `i_load`, `t_soak`, `temp_c`.

**T1 — IMU orientation & drift (both devices)**

- Setup: device on a flat level reference; second reference: phone inclinometer app.
- Procedure: (a) static pitch/roll at 5 poses vs reference; (b) gyro-only relative yaw drift over 10 min after bias calibration, both devices powered; (c) magnetometer disturbance characterization: heading error near a steel table/rebar vs open area.
- Acceptance: static pitch/roll error ≤ 2°; pitch/roll drift ≤ 0.5° per 5 min; combined relative-yaw drift ≤ 2°/min (the re-zero button bounds it in use); disturbance is characterization (records `mag_bias_deg`), not a gate — mitigated by re-zero.
- Logged: `pose_err_deg`, `drift_deg_per_min`, `mag_bias_deg`.

**T2 — ToF accuracy & cadence**

- Setup: pointer clamped on a stand; tape-measured distances; surfaces: white foam board, cardboard, dark fabric (three reflectances).
- Procedure: 30 s continuous ranging at 0.3, 0.5, 1, 2, 3, 4 m per surface; record error, hit rate, cadence.
- Acceptance: error ≤ ±3 cm at ≤ 2 m and ≤ ±5 % beyond, on ≥ 2 of 3 surfaces; sustained cadence ≥ 20 Hz; invalid-read rate < 5 %; no-target behavior documented.
- Logged: `d_true`, `d_meas`, `rate_hz`, `miss_rate`.

**T3 — Device-to-device link**

- Setup: pointer ↔ wearable ping-pong over IP/Wi-Fi (pointer joins the Pi's AP), 12–28 B payload (`d` + quaternion + seq), 10–30 Hz.
- Procedure: (a) 10 min run logging one-way latency (seq-timestamp pairing) and loss; (b) repeat at 2, 5, 10 m including body-blocked (person between devices).
- Acceptance: one-way ≤ 10 ms at p99; loss < 1 % at ≤ 10 m indoor with body between devices.
- Logged: `lat_ms`, `loss_pct`, `range_m`.

**T4 — Renderer load**

- Setup: wearable with the audio output path live — the PCM5102A I²S DAC behind the **HiFiBerry-class ALSA overlay (overlay support verified at this gate before the path is committed)** or the USB-audio fallback; full renderer (generic-HRTF azimuth, carrier-pitch elevation cue, `g(D)` gain) at 48 kHz / 128-sample buffers.
- Procedure: 10 min run; monotonic-clock timing around the render callback per buffer; count underruns; sweep azimuth sectors + distance classes.
- Acceptance: render ≤ 2 ms per 128-sample buffer at p99 (a 2.67 ms period at 48 kHz); underrun count = 0 over 10 min.
- Logged: `render_ms`, `underruns`.

**T5 — End-to-end latency & placement sanity**

- Setup: both devices on the full pipeline; obstacle at known pose.
- Procedure: (a) software timestamps from ToF sample to rendered buffer queue, 10 min of button sweeps; (b) sweep the pointer across a wide obstacle and verify the sound's azimuth tracks the hit; aim above/below head level and verify the elevation cue flips at head height; walk toward the obstacle with the button held and verify monotonic loudness growth.
- Acceptance: motion-to-sound ≤ 100 ms p99; azimuth tracks without jumps; elevation flips at head height; loudness monotonic in `D`.
- Logged: `e2e_ms`, `placement_ok`, `notes`.

**T6 — Offset calibration repeatability**

- Setup: flat grip fixture; ruler/calipers.
- Procedure: measure the head-to-pointer offset components `o` 5×; enter into config; run `geometry` and check `D` against tape-measured head-to-hit distance at 3 poses.
- Acceptance: repeatability ± 2 cm per component; `D` error ≤ ± 6 cm at 1–3 m with nominal grip.
- Logged: `o_component`, `D_err_cm`.

**T7 — Vision pointer tracking (tracking tier)**

- Setup: the **adopted camera set (1 or 2 CSI modules)** on a head-form fixture at the strap stations; printed ArUco/AprilTag marker (dictionary and physical size pinned at this gate) on the pointer shell; tracking **co-resident with the full renderer**, at priority below the audio path.
- Procedure: (a) pointer at tape-measured known poses across the forward hemisphere (0.5–3 m; azimuth sweep; aimed above/below head level) — log position error and tracking rate per pose; (b) record **each** camera's FOV envelope — the tier-switch boundary; (c) 10 min co-residence run at trigger-gated duty: per-buffer render timing and underrun count as in T4, tracking running; (d) low-light/contrast check **per camera**: dim the room stepwise toward evening lighting — record the minimum illumination at which the tag still detects reliably.
- Acceptance: position error ≤ ±5 cm at 0.5–2 m and ≤ ±10 % beyond (provisional — finalize at this gate); cadence ≥ 15 Hz; FOV envelope documented per camera; **the renderer is unharmed by co-residence** — ≤ 2 ms/buffer p99, 0 underruns over the run (the risk-8 gate: if full-rate detection fails it, detect-then-track — detect every Nth frame — is forced here, before the permanent build); and **the camera count (1 or 2) is decided at this gate** — recorded with the per-camera FOV envelopes, the adopted detection cadence, and the tier-1 availability fraction taken from the per-update active-tier telemetry.
- Logged: `p_err_cm`, `track_rate_hz`, `fov_envelope_deg`, `render_ms`, `underruns`. The tag's PnP orientation is not consumed and not gated — position only.

**T8 — UWB ranging & tier fallback (tracking tier)**

- Setup: UWB module pair on both breadboards; **driver bring-up first on both boards** (the selected module's SPI driver validated against the Pi 5 (anchor) and the Pointer (tag) before any ranging run); tape-measured baselines; the active tier logged per sound update.
- Procedure: (a) range error vs tape at 0.5–4 m line-of-sight; (b) body-blocked/NLOS profile — characterization, the fallback tier absorbs degradation; (c) tier-switch runs: sweep the pointer out of camera view into UWB-only and back, repeatedly, including behind-body holds; log the active tier per update and placement continuity across switches.
- Acceptance: range error ≤ ±10 cm at ≤ 3 m LOS; update rate ≥ 10 Hz; NLOS documented (characterization); **no placement jump beyond the T6 `D`-error envelope at any tier switch** — tier changes must be inaudible.
- Logged: `r_err_cm`, `uwb_rate_hz`, `tier`, `placement_jump_cm`.

### Logging format

One CSV per session: `session,test_id,timestamp_ms,metric,value,unit,notes` — the automated pipeline (firmware/host event stream → `bench/capture.py` → `bench/runbooks/<test>.yaml` → `bench/reduce.py` with an acceptance check and `--parity` mode) produces it; no hand transcription. Both devices emit an NDJSON device event stream (sensor frames, render timers, link seq/tx–rx stamps, button state, reset reasons) over the pointer's native USB CDC — and, on the raspi-5 wearable, the same event structure over SSH/tethered USB from the `bench-log` module. The trigger-button state rides every event so `button-held` windows derive mechanically; operator entries arrive as `start`/`mark`/`set` commands with ground truths into the same session stream.

### P3 parity re-run

After soldering (permanent build), re-run T0–T8 on the permanent assemblies with acceptance identical, except the T7 FOV envelope re-characterized for the housing-mounted cameras (housing geometry moves the lenses). Parity is the P3 exit criterion in the hardware-bench mode — **service workmanship, housing geometry, and jack wiring are the new variables**. The re-run is always preceded by the in-house incoming inspection ([`assembly.md`](assembly.md)): nothing returned from the service is powered before inspection passes. *(Under Path B the parity re-run is not executed; §2's Residual-evidence handoff names what replaces its reassurance.)*

## 2. Path B — software / data-sheet bench tests (if the bench phase is skipped)

### 2.1 Scope and honest limits

Path B substitutes **modeling for measurement** test by test; it never redefines the budget of §4.2 — it validates whether the design is *consistent* with it on paper, data feeds, and scaled computations. What Path B cannot deliver, and how the project carries the difference honestly:

- no **measured** drift, latency, or accuracies — the thesis's §4 claims rest on budget/simulation/citation evidence and are labeled as such;
- the **co-residence** question (risk 8) becomes a cycle budget with margins named, not a run with underruns counted;
- the **real indoor magnetometer**, **real course surfaces**, **body-blocked link**, **real low-light camera** behaviors remain unmeasured until the device exists — these become P4/P5 observation items (surviving, unchanged);
- the **protection/thermal** behaviors are carried by the modules' own datasheets since the charge/protection and thermal management are self-contained reference modules (and the physical inspection covers the first power-up).

### 2.2 Instrument inventory (all software-only)

| Instrument | What it is | Feeds |
|---|---|---|
| SPICE (`ngspice`) on [`bench/t0/t0-power-rails.kicad_sch`](../bench/t0/) | schematic-analyzer output → testbenches → simulation; regulators/dividers, inrush, PDN impedance | S0 |
| PC pipeline simulator (`bench/sim/`) — the real `geometry`, `renderer`, and tiering logic compiled for the desktop, driven by synthetic sensor streams | functional end-to-end and placement behavior | S1, S5–S7 |
| Cycle/MAD budgeting — render core measured on desktop hardware, scaled by instruction mix to the Raspberry Pi 5's 4 Cortex-A76 cores (vendor cycle tables `docs/hw-comparison.md`) | timing feasibility without a live loop | S4, S7 |
| Error-propagation Monte-Carlo — the selected parts' datasheet noise/bias/accuracy figures seeded through the placement math into θ/φ/D distributions | statistical error budgets the §7 risks can cite | S1, S2, S8 |
| Datasheet/literature citation ledger — per selected part: accuracy class, ambient limits, interface timings, protection behaviors | every claim that says "the component does this" gets a pinned source | S0, S2, S3, S4, S8 |

### 2.3 S-series mapping — the same gates, software/datasheet evidence

| S-test | Replaces | Instrument(s) | Evidence class | Thresholds carried forward |
|---|---|---|---|---|
| S0 — Power tree | T0 (electrical half) | SPICE `[SIM]` + citation ledger `[CITE]` | rail set-points & divider math exact; inrush approximate; PDN impedance useful; protection/thermal cited | ±3 % rail consistency becomes a *set-point tolerance* claim; the soak/thermal/short-trip rows stay physical (P3 inspection + module datasheets) |
| S1 — IMU error budget | T1 | `fusion` code on desktop over public IMU datasets + Monte-Carlo from the BNO085 datasheet noise/bias `[PROP]` | pitch/roll/yaw error as *predicted distributions* vs the T1 thresholds | static ≤ 2°, drift ≤ 0.5°/5 min become budget claims; the real indoor magnetometer bias is **untested until P4** |
| S2 — ToF accuracy | T2 | datasheet accuracy/mode table + family surface-matrix literature `[CITE]` | error class and cadence-caps as datasheet-cited claims; the driver's no-hit behavior cited | ±3 cm/±5 % and ≥ 20 Hz become datasheet-accuracies **for the nominal conditions**; actual course-surface behavior moves to P4 observation on the real course |
| S3 — Link latency bound | T3 | analytical 802.11 bound (serialization + contention math for the pointer-joins-Pi's-AP topology) + the protocol logic on desktop loopback `[BUD]` | a *bounded* p99 with assumptions named; per-protocol packet loss a clean-channel assumption | structural: real channel behavior, body-blocked loss → P4 observation |
| S4 — Render budget | T4 | render core measured on the desktop, scaled to the Pi 5 `[BUD]`; ALSA-buffering configuration argument `[CITE]` | the ≤ 2 ms/buffer p99 and underrun-free operation become budget + configuration claims for the Linux audio path | real scheduling jitter — **as-yet untested until the device runs**; the I²S-overlay support claim is kernel-version cited and **confirmed at first hardware power-up** |
| S5 — End-to-end placement sim | T5 | desktop pipeline simulator with synthetic sweeps `[SIM]` | all **functional** T5 rows verified: azimuth tracking, elevation flip, monotonic loudness, no-hit cases; the ≤ 100 ms p99 becomes a composed per-stage budget | the composed budget carries named risks instead of a measured p99 |
| S6 — Offset sensitivity | T6 | pipeline sim with ±2 cm-scaled offset perturbation `[SIM]` | verifies θ/φ/D error under the repeatability target; the physical caliper + D-vs-tape rows move to P4 calibration (they only need the assembled device) | the repeatability budget magnitude is carried into the placement error budget |
| S7 — Co-residence budget | T7 | per-frame detection cost for AprilTag-class QVGA detection on Pi 5 silicon at the **adopted camera count N ∈ {1, 2}** (published OpenCV/AprilTag benchmarks scaled) + render per buffer `[BUD]`; FOV envelope geometry from the camera intrinsics `[SIM]`; low-light exposure/motion-blur math from the camera datasheet `[CITE]` | the risk-8 gate becomes a margin argument over the adopted count, with the detect-then-track fallback quantified as the pressure-relief valve; the low-light min-illum and real FOV envelope untested until hardware runs — P4/P5 observation | margins must hold with the fallback counts detected; the honest residual: a real underrun-free co-residence run is never recorded in this path |
| S8 — UWB & tiering | T8 | module-family datasheet DS-TWR accuracy/rate + literature NLOS characterization `[CITE]`; tier-switch continuity simulated in the pipeline `[SIM]` | ±10 cm-class LOS accuracy cited; the no-placement-jump-at-switch rule verified in simulation; inaudibility unchanged (P5) | real body-blocked profiles and real ranging rate are untested until hardware runs (P4 observation) |

### 2.4 Artifacts

All Path-B sessions write machine-readable artifacts into `bench/sim/` — deterministic seeds saved with every stochastic run, the citation ledger pinning each claim's datasheet page — so every Path-B "accepted" maps to an artifact a defence panel can inspect, the same way every Path-A "Metric logged" maps to a CSV session. The thesis artifact chain accepts both proof types; thesis §4 labels its rows by origin: `P2-path-A measured` or `P2-path-B simulated/budgeted/cited`.

### 2.5 Residual-evidence handoff (both paths meet here)

Path B leaves the following residuals explicitly **unmeasured until the device runs** — the thesis's limitations and the P4/P5 phases carry them as observation items, and the risk ledger (§7) annotates them: (i) indoor magnetometer disturbance on the real course (P4 checks); (ii) ToF behavior on the actual course surfaces (P4 pre-study); (iii) link behavior body-blocked (P4); (iv) Linux audio-path jitter under real co-residence (P4); (v) camera low-light minimum and real FOV envelope (P4/P5); (vi) tier-switch audibility (P4/P5, perceptual); (vii) thermal behavior at tracking duty (P3/P4); (viii) protection trips (P3 inspection). Path A records all eight in its matrix instead.
