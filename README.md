# Auris — Development Plan

A system that helps visually impaired users locate objects in a room through simulated auditory cues. The user stands in a room containing various objects; a camera array tracks both the objects and the user's head — position **and** rotation, since a standing user can turn and walk; a headset renders a 3D soundscape where every object continuously emits a distinctive sound anchored to its real position. Turning the head rotates the sound field correctly so it stays fixed to the world.

**Context:** Thesis project. Development runs in two stages, split for **financial reasons**: the **Prototype** stage builds and integrates the *entire* stack — CV pipeline, audio engine, integration & performance — using only existing hardware plus items approved for purchase (§7.2), validating the two risky unknowns end-to-end; the **Production** stage then purchases **all** system components on specialized gear, re-platforms the same contracts, and carries every formal evaluation and the user study. Stack: Python CV → Unity spatial audio over UDP localhost; wired **and** wireless interconnects are supported at every stage. End-to-end latency < 100 ms is an engineering target that is verified, not researched.

---

## 1. End-state vision & measured outcomes

A standing, blindfolded (or visually impaired) user in a room with objects placed around them — on the floor, shelves, and furniture at varied heights. A camera array covers the room and tracks every object plus the user's head as a full 6DoF pose. Through the headset the user hears a **3D soundscape**: each object continuously emits a unique earcon positioned where the object actually sits. As the user turns or walks, sounds stay anchored to their real-world positions. Saying *"find the bottle"* triggers beacon mode, where the bottle pings periodically so the user can home in.

Measured outcomes for the thesis:

- End-to-end latency **< 100 ms** (verified engineering target — see §6); both wired and wireless interconnect paths are measured
- Object localization **< 15° angular error**; absolute distance-error target set once room size/object spread are fixed (provisionally ≤ 0.5 m)
- Blindfolded users locate a named object in **< 30 s** while standing/walking (provisional)
- A quantitative **user study with statistical analysis** (ANOVA / t-tests)

Deliverables: working software (Python + Unity), multi-camera calibration tooling, object dataset, latency/accuracy measurements, and the study results.

---

## 2. System architecture

Runtime pipeline: camera array → capture → two CV pipelines (object perception ‖ head pose) → scene model in one shared room/world frame → head-relative coordinate transform → UDP localhost streamer → Unity audio engine → headset.

**Key architectural principles:**

- **All spatial math lives in Python.** Unity is a "dumb" audio renderer that receives head-relative object positions and places audio sources there; it never computes poses.
- **ONE shared world frame**, fixed to the room and established by multi-camera extrinsic calibration. Every pose — objects and head — lives in this frame. There is no dominant-plane shortcut; objects sit at different heights.
- **Contract-first hardware abstraction.** Cameras, localizers, head-pose estimators, and audio devices sit behind interfaces (`CameraSource`, `AudioSink`, etc.), so Prototype gear swaps to Production gear without touching pipeline code.
- **Transition-first development.** The Prototype is built *against the contracts, not against its hardware*, so moving to Production is a backend swap, not a rewrite (§4, §5).

**Diagrams:** maintained as Mermaid source in [`docs/architecture.mmd`](docs/architecture.mmd) (system pipeline), [`docs/scenestate-update.mmd`](docs/scenestate-update.mmd) (one runtime update cycle), and [`docs/prototype-production-swap.mmd`](docs/prototype-production-swap.mmd) (kept-vs-swapped components). Each file pastes directly into Lucidchart's *Diagram as code* editor.

---

## 3. Technical design per subsystem

### 3.1 Camera capture & calibration

- **Prototype:** any smartphone ≥1080p streaming MJPEG over WiFi — Android via native RTSP/MJPEG (e.g., IP Webcam app), iPhone via DroidCam's Linux client or an iOS MJPEG-server app; Android and iPhone units can be mixed freely (§7.1). **RGB only** — object positions come from ARUCO solvePnP (markers act as one-shot registration stand-ins); no triangulation and no depth at this stage.
- **Production:** specialized cameras covering the room from several angles so objects *and* the user's head stay visible during turns and walks. Capture priority ladder: **① RGB primary → ② multi-view RGB triangulation for live 3D → ③ dedicated depth cameras last** (§7.3).
- **Calibration:** per-camera intrinsics via OpenCV chessboard; then joint **multi-camera extrinsic calibration** registering all cameras into the room/world frame. No dominant-plane shortcut exists at room scale.

