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

- **Prototype:** phones streaming RTSP/MJPEG (e.g., IP Webcam app) over WiFi. RGB only; if depth-like capability is explored, phones triangulate from multiple RGB views.
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

## 7. Hardware roadmap

| Component | Prototype | Production |
|---|---|---|
| Room cameras | Phone(s) (RTSP/MJPEG, WiFi) | Specialized RGB/depth cameras covering the room (e.g., RealSense D435) |
| Head tracking | Phone camera viewing ARUCO markers worn on the sides of the head | Fixed specialized cameras (markerless CV); headset-attached device if CV is unreliable |
| Object marking | ARUCO markers stand in for objects | Real objects, identified by YOLO |
| Audio | Bluetooth earbuds | Low-latency headset (host-side HRTF) |
| Compute | Laptop | Same (GPU for YOLO) |
| Interconnect | WiFi | Wired or wireless — chosen by measured latency vs the < 100 ms standard |

---

## 8. Risks & open questions

- **Generic HRTF accuracy** → use a good generic HRTF; individualized HRTF is natural *future work*.
- **YOLO dataset effort** → keep object set small (≤ 5–8 classes) for the study.
- **Head pose drift when the face turns away** → headset-attached device fallback in Production.
- **Room-scale occlusion / camera coverage** → user body or head rotation can hide markers from a single camera → multiple cameras, marker placement validated in Phase 2; multi-camera handoff must not cause audio jumps.
- **BT earbud latency (~100–200 ms)** → Prototype-only concern; irrelevant to the production headset.
- **Wireless links add latency/jitter** → measured in Phase 4; wireless stays in Production only if end-to-end latency stays < 100 ms, otherwise the system goes wired.
- **Prototype hacks leaking into Production** → prevented contract-first; see §4 and the handoff rule in §5 Phase 1.

Open questions:

- Exact object set/count for the study
- Room dimensions and object spread → finalize distance-error target and time-to-locate threshold
- Query language for speech recognition
- Whether the study recruits real VI participants (ethics board)
