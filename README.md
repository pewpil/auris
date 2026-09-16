		# Cane

> **Formerly Auris.** The former concept, plan, and thesis notes were
> cleared on 2026-09-07; this file is now the canonical **concept and
> development plan** for Cane — a head-mounted, audio-only navigation aid
> driven by a laser pointer, for visually impaired users. Repo paths still
> use the old name (`docs/auris-thesis/`); a physical rename is not
> planned.

## 1. Concept

### 1.1 User and task

The user is visually impaired — or a blindfolded sighted person replicating one — inside a room containing obstacles (tables, walls, chairs). The task: travel from any point in the room to another point, perceiving and avoiding obstacles along the way.

### 1.2 Equipment

1. **Head-mounted wearable** (worn on an adjustable single head strap; 3D-printed mounts hold the components together — [§3.3](#33-structural--mechanical)):
- extracts the user's **head position and orientation**;
- carries the **audio apparatus** (stereo earphones over a 3.5 mm audio jack — any wired earbuds or headset plugs in) that broadcasts spatialized sound to the user's ears;
- carries the **pointer-tracking stack** (added 2026-09-15): wide-FOV RGB cameras left and right of the head with onboard computer vision, plus an ultra-wideband (UWB) radio — the pointer's pose is measured live and feeds the placement ([§2.5](#25-pointer-tracking-stack-vision-uwb-imu)).
2. **Handheld pointer** (its own device): shoots an **invisible laser** toward obstacles and measures **where the shot lands** (hit distance, via time-of-flight ranging). A **press-and-hold button** gates the laser: it fires — and ranging runs — only while the button is held, and stops the moment it is released. An upgrade-path **UWB tag** on the pointer completes the tracking pair ([§2.5](#25-pointer-tracking-stack-vision-uwb-imu)).

### 1.3 Interaction rule

The sound is projected — **in real time** — according to where the laser lands **relative to the user's head**, not relative to the room or the user's body:

- **Direction** — the landing point's bearing in the head's egocentric frame, given the head's *current* orientation. If the user faces straight ahead and the shot lands on their left, they hear it on the left. If they aim to the right while still facing ahead, the shot lands on their right and they hear it on the right. If they then turn their head (e.g., to their left) so that the same landing point ends up behind the head, the sound moves to behind them. The head defines the listener — its position *and* orientation matter; the user's body position does not.
- **Loudness** — the distance from the **head** to the landing point. A landing far from the head is faint; the nearer it lands, the louder it is projected. (Computed head-relative in real time — mechanism in [§2.3](#23-coordinate-frames-and-sound-placement).)
- **Elevation** — whether the landing point is above, at, or below the head's level is also conveyed (a hit on a staircase step below sounds different from a wall sign at eye height). The pointer may be aimed up or down freely; the sound follows the hit in 3-D.
- **Trigger** — the aid sounds only while the pointer's **button is held**. The laser fires — and ranging runs — only then; releasing the button stops the laser and silences the sound. The aid is therefore an **active, on-demand** scanner, in line with the active-sensing rationale of the EyeCane lineage (Maidenbaum et al. 2014, [§2 lit](docs/auris-thesis/literature/02-electronic-travel-aids.md)).

Worked examples (clarified 2026-09-08):

| Scenario | Head facing | Perceived sound |
|---|---|---|
| shot lands left of the user | straight ahead | left side |
| pointer aimed right — shot lands right | straight ahead | right side |
| head turns past the shot's landing point | turned away from the landing point | behind the head |

In all rows the loudness follows the head-to-landing distance (far → faint, near → loud) in real time.

## 2. System overview

### 2.1 Devices

| Device | Roles | Core parts |
|---|---|---|
| **Pointer** (handheld) | ranging + its own orientation; press-and-hold trigger | ESP32-C3-class MCU, 9-DoF IMU, VL53L1X ToF (invisible 940 nm, up to ~4 m), trigger button, battery, 3D-printed shell, UWB tag + tracking beacon (2× LED) (upgrade, [§2.5](#25-pointer-tracking-stack-vision-uwb-imu)) |
| **Wearable** (head) | head pose + audio output + pointer tracking | ESP32-class MCU, 9-DoF IMU, stereo earphones (3.5 mm audio jack), battery, adjustable head strap + 3D-printed mounts ([§3.3](#33-structural--mechanical)), wide-FOV RGB cameras ×2 + UWB anchor (pointer-tracking upgrade, [§2.5](#25-pointer-tracking-stack-vision-uwb-imu)) |
| **Link** | pointer → wearable data | ESP-NOW (connectionless WiFi peer-to-peer), payload = hit distance + pointer orientation (quaternion); UWB ranging pair (upgrade, [§2.5](#25-pointer-tracking-stack-vision-uwb-imu)) |

Audio is rendered **on the wearable**; the ESP-NOW link carries data, never audio (Bluetooth audio would add 150–300 ms by protocol design — wired earphones only), so the motion-to-sound latency stays inside the budget (§4.2). The button state gates everything: with the button up the pointer does not range and sends nothing, and the renderer is silent. The pointer-tracking upgrade ([§2.5](#25-pointer-tracking-stack-vision-uwb-imu)) adds a second, fully on-body sensing path — cameras plus UWB — that refines the pointer's pose; it changes no audio path.

### 2.2 Data flow

```mermaid
flowchart LR
    subgraph pointer["Handheld pointer"]
        pimu["Pointer IMU (9-DoF)"] --> pfus["Pointer yaw fusion (Madgwick)"]
        tof["ToF rangefinder<br/>(VL53L1X, 940 nm)"] --> dist["Hit distance d"]
        btn["Trigger button<br/>(press-and-hold)"] --> mcu1["Pointer MCU (ESP32-C3)"]
        uwbt["UWB tag (upgrade)"]
        beacon["Tracking beacon (2× LED, upgrade)"]
        pfus --> mcu1
        dist --> mcu1
    end
    subgraph wearable["Head-mounted wearable"]
        himu["Head IMU (9-DoF)"] --> hfus["Head yaw fusion (Madgwick)"]
        camL["Wide-FOV camera (left)"] -->         cv["Pointer CV track<br/>(beacon, 15-30 Hz)"]
        camR["Wide-FOV camera (right)"] --> cv
        uwbw["UWB anchor (wearable)"] --> src["Tracking source select<br/>(vision -> UWB+IMU -> nominal offset)"]
        cv --> src
        mcu1 -- "ESP-NOW: d, pointer quaternion" --> geo["Relative geometry (3-D)<br/>(d, pointer & head orientation,<br/>live or nominal pose) -> theta, phi, D"]
        src -- "pointer pose, measured or nominal" --> geo
        hfus --> geo
        geo --> ren["Renderer: HRTF azimuth<br/>+ carrier-pitch elevation cue<br/>+ gain g(D)"]
        ren -- "azimuth, elevation, loudness" --> spk["Stereo earphones"]
        uwbt -. "UWB range" .-> uwbw
        beacon -. "beacon light" .-> cv
    end
```

### 2.3 Coordinate frames and sound placement

- **Head frame H**: origin at the head (relative position, IMU dead reckoning — [§2.4](#24-positioning-relative-pose-only)); the head IMU (Madgwick fusion) senses the **full orientation** — yaw $\mathrm{yaw}_H$ (heading, rotation about the vertical axis), pitch $\mathrm{pitch}_H$ (nodding up/down), and roll (ear-to-shoulder tilt) — and **all three are used**: the landing point is transformed with the full head rotation $R_H$, because the listener's frame — the ears — rotates with the head; a hit level with the eyes when upright is no longer level with the ear axis when the head tilts. The perceived sound position is expressed in this frame — that is what makes the sound move correctly when the user turns, nods, or tilts their head.
- **Pointer frame P**: the pointer IMU likewise senses and fuses full orientation; its aim axis is the unit vector $\hat{u}_P$ extracted from the fused quaternion (roll about the beam axis does not steer the beam, but the complete orientation is fused, shipped, and used in the exact transform; the first-order forms below read it as yaw $\mathrm{yaw}_P$ and pitch $\mathrm{pitch}_P$). The pointer is *not* confined to chest height — it may be aimed up or down freely (walls, signs, staircases) — and the ToF reading $d$ ranges along that aim.
- **Landing point relative to the head** (full 3-D):

  $$
  r \;=\; R_H^{-1}\!\left[\left(p_P + d\,\hat{u}_P\right) - p_H\right]
  \;=\; o + d\,R_H^{-1}\,\hat{u}_P
  $$

  where $R_H$ is the head's full rotation and $o = R_H^{-1}(p_P - p_H)$ is the pointer's offset from the head expressed in the head frame — handled as a **calibrated 3-D constant** (nominal handheld position: forward, lateral, and below the head) rather than tracked, since rigid-body hand tracking is out of scope for an IMU-only build ([§2.4](#24-positioning-relative-pose-only)). Note that two IMUs alone cannot measure their separation: inertial position is double-integrated acceleration whose bias error grows quadratically, and the *difference* of two drifting estimates drifts faster still (Harle 2013; Foxlin 2005, [§5 lit](docs/auris-thesis/literature/05-head-pose-sensing.md)). IMUs give reliable relative **orientation** — yaw *and* pitch — not distance; so the fixed offset is calibrated once. The pointer-tracking upgrade ([§2.5](#25-pointer-tracking-stack-vision-uwb-imu)) lifts exactly this restriction in the extended build: with the pointer in camera view, $p_P$ — and with it the offset — is **measured live**, and the calibrated constant remains the fallback tier.

The placement quantities follow from three live measurements — the ToF range $d$ and both devices' full orientation — plus the one calibrated constant $o$. The exact computation runs on the full rotation matrices ($R_H$ and the pointer's fused orientation); the yaw- and pitch-difference forms quoted below are first-order intuition only. Because $o$ is known, the placement is exact for the calibrated pose; the residual error is only grip/pose variation ([§7](#7-risks-and-limitations)). The renderer recomputes all three on every update, so the sound moves continuously as the user turns, nods, tilts, walks, or re-aims the pointer:

- **Azimuth** —

  $$
  \theta = \operatorname{atan2}\!\left(r_y,\, r_x\right)
  $$

  (to first order $\theta \approx \mathrm{yaw}_P - \mathrm{yaw}_H$, wrapped to $[-180°, 180°)$) — rendered with a generic HRTF.
- **Elevation** —

  $$
  \phi = \operatorname{atan2}\!\left(r_z,\, \sqrt{r_x^2 + r_y^2}\right)
  $$

  (to first order $\phi \approx \mathrm{pitch}_P - \mathrm{pitch}_H$ plus the vertical-offset geometry term) — **rendered explicitly**: the obstacle signal's carrier pitch sits *below* the reference when the hit is below head level and *above* it when the hit is above, with level hits at the reference. This follows the elevation→pitch precedent of the vOICe (Meijer 1992, [§3 lit](docs/auris-thesis/literature/03-audio-ssd-sonification.md)) and 3-D scene sonification (Bujacz et al. 2011, same file). Spectral HRTF elevation is deliberately *not* relied upon: it is the channel that degrades with non-individualized HRTFs (Planinec et al. 2023, [§4 lit](docs/auris-thesis/literature/04-spatial-audio-hrtf.md)), whereas a carrier-pitch cue is robust on any earphones and needs no per-user calibration. Staircases are the motivating case: aiming down a flight of steps must sound distinctly *lower* than a wall at eye height.
- **Loudness distance** —

  $$
  D = \lVert r \rVert,
  $$

  the head-to-landing distance. Note what each symbol is: $d$ is the ToF reading — *pointer*-to-hit — but the gain keys to $D = \lVert r \rVert$, the *head*-to-hit distance obtained from $d$ through the calibrated offset $o$ and the relative aim direction $R_H^{-1}\hat{u}_P$. Keying the gain to $d$ raw would treat the pointer as the listener and overstate loudness by up to $\lVert o \rVert \approx 0.5\,\mathrm{m}$ — negligible at meters of range but several dB at close range (inverse-square: halving the distance is $+6\,\mathrm{dB}$), exactly the collision-warning regime. The distance still never involves the head's *room* position: as the user walks toward the obstacle with the button held, the ToF range $d$ shrinks and $D$ follows in real time — proximity is carried by the ranging, not by dead-reckoned position.

The head's position matters only **relative to the pointer** — body geometry, captured by the calibrated offset $o$. The user's body position never enters the placement ([§1.3](#13-interaction-rule)), and neither does the head's dead-reckoned room position, which exists for telemetry only ([§2.4](#24-positioning-relative-pose-only)).

Placement rules:

1. **Azimuth** — render the sound at azimuth $\theta$ with a generic HRTF. Full-azimuth rendering makes the "behind" cases automatic (no hand-coded sectors needed).
2. **Elevation** — render the above/at/below relation with the carrier-pitch cue ($\phi$ below/at/above head level → lower/ reference/higher pitch); tune the pitch spread in P4 so a stairs-versus-wall difference is obvious without sounding alien.
3. **Loudness** — gain $g(D)$ monotonic decreasing in the head-to-landing distance $D$, calibrated on the prototype (perceived loudness is not linear in dB, so the curve is tuned in P4). Near hits saturate at maximum; far hits floor at a faint but audible minimum.
4. **No-hit edge case** — while the button is held, a missing return ($d$ beyond the ToF range, or lost on glass/black surfaces) renders silence or a low "no contact" tick. With the button up the aid is silent by design — "not scanning" is the default state.

### 2.4 Positioning: relative pose only

The wearable's head **position** comes from IMU dead reckoning only — no room infrastructure. Literature shows unaided inertial position drifts far too fast for absolute room-scale tracking (Harle 2013; Foxlin 2005, see [§7](#7-risks-and-limitations)), so:

- position is treated as **relative**, valid for short traverses from a known start point;
- the point-to-point room task is judged by **observed behavior** plus an external reference, not by the device's own position estimate;
- the position channel is for **telemetry only** (user went A→B): sound placement never consumes it — $D$, $\theta$, and $\phi$ come from the ToF range, both orientations, and the calibrated head-to-pointer offset ([§2.3](#23-coordinate-frames-and-sound-placement)).

**Self-containment audit** — every runtime placement input and its onboard source:

| Placement input | Source | External dependency |
|---|---|---|
| Head orientation ($R_H$, full 3-D) | head-worn 9-DoF IMU, Madgwick fusion | none — gravity and the magnetic field are passive physical references, not infrastructure |
| Pointer orientation (quaternion) | pointer's own 9-DoF IMU, fused onboard | none |
| ToF range $d$ | VL53L1X on the pointer | none — active ranging |
| Head-to-pointer offset $o$ | one-time calibration (body geometry) | none |
| Pointer pose $p_P$ (upgrade tier, [§2.5](#25-pointer-tracking-stack-vision-uwb-imu)) | left/right wide-FOV cameras (CV beacon) + UWB ranging pair | none — on-body sensors |
| Head room position | head-IMU dead reckoning | none — telemetry only, never used for placement |

Placement is therefore fully self-contained: no beacons, room cameras, motion capture, UWB anchors, or room instrumentation participate in it — the pointer-tracking upgrade's cameras and UWB pair are carried by the two devices themselves ([§2.5](#25-pointer-tracking-stack-vision-uwb-imu)), so nothing external is introduced. The remaining accuracy threats (magnetometer disturbance, grip/pose variation) are internal and are mitigated onboard — see [§7](#7-risks-and-limitations); they never motivate external aiding.

### 2.5 Pointer tracking stack (vision, UWB, IMU)

The frozen placement ([§2.3](#23-coordinate-frames-and-sound-placement)) treats the pointer's position as a calibrated constant — exact at the calibrated grip, perturbed by grip and pose variation (±15 cm horizontal, up to ~±0.3 m vertical; [§7](#7-risks-and-limitations) risk 7). The pointer-tracking upgrade strengthens the **life realism of the sound projection**: when the pointer's pose is measured live, the sound lands exactly where the laser hit, not where the calibration assumed the hand was. Everything stays on the two devices — wide-FOV RGB cameras and a UWB radio on the wearable, a UWB tag on the pointer — so nothing external is introduced and the self-containment audit above still holds. The pointer's **orientation** keeps coming from its own IMU over ESP-NOW; the cameras and UWB refine its **position**, which is precisely the input the constant $o$ stood in for.

Three tiers feed the placement — only the source of $p_P$ (and with it the live offset $o = R_H^{-1}(p_P - p_H)$) changes between them:

1. **Vision (primary)** — two wide-FOV RGB cameras sit left and right of the user's head in rigid printed brackets on the head strap (stable extrinsics; the battery counterweights at the rear strap station — [§3.7](#37-component-notes)); onboard computer vision localizes the pointer at 15–30 Hz whenever it is inside their field of view, and **only while the button is held** — the cameras follow the aid's trigger gating ([§1.3](#13-interaction-rule)), so the added power draw is duty-cycled like everything else. The marker is a **two-LED tracking beacon** on the pointer shell (decision 2026-09-15): two bright visible-red LEDs at a fixed, calibrated ~8–10 cm separation — thresholding finds the two centroids, the mid-point gives bearing, and the **angular subtense of the known baseline gives range**, so one camera solves the position on its own every frame at ~1–3 ms per QVGA frame (the second camera is redundancy and occlusion coverage; the pointer's orientation keeps coming from its own IMU over ESP-NOW). A passive printed dot constellation is the no-power alternate ([§3.8](#38-pointer-tracking-hardware--notes--contingencies)). The head-flanking pair covers the forward hemisphere; a pointer behind the user is out of view by construction — which is exactly when the next tier takes over.
2. **UWB + IMU (fallback)** — when the pointer leaves the cameras' field of view, the wearable–pointer UWB ranging pair supplies the head-to-pointer distance which, combined with the pointer's IMU orientation and the nominal offset direction, re-anchors $p_P$ far better than the constant alone.
3. **Nominal offset (last resort)** — if UWB is also unavailable (body blockage, NLOS), the frozen scheme runs unchanged: calibrated constant $o$, both IMU orientations, ToF range $d$ ([§2.3](#23-coordinate-frames-and-sound-placement)).

Tier selection is explicit: every sound update's telemetry records which tier produced the pose that drove it, so the study can quantify how often each tier served and how placement error varies per tier — the realism gain is measured, not assumed. Tier switching must be transparent (no perceptible placement jump at a switch — P4/P5 validation, [§7](#7-risks-and-limitations) risk 9). The motion-to-sound budget ([§4.2](#42-latency-budget-motion-to-sound-target--100-ms)) is unchanged: the ToF → ESP-NOW → render path is untouched, and tracking latency degrades placement accuracy, never sound freshness. Integration: the tiers are **validated on the breadboards at P2** (T7–T8, [`docs/bench-tests.md`](docs/bench-tests.md)) and **tuned with the placement at P4** — before the soldered build, so a tracking-tier failure forces its design decision cheaply ([§9](#9-open-items)); the upgrade hardware is priced as ✚ rows in the device BOMs ([§3.1](#31-pointer--electronics), [§3.2](#32-wearable--electronics)), with contingencies in [§3.8](#38-pointer-tracking-hardware--notes--contingencies).

## 3. Hardware

Everything required to **build and run** Cane — not only the electronics: the two devices' electronics, the 3D-printed mounting structures (the pointer's shell; the wearable's strap mounts — [§3.3](#33-structural--mechanical)), one-time assembly/bench tools, and the evaluation hardware. Prices surveyed **2026-09-08** from Philippine retailers, in Philippine pesos (₱); revised **2026-09-10** (wearable MCU → ESP32-S3, re-zero button, TF-Luna alternate); **2026-09-15**: pointer-tracking hardware integrated into the device BOMs as ✚ rows ([§2.5](#25-pointer-tracking-stack-vision-uwb-imu)). Prices with a store link were verified against the listing on their survey date; items marked *(est)* are typical Philippine-market prices to pin down at purchase time.

### 3.1 Pointer — electronics

| #   | Component                                                                                                                                                                                                                    | Qty | Unit ₱              | Subtotal ₱      | Source                                                                                                                                                                                                                                        |
| --- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --- | ------------------- | --------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1   | ESP32-C3 SuperMini (BLE MCU) — the P2 tracking gate may swap this row to a DevKit-class C3 board to free pins for the UWB SPI bus ([§3.7](#37-component-notes), [§3.8](#38-pointer-tracking-hardware--notes--contingencies)) | 1   | 355                 | 355             | [Circuitrocks](https://circuit.rocks/products/esp32-c3-super-mini-development-board); ₱151 on [Lazada PH](https://h5.lazada.com.ph/products/esp32-c3-development-board-esp32-c3-supermini-wifi-bluetooth-for-arduino-i4393598793.html)        |
| 2   | VL53L1X ToF rangefinder (940 nm, ~4 m; alternate: TF-Luna, §3.7)                                                                                                                                                             | 1   | 525                 | 525             | [Shopee PH](https://shopee.ph/COD-VL53L1X-laser-sensor-module-TOF-time-of-flight-4-meter-ranging-i.1804393363.53909554436)                                                                                                                    |
| 3   | GY-9250 (MPU-9250, 9-DoF IMU)                                                                                                                                                                                                | 1   | 400                 | 400             | [Lazada PH](https://www.lazada.com.ph/products/mpu9250-mpu6500-9-9-dof-16-bit-gyroscope-acceleration-magnetic-sensor-accelerator-module-iicspi-i15524063344.html)                                                                             |
| 4   | Tactile trigger button (6×6 mm)                                                                                                                                                                                              | 1   | 10 *(est)*          | 10              | Lazada/Shopee PH (assortment kits)                                                                                                                                                                                                            |
| 5   | TP4056 USB-C charge board (w/ protection)                                                                                                                                                                                    | 1   | 30                  | 30              | [Makerlab PH](https://makerlab.ph/products/type-c-micro-usb-5v-1a-18650-tp4056-lithium-battery-charger-module-charging-board-with-protection)                                                                                                 |
| 6   | 18650 Li-ion 2600 mAh cell (Kaizen 2-pc pack ₱369)                                                                                                                                                                           | 1   | 185                 | 185             | [Kaizen PH](https://kaizenphilippines.com/products/kaizen-3-7v-18650-2600mah-15a-rechargeable-battery-2pc-lithium-ion-battery)                                                                                                                |
| 7   | Misc: perfboard, dupont/hookup wire, slide switch                                                                                                                                                                            | —   | 120 *(est)*         | 120             | Lazada/Shopee PH                                                                                                                                                                                                                              |
| 8   | UWB tag module (DW1000 class, e.g., DWM1000; DW3000-class alternate) — the pointer half of the wearable–pointer ranging pair ✚                                                                                               | 1   | 1,200–2,500 *(est)* | 1,200–2,500     | Lazada/Shopee PH — pin exact module, range, and update rate at purchase ([§3.8](#38-pointer-tracking-hardware--notes--contingencies))                                                                                                         |
| 9   | Tracking beacon: 2× bright visible-red LEDs + dropper resistor on the shell (fixed, calibrated ~8–10 cm separation; lit **only while the button is held** — [§1.3](#13-interaction-rule)) ✚                                  | 1   | 30–80 *(est)*       | 30–80           | Lazada/Shopee PH — pin exact LED + driver at purchase; IR variant possible with the OV5640's IR-cut filter removed ([§3.8](#38-pointer-tracking-hardware--notes--contingencies)); passive printed dot constellation is the no-power alternate |
|     | **Pointer subtotal — frozen core**                                                                                                                                                                                           |     |                     | **1,625**       |                                                                                                                                                                                                                                               |
|     | **Pointer subtotal — with tracking ✚**                                                                                                                                                                                       |     |                     | **2,855–4,155** |                                                                                                                                                                                                                                               |

### 3.2 Wearable — electronics

| # | Component | Qty | Unit ₱ | Subtotal ₱ | Source |
|---|---|---|---|---|---|
| 1 | Seeed XIAO ESP32-S3 (S3 MCU: SIMD DSP + per-core FPU, I²S out, BLE 5) — the P2 tracking gate may swap this row to a DevKit-class ESP32-S3 board to free pins for the two head cameras + UWB ([§3.7](#37-component-notes), [§3.8](#38-pointer-tracking-hardware--notes--contingencies)) | 1 | 499 | 499 | [Makerlab PH](https://makerlab.ph/products/seeed-xiao-esp32-s3-113991114) |
| 2 | GY-9250 (MPU-9250, 9-DoF IMU) — same part as pointer | 1 | 400 | 400 | [Lazada PH](https://www.lazada.com.ph/products/mpu9250-mpu6500-9-9-dof-16-bit-gyroscope-acceleration-magnetic-sensor-accelerator-module-iicspi-i15524063344.html) |
| 3 | MAX98357A I²S 3 W Class-D amp — **qty 2 required** (mono amp; one per ear, SD-pin strapped L/R) | 2 | 499 | 998 | [Circuitrocks (Adafruit breakout)](https://circuit.rocks/products/i2s-3w-class-d-amplifier-breakout-max98357a-adafruit); generic clones cheaper on Lazada/Shopee |
| 4 | Stereo wired earphones, 3.5 mm (BAVIN HX820) — wired is mandatory (BT audio latency, §2.1) | 1 | 118 | 118 | [Lazada PH](https://www.lazada.com.ph/products/pdp-i3057481002.html) |
| 5 | TP4056 USB-C charge board (w/ protection) | 1 | 30 | 30 | [Makerlab PH](https://makerlab.ph/products/type-c-micro-usb-5v-1a-18650-tp4056-lithium-battery-charger-module-charging-board-with-protection) |
| 6 | 18650 Li-ion 2600 mAh cell | 1 | 185 | 185 | [Kaizen PH](https://kaizenphilippines.com/products/kaizen-3-7v-18650-2600mah-15a-rechargeable-battery-2pc-lithium-ion-battery) |
| 7 | Misc: perfboard, wires, 3.5 mm jack breakout | — | 120 *(est)* | 120 | Lazada/Shopee PH |
| 8 | Tactile re-zero button (6×6 mm; shares assortment kit) | 1 | 10 *(est)* | 10 | Lazada/Shopee PH |
| 9 | **OV5640 camera module, 24-pin DVP, wide-FOV lens ≥ 160° diagonal** (RGB565/YUV, fixed focus; QVGA @ 120 fps capability — ~US$6.30–8.90/unit; 68°-lens dev-board bundles do **not** qualify — order the wide-lens variant) ✚ | 2 | 400–700 *(est)* | 800–1,400 | Lazada/Shopee PH / AliExpress — verify on arrival: real FOV, low-light behavior, 24-pin DVP pinout vs the chosen board ([§3.7](#37-component-notes)); streams **only while the button is held** ([§1.3](#13-interaction-rule)), so the draw is trigger-gated. Budget alternate: OV2640 wide-lens (cheaper, ~120–140°, 2 MP). Motion alternate if T7 shows rolling-shutter smearing: OV9281 global shutter (mono — deviates from the RGB spec, fallback only) |
| 10 | UWB anchor module (DW1000 class, e.g., DWM1000; DW3000-class alternate) — the wearable half of the ranging pair ✚ | 1 | 1,200–2,500 *(est)* | 1,200–2,500 | Lazada/Shopee PH — pin exact module, range, and update rate at purchase ([§3.8](#38-pointer-tracking-hardware--notes--contingencies)) |
| 11 | Camera mounts, flex/PD cables, wiring ✚ | — | 150 *(est)* | 150 | Lazada/Shopee PH |
| | **Wearable subtotal — frozen core** | | | **2,360** | |
| | **Wearable subtotal — with tracking ✚** | | | **4,510–6,410** | |

**✚ = pointer-tracking hardware** ([§2.5](#25-pointer-tracking-stack-vision-uwb-imu)) — validated on the breadboards at P2 (T7–T8, [`docs/bench-tests.md`](docs/bench-tests.md)) and removable at that gate; the frozen-core subtotals show the aid without it.

### 3.3 Structural & mechanical

| # | Component | Qty | Unit ₱ | Subtotal ₱ | Source |
|---|---|---|---|---|---|
| 1 | FDM 3D printer — Creality Ender 3 V3 SE (one-time capital) | 1 | 11,199 | 11,199 | [Makerlab PH](https://www.makerlab.ph/products/ender-3-v3-se-3d-printer) — official Creality PH distributor, 1-yr warranty, free-filament promo. Alternative: print service such as [Flarelab](https://flarelab.com) (quote-based) |
| 2 | PLA filament 1.75 mm, 1 kg (Creality Ender PLA) | 1 | 700 *(est)* | 700 | [Lazada PH](https://www.lazada.com.ph/products/creality-ender-pla-filament-1kg-175mm-i4337055224.html) / Shopee PH — pin exact price at purchase |
| 3 | Elastic head strap (single band, goggle-style) + adjustment fasteners + comfort padding | — | 75 *(est)* | 75 | Lazada/Shopee PH — pin exact strap, fasteners, and padding at purchase |
| 4 | 18650 battery holders | 2 | 15 *(est)* | 30 | Lazada/Shopee PH |
| 5 | M2/M3 screws, nuts, velcro, zip ties | — | 100 *(est)* | 100 | Lazada/Shopee PH |
| 6 | Adhesive: hot glue/epoxy, double-sided tape | — | 100 *(est)* | 100 | Lazada/Shopee PH |
| | **Structural subtotal (excl. printer)** | | | **1,005** | |

### 3.4 Assembly & bench tools (one-time)

| # | Tool | Qty | Unit ₱ | Subtotal ₱ | Source |
|---|---|---|---|---|---|
| 1 | 60 W adjustable soldering iron kit | 1 | 300 *(est)* | 300 | Lazada/Shopee PH kits; premium Hakko 60 W ≈ ₱1,561 ([ToolsPH price list](https://toolsph.com/soldering-iron-price)) |
| 2 | Digital multimeter (DT830 class) | 1 | 250 *(est)* | 250 | [Lazada PH](https://www.lazada.com.ph/products/dt830-digital-multimeter-multi-tester-holdpeak-manual-ranging-multi-tester-i4454326879.html) |
| 3 | Wire strippers, cutters, pliers | — | 200 *(est)* | 200 | Lazada/Shopee PH |
| 4 | Helping-hands / PCB holder | 1 | 150 *(est)* | 150 | Lazada/Shopee PH |
| 5 | USB cables for flashing (USB-C + Micro-USB) | 2 | 35 | 70 | [Circuitrocks](https://circuit.rocks/products/circuitrocks-usb-cable-type-c-type-b-to-type-a-male-for-arduino-uno-mega) |
| 6 | Desoldering wick + flux | — | 100 *(est)* | 100 | Lazada/Shopee PH |
| | **Tools subtotal** | | | **1,070** | |

### 3.5 Evaluation hardware (one-time)

| # | Item | Qty | Unit ₱ | Subtotal ₱ | Source |
|---|---|---|---|---|---|
| 1 | Blindfold / sleep mask (for sighted participants) | 2 | 50 *(est)* | 100 | Lazada/Shopee PH |
| 2 | Floor marking tape + 5 m measuring tape (course layout; ToF ground truth) | — | 200 *(est)* | 200 | Lazada/Shopee PH |
| 3 | Obstacle props (cardboard boxes, foam blocks) | — | 200 *(est)* | 200 | Often sourced free; budgeted as new |
| 4 | Timing — phone/smartwatch | — | 0 | 0 | On hand |
| | **Evaluation subtotal** | | | **500** | |

### 3.6 Totals

| Block | ₱ |
|---|---|
| Pointer electronics — frozen core | 1,625 |
| Wearable electronics — frozen core | 2,360 |
| Structural & mechanical (excl. printer) | 1,005 |
| Assembly & bench tools | 1,070 |
| Evaluation hardware | 500 |
| **Subtotal — frozen core (excl. printer)** | **6,560** |
| Pointer-tracking hardware ✚ ([§2.5](#25-pointer-tracking-stack-vision-uwb-imu)) | 3,380–6,630 *(est)* |
| **Subtotal — with tracking (excl. printer)** | **9,940–13,190** |
| Contingency 20% (spares, shipping, promo drift) | 1,988–2,638 |
| **Total — print-service path** (housings quoted, ~₱1,000 *(est)*) | **≈ 12,900–16,800** |
| + own printer: Creality Ender 3 V3 SE (replaces the housings quote) | 11,199 |
| **Total — own-printer path** | **≈ 23,100–27,000** |

The two totals differ only in how the housings are produced: quote the two small housings to a print service such as Flarelab (service fee assumed ₱1,000 *(est)*, pending quote), or buy the printer outright (Makerlab, with free-filament promo and 1-yr local warranty). The ✚ tracking block is **inside both totals**; if the P2 gate rejects the tracking tier, subtract its ₱3,380–6,630 block — the frozen-core rows are that budget (recomputed 2026-09-15: tracking hardware integrated into the device BOMs, marker = two-LED tracking beacon). A purchase decision gate — see [§9](#9-open-items).

### 3.7 Component notes

- **IMU (both devices)** — GY-9250/MPU-9250 is one part number across both devices to simplify fusion. Upgrade path if P2 magnetometer fusion proves noisy in the actual room: BNO055/BNO085 (factory-fused, ~₱1,200–2,000 each) — a budget-relevant decision gate at P2.
- **MCU alternates** — the ESP32-C3 SuperMini also lists at ₱151 on Lazada PH; the Seeed XIAO ESP32-S3 (₱499, Makerlab PH; SIMD vector DSP + per-core FPU for the renderer, onboard LiPo charging) is the wearable reference, with ESP32-S3 SuperMini/DevKit clones (≈₱300–700 on Lazada/Shopee) as drop-in alternates. With the pointer-tracking upgrade validated at P2, the **board class becomes a real decision on both devices**: the XIAO cannot host two DVP cameras + UWB (SPI) alongside the frozen core, and the C3 SuperMini is pin-tight for the UWB module — the T7/T8 gates pick between staying on the reference boards (with a camera co-processor or single-port alternate-frame capture) and moving to DevKit-class boards with pins to spare ([§3.8](#38-pointer-tracking-hardware--notes--contingencies), [`docs/bench-tests.md`](docs/bench-tests.md)). The MCU **family** stays ESP32 either way: ESP-NOW and the frozen latency architecture depend on it, and every escape hatch (co-processor, DevKit-class boards) is an ESP32-family part — an off-ESP32 redesign is not on the table, so T7 decides only the worst-case shape of the ESP32 answer.
- **Pre-soldered ordering constraint** — P2 permits no soldering, so order every module (both MCUs, the IMUs, the ToF, and the tracking modules — the two cameras and the UWB pair) with headers **pre-soldered**; verify "pre-soldered / headers attached" on the listing before checkout. Unsoldered arrivals are set aside for the P3 build.
- **Amplifier** — the MAX98357A is a **mono** amp: binaural placement needs independent left/right channels, so **two units are required** (one per ear; each amp's SD pin is strapped to select its channel per the datasheet — verify strapping on the purchased breakout). The ₱499 line is the Adafruit breakout; generic modules on Lazada/Shopee are substantially cheaper — a third unit as spare is optional.
- **Ranging** — VL53L0X (2 m) is cheaper but undershoots the ~4 m room-scale task; the VL53L1X (4 m, 940 nm invisible VCSEL) is kept, with the receive ROI narrowed to the central zone in firmware to tighten the hit spot at range. If P2 shows the 4 m ceiling or the default beam width limiting the course, the TF-Luna (0.2–8 m, ±6 cm < 3 m, 100 Hz default, 2° FoV, UART/I²C, 850 nm, ≈₱800–1,000 *(est)*) is the single-part alternate — verify its beam width on the datasheet at purchase.
- **Re-zero button** — one-press yaw re-anchor at rest (pitch/roll are gravity-anchored and drift-free); turns magnetometer disturbance from a silent heading bias into a bounded user action ([§7](#7-risks-and-limitations) risk 2), and doubles as the P4 offset-calibration trigger.
- **Audio** — air-conduction stereo earphones preserve localization quality (Ferrand 2019; Planinec et al. 2023, see [§4 lit](docs/auris-thesis/literature/04-spatial-audio-hrtf.md)); **wired output is mandatory** — Bluetooth audio adds 150–300 ms by protocol design and would break the placement latency budget ([§2.1](#21-devices)); if the faint-far floor turns hissy, a 32 Ω pair is the cheap remedy.
- **3D printer** — the Ender 3 V3 SE (₱11,199, Makerlab PH) is listed as the reference because the concept requires the wearables to be 3D printed in-house; university/lab printer access would remove this line entirely — confirm before purchase approval.
- **Strap-mounted wearable** — the wearable is not a printed helmet: a single elastic head strap (goggle-style, [§3.3](#33-structural--mechanical)) carries the components in 3D-printed mounts — rigid brackets for the two cameras (stable extrinsics for the vision tier, [§2.5](#25-pointer-tracking-stack-vision-uwb-imu)) and the head IMU, the battery holder at the rear station as counterweight, boards and padding against the band. Strap slip shifts the whole assembly relative to the ear axis, so **donning is a ritual**: fit the strap, hold still, press re-zero (the re-zero button above; [§7](#7-risks-and-limitations) risk 10) — pitch/roll are gravity-anchored to the *device*, so tilt alignment comes from fit discipline. Audio is interoperable by design: the wearable exposes a 3.5 mm audio jack, so any wired earbuds or headset plugs in (the BAVIN HX820, [§3.2](#32-wearable--electronics) row 4, is the budget reference unit; wired output stays mandatory — [§2.1](#21-devices)); earphone cables route along the strap.

### 3.8 Pointer-tracking hardware — notes & contingencies

The ✚ rows live **in the device BOMs** — [§3.1](#31-pointer--electronics) (UWB tag, tracking beacon) and [§3.2](#32-wearable--electronics) (2× OV5640 cameras, UWB anchor, mounts) — not in a side list. Recap of the placement rule: on-body only, so the self-containment audit ([§2.4](#24-positioning-relative-pose-only)) still holds; validated at P2 (T7–T8, [`docs/bench-tests.md`](docs/bench-tests.md)), tuned at P4; removable at that gate (the frozen-core subtotals and the ₱3,380–6,630 ✚ block in [§3.6](#36-totals) bracket the decision). Ordering follows the same pre-soldered rule as everything else ([§3.7](#37-component-notes)).

**Marker scheme (decided 2026-09-15): the vision tier's marker is a two-LED tracking beacon** ([§3.1](#31-pointer--electronics) row 9) — two bright visible-red LEDs at a fixed, calibrated ~8–10 cm separation, lit only while the button is held. Detection is threshold → two centroids → bearing (mid-point) + range (baseline angular subtense) at ~1–3 ms per QVGA frame — full detection every frame, no detect-then-track juggling; the pointer's orientation still rides ESP-NOW from its IMU, so the beacon carries only position. Default color visible red (works with any OV5640 variant, no filter changes); IR variant possible with the OV5640's IR-cut filter removed (lowest visual disturbance); the passive printed dot constellation is the no-power alternate (~3–10 ms detection). Ambient-light strategy: LEDs steady while the button is held + shape gating (two spots at the expected separation); flash-sync rejection only if T7's bright-light check shows interference. Square library tags (ArUco/AprilTag) are deliberately not used — their quad-detection cost is what made the vision tier look heavy, and the architecture needs only position.

Board-level: the ESP32-S3 exposes a single DVP camera interface — two head cameras need a mux, alternate-frame capture on one port, a DevKit-class board, or a camera co-processor, and UWB needs an SPI bus — with pressure on **both** devices, since the C3 SuperMini is also pin-tight for the UWB tag; the T7/T8 gates settle the board classes ([§3.7](#37-component-notes), risk 8). Contingency: if the reference boards cannot carry the extras, two DevKit-class ESP32 boards or one camera co-processor (**₱300–1,500 *(est)***, within the [§3.7](#37-component-notes) alternates' range) substitute into the [§3.1](#31-pointer--electronics)/[§3.2](#32-wearable--electronics) MCU rows — a swap inside the totals' spread, not a new budget line. Power: camera streaming is the largest new draw — trigger-gated to button-hold ([§2.5](#25-pointer-tracking-stack-vision-uwb-imu)), and T0 re-checks the 18650 budget with tracking active.

## 4. Software

### 4.1 Firmware modules

| Module | Device | Responsibility |
|---|---|---|
| `fusion` (Madgwick) | both | full quaternion (yaw/pitch/roll) from IMU at ~100 Hz |
| `tof` driver | pointer | ranged readings at 20–50 Hz **while the button is held**, no-hit handling; idle otherwise |
| `link` (ESP-NOW) | both | connectionless WiFi peer-to-peer; ship `d` + pointer quaternion → wearable at 10–30 Hz |
| `geometry` | wearable | $\theta$, $\phi$ (3-D direction) and $D$ (head-to-landing distance) from $d$, both orientations, and the calibrated offset; edge-case classification |
| `renderer` | wearable | generic-HRTF azimuth + carrier-pitch elevation cue, $g(D)$ gain curve |
| `telemetry` | wearable | relative-pose dead reckoning for logging |
| `pointer-track` (CV) ✚ | wearable | localizes the pointer from the left/right wide-FOV cameras at 15–30 Hz while in view: threshold → **two beacon centroids** → bearing (mid-point) + range (known-baseline angular subtense), ~1–3 ms at QVGA — **full detection every frame** (the two-LED beacon, [§2.5](#25-pointer-tracking-stack-vision-uwb-imu), is what makes single-level detection cheap; a printed-pattern or learned detector would need the two-level detect-then-track scheme instead — for scale, ESP-WHO-class CNN detection runs only ~10 fps at QVGA on the S3); yields a measured $p_P$ for [§2.3](#23-coordinate-frames-and-sound-placement) |
| `uwb-range` ✚ | both | wearable–pointer UWB ranging for the fallback tier |
| `track-source` ✚ | wearable | tier selection (vision → UWB + IMU → nominal offset, [§2.5](#25-pointer-tracking-stack-vision-uwb-imu)); records the active tier in every sound-update telemetry record |

✚ rows belong to the pointer-tracking upgrade; they are validated at P2 (T7–T8, [`docs/bench-tests.md`](docs/bench-tests.md)) and tuned at P4 — the frozen core pipeline is unchanged.

### 4.2 Latency budget (motion-to-sound, target ≤ 100 ms)

| Stage | Target |
|---|---|
| IMU fusion update (100 Hz) | ≤ 10 ms |
| ToF ranging cadence (≥ 20 Hz) | ≤ 50 ms |
| ESP-NOW link hop (≥ 10 Hz) | ≤ 10 ms |
| HRTF render per buffer (48 kHz / 128 samples) | ≤ 5 ms |
| Total | **≤ 100 ms** |

Bench tests for each stage are P2 exit criteria (§6).

### 4.3 EDA toolchain (KiCad and MCP)

The electronics of both devices are designed in **KiCad** (`kicad-cli` 10.0.6 on the build machine; toolchain decision 2026-09-15), one project per device under [`electronics/`](electronics/):

- [`electronics/cane-wearable/`](electronics/cane-wearable/) — the cane-concept **head-mounted wearable** board ([§3.2](#32-wearable--electronics): ESP32-S3, 9-DoF IMU, 2× MAX98357A stereo I²S out, power)
- [`electronics/cane-pointer/`](electronics/cane-pointer/) — the cane-concept **handheld pointer** board ([§3.1](#31-pointer--electronics): ESP32-C3, 9-DoF IMU, VL53L1X ToF, trigger button, power); the tracking beacon (2× LED) and UWB tag circuits join this schematic once the P2 gate pins them ([§3.8](#38-pointer-tracking-hardware--notes--contingencies))

AI-assisted design runs through the **[mcp-server-kicad](https://github.com/ProductOfAmerica/mcp-server-kicad)** MCP server (109 tools, MIT; wired into this repo's opencode config as `mcp.kicad`): schematic capture, PCB layout, ERC/DRC via `kicad-cli`, and manufacturing exports. Ground rules: the AI drafts and checks, but every design is **reviewed by a human in the KiCad GUI before anything is fabricated** — ERC/DRC reports are generated artifacts, not review substitutes. Schematic and layout figures for the thesis ([`docs/auris-thesis/README.md` §3](docs/auris-thesis/README.md)) are exported from these projects.

## 5. Evaluation plan

Paradigm justified by the consolidated literature ([`docs/auris-thesis/literature/07-evaluation-methodology.md`](docs/auris-thesis/literature/07-evaluation-methodology.md)):

- **Participants**: blindfolded sighted users (primary), replicating VI per the concept; validity support: dos Santos et al. 2021 (blindfolded users are slower → conservative results), Giudice et al. 2020 (sighted controls as best-case bound).
- **Course**: obstacle room course in the style of Roentgen et al. 2012b (standardized layouts, pre-randomized order).
- **Conditions**: aid-off baseline vs. aid-on; multiple layouts.
- **Metrics**:
- objective — collisions, completion time, Percentage Preferred Walking Speed (PPWS), localization accuracy (point-to-sound direction test, azimuth and elevation);
- subjective — SUS (after Kilian et al. 2022) and NASA-TLX (after Pittet et al. 2026).

## 6. Development phases

| Phase | Focus | Exit criteria | Thesis mapping |
|---|---|---|---|
| **P1 — Architecture freeze** *(complete 2026-09-08)* | this document; component selection | parts chosen and ordered; interfaces fixed | §3.2, §3.5, §3.7 (block diagram) |
| **P2 — Bench tests** | non-permanent breadboard assembly of both devices, **zero soldering** (pre-soldered modules only); module bench tests; full-pipeline functionality on breadboards (protocol + wiring maps: [`docs/bench-tests.md`](docs/bench-tests.md)); **plus the pointer-tracking tiers ([§2.5](#25-pointer-tracking-stack-vision-uwb-imu))** — camera/CV and UWB modules on the breadboards under the same pre-soldered rule, T7–T8 gates, module and board-level choices pinned at the gate | every §4.2 stage meets budget; end-to-end sound placement works on breadboards; drift curves recorded; **T7–T8 (tracking tiers) pass** | §3.4 (experiments), §4 (partial results) |
| **P3 — Prototype build** | 3D-print housings; solder the validated breadboard design into permanent assemblies; battery/switch/jack integration | both devices run end-to-end in the soldered builds with breadboard parity (§4.2) | §3.6 (details of the components) |
| **P4 — Integration & calibration** | full pipeline walking; $g(D)$ + pointer-offset calibration; elevation-cue tuning; magnetometer disturbance checks | calibrated sound placement works on the course | §3.5, §3.8 (flowcharts) |
| **P5 — Pilot evaluation** | blindfolded-sighted study on the room course | metrics + questionnaires collected | §4 (results and discussions) |
| **P6 — Thesis drafting** | write-up from literature notes and P2–P5 artifacts | `paper.md` + `ieee.md` in lockstep, per [`manuscript.md`](docs/auris-thesis/manuscript.md) | §2–§5, all sections |

## 7. Risks and limitations

1. **Position drift (IMU-only)** — unaided inertial position is unusable for absolute room-scale tracking over long sessions (Harle 2013; Foxlin 2005). Mitigation: relative pose only, bounded sessions, known start point; drift degrades telemetry only — sound placement never uses it (§2.3, §2.4).
2. **Heading error near ferromagnetic material** — tables, appliances, and fixtures disturb the magnetometer; Roetenberg et al. 2005 shows heading error rotates the perceived sound direction directly. Mitigation: disturbance compensation, P4 checks on the actual room.
3. **Generic HRTF quality** — azimuth is robust to non-individualized HRTFs; elevation is precisely the channel that degrades (Mendonça 2014; Planinec et al. 2023). Mitigation: elevation is carried by the explicit carrier-pitch cue, not spectral HRTF elevation (§2.3); short familiarization period built into P5.
4. **Audio-only guidance overshoot** — audio cues can cause users to turn past a target heading (Slade et al. 2021). Mitigation: Cane's sound is a **positional report**, not a steering command; the user owns navigation.
5. **ToF surface behaviors** — glass, dark, or highly reflective targets and strong ambient IR degrade returns (VL53L1X datasheet, vendor footnote). Mitigation: P2 surface matrix before committing to the course props.
6. **Loudness ≠ linear distance** — perceived loudness is nonlinear; the $g(D)$ head-to-landing curve is calibrated empirically in P4 rather than assumed.
7. **Handheld offset is not truly constant** — the calibrated head-to-pointer constant assumes a nominal pose; extending or tucking the arm, and raising/lowering it to aim up or down at stairs, shifts the offset (±15 cm horizontal, up to ~±0.3 m vertical), perturbing $D$, $\theta$, and $\phi$ ([§2.3](#23-coordinate-frames-and-sound-placement)). Mitigation: calibrate at a natural grip in P4; keep $g(D)$ and the elevation pitch spread gradual so offset variation is a small perceptual change; observe real-pose variation in P5.
8. **CV tracking compute on the wearable** — the pointer-tracking upgrade ([§2.5](#25-pointer-tracking-stack-vision-uwb-imu)) adds two camera streams and beacon detection to the same ESP32-S3 that renders audio; contention could push render buffers past the ≤ 5 ms/buffer gate into underruns. Mitigation: the tracking beacon reduces detection to threshold → two centroids (~1–3 ms at QVGA, full detection every frame; for scale, ESP-WHO-class CNN detection runs ~10 fps at QVGA on the S3 — a printed-pattern or learned detector would need the two-level detect-then-track scheme instead), QVGA mono frames, tracking on a task with priority below the audio path, rendering on one core and vision on the other; tracking latency degrades placement accuracy, never sound freshness; the P2 T7 co-residence gate decides the board-level answer (mux / alternate frames / DevKit-class board / camera co-processor) before the soldered build ([§6](#6-development-phases)).
9. **Tracking tiers must degrade gracefully** — the cameras cover the forward hemisphere only (fast head swings and behind-body aiming are out of view by construction), and UWB ranges degrade when body-blocked (NLOS). Mitigation: the explicit three-tier fallback ([§2.5](#25-pointer-tracking-stack-vision-uwb-imu)) with the active tier in every sound-update telemetry record; P2 validates the switch logic on the bench (T8), and P4/P5 validate that a switch is never audible as a placement jump.
10. **Strap slip misaligns the device frame with the head frame** — on the strap-mounted wearable ([§3.7](#37-component-notes)), strap slip or tilt rotates the head IMU and the cameras relative to the ear axis: a yaw offset biases every rendered azimuth directly, a tilt biases elevation. Mitigation: rigid printed stations keep the cameras and IMU fixed relative to the strap; a per-doning re-zero at rest re-anchors yaw (the re-zero button, [§3.7](#37-component-notes)); fit discipline covers pitch/roll (gravity anchors them to the device only); P4 adds a fit check to the calibration ritual and P5 observes real slippage over a session.

## 8. Where things live

- **Achievability statement** — [`docs/achievability.md`](docs/achievability.md): the standing justification that hit-sound placement is exact, head-relative, and fully self-contained; the perceptual-cue evidence base, the error budget, and the C1–C11 caveat ledger with falsifiability gates.
- **P2 bench protocol** — [`docs/bench-tests.md`](docs/bench-tests.md): wiring maps for both devices, power bring-up rules, and the T0–T8 test matrix with acceptance thresholds (T7–T8 = the tracking-tier gates); parity re-run at P3.
- **Electronics designs** — [`electronics/`](electronics/): one KiCad project per device ([cane-wearable](electronics/cane-wearable/), [cane-pointer](electronics/cane-pointer/)), AI-assisted through the toolchain in [§4.3](#43-eda-toolchain-kicad-and-mcp).
- **Literature consolidation** — [`docs/auris-thesis/literature/README.md`](docs/auris-thesis/literature/README.md): 46 verified annotated entries across 7 themes; feeds thesis §2.
- **Thesis writing plan** — [`docs/auris-thesis/README.md`](docs/auris-thesis/README.md): the section→content→phase drafting plan for the thesis twins, with the measurable-outcomes spine (built 2026-09-11; conventions in [`docs/auris-thesis/manuscript.md`](docs/auris-thesis/manuscript.md)).
- **Thesis skeleton** — [`docs/auris-thesis/paper.md`](docs/auris-thesis/paper.md) (APA twin) and [`docs/auris-thesis/ieee.md`](docs/auris-thesis/ieee.md) (IEEE twin); conventions in [`docs/auris-thesis/manuscript.md`](docs/auris-thesis/manuscript.md).
- **Session context** — [`.opencode/concept.md`](.opencode/concept.md) and [`.opencode/plan.md`](.opencode/plan.md) point here as the canonical source; raw requirements in [`.opencode/new.md`](.opencode/new.md); standing rules in [`.opencode/instruction.md`](.opencode/instruction.md).

## 9. Open items

- **Wearable form factor** — resolved 2026-09-16: the wearable is **strap-mounted** — a single elastic head strap (goggle-style) with 3D-printed component mounts (rigid camera/IMU brackets; the battery rides the rear station as counterweight) instead of a printed helmet shell ([§1.2](#12-equipment), [§3.3](#33-structural--mechanical), [§3.7](#37-component-notes)); the 3.5 mm audio jack keeps earphones interoperable (any wired earbuds or headset). The pointer keeps its 3D-printed shell ([§2.1](#21-devices)). Wearable-side print volume shrinks to small mounts — the print-service estimate ([§3.6](#36-totals), *(est)* pending quote) can only drop.
- **Budget & purchase approval** — full hardware requirements with Philippine-market pricing are complete in [§3](#3-hardware) (researched 2026-09-08); the remaining open items are the own-printer vs. print-service decision and the purchase-approval sign-off itself; the pointer-tracking hardware (✚ rows in [§3.1](#31-pointer--electronics)/[§3.2](#32-wearable--electronics), ≈ ₱3,400–6,600 *(est)*, contingencies in [§3.8](#38-pointer-tracking-hardware--notes--contingencies)) is inside the [§3.6](#36-totals) totals.
- **Thesis writing schedule** — resolved 2026-09-11: the section→phase drafting plan is in [`docs/auris-thesis/README.md`](docs/auris-thesis/README.md), integrated with the phase exits above.
- **Pointer-tracking upgrade** — integration point decided: the tiers validate on the breadboards at **P2** (T7–T8, [`docs/bench-tests.md`](docs/bench-tests.md)) and are tuned at P4 with the placement. Remaining pins before the P2 purchase approval: the ESP32-S3 DVP strategy (mux / alternate-frame capture / DevKit-class board / camera co-processor — risk 8), the pointer-side board class for the UWB SPI bus (SuperMini pin-tight — T8), and the UWB module (DW1000/DW3000 class). **Decided**: the camera — **OV5640-class, 24-pin DVP, ≥ 160° wide lens** ([§3.8](#38-pointer-tracking-hardware--notes--contingencies)) — and the marker scheme — **two-LED tracking beacon** (visible red; passive dot constellation = no-power alternate), with the exact LED/driver part and a bright-light rejection check remaining at T7; the ✚ tracking block (₱3,380–6,630 *(est)*) is inside the [§3.6](#36-totals) totals and a board-class swap changes the [§3.1](#31-pointer--electronics)/[§3.2](#32-wearable--electronics) MCU rows within the alternates' range ([§3.7](#37-component-notes)).