### 3.2 Object perception

- **Prototype (`MarkerLocalizer` + `ObjectDetector` bring-up, RGB-only):** ARUCO markers stand in for real objects — a marker's solvePnP pose gives its 3D position directly in the camera/world frame, no plane assumption, any height. In parallel, YOLO (v8/v11) object identification is brought up early on contributed GPUs: small custom dataset (~100–300 images/class), **training offloaded to Google Colab (free T4), inference local on the RTX 4060**.
- **Production (`ObjectLocalizer` backends follow the capture ladder):** YOLO (v8/v11, Ultralytics) fine-tuned on a small custom dataset (~100–300 images/class) detects objects. World-frame positions escalate: ① one-shot setup registration of static objects (temporary ARUCO tags / laser meter), ② `TriangulatedLocalizer` — multi-view RGB triangulation for live re-localization when objects move, ③ a depth-camera backend only if both prove insufficient.
- **Tracking:** Kalman filter / ByteTrack on detections for temporal stability, so audio positions don't flicker between frames or across camera handoffs.

### 3.3 Head tracking (6DoF — position and orientation)

- **Prototype:** ARUCO markers worn on **each side of the user's head**. Both markers form a rigid body; solving their poses gives full head position + orientation in the world frame. Target 30–60 Hz with ordinary phone cameras.
- **Production:** markerless CV first; if unreliable — e.g., face turned away or occluded — fall back to a device attached to the headset for more accurate relative head orientation (IMU fusion, complementary/Kalman filter).
- **Smoothing:** low-pass / One-Euro filtering to keep audio positions jitter-free; multi-camera handoff must not cause audible jumps.

### 3.4 Coordinate transform (the core math)

- World frame: origin fixed to the room (e.g., floor center), z up. Established once by calibration.
- Head frame: origin at head position, axes from head rotation (Rodrigues → rotation matrix). Because the user walks, the transform uses the **full 6DoF head pose**, never rotation alone.
- For every object compute head-relative **azimuth φ, elevation θ, distance r**. These three values per object are streamed to Unity; this single transform is what keeps the soundscape anchored as the user turns and moves.

### 3.5 Spatial audio engine (Unity)

- **HRTF renderer:** 3D Tune-In Toolkit (Unity plugin) as primary — open-source, standard in assistive-audio research. Alternative: Steam Audio (stronger physical acoustics), Unity built-in spatializer for quick tests.
- **Sonification design:**
  - *Always-on (core concept):* each object class gets a distinctive continuous earcon (water bottle = soft bubbling, mug = chime, book = page rustle) rendered through the HRTF spatializer at the object's head-relative position.
  - *Query/beacon mode (extension):* keyboard first, then speech recognition in Python (Whisper/Vosk) or a button → target object emits periodic ~100 ms pings (noise bursts are easy to localize).
  - *Distance cues:* gain attenuation, low-pass filtering, slight reverb as distance grows.
- **Devices (`AudioSink`):** BT earbuds in Prototype (classic-BT SBC adds ~150–250 ms — measure it early). Production supports two sanctioned paths: a **wired** headset (zero codec latency) or a **wireless** headset using a proprietary 2.4 GHz USB dongle (~15–40 ms, gaming-grade) or LE Audio/LC3 (< 40 ms) — classic Bluetooth codecs (SBC/AAC/LDAC ≈ 100–250 ms) are the wireless option to avoid.

### 3.6 Networking & interconnect

