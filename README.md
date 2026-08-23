# Auris — Development Plan

A system that helps visually impaired users locate objects in a room through simulated auditory cues. The user stands in a room containing various objects; a camera array tracks both the objects and the user's head — position **and** rotation, since a standing user can turn and walk; a headset renders a 3D soundscape where every object continuously emits a distinctive sound anchored to its real position. Turning the head rotates the sound field correctly so it stays fixed to the world.

**Context:** Thesis project. Development runs in two stages: a throwaway **Prototype** on non-specialized gear (phones as cameras, ARUCO markers, Bluetooth earbuds) that validates the two risky unknowns, followed by the **Production** system on specialized gear, which carries all formal evaluation and the user study. Stack: Python CV → Unity spatial audio over UDP localhost. End-to-end latency < 100 ms is an engineering target that is verified, not researched.

---

## 1. End-state vision & measured outcomes

A standing, blindfolded (or visually impaired) user in a room with objects placed around them — on the floor, shelves, and furniture at varied heights. A camera array covers the room and tracks every object plus the user's head as a full 6DoF pose. Through the headset the user hears a **3D soundscape**: each object continuously emits a unique earcon positioned where the object actually sits. As the user turns or walks, sounds stay anchored to their real-world positions. Saying *"find the bottle"* triggers beacon mode, where the bottle pings periodically so the user can home in.

Measured outcomes for the thesis:

- End-to-end latency **< 100 ms** (verified engineering target — see §6)
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

- **Prototype:** any smartphone ≥1080p streaming MJPEG over WiFi — Android via native RTSP/MJPEG (e.g., IP Webcam app), iPhone via DroidCam's Linux client or an iOS MJPEG-server app; Android and iPhone units can be mixed freely (§7.1). RGB only; if depth-like capability is explored, phones triangulate from multiple RGB views.
- **Production:** specialized RGB cameras — or depth cameras if RGB proves insufficient — covering the room from several angles so objects *and* the user's head stay visible during turns and walks.
- **Calibration:** per-camera intrinsics via OpenCV chessboard; then joint **multi-camera extrinsic calibration** registering all cameras into the room/world frame. No dominant-plane shortcut exists at room scale.

### 3.2 Object perception

- **Prototype (`MarkerLocalizer`):** ARUCO markers stand in for real objects. A marker's solvePnP pose gives its 3D position directly in the camera/world frame — no plane assumption, works at any height.
- **Production (`DepthLocalizer`):** YOLO (v8/v11, Ultralytics) fine-tuned on a small custom dataset (~100–300 images/class) detects objects; depth cameras or stereo/multi-view RGB triangulation yields world-frame 3D positions.
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
- **Devices (`AudioSink`):** BT earbuds in Prototype (SBC codec adds ~100–200 ms — measure it early; Prototype-only concern); Production headset wired or low-latency codec (LDAC/aptX Low Latency).

### 3.6 Networking & interconnect decision

- Python → Unity over **UDP** localhost with a compact binary protocol (protobuf/msgpack) at ~30–60 Hz.
- Camera links may be wired or wireless in Production; **the choice is made by measured end-to-end latency against the < 100 ms standard** (Phase 4). Wireless stays only if it meets the standard.
- This is an engineering verification step, **not a thesis chapter**: latency is a requirement to satisfy, not a research question.

---

## 4. Data contracts (contract-first, critical for the transition)

Data schemas are defined **before** writing either implementation; both stages conform to the same contracts so the hardware swap never touches the rest of the system. All poses live in the one shared room/world frame. Because the user stands (and walks), head pose means full **6DoF** — position + orientation.

1. `Frame` — pixels + timestamp + camera intrinsics (capture abstraction)
2. `ObjectLocalizer` — `MarkerLocalizer` (ARUCO solvePnP, Prototype) → `DepthLocalizer` (depth/stereo triangulation, Production)
3. `HeadPoseEstimator` — ARUCO side-of-head markers (Prototype) → markerless CV, headset-attached device fallback (Production)
4. `SceneState` — UDP schema: object id/class/azimuth/elevation/distance, all relative to the current head pose. Defined and **frozen in the Prototype**, so the Unity engine is untouched by the hardware swap
5. `AudioSink` — output-device abstraction (BT earbuds → production headset)

---

## 5. Development phases

