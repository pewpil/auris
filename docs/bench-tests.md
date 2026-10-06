# Cane — Verification protocol: acceptance specification & evidence ledger

> **Decision 2026-10-01: there is no hardware bench phase.** The T0–T8 thresholds are **retained as the acceptance specification** — the numbers the design must meet, unchanged — but they are **not measured on breadboards**. Every gate is discharged by software or datasheet evidence, and the physical device measures itself in use (P4 calibration, P5 study).

This document therefore has two parts:

- **§1 — Acceptance specification (the T0–T8 matrix).** The thresholds, method outlines, and acceptance rows of the original bench protocol, **retained but not executed as measurement**. They are the target each evidence entry must satisfy, the reference vocabulary of the thesis writing, and the checklist the P3 smoke check and P4/P5 observation report against. Nothing here is claimed as measured.
- **§2 — The evidence program** (the S-series). What actually discharges each gate: the desktop pipeline simulator, SPICE, cycle/MAD budgets, Monte-Carlo error propagation, and the citation ledger — plus the P3 smoke check and the residual-evidence handoff to P4/P5.

**Evidence classes** (every entry in §2 is tagged, and the tags carry into the thesis):

| Tag | Meaning |
|---|---|
| **[SIM]** | simulated — the real `geometry`/`renderer`/tiering code running on a desktop PC against synthetic streams, or a circuit simulated in SPICE |
| **[BUD]** | budgeted — instruction/cycle counts of the compiled code scaled to the Pi 5's A76 cores against vendor cycle tables |
| **[PROP]** | propagated — the selected parts' datasheet figures seeded through the placement math into θ/φ/D error distributions (Monte-Carlo) |
| **[CITE]** | cited — datasheet/literature carry, used **only where no simulation or budget method exists**; such claims are labelled as citation-class and are never presented as measured |

**The one rule that matters:** where no software method exists for a claim, **the component datasheet is the evidence** — and the claim says so. Nothing is quietly upgraded from citation to measurement.

> **Always in force — first-power-up bring-up and incoming inspection.** Whenever hardware is *first* powered, the incoming inspection and current-limited first power-up of [`assembly.md`](assembly.md) apply: visual solder-joint check, per-net continuity + no-short rails, battery-path polarity, current-limited energization, rail verification with a multimeter **before** modules attach, and one firmware-flash/logger-visibility check. With no bench phase this is the project's only hardware-safety gate before P4 calibration, so it is not optional.

## 1. Acceptance specification (T0–T8 — retained, not executed as measurement)

> **What this section is for.** Every threshold below is retained unchanged — it is the number the design must meet, and each §2 evidence entry is judged against it. What is **not** claimed is that any of them was measured. No breadboard session, no T-matrix log, and no T-row "accepted" mark exists in this project. Where a row's procedure implies physical action, that action has moved to §2's evidence program or to P4/P5 observation; where no software method exists, the row is carried by citation and labelled as such.