- Python → Unity over **UDP** localhost with a compact binary protocol (protobuf/msgpack) at ~30–60 Hz.
- **Both wired and wireless interconnects are supported at every stage**, and §7 specifies components for both. The < 100 ms end-to-end target is kept; measured reality per link type:
  - *Wired cameras (USB):* negligible link latency — the path most likely to meet the target.
  - *Wireless cameras:* tuned MJPEG/raw-UDP over a dedicated WiFi 6 AP measures ~80–150 ms best case; naive RTSP/H.264 lands at 200–300 ms or worse (decoder buffering); Raspberry Pi edge-node WebRTC floors around ~200 ms.
  - *Wireless audio:* 2.4 GHz dongle ~15–40 ms vs classic BT ~150–250 ms.
  - A fully-wireless end-to-end chain therefore realistically measures ~150–400 ms. The gap is documented honestly: mitigations (dedicated AP, tuned low-latency streams, codec selection) are applied and measurements reported per path — the networking itself stays outside thesis scope.
- This is an engineering verification step, **not a thesis chapter**: latency is a requirement to satisfy and report, not a research question.

---

## 4. Data contracts (contract-first, critical for the transition)

Data schemas are defined **before** writing either implementation; both stages conform to the same contracts so the hardware swap never touches the rest of the system. All poses live in the one shared room/world frame. Because the user stands (and walks), head pose means full **6DoF** — position + orientation.

1. `Frame` — pixels + timestamp + camera intrinsics (capture abstraction)
2. `ObjectLocalizer` — `MarkerLocalizer` (ARUCO solvePnP, Prototype, RGB-only) → `TriangulatedLocalizer` (multi-view triangulation, Production) with an optional depth-camera backend
3. `HeadPoseEstimator` — ARUCO side-of-head markers (Prototype) → markerless CV, headset-attached device fallback (Production)
4. `SceneState` — UDP schema: object id/class/azimuth/elevation/distance, all relative to the current head pose. Defined and **frozen in the Prototype**, so the Unity engine is untouched by the hardware swap
5. `AudioSink` — output-device abstraction (BT earbuds → production headset)

---

## 5. Development phases

The stages exist for **financial reasons**: everything is built first on existing + approved hardware (Prototype), and all components are purchased only once the design has proven itself (Production). Both stages perform CV, audio, and integration/performance work.

| Phase | Stage | Duration | Milestone / exit criterion |
|---|---|---|---|
| **0. Literature review** | — | 2–3 wks | HRTF/binaural rendering, sonification & earcons, assistive object-locating systems. Gap analysis → justifies design choices. |
| **1. Prototype CV pipeline** | Prototype | 4–6 wks | WiFi phone capture behind `CameraSource`; `MarkerLocalizer` (solvePnP); early `ObjectDetector` bring-up — YOLO v8/v11, small custom dataset, training via Google Colab T4 / inference on the RTX 4060; ARUCO side-of-head `HeadPoseEstimator` (6DoF, 30–60 Hz); calibration tooling establishing the room/world frame. **RGB only — no triangulation, no depth.** |
| **2. Prototype audio engine** | Prototype | 3–4 wks | Unity scene + HRTF spatializer, earcon library, always-on + beacon mode (keyboard first), distance cues, `AudioSink` over BT earbuds. |
| **3. Prototype integration & performance** | Prototype | 3–4 wks | SceneState/UDP contract defined and **frozen**; threaded/async pipeline; end-to-end latency measured & mitigated on wired and wireless paths; validates the two risky unknowns: (A) is head-rotation-tracked spatial audio intuitive enough to locate objects? (B) does marker-based head tracking hold up at 30–60 Hz? **Built against the contracts, not the hardware — every piece must swap cleanly later.** |
| — | *Financial gate* | — | All Production components purchased (§7.3–§7.4); per-item approvals recorded in §7.2. |
| **4. Production CV pipeline** | Production | 4–6 wks | Specialized cameras (wired array and/or wireless nodes); multi-camera room-scale world-frame calibration; trained YOLO + real-object localization (one-shot registration escalating to multi-view triangulation; depth only if insufficient); IMU-fused 6DoF head pose with smoothing and camera-handoff stability. Validate CV accuracy vs tape-measure/ARUCO ground truth. |
| **5. Production audio & integration** | Production | 3–4 wks | Engines ported behind unchanged contracts onto production gear; full-room deployment; end-to-end latency verified per interconnect path. |
| **6. User study & thesis writing** | — | 6–10 wks | See §6. |