| Phase | Duration | Milestone / exit criterion |
|---|---|---|
| **0. Literature review** | 2–3 wks | HRTF/binaural rendering, sonification & earcons, assistive object-locating systems. Gap analysis → justifies design choices. |
| **1. Prototype (thin vertical slice)** | ~3 wks | Validates ONLY the two risky unknowns: (A) is head-rotation-tracked spatial audio intuitive enough to locate objects? (B) does ARUCO-marker head tracking hold up at 30–60 Hz on ordinary cameras? Scope: 1 phone camera + ARUCO markers on the sides of the head + one marker "object" + one spatialized loop in Unity + BT earbuds. NO YOLO, NO speech, NO hybrid modes, NO full room calibration. SceneState/UDP contract defined here and frozen. **Built against the contracts, not the hardware — every piece must swap cleanly later.** |
| **2. Production CV pipeline** | 4–6 wks | Specialized cameras; multi-camera world-frame calibration (room scale); trained YOLO + DepthLocalizer; 6DoF head pose with smoothing and camera-handoff stability; head-relative transform. Validate CV accuracy vs tape-measure/ARUCO ground truth. |
| **3. Production audio engine** | 3–4 wks | Unity scene, 3D Tune-In integration, earcon library, hybrid always-on + beacon mode (keyboard first, then speech), UDP protocol, distance cues. |
| **4. Integration & performance** | 3–4 wks | Threaded/async pipeline; tune to end-to-end < 100 ms; measure and decide wired vs wireless interconnect. Latency is an engineering target, not a thesis chapter. |
| **5. User study & thesis writing** | 6–10 wks | See §6. |

**Total: ~9–12 months** (part-time schedule roughly doubles this).

---

## 6. Evaluation plan (thesis)

All formal evaluation runs on the **Production system only**.

1. **CV accuracy:** detection precision/recall; object-position error vs tape-measure ground truth; head-pose error (position + orientation °) vs ARUCO/IMU ground truth.
2. **System performance (verification):** end-to-end latency < 100 ms — this measurement also decides the Production interconnect; audio-position stability during head turns and camera handoffs.
3. **User study (N ≈ 10–15, blindfolded sighted + optionally VI participants):** named-object localization while standing/walking in the room. Metrics: angular error, distance error, time-to-locate, path efficiency. Questionnaires: SUS, NASA-TLX.
4. **Hybrid-mode comparison:** always-on vs beacon vs both.
5. **Statistics:** ANOVA / t-tests on the above.

> ⚠️ If human participants are included, check the institution's ethics/IRB approval early — it affects scheduling.

---

## 7. Hardware requirements & cost breakdown

The two-stage hardware strategy mirrors the development stages: the Prototype runs on whatever non-specialized gear is available, while the Production system is specified once, acquired deliberately, and carries every formal evaluation. Each component below states what it is, which contract it fulfills, and why it is needed.

### 7.1 Prototype rig (Stage 1)

Goal: validate the two risky unknowns (head-rotation-tracked audio intuitiveness; ARUCO-marker head tracking at 30–60 Hz) with minimal spend, reusing personal devices wherever possible.

**Smartphone(s) — capture (`CameraSource`)**
- What/why: the rig's imaging devices. Each streams video over WiFi so the Python pipeline receives `Frame`s exactly as it will from Production cameras.
- Specs: any smartphone with ≥1080p rear camera, 720p–1080p @ ≥30 fps; lock focus/exposure for tracking stability.
- **Android route:** **IP Webcam** (or similar) serving native RTSP/MJPEG.
- **iPhone route:** DroidCam iOS app + official Linux client (droidcam), or an iOS MJPEG-server app feeding OpenCV directly. iOS has no native RTSP server, so the route's added stream latency must be measured early (same treatment as the BT earbuds).
- **Mixed fleets work:** each device is just an independent `CameraSource` backend. Synchronization is host-timestamp based, calibration registers every camera into the shared world frame regardless of brand, and per-source latency/color differences are absorbed by per-camera capture threads plus smoothing filters.
- One phone suffices for the Phase 1 slice; a second (any OS) approximates multi-camera handoff later.

**Phone tripod mounts**
- What/why: hold the phone rigidly and repeatably — loose, handheld cameras break both calibration and marker tracking.
- Specs: spring clamps with standard ¼"-20 tripod thread; small desk tripod or shelf mount.

**Printed ARUCO markers — object stand-ins + head rig**
- What/why: markers replace real objects (`MarkerLocalizer` solves their pose directly, no plane assumption), and a pair worn on the head gives the `HeadPoseEstimator` its rigid reference body.
- Specs: matte A4 prints (glare kills detection), 5×5 dictionary (e.g., `DICT_5X5_50`), 5–10 cm per side; two mounted on a cap/headband at ear height (left/right), one per tagged object.

