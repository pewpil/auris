# Cane — Achievability of exact, head-relative, self-contained sound placement

This document justifies, against the locked architecture ([§2](../README.md#2-system-overview), [§3](../README.md#3-hardware), [§4](../README.md#4-software)) and the verified literature ([§8](../README.md#8-where-things-live)), the central achievability claim of the Cane concept: that the sound rendered for a laser hit is projected **exactly where the hit is relative to the user's head**, with life-realism on the perceptually dominant dimensions, using **no external source of any kind** beyond the pointer and the head-mounted wearable. It states the claim precisely, shows the geometry is exact rather than estimated, audits self-containment input by input, accounts for each perceptual cue against published evidence, and then enumerates the caveats, their bounds, and who mitigates them. The claim is stated so that it is **falsifiable** — the evaluation plan in [§5](../README.md#5-evaluation-plan) measures exactly the ways it could fail.

## 1. The claim

While the pointer button is held, the wearable renders a sound as if it were emitted from the laser hit point $q$ — the user hears it coming from the obstacle's direction, at the obstacle's distance (near = loud, far = faint), and above/at/below head level — and the rendering stays **locked to $q$** in the real world while the user moves their head and hand. Nothing outside the two worn/held devices participates in producing this.

Two clarifications of scope, because "life realism" needs definition:

- The **spatial** attributes are the claim: azimuth, distance, elevation, and their live updating under head/hand motion. The source *signal* is a designed sonification — a laser spot has no natural sound; the concept deliberately assigns the hit a synthetic voice. Realism is claimed for the *placement*, not the *timbre of a nonexistent natural source*.
- The sound exists **exactly as long as the laser exists** (button held), then goes silent — that is the interaction metaphor ([§1.3](../README.md#13-interaction-rule)), not a fidelity shortfall.

## 2. The placement is algebraically exact, not sensor-fused guesswork

The egocentric placement vector is ([§2.3](../README.md#23-coordinate-frames-and-sound-placement)):

$$
r \;=\; R_H^{-1}\!\left[\left(p_P + d\,\hat{u}_P\right) - p_H\right] \;=\; o + d\,R_H^{-1}\,\hat{u}_P
$$

with $\theta = \operatorname{atan2}(r_y, r_x)$ (azimuth), $\phi = \operatorname{atan2}\!\left(r_z,\, \sqrt{r_x^2 + r_y^2}\right)$ (elevation), and $D = \lVert r \rVert$ (head-to-hit distance). Three consequences follow directly:

- **No position tracking is needed.** The head and pointer room positions $p_H$, $p_P$ appear in the raw expression only as their difference $p_P - p_H$ — body geometry — which is the calibrated constant $o$. The runtime formula consumes two orientations, one range, and one constant. There is nothing for an external tracking system to provide: the formula does not ask for a position.
- **Everything is head-referenced by construction.** $r$ is the vector from the head to the hit, expressed in the head's frame — so azimuth, elevation, and loudness ($D = \lVert r \rVert$) are the values a listener at the head would experience. The pointer never biases the placement; it is a ranging probe whose reading $d$ is *converted* to head-referenced quantities through $o$ and the relative aim direction.
- **The error budget is finite and enumerable.** Given sensor errors $\varepsilon_R$ (orientation), $\varepsilon_d$ (range), $\varepsilon_o$ (offset), the egocentric error is bounded by $\lVert \varepsilon_o \rVert + \lVert \varepsilon_d \rVert + d\lVert \varepsilon_R \rVert$-scale terms. Concretely at the bench budgets ([`bench-tests.md`](bench-tests.md)): offset repeatability ±2 cm, ToF ±3 cm at ≤2 m, and 1° of aim error sweeping the hit ~7 cm laterally at 4 m — all against human localization acuity of order degrees in azimuth and >1 dB in level, so the geometry contributes errors *below* the perceptual threshold of the rendered cues.

This is the structural reason the system can be exact with only onboard sensing: the **egocentric frame is the listener's own frame**, and in that frame the only positional quantity that exists — $o$ — is a constant measured once.

## 3. Self-containment audit: why nothing external is needed, wanted, or missed

The requirement is strict: **no sources other than the pointer and the wearable** — no cameras, beacons, motion capture, UWB anchors, phone tethering, or room instrumentation at runtime. Every placement input has an onboard source ([§2.4](../README.md#24-positioning-relative-pose-only)):

| Placement input | Onboard source | Why no external alternative would help |
|---|---|---|
| Head orientation $R_H$ (full 3-D) | head-worn 9-DoF IMU, Madgwick-class fusion | IMU orientation for binaural head tracking is validated at Arduino/ESP32-class hardware (Franček et al. 2023, [§5 lit](auris-thesis/literature/05-head-pose-sensing.md)); gravity and Earth's magnetic field are passive physical references, not infrastructure |
| Pointer orientation (quaternion) | pointer's own 9-DoF IMU, fused onboard | same evidence base; two IMUs also cannot measure their separation, which is exactly why $o$ is calibrated instead of tracked |
| Hit range $d$ | VL53L1X (940 nm) on the pointer, active ranging | ranging is intrinsic to the pointer — the laser must hit the obstacle to have a hit, so the distance is measured where it is defined |
| Head-to-pointer offset $o$ | one-time calibration (body geometry) | a constant cannot degrade in the field; drift is not a property of constants |

The deeper point is that **external aiding could not improve the placement even if permitted**: it would at best estimate $R_H$, $R_P$, $d$, or $o$ more accurately — and those are already bounded well inside the perceptual tolerances of the rendered cues (§2 above). What external systems would actually add is cost, setup burden, calibration, fragility, and privacy exposure, while the formula's inputs stay the same. The one quantity external tech famously provides — absolute room position of the user — is the only input the placement *never consumes*; the head's dead-reckoned position exists for telemetry logging only (Harle 2013; Foxlin 2005, [§5 lit](auris-thesis/literature/05-head-pose-sensing.md), show inertial position is unusable for room-scale absolute tracking — which is why the architecture never depends on it).

## 4. Achievability of each perceptual cue (with evidence)

- **Azimuth (left/right/behind)** — *high fidelity, robust to generic HRTFs.* ITD/ILD azimuth cues are the channel least degraded by non-individualized HRTFs: with head tracking, azimuth discrimination holds up while elevation degrades (Planinec et al. 2023). HRTF binaural filtering beats stereo panning for orienting to static and moving sources (Ferrand et al. 2019), and blind listeners localize HRTF-rendered virtual sources at usable accuracy (Wersényi 2012), with adaptation to non-individualized cues improving reliably over short training (Mendonça 2014).
- **Distance (near = loud, far = faint)** — *the most exact cue in the system.* The gain keys to $D = \lVert r \rVert$, the true head-to-hit distance, updated live from ranging as the user walks with the button held (§2). Encoding obstacle distance into an auditory parameter is a learnable, mobility-grade mapping (Bujacz et al. 2011), and the ETA literature supplies the established parameter-mapping vocabulary (Bujacz & Strumiłło 2016).
- **Elevation (above/at/below head)** — *reliable and learnable, acoustically stylized.* Real pinna elevation cues are spectral and demand individualized HRTFs — precisely the channel that degrades on generic HRTFs (Planinec et al. 2023). Cane therefore renders elevation as an explicit carrier-pitch cue (below head = lower carrier, above = higher, level = reference), following the vOICe's elevation→pitch precedent (Meijer 1992) whose expert users localize synthesized sounds in real space (Ward & Meijer 2010). The information is conveyed robustly on any earphones; what is traded away is photorealism of the *timbre*, not legibility of *where*.
- **Dynamics (the sound stays put while the user moves)** — *high fidelity, and the system's structural advantage.* Because placement is recomputed from live dual-IMU orientation + live ranging each frame (20–50 Hz refresh), the rendered source stays fixed in the room while the head rotates — the head-locked architecture of Headlock (Fiannaca et al. 2014), driven here by IMU-grade head tracking that is validated adequate for binaural synthesis (Franček et al. 2023). Head-tracked motion parallax is also the strongest available counter to front–back confusion and in-head perception with generic HRTFs, so the closed loop is doing perceptual work, not just updating coordinates.

## 5. Latency and compute achievability

- The motion-to-sound budget is ≤100 ms across five stages ([§4.2](../README.md#42-latency-budget-motion-to-sound-target--100-ms)): fusion ≤10 ms, ToF cadence ≤50 ms, ESP-NOW hop ≤10 ms, render ≤5 ms per 48 kHz/128-sample buffer. Each stage's bench gate is a P2 exit criterion with concrete thresholds ([`bench-tests.md`](bench-tests.md) T0–T6).
- The link choice is latency-motivated: BLE connection intervals and their jitter threaten the budget audibly (placement *wobble* reads as unreality), while connectionless ESP-NOW at ~2–8 ms one-way keeps the ≤10 ms stage with margin; Bluetooth *audio* is categorically excluded (150–300 ms by protocol) and the design mandates wired earphones ([§2.1](../README.md#21-devices)).
- The wearable MCU is sized for the renderer: the ESP32-S3's SIMD vector DSP and per-core FPU run stereo HRTF filtering + the carrier-pitch cue + gain inside the per-buffer budget with headroom for later realism additions (e.g., a distance low-pass); render CPU load is gated at P2 (T4: ≤5 ms p99, zero underruns over 10 min).
- Residual latency is *perceivable but honest*: ~100 ms reads as a responsive reflector, not as a broken causal link; the sweep test (T5) verifies placement tracks a moving aim without jumps.

## 6. Caveats and considerations — the honest ledger

Each caveat below is bounded, has a named mitigation, and an owner phase; none is a stack failure, and all are measurable by the existing plan.

- **C1 — Generic-HRTF artifacts.** Front–back confusion and weaker externalization occur with non-individualized HRTFs, especially for static sources. Mitigations: live head tracking (motion parallax resolves front–back dynamically), air-conduction earphones, redundant loudness/elevation cues, and a short familiarization period in the pilot ([§5](../README.md#5-evaluation-plan)). Quantified directly by the localization-accuracy metric — the plan does not assume this problem away.
- **C2 — Elevation is stylized.** The carrier-pitch cue conveys above/at/below robustly but does not replicate pinna spectra; photoreal elevation on generic HRTFs is not achievable on this budget, and chasing it would trade the reliable azimuth channel for an unreliable one. Upgrade path: per-user HRTF measurement later requires no architecture change.
- **C3 — IMU drift and magnetic disturbance.** Pitch/roll are gravity-anchored and effectively drift-free; yaw drifts slowly and the magnetometer is disturbed by ferromagnetic furniture (Roetenberg et al. 2005). Mitigation: the re-zero button bounds yaw error to user-action scale; P4 checks on the actual room ([§7](../README.md#7-risks-and-limitations) risk 2).
- **C4 — The offset is only approximately constant.** Hand pose varies (extending/reaching the arm, raising to aim down stairs): ±15 cm horizontal, up to ~±0.3 m vertical, perturbing $D$, $\theta$, $\phi$. Mitigation: calibrate at a natural grip; keep the $g(D)$ curve and pitch spread gradual so pose variation is a small perceptual change; observe real-pose variation in the pilot (§7 risk 7).
- **C5 — ToF physics.** The VL53L1X's ~4 m ceiling saturates the far end of the loudness curve; glass/dark/specular surfaces and strong ambient IR degrade returns; the default beam width blurs the hit spot at range. Mitigations: firmware ROI narrowing; P2 surface matrix before committing course props; TF-Luna (8 m, 2° FoV) as the drop-in alternate (§7 risk 5, [§3.7](../README.md#37-component-notes)).
- **C6 — Loudness is not linear distance.** Perceived level is nonlinear in dB and compresses at distance; the $g(D)$ curve is therefore calibrated empirically at P4, with a faint-but-audible floor so far hits never vanish (§7 risk 6).
- **C7 — Stereo requires two amps.** The MAX98357A is mono; binaural placement needs independent per-ear channels — two units, SD-strapped L/R; wired earphones only ([§3.7](../README.md#37-component-notes)).
- **C8 — No room acoustics.** Real distant sources are colored by air absorption and the room's reverberation; Cane has no room model and adds none. Loudness carries distance; a distance low-pass is an optional future add. Fake reverb would be decoration, not realism, and is deliberately omitted.
- **C9 — Position is never used for placement.** A user walking without the button held generates no sound updates; that is semantics (the hit "speaks" only while it exists), not tracking failure. Proximity during button-held walking is carried by live ranging (§2, [§2.4](../README.md#24-positioning-relative-pose-only)).
- **C10 — The claim's scope.** "Exact" refers to the geometry (§2): the algebra is exact and the sensor-error budget is sub-perceptual. "Life realism" refers to the cue accounting in §4: azimuth, distance, and dynamics are claimed at high fidelity; elevation is claimed as robust-and-learnable but stylized; reverberant realism is out of scope.

## 7. Falsifiability — how the claim gets tested

The claim is engineered to be refutable, not rhetorical. Each failure mode has a gate that produces numbers:

- **P2** ([`bench-tests.md`](bench-tests.md)): orientation error/drift (T1) falsifies the IMU premise; ToF accuracy/cadence (T2) falsifies the ranging premise; ESP-NOW latency/loss (T3) and render load (T4) falsify the ≤100 ms budget; end-to-end placement sweeps (T5) falsify the geometry pipeline; offset repeatability (T6) falsifies the calibration premise.
- **P4** falsifies the perceptual mappings: calibrated $g(D)$ and elevation-cue tuning against real user reports.
- **P5** falsifies the end claim with people: point-to-sound localization accuracy (azimuth and elevation), collisions, PPWS, completion time, SUS, NASA-TLX on a standardized obstacle course (Roentgen et al. 2012b), with blindfolded-sighted participants as a conservative model (dos Santos et al. 2021; Giudice et al. 2020) — the paradigm Kilian et al. 2022 validated for ToF-mapped aids.

If any gate fails, the failure is localized to a named caveat (C1–C10) with a named mitigation or an explicit redesign decision — achievability is a measured property of this system, not an assertion.

## 8. Verdict

The exact head-relative projection of the laser-hit sound is achievable with the locked architecture because the math never needed what the constraint forbids: the egocentric formula consumes two onboard orientations, one onboard range, and one calibrated constant, and the perceptual channel is the one arena where published evidence is strongest for generic, non-individualized, embedded-class rendering (azimuth and distance), with a deliberately robust substitute for the weakest channel (elevation) and an architecture that turns head motion — the listener's own most powerful localization behavior — into part of the display itself. The residual gaps (HRTF artifacts, stylized elevation, absent room acoustics, sensor error terms) are enumerated, bounded, mitigated in hardware or firmware, and scheduled to be measured before anything is soldered.