**Total: ~9–12 months** (part-time schedule roughly doubles this).

---

## 6. Evaluation plan (thesis)

All formal evaluation runs on the **Production system only**.

1. **CV accuracy:** detection precision/recall; object-position error vs tape-measure ground truth; head-pose error (position + orientation °) vs ARUCO/IMU ground truth.
2. **System performance (verification):** end-to-end latency < 100 ms, measured separately for the wired and wireless interconnect paths; audio-position stability during head turns and camera handoffs.
3. **User study (N ≈ 10–15, blindfolded sighted + optionally VI participants):** named-object localization while standing/walking in the room. Metrics: angular error, distance error, time-to-locate, path efficiency. Questionnaires: SUS, NASA-TLX.
4. **Hybrid-mode comparison:** always-on vs beacon vs both.
5. **Statistics:** ANOVA / t-tests on the above.

> ⚠️ If human participants are included, check the institution's ethics/IRB approval early — it affects scheduling.

---

## 7. Hardware requirements & cost breakdown

The two-stage hardware strategy exists for **financial reasons**: the Prototype stage uses only what already exists plus individually approved small purchases (§7.1–§7.2), while the Production stage buys **all** system components once the prototype has proven the design (§7.3–§7.4). Wired and wireless variants of network-facing components are specified for both stages. Each component below states what it is, which contract it fulfills, and why it is needed.

### 7.1 Prototype rig (Stage 1)

Goal: run the entire stack — CV pipeline, audio engine, integration & performance — on existing devices plus approved purchases (§7.2), validating the two risky unknowns end-to-end before any major spend.

**Smartphone(s) — capture (`CameraSource`)**
- What/why: the rig's imaging devices. Each streams video over WiFi so the Python pipeline receives `Frame`s exactly as it will from Production cameras.
- Specs: any smartphone with ≥1080p rear camera, 720p–1080p @ ≥30 fps; lock focus/exposure for tracking stability.
- **Android route:** **IP Webcam** (or similar) serving native RTSP/MJPEG.
- **iPhone route:** DroidCam iOS app + official Linux client (droidcam), or an iOS MJPEG-server app feeding OpenCV directly. iOS has no native RTSP server, so the route's added stream latency must be measured early (same treatment as the BT earbuds).
- **Mixed fleets work:** each device is just an independent `CameraSource` backend. Synchronization is host-timestamp based, calibration registers every camera into the shared world frame regardless of brand, and per-source latency/color differences are absorbed by per-camera capture threads plus smoothing filters.
- One phone suffices to start; a second (any OS) approximates multi-camera handoff.

**Phone tripod mounts**
- What/why: hold the phone rigidly and repeatably — loose, handheld cameras break both calibration and marker tracking.
- Specs: spring clamps with standard ¼"-20 tripod thread; small desk tripod or shelf mount.

**Printed ARUCO markers — object stand-ins + head rig**
- What/why: markers replace real objects (`MarkerLocalizer` solves their pose directly, no plane assumption), and a pair worn on the head gives the `HeadPoseEstimator` its rigid reference body.
- Specs: matte A4 prints (glare kills detection), 5×5 dictionary (e.g., `DICT_5X5_50`), 5–10 cm per side; two mounted on a cap/headband at ear height (left/right), one per tagged object.

**Bluetooth earbuds — output (`AudioSink`)**
- What/why: the listening device for the spatialized soundscape; good enough to judge intuitiveness, cheap enough to be throwaway.
- Specs: any TWS set. Expect SBC codec latency of ~100–200 ms — measure it early; this is a Prototype-only concern and never informs Production design.

**Desktop PCs — compute**
- What/why: primary hosts for Python CV, the UDP streamer, Unity, and YOLO object-ID work — two researcher-owned machines (§7.2). **PC #1 (Ryzen 5 3500 + RTX 4060, 64 GB)** leads: YOLO inference runs on its GPU while **training is offloaded to Google Colab (free T4)** so it never competes with live sessions; local training is the offline fallback. **PC #2 (i7-10700 + RTX 3050, 16 GB)** is the fallback host per the §7.2 escalation ladder.
- Specs floor met: ARUCO solvePnP costs a few ms/frame on CPU; YOLOv8n/s trains comfortably at this dataset scale on an 8 GB-class GPU; HRTF rendering is CPU/DSP work. Python 3.10+, Unity 2022 LTS. If multi-phone MJPEG decode ever lags during all-camera sessions, drop streams to 720p.
- The laptop remains a backup / stream-test client only.