**Purchasing consequence of the no-bench decision.** The old no-soldering rule ("every module ordered with headers pre-soldered, nothing soldered at the bench") is retired with the bench; the *purchasing* half survives as the **connector-ready rule** serving the P3 service build ([README §3](../README.md#3-hardware) constraint 2, [`purchase-list.md`](purchase-list.md) §10): modules arrive with headers/connectors fitted or consumer-ready, because nothing is hand-soldered in-house and fine-pitch bare ICs are excluded outright. The tracking hardware ([README §2.5](../README.md#25-pointer-tracking-stack-vision-uwb-imu)) stays in the cart under the same rule; the evidence gates decide *how* it is built, never *whether*.

### Bring-up safety (gate to anything energized)

> **With no bench phase this is the project's only hardware-safety gate before P4 calibration**, so it is applied at the P3 incoming inspection and again at first power-up, without exception.

- Check battery-cell polarity twice before insertion; never park a bare cell on metal.
- First power-up per device through a multimeter in-line (mA range) or a current-limited USB source.
- Grounds common per device: board ground, sensor ground, battery negative on one rail.
- Until the T0 power case is evidenced and the first power-up passes, both devices run from **USB/bank ports** (the pointer's USB-C charge board and the wearable's PD bank), never from exposed battery wiring. Watch for brownout-type resets on a weak source — that is source behaviour, not device failure.
- Power off before any wiring change; hot-plugging I²C/I²S is how modules die. On the raspi-5 wearable the same rule extends to its OS: power-off follows its boot discipline (no cutting power to a writing filesystem).
- Firmware flashing over the boards' USB ports is bring-up infrastructure alongside the wiring.

### Wiring maps → carrier-board design

> **The per-device pin/connect assignments are now a P3 design entry, not a bench artifact.** They are drawn as part of the carrier-board KiCad designs ([README §4.3](#43-eda-toolchain-kicad-and-mcp)) from [`hardware.md`](hardware.md)'s picks, and reviewed at the P2 software exit before the fab order is placed — which is when the bench used to hand them over. The map covers: the pointer's shared I²C bus (IMU + ToF at distinct fixed addresses), the UWB SPI + IRQ/reset, the wearable's I²S-DAC wiring + `[HiFiBerry-class ALSA overlay]`, the CSI cameras to the Pi's native lanes, and battery/charge lanes common-ground per device. Constraints: avoid strapping pins and keep the debug UART unshared on the pointer; independent per-ear channels on the wearable; sensor rail from each board's own regulator per the map.

**I²C + SPI + I²S are planned as one coordinated pin/overlay set (raspi-5 class).** The wearable needs the BNO085 IMU on I²C **and** the UWB anchor on SPI **and** the DAC on I²S simultaneously; on the Pi 5 those overlays contend for GPIO, so the Raspberry Pi documentation is explicit that "all other peripheral overlays that use conflicting GPIO pins must be disabled" and that any `dtparam`s enabling I²C or SPI on those pins must be commented out or inverted. The map therefore resolves the three overlays (and the CSI lanes) against one GPIO budget *as a whole*, picks non-conflicting pins, and records the exact `config.txt`/overlay set — it is not three independent per-row choices. The resolution is a **P2 design review item** (can the three overlays coexist as specified?) and its reality is **confirmed at first power-up** — P4 observes the DAC audio path and the UWB driver bring-up — with the USB-audio-dongle as the audio fallback if the I²S overlay cannot be made to coexist.

> **Ordering exposure on the wearable, stated plainly.** The carrier PCBs are fabricated at the **P2 exit**, while this overlay set is only *reviewed* there and confirmed at P4 first power-up — so a conflict between the planned I²C/SPI/I²S pin set and the kernel's actual behaviour is discovered **after** the boards exist, and costs a **re-fab**. There is no pre-fabrication bring-up step: the purchased compute board (2026-10-03) is not powered with these modules before the fab order. What absorbs it: **≥ 2 spare carrier PCBs per device** ([`assembly.md`](assembly.md) §4) and the schedule's **contingency 7**; and what avoids needing it: the review is done on paper against the Raspberry Pi overlay documentation before the order, with the **USB-audio-dongle** as the purchase-free fallback if the I²S overlay cannot be made to coexist. Recorded rather than solved, because the board class is now purchased and carries no alternative ([README §7](../README.md#7-risks-and-limitations) risk 12).

**Interface pressures settled by the selection, carried as evidence (risk 8).** On the pointer: UWB SPI + IRQ/reset. On the wearable: the two CSI lanes close the camera-mux question in hardware — the capture strategy (per-camera frame cadence at detection rate) is a *software* question answered by the S7 budget; the pointer's Wi-Fi joining the Pi's AP is exercised by the S3 loopback.

### Test matrix (specification — not executed)

Acceptance derives from the §4.2 budget (motion-to-sound ≤ 100 ms). Row order = dependency order, preserved so each row's evidence dependency in §2.3 is traceable. **Every "Logged:" line names the telemetry that *would* carry the row and that P4/P5 actually produces** — the `bench-log` module and the per-update active-tier record ([README §4.1](../README.md#41-firmware-modules)) make the log fields real on the worn device, so the measurement arrives from the evaluation phases rather than from a bench session.

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

- Setup: wearable with the audio output path live — the PCM5102A I²S DAC behind the **HiFiBerry-class ALSA overlay (the overlay's kernel-version support is cited at P2 and confirmed at first power-up before the path is committed)** or the USB-audio fallback; full renderer (generic-HRTF azimuth, carrier-pitch elevation cue, `g(D)` gain) at 48 kHz / 128-sample buffers.
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

- Setup: the **adopted camera set (1 or 2 CSI modules)** on the strap stations; printed ArUco/AprilTag marker (dictionary and physical size pinned at P2) on the pointer shell; tracking **co-resident with the full renderer**, at priority below the audio path.
- Procedure: (a) pointer at tape-measured known poses across the forward hemisphere (0.5–3 m; azimuth sweep; aimed above/below head level) — log position error and tracking rate per pose; (b) record **each** camera's FOV envelope — the tier-switch boundary; (c) 10 min co-residence run at trigger-gated duty: per-buffer render timing and underrun count as in T4, tracking running; (d) low-light/contrast check **per camera**: dim the room stepwise toward evening lighting — record the minimum illumination at which the tag still detects reliably.
- Acceptance: position error ≤ ±5 cm at 0.5–2 m and ≤ ±10 % beyond (provisional — pinned at P2 from the `[PROP]` distribution); cadence ≥ 15 Hz; FOV envelope documented per camera; **the renderer is unharmed by co-residence** — ≤ 2 ms/buffer p99, 0 underruns over the run (the risk-8 gate: if the budget margin does not hold, detect-then-track — detect every Nth frame — is adopted, before the permanent build); and **the camera count (1 or 2) is decided at P2** from the S7 FOV-geometry and co-residence budget, recorded with the per-camera FOV envelopes and the adopted detection cadence — with the tier-1 availability fraction and the real FOV envelope observed at P4/P5 from the per-update active-tier telemetry.
- Logged: `p_err_cm`, `track_rate_hz`, `fov_envelope_deg`, `render_ms`, `underruns`. The tag's PnP orientation is not consumed and not gated — position only.

**T8 — UWB ranging & tier fallback (tracking tier)**

- Setup: UWB module pair on both devices; **driver bring-up first on both boards** (the selected module's SPI driver confirmed against the Pi 5 (anchor) and the pointer (tag) at first power-up before any ranging is trusted); tape-measured baselines; the active tier logged per sound update.
- Procedure: (a) range error vs tape at 0.5–4 m line-of-sight; (b) body-blocked/NLOS profile — characterization, the fallback tier absorbs degradation; (c) tier-switch runs: sweep the pointer out of camera view into UWB-only and back, repeatedly, including behind-body holds; log the active tier per update and placement continuity across switches.
- Acceptance: range error ≤ ±10 cm at ≤ 3 m LOS; update rate ≥ 10 Hz; NLOS documented (characterization); **no placement jump beyond the T6 `D`-error envelope at any tier switch** — tier changes must be inaudible.
- Logged: `r_err_cm`, `uwb_rate_hz`, `tier`, `placement_jump_cm`.

### Telemetry format (the log survives; its producer moves to P4/P5)

The `bench-log` event stream is **kept and re-pointed** ([README §4.1](../README.md#41-firmware-modules)): it is no longer the instrumentation behind a bench session, it is the instrumentation behind **P4 calibration and the P5 study**. One CSV per session: `session,test_id,timestamp_ms,metric,value,unit,notes` — the automated pipeline (firmware/host event stream → `bench/capture.py` → `bench/runbooks/<test>.yaml` → `bench/reduce.py` with an acceptance check) produces it; no hand transcription. Both devices emit an NDJSON device event stream (sensor frames, render timers, link seq/tx–rx stamps, button state, reset reasons) over the pointer's native USB CDC — and, on the raspi-5 wearable, the same event structure over SSH/tethered USB from the `bench-log` module. The trigger-button state rides every event so `button-held` windows derive mechanically; operator entries arrive as `start`/`mark`/`set` commands with ground truths into the same session stream. The `--parity` mode of `reduce.py` retires with the parity re-run below.

### P3 functional smoke check (the build-acceptance gate)

**Parity is gone.** With no bench phase there is no T0–T8 re-run on the permanent assemblies, so P3 cannot exit on "the soldered build matches the breadboard". What replaces it is deliberately *not* a test — it is the question "does the soldered unit work":

1. **Incoming inspection passes** ([`assembly.md`](assembly.md)): visual solder-joint check, per-net continuity, no-short rails, battery-path polarity. Nothing returned from the service is powered before inspection passes.
2. **Current-limited first power-up**: rails verified with a multimeter before modules attach; `bench-log` stream live.
3. **Firmware flashed** on both devices and both boot cleanly (no brownout resets; the raspi-5 booted and shut down per its boot discipline).
4. **One end-to-end functional run**: the full pipeline audible, and one scripted placement run end to end (obstacle at a known pose; button held; sound azimuth follows the pointer).

**What the smoke check does not do:** it does not compare against a T-row, claim any threshold, or count as evidence for §1. A unit that passes all four still has every threshold carried by §2 evidence until P4/P5 telemetry observes it. *Service workmanship, housing geometry, and jack wiring are the new variables it can expose.*

## 2. The evidence program (S-series) — discharging the gates without hardware

This is the **sole** verification path. Each S-row answers one §1 acceptance row with the strongest evidence class available, in a fixed fallback order:

**`[SIM]` → `[BUD]` → `[PROP]` → `[CITE]`.** Run the real code on synthetic inputs if you can; budget the compiled code against the Pi 5's silicon if you cannot simulate it; propagate the selected parts' datasheet figures through the placement math if you cannot budget it; and where **no software method exists at all, the datasheet or the family literature carries the claim, labelled `[CITE]`**. The order is not a preference — it is the thesis's defence against a silent upgrade: a row that ends at `[CITE]` is visibly weaker than one that reaches `[SIM]`, and that difference is reported rather than hidden.

### 2.1 Scope and honest limits

The evidence program substitutes **modeling for measurement** row by row; it never redefines the §4.2 budget — it tests whether the design is *consistent* with it on paper, data feeds, and scaled computations. What it cannot deliver, and how the project carries the difference honestly:

- no **measured** drift, latency, or accuracies — the thesis's §4 claims rest on budget/simulation/citation evidence and are labelled by origin;
- the **co-residence** question (risk 8) becomes a cycle budget with margins named, not a run with underruns counted;
- the **tier-handoff distribution** (risk 9) — how often tracking actually falls from tier 1 to tier 2, and the real NLOS error spread — is **not simulable**. The *logic* is simulated; the *frequency* is cited and left to P4/P5 telemetry. This is the largest single residual in the project, §2.5 names it, and §7 risk 9 states it;
- the **real indoor magnetometer**, **real course surfaces**, **body-blocked link**, and **real low-light camera** behaviours remain unmeasured until the device exists — these become P4/P5 observation items (§2.5);
- the **protection/thermal** behaviours are carried by the modules' own datasheets since the charge/protection and thermal management are self-contained reference modules (and the physical inspection covers the first power-up).

> **What partly compensates.** The `bench-log` module and the per-update active-tier telemetry stay in the firmware ([README §4.1](../README.md#41-firmware-modules)), so residuals (i)–(vi) below are **measured on the real worn device** during P4 calibration and the P5 study — not on a breadboard, on the evaluated hardware, with real users. That is a better provenance than a bench session for several of them (the magnetometer disturbance in a real room, the link body-blocked by a real body), and a worse one for none. It is not a substitute for the pre-build gate; it is the measurement that survives the decision to have no pre-build gate.

### 2.2 Instrument inventory (all software-only)

| Instrument | What it is | Feeds |
|---|---|---|
| SPICE (`ngspice`) on [`bench/t0/t0-power-rails.kicad_sch`](../bench/t0/) | schematic-analyzer output → testbenches → simulation; regulators/dividers, inrush, PDN impedance | S0 |
| PC pipeline simulator (`bench/sim/`) — the real `geometry`, `renderer`, and tiering logic compiled for the desktop, driven by synthetic sensor streams | functional end-to-end and placement behavior | S1, S5–S7 |
| Cycle/MAD budgeting — render core measured on desktop hardware, scaled by instruction mix to the Raspberry Pi 5's 4 Cortex-A76 cores (vendor cycle tables `docs/hw-comparison.md`) | timing feasibility without a live loop | S4, S7 |
| Error-propagation Monte-Carlo — the selected parts' datasheet noise/bias/accuracy figures seeded through the placement math into θ/φ/D distributions | statistical error budgets the §7 risks can cite | S1, S2, S8 |
| Datasheet/literature citation ledger — per selected part: accuracy class, ambient limits, interface timings, protection behaviors | every claim that says "the component does this" gets a pinned source | S0, S2, S3, S4, S8 |

### 2.3 S-series mapping — the same gates, software/datasheet evidence

| S-test | Discharges | Instrument(s) | Evidence class | Thresholds carried forward |
|---|---|---|---|---|
| S0 — Power tree | T0 (electrical half) | SPICE `[SIM]` + citation ledger `[CITE]` | rail set-points & divider math exact; inrush approximate; PDN impedance useful; protection/thermal cited | ±3 % rail consistency becomes a *set-point tolerance* claim; the soak/thermal/short-trip rows stay physical (P3 inspection + module datasheets) |
| S1 — IMU error budget | T1 | `fusion` code on desktop over public IMU datasets + Monte-Carlo from the BNO085 datasheet noise/bias `[PROP]` | pitch/roll/yaw error as *predicted distributions* vs the T1 thresholds | static ≤ 2°, drift ≤ 0.5°/5 min become budget claims; the real indoor magnetometer bias is **untested until P4** |
| S2 — ToF accuracy | T2 | datasheet accuracy/mode table + family surface-matrix literature `[CITE]` | error class and cadence-caps as datasheet-cited claims; the driver's no-hit behavior cited | ±3 cm/±5 % and ≥ 20 Hz become datasheet-accuracies **for the nominal conditions**; actual course-surface behavior moves to P4 observation on the real course |
| S3 — Link latency bound | T3 | analytical 802.11 bound (serialization + contention math for the pointer-joins-Pi's-AP topology) + the protocol logic on desktop loopback `[BUD]` | a *bounded* p99 with assumptions named; per-protocol packet loss a clean-channel assumption | structural: real channel behavior, body-blocked loss → P4 observation |
| S4 — Render budget | T4 | render core measured on the desktop, scaled to the Pi 5 `[BUD]`; ALSA-buffering configuration argument `[CITE]` | the ≤ 2 ms/buffer p99 and underrun-free operation become budget + configuration claims for the Linux audio path | real scheduling jitter — **as-yet untested until the device runs**; the I²S-overlay support claim is kernel-version cited and **confirmed at first hardware power-up** |
| S5 — End-to-end placement sim | T5 | desktop pipeline simulator with synthetic sweeps `[SIM]` | all **functional** T5 rows verified: azimuth tracking, elevation flip, monotonic loudness, no-hit cases; the ≤ 100 ms p99 becomes a composed per-stage budget | the composed budget carries named risks instead of a measured p99 |
| S6 — Offset sensitivity | T6 | pipeline sim with ±2 cm-scaled offset perturbation `[SIM]` | verifies θ/φ/D error under the repeatability target; the physical caliper + D-vs-tape rows move to P4 calibration (they only need the assembled device) | the repeatability budget magnitude is carried into the placement error budget |
| S7 — Co-residence, FOV & camera count | T7 | per-frame detection cost for AprilTag-class QVGA detection on Pi 5 silicon at each **candidate camera count N ∈ {1, 2}** (published OpenCV/AprilTag benchmarks scaled) + render per buffer `[BUD]`; FOV envelope geometry from the camera intrinsics `[SIM]`; low-light exposure/motion-blur math from the camera datasheet `[CITE]` | **the camera count is decided here** — the risk-8 gate becomes a margin argument over the adopted count, with the detect-then-track fallback quantified as the pressure-relief valve; **the marker dictionary and physical tag size are pinned here** as config parameters (AprilTag 36h11 or ArUco 4×4/5×5, ~50–60 mm tag), so a later change is a config edit and not a re-buy; the low-light min-illum and the real FOV envelope stay untested until hardware runs — P4/P5 observation | margins must hold with the fallback counts detected; honest residual: a real underrun-free co-residence run is never recorded in this project |
| S8 — UWB & tiering | T8 | module-family datasheet DS-TWR accuracy/rate + literature NLOS characterization `[CITE]`; tier-switch continuity simulated in the pipeline `[SIM]` | ±10 cm-class LOS accuracy cited; the no-placement-jump-at-switch rule verified in simulation; inaudibility unchanged (P5) | real body-blocked profiles and real ranging rate are untested until hardware runs (P4 observation); **the tier-handoff frequency is not evidence in this row at all** — it is a P4/P5 telemetry measurement |

### 2.4 Artifacts

Every S-row writes machine-readable artifacts into `bench/sim/` — deterministic seeds saved with every stochastic run, the citation ledger pinning each claim's datasheet page, the budget sheets recording the instruction counts and cycle assumptions — so every S-row "accepted" maps to an artifact a defence panel can inspect. Thesis §4 labels every row by its evidence class, so no row can be read as measured when it is not:

| Label | Means |
|---|---|
| `P2-simulated` | `[SIM]` — real code on synthetic inputs |
| `P2-budgeted` | `[BUD]` — compiled-code counts scaled to the A76 |
| `P2-propagated` | `[PROP]` — datasheet figures through the placement math |
| `P2-cited` | `[CITE]` — datasheet/literature carry, no software method existed |

### 2.5 Residual-evidence handoff to P4/P5

The following are left explicitly **unmeasured until the device runs**. They become the thesis's limitations list, the P4/P5 observation agenda, and the annotations in §7's risk ledger — each one names where it is picked up:

| # | Residual | Class of evidence that could ever have settled it | Picked up at |
|---|---|---|---|
| i | indoor magnetometer disturbance on the real course | measurement only — no datasheet or simulation covers a real room | P4 (first empirical check of the fusion path) |
| ii | ToF behaviour on the actual course surfaces | measurement only | P4, pre-study |
| iii | link behaviour body-blocked by a real body | measurement only | P4 |
| iv | Linux audio-path jitter under real co-residence | measurement only | P4 (`render_ms`, `underruns` telemetry) |
| v | camera low-light minimum and the real FOV envelope | measurement only | P4/P5 |
| vi | tier-switch audibility | perceptual measurement | P5 |
| vii | **tier-handoff frequency and the NLOS error spread** (risk 9) | measurement only — **not simulable, not budgetable** | P4/P5 per-update active-tier telemetry |
| viii | thermal behaviour at tracking duty | measurement only | P3/P4 |
| ix | protection trips | measurement only | P3 incoming inspection |

Residual vii is the one the no-bench decision actually costs the most: the tracking tier's central claim — that the ladder degrades gracefully rather than collapsing — is evidenced only on its *logic*, and its *behaviour* rests on P4/P5 telemetry that does not exist yet.