**Bluetooth earbuds — output (`AudioSink`)**
- What/why: the listening device for the spatialized soundscape; good enough to judge intuitiveness, cheap enough to be throwaway.
- Specs: any TWS set. Expect SBC codec latency of ~100–200 ms — measure it early; this is a Prototype-only concern and never informs Production design.

**Laptop — compute**
- What/why: hosts Python CV, the UDP streamer, and Unity simultaneously.
- Specs: existing laptop is fine — ARUCO solvePnP plus a single Unity stream run comfortably on CPU. Python 3.10+, Unity 2022 LTS; discrete GPU not required at this stage.

**Home WiFi router**
- What/why: transports the phone's video stream to the laptop.
- Specs: existing router acceptable; prefer 5 GHz for stream stability; phone and laptop on the same band.

**Measurement & consumables**
- What/why: sanity-check ground truth (is the sound where the marker actually is?) and calibration input.
- Specs: steel tape measure; printed OpenCV chessboard; tape/adhesive for markers.

### 7.2 Production system (Stage 2)

Goal: a fixed, calibrated room installation accurate enough for formal evaluation, with every component swappable behind its contract.

**Depth camera array — Intel RealSense D435 (×2–3) — capture (`CameraSource`)**
- What/why: active infrared-stereo depth cameras giving metric depth per pixel, so `DepthLocalizer` obtains world-frame object positions directly at any height — no plane assumption — and the same array tracks the user's head through turns and walks.
- Specs: global shutter, up to 90 fps depth, ~87°×58° FOV, USB-C 3.1, SDK 2.0 on Linux. The D435i variant adds an onboard IMU (handy reference, not required).
- Placement: two cameras cover opposite room diagonals; **three recommended** so the walking user's body rarely occludes every view at once.
- If plain RGB proves sufficient in Phase 2 (concept.md's sanctioned fallback), switch to the RGB array below — same contracts, different localization backend.

**RGB camera array (OR-alternative) — Arducam OV9782 color global shutter (×3)**
- What/why: concept.md's sanctioned alternative — if plain RGB proves sufficient in Phase 2, three calibrated color cameras localize objects via **multi-view triangulation** instead of per-pixel depth. Same `CameraSource` / `ObjectLocalizer` contracts; only the backend swaps.
- Specs: OV9782 1 MP color global-shutter sensor, UVC (driverless on Linux), low-distortion M12 lens; external-trigger sync is supported across Arducam's global-shutter family, while software sync is acceptable for a near-static room.
- Trade-offs vs the D435: cuts roughly ₱45,000–57,000 off the camera subsystem and removes depth-map dependence; costs more integration effort (triangulation pipeline, stricter sync discipline) and gives no out-of-the-box metric depth.
- Cheaper mono variant (OV9281) exists but complicates standard color-based YOLO — choose it only if grayscale retraining is acceptable.

**Camera mounting & USB infrastructure**
- What/why: extrinsic calibration is only valid while cameras stay perfectly still; and every USB camera demands dependable bandwidth.
- Specs: sturdy tripods or wall clamps (¼"-20 UNC); powered USB-C hub; active extensions beyond 2 m; plan host-controller lanes so cameras don't share bandwidth.
- Carry-over facts: retail D435 units include their own mini tripod (bench use), and any prototype tripod with a standard ¼"-20 screw can hold a D435 (~72 g) — partial reuse is real. Phone cradles do not transfer: they clamp a handset shape rather than a threaded mount, and desk stands don't suit room-corner placement.

**Wired headset — output (`AudioSink`)**
- What/why: delivers the binaural soundscape with zero wireless-codec latency; HRTF rendering happens host-side.
- Specs: closed-back wired headphones (ATH-M20x class or similar). Low-latency Bluetooth (aptX-LL/LDAC) is acceptable only if it survives the Phase 4 latency measurement.

**Headset-attached orientation device (fallback)**
- What/why: concept.md's designated fallback — when cameras lose the face (turned away, occluded), a head-worn IMU keeps *orientation* accurate. Head *position* still comes from the camera array; the device contributes rotation only.
- Specs: BNO085 9-axis IMU module (fused absolute orientation output) + ESP32 dev board streaming over USB-serial/BLE + light head strap integrated with the headset.

**GPU workstation — compute**
- What/why: trains and runs YOLO, ingests multiple camera streams, computes transforms, and hosts Unity.
- Specs floor: 8 GB VRAM GPU (RTX 4060 class), 6-core CPU, 32 GB RAM, Ubuntu 22.04+, multiple USB 3.1 ports.
- Options: (a) drop the GPU into an existing desktop; (b) purpose-built tower; (c) gaming laptop — verify sustained thermals for hours-long inference sessions.

**Wired network kit**
- What/why: default camera→host interconnect chosen to protect the < 100 ms budget before wireless is even considered.
- Specs: Cat6 patch cables + 5-port gigabit switch. Wireless remains a candidate only if Phase 4 measures end-to-end latency under the standard with it.

**Ground-truth & validation kit**
- What/why: thesis evaluation needs independent truth to score CV accuracy against.
- Specs: laser distance meter + steel tape (object-position ground truth); spare printed markers (head-pose validation reference in Phase 2).

### 7.3 Cost breakdown (Philippine Pesos)

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
| Mounts, powered hub, cables | 3,000 – 6,000 | room-scale rigging; see carry-over facts in §7.2 |
| Wired headset | 2,000 – 4,500 | |
| IMU fallback kit (BNO085 + ESP32 + strap) | 1,300 – 2,800 | |
| Network kit (Cat6 + gigabit switch) | 1,000 – 2,000 | |
| Laser meter + validation markers | 1,500 – 2,500 | |
| **Common subtotal** | **≈ 8,800 – 17,800** | |

**Camera subsystem — pick one:**

| Option | Est. cost (₱) | Notes |
|---|---|---|
| Depth ×2 — RealSense D435 | 42,000 – 50,000 | $314 ea. list + landing; mini tripod included |
| Depth ×3 — RealSense D435 *(recommended)* | 63,000 – 75,000 | occlusion-robust coverage |
| RGB ×3 — Arducam OV9782 color GS *(OR-alternative)* | 10,500 – 18,000 | ≈ ₱3,500–6,000/unit landed; triangulation backend |

**Compute — pick one:**

| Option | Est. cost (₱) | Notes |
|---|---|---|
| GPU upgrade (RTX 4060 class, + PSU if needed) | 22,000 – 30,000 | into an existing desktop |
| Dedicated workstation (Ryzen 5/i5, 32 GB, RTX 4060–4070) | 60,000 – 90,000 | |

**Grand totals** (common + camera choice + compute choice):

| | GPU upgrade | Dedicated workstation |
|---|---|---|
| Depth ×2 | ≈ 73,000 – 98,000 | ≈ 111,000 – 158,000 |
| Depth ×3 | ≈ 94,000 – 123,000 | **≈ 132,000 – 183,000** *(reference build)* |
| RGB ×3 (OV9782) | **≈ 41,000 – 66,000** | ≈ 79,000 – 126,000 |

**Cost notes:**

- In the depth configurations **cameras dominate** (~45–55%); the RGB-first path cuts total system cost by roughly 40%.
- **OV9782 chosen over mono OV9281** to keep standard color-based YOLO; mono would force grayscale retraining.
- The RGB-vs-depth decision gate is the Phase 2 RGB-sufficiency test — but the D435's official store flags a **2–3 week lead time and tariff surcharge**, so a depth decision must be ordered well before Phase 2 starts.
- Reusing the Prototype laptop as the Unity host can shave ₱10,000–25,000 off any cell above.

---

## 8. Risks & open questions

- **Generic HRTF accuracy** → use a good generic HRTF; individualized HRTF is natural *future work*.
- **YOLO dataset effort** → keep object set small (≤ 5–8 classes) for the study.
- **Head pose drift when the face turns away** → headset-attached device fallback in Production.
- **Room-scale occlusion / camera coverage** → user body or head rotation can hide markers from a single camera → multiple cameras, marker placement validated in Phase 2; multi-camera handoff must not cause audio jumps.
- **BT earbud latency (~100–200 ms)** → Prototype-only concern; irrelevant to the production headset.
- **Wireless links add latency/jitter** → prototype phone-stream routes (Android RTSP/MJPEG, iOS/DroidCam-over-WiFi) are measured early as Prototype-only concerns; in Production, wireless is measured in Phase 4 and stays only if end-to-end latency < 100 ms holds, otherwise the system goes wired.
- **Prototype hacks leaking into Production** → prevented contract-first; see §4 and the handoff rule in §5 Phase 1.

Open questions:

- Exact object set/count for the study
- Room dimensions and object spread → finalize distance-error target and time-to-locate threshold
- Query language for speech recognition
- Whether the study recruits real VI participants (ethics board)