**Home WiFi router**
- What/why: transports the phone's video stream to the laptop.
- Specs: existing router acceptable; prefer 5 GHz for stream stability; phone and laptop on the same band.

**Measurement & consumables**
- What/why: sanity-check ground truth (is the sound where the marker actually is?) and calibration input.
- Specs: steel tape measure; printed OpenCV chessboard; tape/adhesive for markers.

### 7.2 Prototype hardware purchase approval

The Prototype stage buys **only** the items approved below — everything else must come from existing personal equipment (marked ₱0). Each researcher marks **yes** in their column to approve that item's purchase for the prototype stage; purchases happen jointly.

| Component | Subsystem | Qty | Price/unit (₱) | Cost (₱) | Requirement | Researcher 1 | Researcher 2 | Researcher 3 |
|---|---|---|---|---|---|---|---|---|
| Smartphone #1 | Capture (wireless) | 1 | 0 *(existing; buy-alt 4,000 – 8,000)* | 0 | Required | yes | | |
| Smartphone #2 *(optional second camera)* | Capture (wireless) | 1 | 0 *(existing)* | 0 | Optional | yes | | |
| Desktop PC #1 — Ryzen 5 3500 + RTX 4060, 64 GB | Compute | 1 | 0 *(existing)* | 0 | Required *(primary compute host)* | yes | | |
| Desktop PC #2 — i7-10700 + RTX 3050, 16 GB | Compute | 1 | 0 *(existing)* | 0 | Optional *(fallback host; see compute ladder below)* | yes | | |
| Laptop | Compute | 1 | 0 *(existing)* | 0 | Optional *(backup / stream-test client)* | yes | | |
| Home WiFi router | Networking (wireless) | 1 | 0 *(existing)* | 0 | Required | yes | | |
| Bluetooth TWS earbuds | Audio output (wireless) | 1 | 0 *(existing; buy-alt 800 – 2,500)* | 0 | Required | yes | | |
| Phone tripod mounts | Mounting | 2 | 150 – 350 | 300 – 700 | Required | yes | | |
| Mini desk tripods *(alt to clamps)* | Mounting | 2 | 100 – 250 | 200 – 500 | Optional | yes | | |
| Matte A4 ARUCO prints | Perception consumables | 10 | 10 – 20 | 100 – 200 | Required | yes | | |
| Cap/headband for side markers | Head tracking | 1 | 50 – 150 | 50 – 150 | Required | yes | | |
| Steel tape measure | Calibration / ground truth | 1 | 80 – 250 | 80 – 250 | Required | yes | | |
| Chessboard print + tape/adhesive | Calibration consumables | 1 | 50 – 150 | 50 – 150 | Required | yes | | |
| Dedicated WiFi 6 router (Archer AX23-class) | Networking (wireless) | 1 | 2,700 – 3,000 | 2,700 – 3,000 | Conditional *(only if home AP proves congested)* | yes | | |
| Gigabit switch + Ethernet patch cables | Networking (wired) | 1 | 800 – 1,500 | 800 – 1,500 | Optional *(wired laptop↔router stability path)* | yes | | |
| Power bank (long streaming sessions) | Power | 1 | 500 – 1,200 | 500 – 1,200 | Optional | yes | | |
| **Required-purchase subtotal** | | | | **≈ 580 – 1,450** | | yes | | |

*The Subsystem field says what the component handles; networking items are marked wired, wireless, or either. Prices surveyed August 2026 via Shopee/Lazada PH street channels.*

*Compute escalation ladder if YOLO/pipeline work stalls on compute: **① no-swap** — PC #1 hosts everything, training offloaded to Google Colab (free T4), inference local on the RTX 4060; **② two-box split** — CV stays on PC #1 while Unity/audio moves to PC #2 over LAN UDP; **③ parts swap** — consolidate i7-10700 + RTX 4060 + 64 GB into one chassis (Cooler Master MWE 750 230V verified adequate at ~300–350 W system peak vs 750 W). Escalate one rung only when limits are actually hit.*

### 7.3 Production system (Stage 2)

Goal: a fixed, calibrated room installation accurate enough for formal evaluation, with every component swappable behind its contract.

**RGB camera array — Arducam OV9782 color global shutter (×3) — recommended wired path (`CameraSource`)**
- What/why: primary production capture, tier ① of the ladder. Three calibrated color cameras detect objects with YOLO and localize them via one-shot setup registration escalating to **multi-view triangulation** — same `CameraSource` / `ObjectLocalizer` contracts throughout.
- Specs: OV9782 1 MP color global-shutter sensor, UVC (driverless on Linux), low-distortion M12 lens; external-trigger sync is supported across Arducam's global-shutter family, while software sync is acceptable for a near-static room.
- Cheaper mono variant (OV9281) exists but complicates standard color-based YOLO — choose it only if grayscale retraining is acceptable.

**Wireless camera array — Raspberry Pi 5 + Camera Module 3 edge nodes (×3) — recommended wireless path (`CameraSource`)**
- What/why: the same RGB-first design fully cut loose from cables. Each node streams tuned MJPEG/raw-UDP (MediaMTX/WebRTC class) over a dedicated WiFi 6 AP into the same `Frame` contract — no USB tethering anywhere.
- Specs per node: RPi 5 (4 GB) + Camera Module 3 + PSU/microSD/case ≈ ₱7,000–9,000 landed; stream latency floors around ~200 ms even when tuned.
- Budget alternative: commercial RTSP IP cameras (₱2,500–6,000/unit) — cheapest per unit but typical 200–300 ms stream latency, rolling shutter, and no control over frame sync.
- Trade-offs vs wired USB arrays: total cable freedom for room-corner placement; pays a ~₱8k+ premium over the OV9782 array and accepts higher stream latency (§3.6).

**Dedicated depth array — Intel RealSense D435 (×2–3) — optional upgrade (`CameraSource`)**
- What/why: tier ③ of the ladder — bought only if one-shot registration and multi-view triangulation both prove insufficient in Phase 4. Its unique value is single-view robustness: metric depth per pixel means one unoccluded view still yields a world-frame position when the walking user blocks every other camera.
- Specs: global shutter, up to 90 fps depth, ~87°×58° FOV, USB-C 3.1, SDK 2.0 on Linux; the D435i variant adds an onboard IMU (handy reference, not required).
- Placement if used: two cameras cover opposite room diagonals; three so the walking user's body rarely occludes every view at once.

**Camera mounting & USB infrastructure**
- What/why: extrinsic calibration is only valid while cameras stay perfectly still; and every USB camera demands dependable bandwidth.
- Specs: sturdy tripods or wall clamps (¼"-20 UNC); powered USB-C hub; active extensions beyond 2 m; plan host-controller lanes so cameras don't share bandwidth.
- Carry-over facts: retail D435 units include their own mini tripod (bench use), and any prototype tripod with a standard ¼"-20 screw can hold a D435 (~72 g) — partial reuse is real. Phone cradles do not transfer: they clamp a handset shape rather than a threaded mount, and desk stands don't suit room-corner placement.

**Production headset — output (`AudioSink`, wired or wireless)**
- What/why: delivers the binaural soundscape; HRTF rendering happens host-side. Both sanctioned paths keep added latency far below classic Bluetooth.
- Wired: closed-back headphones (ATH-M20x class, ₱2,000–4,500) — zero codec latency, the safest path for the latency budget.
- Wireless: gaming-grade **2.4 GHz USB-dongle** headset (~15–40 ms, ₱1,500–4,500 locally) or LE Audio/LC3 (< 40 ms) where available. Avoid classic-BT codecs (SBC/AAC/LDAC = 100–250 ms).

**Headset-attached orientation device (fallback)**
- What/why: concept.md's designated fallback — when cameras lose the face (turned away, occluded), a head-worn IMU keeps *orientation* accurate. Head *position* still comes from the camera array; the device contributes rotation only.
- Specs: BNO085 9-axis IMU module (fused absolute orientation output) + ESP32 dev board streaming over USB-serial/BLE + light head strap integrated with the headset.

**GPU workstation — compute**
- What/why: trains and runs YOLO, ingests multiple camera streams, computes transforms, and hosts Unity.
- Specs floor: 8 GB VRAM GPU (RTX 4060 class), 6-core CPU, 32 GB RAM, Ubuntu 22.04+, multiple USB 3.1 ports.
- Options: (a) drop the GPU into an existing desktop; (b) purpose-built tower; (c) gaming laptop — verify sustained thermals for hours-long inference sessions.

**Network kit — wired and wireless**
- What/why: both interconnects are supported, so both kits are specified; per-experiment choice is made by measurement (§3.6).
- Wired: Cat6 patch cables + 5-port gigabit switch (₱1,000–2,000) — the lowest-latency camera→host path.
- Wireless: dedicated WiFi 6 access point (TP-Link Archer AX23-class, ₱2,700–3,000) on its own SSID/channel so camera streams never share airtime with household traffic.

**Ground-truth & validation kit**
- What/why: thesis evaluation needs independent truth to score CV accuracy against.
- Specs: laser distance meter + steel tape (object-position ground truth); spare printed markers (head-pose validation reference in Phase 4).

### 7.4 Cost breakdown (Philippine Pesos)

> **Assumptions:** prices surveyed August 2026 via Philippine street channels (Shopee/Lazada/official stores), USD converted at **₱61/USD** (Aug 2026 mid-market). Import shipping, customs duties, and FX movement (2026 band ≈ ₱57–62) are **not** included. Items marked *(existing)* assume personal assets are reused at zero cost.

**Prototype:**

| Item | Est. cost (₱) | Notes |
|---|---|---|
| Phone tripod mounts ×2 | 400 – 1,200 | generic clamps |
| ARUCO marker printing | 100 – 200 | matte A4 |
| Tape measure, chessboard, misc | 300 – 500 | |
| Bluetooth earbuds | 0 *(existing)* | budget TWS alternative: 800 – 2,500 |
| Smartphone | 0 *(existing)* | budget/second-hand alternative: 4,000 – 8,000 |
| Laptop + WiFi router | 0 *(existing)* | |
| **Total — everything reusable** | **≈ 800 – 1,900** | |
| **Total — if earbuds + phone must be bought** | **≈ 5,600 – 12,600** | |

**Production common items** (bought regardless of camera/compute choice):

| Item | Est. cost (₱) | Notes |
|---|---|---|
| Mounts, powered hub, cables | 3,000 – 6,000 | room-scale rigging; see carry-over facts in §7.3 |
| Headset — wired *(buy one of the two)* | 2,000 – 4,500 | zero-codec-latency path |
| Headset — 2.4 GHz dongle wireless *(buy one of the two)* | 1,500 – 4,500 | wireless path |
| IMU fallback kit (BNO085 + ESP32 + strap) | 1,300 – 2,800 | BLE stream — already wireless-capable |
| Network kit — wired (Cat6 + gigabit switch) | 1,000 – 2,000 | both network kits are bought: dual-path policy |
| Network kit — wireless (dedicated WiFi 6 AP) | 2,700 – 3,000 | Archer AX23-class |
| Laser meter + validation markers | 1,500 – 2,500 | |
| **Common subtotal** (headset counted once) | **≈ 11,000 – 20,800** | |

**Camera subsystem — pick one (listed in priority order):**

| Option | Est. cost (₱) | Notes |
|---|---|---|
| RGB ×3 — Arducam OV9782 color GS (USB wired) *(recommended wired)* | 10,500 – 18,000 | ≈ ₱3,500–6,000/unit landed; registration → triangulation backend |
| Wireless RGB ×3 — RPi 5 + Camera Module 3 edge nodes *(recommended wireless)* | 21,000 – 27,000 | ≈ ₱7,000–9,000/node; MediaMTX/WebRTC, ~200 ms floor |
| Wireless RGB ×3 — RTSP IP cameras *(budget alt)* | 7,500 – 18,000 | ₱2,500–6,000/unit; 200–300 ms typical |
| Depth ×2 — RealSense D435 (USB wired; optional upgrade) | 42,000 – 50,000 | $314 ea. list + landing; mini tripod included |
| Depth ×3 — RealSense D435 (USB wired; optional upgrade) | 63,000 – 75,000 | single-view occlusion redundancy |

**Compute — pick one:**

| Option | Est. cost (₱) | Notes |
|---|---|---|
| GPU upgrade (RTX 4060 class, + PSU if needed) | 22,000 – 30,000 | into an existing desktop |
| Dedicated workstation (Ryzen 5/i5, 32 GB, RTX 4060–4070) | 60,000 – 90,000 | |

**Grand totals** (common + camera choice + compute choice):

| Camera choice (priority order) | + GPU upgrade | + Dedicated workstation |
|---|---|---|
| RGB ×3 OV9782 (USB) | ≈ 43,500 – 71,800 | **≈ 81,500 – 131,800** *(reference build)* |
| Wireless ×3 RPi nodes | ≈ 54,000 – 77,800 | ≈ 92,000 – 137,800 |
| Depth ×2 (optional upgrade) | ≈ 75,000 – 100,800 | ≈ 113,000 – 160,800 |
| Depth ×3 (optional upgrade) | ≈ 96,000 – 125,800 | ≈ 134,000 – 185,800 |

*(The IP-camera budget alternative lands between the OV9782 and RPi-node totals.)*

**Cost notes:**

- The RGB-first priority ladder keeps the reference build affordable; depth upgrades add **₱31,000–57,000** over the OV9782 array and dominate cost if chosen.
- **Dual-path premium:** buying both network kits and specifying wired *and* wireless cameras adds flexibility for measurement and study conditions; the wired array remains the lowest-latency reference while wireless nodes trade ~₱8–12k and +100–200 ms of stream latency for placement freedom.
- **OV9782 chosen over mono OV9281** to keep standard color-based YOLO; mono would force grayscale retraining.
- Depth remains an *upgrade*, not a default: add D435s only if Phase 4 shows registration and triangulation insufficient. If ordering them, note the official store flags a **2–3 week lead time and tariff surcharge**.
- Reusing existing researcher hardware (desktops/laptop) as Prototype dev hosts keeps Stage 1 near-zero cost; Production compute is still purchased new at the financial gate.

---

## 8. Risks & open questions

- **Generic HRTF accuracy** → use a good generic HRTF; individualized HRTF is natural *future work*.
- **YOLO dataset effort** → keep object set small (≤ 5–8 classes) for the study.
- **Head pose drift when the face turns away** → headset-attached device fallback in Production.
- **Room-scale occlusion / camera coverage** → user body or head rotation can hide markers from a single camera → multiple cameras, marker placement validated in Phase 4; multi-camera handoff must not cause audio jumps.
- **Classic-BT earbud latency (~150–250 ms)** → measured early in the Prototype; Production wireless audio uses 2.4 GHz dongle/LC3 instead.
- **Wireless links add latency/jitter** → measured reality: tuned MJPEG/raw-UDP ~80–150 ms, RTSP/IP-cam 200–300 ms, RPi-edge WebRTC ~200 ms; classic-BT audio 150–250 ms vs 2.4 GHz dongle 15–40 ms. Both interconnects are built and measured (Phase 3 Prototype, Phase 5 Production); the gap vs the 100 ms target is mitigated (dedicated AP, tuned streams, codec choice) and reported honestly, not studied.
- **Prototype hacks leaking into Production** → prevented contract-first; see §4 and the handoff rule in §5 Phase 3.

Open questions:

- Exact object set/count for the study
- Room dimensions and object spread → finalize distance-error target and time-to-locate threshold
- Query language for speech recognition
- Whether the study recruits real VI participants (ethics board)
