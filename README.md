# Auris

Auris is a head-mounted navigation aid that helps a visually impaired — or blindfolded (replicating VI for the study) — user locate a desired object in a room. The user asks for an object by voice; the system, having mapped the room from the wearable's depth sensing, projects that object's characteristic sound — rendered spatially, as if the object itself were emitting it — and the user follows the sound. If the object cannot be found or has moved, the user is prompted to look around more.

> The former development plan, architecture, contracts, phases, hardware/cost breakdowns, evaluation plan, diagrams, site, and system code were cleared on 2026-09-03. This is the new development plan, decided 2026-09-04, written from the consolidated concept (see [§1](#1-concept)) and the literature consolidation in [`docs/auris-thesis/notes/`](docs/auris-thesis/notes/).

## Where things live

- Development plan (this document)
- Concept: [`.opencode/concept.md`](.opencode/concept.md) · plan summary for AI sessions: [`.opencode/plan.md`](.opencode/plan.md)
- Thesis skeleton: [`docs/auris-thesis/paper.md`](docs/auris-thesis/paper.md); IEEE variant planned (not created): `docs/auris-thesis/ieee.md`
- Literature-review notes: [`docs/auris-thesis/notes/`](docs/auris-thesis/notes/)

## 1. Concept

The full concept text lives in [`.opencode/concept.md`](.opencode/concept.md). In brief:

- **User** — visually impaired, or blindfolded (replicating VI for the study).
- **Environment** — a single indoor room with obstacles (furniture, walls) and various everyday objects.
- **Goal** — the user locates their desired object.
- **Navigation aid** — sounds are projected by the head-mounted wearable according to the desired object's position and the user's head position and orientation, rendered **as if the object itself emits the sound**; the user follows the sound.

### 1.1 Task loop (one session)

1. **Map** — the user looks around; depth frames + 6-DoF head pose accumulate into a room map (scanning phase).
2. **Query** — the user names the target object by voice (one of 12 everyday classes, [§8](#8-audio-design)), or the experimenter triggers it ([§7](#7-software-stack)).
3. **Localize** — the system resolves the target's position from the map. If the target is absent or has moved, the user is prompted to look around more (the *re-look prompt*, [§3](#3-data-flow-and-module-contracts)).
4. **Guide** — the target's characteristic sound is projected spatially at the object's position, head-tracked and distance-encoded ([§8](#8-audio-design)); the user walks to it and reaches for it.
5. **Record** — the entire session is captured for offline analysis and evaluation metrics ([§9](#9-evaluation-plan)).

### 1.2 Locked decisions (2026-09-04)

| # | Decision | Choice |
|---|---|---|
| D1 | Stage split rationale | **Financial** — Prototype on already-available hardware (LiDAR iPhone = RGB + depth + head pose); Production purchases the full hardware set ([§4](#4-development-stages)) |
| D2 | Computing unit | **Desktop**; wearable↔desktop link tested **wireless-first**, long-USB tether as fallback ([§2.2](#22-link)) |
| D3 | Audio output | **Perforated over-ear headphones** (Sound of Vision precedent — keeps ambient hearing) |
| D4 | Object detection | **COCO-pretrained YOLOv8 restricted to 12 everyday classes**; fine-tune only if evaluation shows misses ([§7](#7-software-stack)) |
| D5 | Room mapping | **Staged** — Prototype: ARKit 6-DoF pose + Open3D depth fusion; Production: ROS 2 + RTAB-Map only if the camera swaps off ARKit ([§2.3](#23-computing-unit), [§4](#4-development-stages)) |
| D6 | Audio rendering | **Generic-HRTF binaural + head tracking**, rendered in **Unity (Steam Audio)** — decision logic stays in Python ([§8](#8-audio-design)) |
| D7 | Query input | **Voice command** (speech-to-text on desktop) **+ experimenter-trigger fallback** |
| D8 | Integration | **Staged plain Python** (ZeroMQ messaging) **+ built-in session recorder/replayer** (rosbag-like) as a Phase 0 deliverable; every session recorded — standing requirement |
| D9 | Production camera | **Intel RealSense D435i confirmed** — built-in IMU covers pose estimation, so no standalone IMU purchase |
| D10 | Evaluation scope | **Spatial-audio guidance only** — guidance output strictly non-verbal (no spoken directions/object names); no speech-only comparison baseline; voice remains the query input (D7) |

## 2. System architecture

Two physical units connected by a data link:

### 2.1 Head unit

**Prototype.** A LiDAR iPhone mounted on a head strap:

- **RGB camera** — real-time imagery for object detection.
- **Depth** — ARKit `sceneDepth` (LiDAR, ~256×192 metric depth points, effective range ≈ 0.3–5 m; sufficient for a room, degrades with distance/lighting).
- **Head position and orientation** — ARKit visual-inertial odometry (camera fused with the iPhone's **built-in IMU** — gyroscope + accelerometer) gives 6-DoF head pose; the IMU is inside the phone, so no separate IMU hardware is needed at Prototype (device on head ≈ head pose).
- **Sound output** — perforated over-ear headphones (D3), wired to the desktop's audio interface (long cable accepted; Bluetooth latency ≈ 100–200 ms would break the latency budget, [§8](#8-audio-design)).

**Production.** A custom **3D-printed head-mounted wearable** housing a RealSense-class RGB-D camera — whose **built-in IMU** (the "i" in D435i) replaces the iPhone's for pose estimation via RTAB-Map ([§4.2](#42-production)); headphones unchanged.

### 2.2 Link

- **Wireless-first** — the iPhone streams RGB + depth + pose over Wi-Fi to the desktop (existing streaming app such as Record3D, or a small custom Swift/ARKit app; decided in Phase 0). Rationale: the tether alternative is a long cable lying across a room a blindfolded user walks in — trip hazard and behavioral confound. Wi-Fi runs on a **dedicated 5 GHz hotspot** (not building infrastructure).
- **Dual-channel link** — latency-sensitive and latency-tolerant traffic split: **head pose** travels as tiny UDP packets (~36 bytes) at ~60 Hz on the low-latency path (the motion-to-sound loop, target ≤ ~30 ms — head-tracker lag becomes perceptible around there); **RGB/depth frames** travel on the heavier channel at 15–30 Hz, where Wi-Fi delay (~30–60 ms) is harmless because objects are static and their sound position changes only on re-localization ([§8](#8-audio-design)).
- **Link watchdog** — loss/dropout raises an audible "signal lost" cue and pauses the trial; the recorder logs the gap.
- **Fallback** — a long USB cable to the desktop (USB ports run to the desk area), with cable routing overhead/taped to keep the walking area clear.
- A **decision gate in Phase 5** picks the link for the evaluation study based on measured **motion-to-sound** latency and jitter ([§5](#5-development-phases)).

### 2.3 Computing unit (desktop)

Plain-Python processes over ZeroMQ (D5, D8). At Production, mapping/localization port to ROS 2 + RTAB-Map if the RealSense swap happens.

1. **Sensor ingest** — receives RGB frames, depth frames, and head pose; stamps them on a monotonic clock; republishes on the message bus.
2. **Perception (computer vision)** — YOLOv8 (COCO-pretrained, restricted to the 12 classes) detects objects in RGB imagery.
3. **Mapping (mapped-out room)** — fuses depth + head pose into a 3D room map (Open3D TSDF at Prototype); supports map save/load; at Production, RTAB-Map adds pose estimation and cross-session relocalization.
4. **Object localization** — anchors detections in the map, maintains object states across views, resolves the requested target, and fires the **re-look prompt** when the target is absent, ambiguous, or moved.
5. **Audio simulation** — rendered by a **Unity application (Steam Audio)**: the room map laid out as a scene, audio sources positioned at object states, an `AudioListener` driven by head pose (~60 Hz) for head-tracked HRTF binaural output; encodes distance via attenuation curves; renders the re-look prompt and closing-pulse behaviors on command from the Python decision logic (thin renderer — logic in Python, DSP in Unity).
6. **Query handling** — speech-to-text maps the user's utterance to one of the 12 classes; experimenter keyboard trigger as fallback (D7).
7. **Session recorder / replayer** — subscribes to every stream and writes a timestamped log to disk; replays sessions through the pipeline offline (D8).

### 2.4 Architecture diagram

```mermaid
flowchart LR
  subgraph HEAD["Head unit (Prototype: iPhone; Production: 3D-printed wearable)"]
    CAM["RGB camera"]
    DEP["Depth sensor (LiDAR / RealSense)"]
    POSE["Head pose (ARKit VIO / RTAB-Map)"]
    HP["Perforated over-ear headphones"]
  end
  LINK["Link (Wi-Fi first; USB tether fallback)"]
  subgraph DESK["Desktop computing unit"]
    ING["Sensor ingest"]
    DET["Perception (YOLOv8, 12 classes)"]
    MAP["Mapping (depth + pose fusion)"]
    LOC["Object localization"]
    QRY["Query handling (STT + trigger)"]
    AUD["Audio simulation (Unity + Steam Audio, HRTF)"]
    REC["Session recorder / replayer"]
  end
  CAM --> LINK
  DEP --> LINK
  POSE --> LINK
  LINK --> ING
  ING --> DET
  ING --> MAP
  ING --> AUD
  DET --> LOC
  MAP --> LOC
  QRY --> LOC
  LOC --> AUD
  AUD --> HP
  ING --> REC
  DET --> REC
  MAP --> REC
  LOC --> REC
  QRY --> REC
  AUD --> REC
```

## 3. Data flow and module contracts

All messages are timestamped (monotonic + wall clock) and published on the ZeroMQ bus. The recorder stores exactly these messages, so replay reproduces a session bit-for-bit at the message level.

| Message | Producer → Consumer | Fields (units) |
|---|---|---|
| `Frame` | ingest → perception, recorder | RGB image, camera intrinsics, seq, timestamp |
| `DepthFrame` | ingest → mapping, recorder | metric depth (m, float16, aligned to RGB), seq, timestamp |
| `HeadPose` | ingest → mapping, localization, audio, recorder | 6-DoF pose: position (m) + quaternion (w, x, y, z), confidence, timestamp |
| `Detections` | perception → localization, recorder | list of {class_id, class_name, bbox (px), confidence}, timestamp |
| `ObjectStates` | localization → audio, recorder | list of {object_id, class, position (m, map frame), last_seen, confidence} |
| `TargetCommand` | query handling → localization, recorder | {source: voice\|trigger, class_name}, timestamp |
| `LocalizationResult` | localization → audio, recorder | {status: found\|not_found\|ambiguous, object_id, position (m, map frame)}, timestamp |
| `AudioCommand` | localization → audio, recorder | {type: object_sound\|relook_prompt\|idle, class, position (m, map frame)}, timestamp |

**Coordinate frames** — documented in-repo: *camera frame* (ARKit convention: Y up, −Z forward at session start), *head frame* (co-located with camera), *room/map frame* (the map's origin frame). Units: meters, radians, quaternion (w, x, y, z). The audio node converts map-frame positions to head-relative azimuth/elevation/distance using the latest `HeadPose`.

**Unity bridge.** The audio-simulation module runs as a Unity application (D6). A NetMQ bridge (ZeroMQ for C#) inside Unity subscribes to the same bus: `HeadPose` drives the `AudioListener` transform at ~60 Hz, `ObjectStates` position the audio sources, and `AudioCommand` triggers loops, pulse rates, and the prompt chime. Decision logic stays in Python; Unity is the rendering engine only. If NetMQ proves heavy inside Unity, the Phase 4 spike falls back to a tiny UDP bridge carrying the same schemas.

**Recording format** — HDF5 (video/depth as compressed chunks) + SQLite/JSONL index of non-image messages; the replayer feeds stored streams back through ingest. Image and depth payloads travel as **binary buffers with JSON headers** (never JSON-encoded arrays) to keep serialization off the latency budget; recorder storage is sized for full study capture (depth + RGB at 15–30 Hz across the main study + VI pilot — plan disk space in Phase 0).

## 4. Development stages

### 4.1 Prototype

Built **entirely from already-available hardware** (D1 — financial rationale: build on what the team has, defer purchases until the concept is validated):

- LiDAR iPhone (12 Pro / 13 Pro class) as the sensor + head-strap mount (cheap purchase or simple printed cradle)
- Desktop with NVIDIA GPU
- Perforated over-ear headphones

Scope: prove the concept end-to-end ([§5](#5-development-phases), Phases 0–5) and run the evaluation study (Phase 6).

### 4.2 Production

Purchased and integrated only after the Prototype validates the concept:

- Intel RealSense **D435i** (confirmed, D9) replacing the iPhone — native USB streaming, better depth range/frame rate, built-in IMU for pose
- Custom **3D-printed head-mounted wearable** enclosure
- Software port: ingest via `pyrealsense2`; pose estimation and cross-session relocalization via **ROS 2 + RTAB-Map** (D5) — mechanical swap thanks to the module contracts in [§3](#3-data-flow-and-module-contracts)

Scope: Phase 7 — executed as the **ARKit → ROS 2 transition** ([§4.3](#43-the-arkit-to-ros-2-transition), Phases 7a–7c), revalidating the full loop at parity with the Prototype.

### 4.3 The ARKit to ROS 2 transition (Prototype → Production)

The Prototype→Production move is the project's single largest architectural change: it swaps the sensing and pose stack while everything downstream is preserved by the module contracts ([§3](#3-data-flow-and-module-contracts)). The swap map:

| Concern | Prototype (ARKit / iPhone) | Production (D435i + ROS 2) | Preserved unchanged |
|---|---|---|---|
| Sensor ingest | Record3D / custom Swift stream over Wi-Fi or USB → ZeroMQ | `pyrealsense2` ingest, native USB → message bus | `Frame` / `DepthFrame` schemas |
| Head pose | ARKit VIO (on-device, streamed at ~60 Hz) | RTAB-Map VIO (`rtabmap_ros`, camera + D435i built-in IMU) | `HeadPose` schema; calibrated camera→head-center transform ([§10](#10-risks-and-mitigations)) |
| Room mapping | Open3D TSDF fusion | RTAB-Map graph SLAM | map save/load interface |
| Relocalization | single-session only (ARKit world maps not portable) | **cross-session** (RTAB-Map map database) | object-localization logic |
| Link | dual-channel Wi-Fi + USB tether, decision gate ([§2.2](#22-link)) | native USB tether (D435i has no wireless link) | watchdog; latency budget; wireless revisit only if worn compute is ever added (open) |
| Everything downstream | — | — | detection, object localization, audio simulation, query handling, session recorder — byte-identical modules |

This is why the phases carry transition hooks: the bench protocol is **re-runnable** (Phase 0), the mapping node consumes `HeadPose` from the bus — **pose-source agnostic by contract** (Phase 2), and the swap itself is executed as three gated sub-phases **7a–7c** ([§5](#5-development-phases)).

## 5. Development phases

Each phase lists goal, tasks, deliverables, and exit criteria.

### Phase 0: Bench and link validation

- **Goal** — prove the sensing pipeline's quality and latency before any system building.
- **Tasks** — choose the streaming route (Record3D vs custom Swift/ARKit app); stream RGB + depth + pose over Wi-Fi and over USB; measure latency and frame rate for both links; set up the Python project skeleton, ZeroMQ bus, and message schemas ([§3](#3-data-flow-and-module-contracts)); **build the session recorder/replayer** (D8); audio loopback test (**Unity + Steam Audio renders an HRTF tone head-tracked through the headphones; measure Unity's audio output latency**); write the bench protocol to be **re-runnable on Production hardware** (Phase 7a, [§4.3](#43-the-arkit-to-ros-2-transition)).
- **Deliverables** — streaming-route decision + bench report (latency table, wireless vs tether); recorder/replayer; repo skeleton; schema documentation.
- **Exit criteria** — sensor→desktop ≤ ~30 ms over tether; wireless latency characterized; a sample session recorded and replayed successfully.

### Phase 1: Perception

- **Goal** — reliable 12-class detection on room-scale RGB frames.
- **Tasks** — integrate Ultralytics YOLOv8 with COCO weights restricted to the 12 classes (D4); test on the live stream; collect sample frames in the target room; fine-tune **only if** class-level misses appear; **confusion-matrix check on the similar pairs** (cup/bottle, laptop/keyboard, remote/phone) with per-class confidence thresholds ([§10](#10-risks-and-mitigations)); tune input resolution and FPS against the GPU budget.
- **Deliverables** — detection node; per-class detection quality report on sample data.
- **Exit criteria** — all 12 classes detected on placed objects at room distances at ≥ 10 FPS, with confidence adequate for localization gating.

### Phase 2: Room mapping

- **Goal** — build the mapped-out room from depth + ARKit pose.
- **Tasks** — integrate Open3D TSDF fusion; align depth to RGB; choose voxel size; map save/load; visualize; assess drift over a room-scale scan; keep the fusion **pose-source agnostic by contract** — it consumes `HeadPose` from the bus (ARKit now, RTAB-Map later: [§4.3](#43-the-arkit-to-ros-2-transition)).
- **Deliverables** — mapping node; saved maps; drift assessment.
- **Exit criteria** — a room scan yields a geometrically consistent map (walls flat, objects within ~10 cm at 3 m); map save/load round-trips.

### Phase 3: Object localization

- **Goal** — the target object's position in the room map, plus the re-look logic.
- **Tasks** — project detections into the map frame using head pose; object persistence (associate detections across views via class + distance + overlap gating); **ID-stability metric + hysteresis** on the state machine so anchors don't churn ([§10](#10-risks-and-mitigations)); object state machine (candidate → confirmed → stale); target resolution (highest-confidence candidate; ambiguity handling); **re-look prompt triggers** — no candidate found, target stale/moved ([§8](#8-audio-design) for the prompt sound); experimenter-trigger fallback path (D7).
- **Deliverables** — localization node with object states; localization accuracy test against known object placements.
- **Exit criteria** — localization error ≤ ~15 cm at typical room distances; re-look prompt fires correctly in scripted not-found/moved scenarios.

### Phase 4: Audio simulation

- **Goal** — head-tracked spatial sound that makes an object "speak" from its position.
- **Tasks** — **Unity audio scene + NetMQ bridge** (room-map scene; `AudioListener` driven by `HeadPose` at ~60 Hz; audio sources positioned from `ObjectStates`; `AudioCommand` handling — D6, [§3](#3-data-flow-and-module-contracts)); **Steam Audio** configured with a generic SOFA HRTF set; the 12 per-class sound assets ([§8](#8-audio-design)) with loudness normalization + **identification pilot** (blindfolded listeners name each sound's class — [§10](#10-risks-and-mitigations)); distance encoding (attenuation curves + distance-appropriate filtering); **near-field behavior** (volume cap + discrete closing-pulse guidance inside ~1 m — pulse *logic* in Python, rendering in Unity; see [§10](#10-risks-and-mitigations)); re-look prompt chime; motion-to-sound latency measurement **through the full Unity path** (pose → NetMQ → Unity → DAC), tuning Unity's DSP buffer/"best latency" settings.
- **Deliverables** — Unity audio scene + bridge; 12 sound assets; audible demo; latency report (motion-to-sound and full chain, per [§8](#8-audio-design)).
- **Exit criteria** — a blindfolded listener localizes the sound in azimuth in an informal test; **motion-to-sound latency ≤ ~30 ms through the Unity path over the link**; full-pipeline budget on track for < 100 ms.

### Phase 5: Closed-loop integration

- **Goal** — the end-to-end find-an-object task works on the desktop with a blindfolded user.
- **Tasks** — integrate all nodes; voice query (STT constrained to the 12 class names) + trigger fallback; **wireless-vs-tether decision gate** (D2); tune the pipeline against the **< 100 ms end-to-end budget** (Sound of Vision benchmark, Hoffmann et al. 2018 — see [`docs/auris-thesis/notes/sov-hoffmann2018-evaluation.md`](docs/auris-thesis/notes/sov-hoffmann2018-evaluation.md)); handle failure modes (target moved → re-detect; re-look prompt); pilot self-test as a blindfolded experimenter.
- **Deliverables** — integrated system; measured latency breakdown (sensor → link → perception → localization → audio); pilot run notes.
- **Exit criteria** — a blindfolded experimenter finds 10/10 placed objects in a furnished room; the link decision is made; end-to-end latency measured and documented.

### Phase 6: Evaluation study

- **Goal** — measure whether spatial-audio guidance helps users find objects.
- **Tasks** — finalize the protocol ([§9](#9-evaluation-plan)); institutional ethics/IRB if required (accessible consent materials for any VI participants); recruit blindfolded-sighted participants + begin VI-organization outreach for the 1–2-participant VI pilot; run the evaluation trials (single spatial-audio condition, D10); collect metrics; analyze (VI pilot analyzed separately). All sessions recorded (D8) for offline trajectory/latency analysis.
- **Deliverables** — protocol document, consent forms, data, results (feeds the thesis *Results and Discussions* chapter in [`docs/auris-thesis/paper.md`](docs/auris-thesis/paper.md)).
- **Exit criteria** — target N completed; analysis done.

### Phase 7: Production iteration

- **Goal** — the integrated 3D-printed head wearable with purchased hardware ([§4.2](#42-production)), executed as the ARKit → ROS 2 transition ([§4.3](#43-the-arkit-to-ros-2-transition)) in three gated sub-phases.

#### Phase 7a: Production hardware bring-up

- **Goal** — the new head unit streams and benches at least as well as the iPhone did.
- **Tasks** — purchase the D435i (D9); design and print the head-mounted wearable (rigid mount; measure the camera→head-center transform once — [§10](#10-risks-and-mitigations)); wire `pyrealsense2` ingest (native USB); **re-run the Phase 0 bench protocol** on the new sensor (latency, depth quality, IMU rate).
- **Deliverables** — Production head unit; bench report in the Phase 0 format.
- **Exit criteria** — sensor→desktop ≤ Phase 0 tether numbers; depth quality ≥ Prototype within the room; the mount holds calibration across a full session.

#### Phase 7b: Software port (ROS 2 + RTAB-Map)

- **Goal** — pose estimation and mapping move off ARKit behind unchanged contracts.
- **Tasks** — introduce ROS 2 + `rtabmap_ros` for VIO pose, graph-SLAM mapping, and **cross-session relocalization** (D5); keep `HeadPose` / `DepthFrame` / map interfaces identical; **regression-test against recorded Prototype sessions** — the replayer (D8) drives the port proof.
- **Deliverables** — ported pose/mapping stack; regression report from replayed sessions.
- **Exit criteria** — replayed Prototype sessions yield equivalent localization results under RTAB-Map; cross-session relocalization demonstrated (map today, relocalize tomorrow).

#### Phase 7c: Revalidation

- **Goal** — the full Production loop matches Prototype performance.
- **Tasks** — re-run the Phase 1–5 exit checks on Production hardware; re-check the latency budgets ([§8](#8-audio-design): < 100 ms chain, ≤ ~30 ms motion-to-sound); pilot find-an-object run with a blindfolded experimenter.
- **Deliverables** — validation report; updated latency breakdown.
- **Exit criteria** — the Production system completes the Phase 5 exit task at parity with the Prototype.

## 6. Hardware plan

### 6.1 Prototype bill of materials (already available)

| Item | Role | Status |
|---|---|---|
| LiDAR iPhone (12 Pro / 13 Pro class) | RGB + depth (ARKit `sceneDepth`) + 6-DoF head pose (ARKit VIO — the phone's **built-in IMU** fused with its camera; no separate IMU hardware) | owned |
| Head strap / iPhone cradle | head mount | cheap purchase or printed cradle |
| Desktop with NVIDIA GPU (≥ 6 GB VRAM — verify) | computing unit | owned |
| Perforated over-ear headphones | audio output (D3) | owned |
| Long USB-C cable + Wi-Fi access point | data link (D2) | owned |

### 6.2 Production purchases

| Item | Role | Est. cost | Rationale |
|---|---|---|---|
| Intel RealSense **D435i** (D9) | RGB-D + IMU sensor | ~USD 300–350 | native USB, 90 FPS depth, head-mount friendly; **built-in IMU — buy the "i" variant specifically** (pose estimation requires it); precedented by Fei et al. 2024 and Lee & Medioni 2016 |
| 3D-printed head-mounted wearable | enclosure/mount | filament cost | the custom head unit (retained old-concept element) |
| Cables, straps, fasteners | integration | ~USD 30 | — |
| ~~Standalone 9-DoF IMU module (e.g., BNO085/BMI088 breakout)~~ | ~~head-pose IMU~~ | — | **Not required** — the confirmed D435i (D9) has a built-in IMU. Keep this row only as a contingency if the component route is ever revisited (an IMU is mandatory at every stage; standalone purchase only if the camera lacks one) |

## 7. Software stack

| Module | Choice | Notes |
|---|---|---|
| Streaming (Prototype) | Record3D app **or** custom Swift/ARKit app | Record3D streams RGB-D + pose via Wi-Fi/USB; custom app if more control needed — decided Phase 0 |
| Message bus | ZeroMQ (PUB/SUB) + JSON schemas | per [§3](#3-data-flow-and-module-contracts) |
| Detection | Ultralytics **YOLOv8**, COCO weights | restricted to the 12 classes; fine-tune only on evidence of misses (D4) |
| Mapping (Prototype) | **Open3D** TSDF fusion + numpy/scipy | depth + ARKit pose; save/load (D5) |
| Mapping (Production) | ROS 2 + **RTAB-Map** (`rtabmap_ros`) | only if the camera swaps off ARKit; adds relocalization (D5) |
| Object localization | custom Python | contracts in [§3](#3-data-flow-and-module-contracts) |
| Query input | **faster-whisper** (small model) constrained to class keywords + keyboard trigger | D7 |
| Audio rendering | **Unity + Steam Audio** (HRTF binaural, head-tracked `AudioListener`, SOFA custom-HRTF import, distance-attenuation curves) as the audio runtime, driven over the bus (NetMQ) by Python decision logic; fallback: Unity's built-in spatializer; last resort: custom Python renderer (`sounddevice` + SOFA convolution) | D6 |
| Recorder/replayer | **HDF5** (`h5py`) + SQLite index | D8 |
| OS | Desktop: **Ubuntu 22.04+ LTS** (official CUDA driver setup; native ROS 2 support at Production; **Unity Hub + Editor supported**); iOS 16+ (iPhone) | dependency lockfile (uv/pip-tools) kept for reproducibility regardless of distro |
| Deferred (Production) | `pyrealsense2`, ROS 2 Humble+, `rtabmap_ros` | [§4.2](#42-production) |

## 8. Audio design

**Rendering.** Generic-HRTF binaural rendering with head tracking (D6), implemented in **Unity with the Steam Audio spatializer** (free; imports custom SOFA HRTFs, so a generic set is used per the psychoacoustic justification: azimuth localization remains accurate with non-individualized HRTFs — Wenzel et al. 1993, [`docs/auris-thesis/notes/wenzel1993-hrtf.md`](docs/auris-thesis/notes/wenzel1993-hrtf.md) — and head-tracked virtual auditory displays approach free-field performance — Romigh et al. 2015, [`docs/auris-thesis/notes/romigh2015-head-tracked-vad.md`](docs/auris-thesis/notes/romigh2015-head-tracked-vad.md)). Head pose drives the `AudioListener` transform; each object state is an audio source; distance encoding uses Unity's attenuation curves plus distance-appropriate filtering. Perforated over-ear headphones keep the room audible (Sound of Vision precedent; D3). The Unity scene doubles as a **live visual debug view** (map, object states, sound positions) for the experimenter — not part of the user-facing system.

**Sound assignment — the 12 classes.** Each class gets a characteristic **auditory icon** — the sound the object would plausibly emit (Gaver 1986 — [`docs/auris-thesis/notes/sound-design-foundations.md`](docs/auris-thesis/notes/sound-design-foundations.md)); an earcon-style fallback (Blattner et al. 1989) covers any class without a natural sound. All 12 are COCO classes, so no custom training data is required (D4).

| # | Class | COCO name | Typical placement | Sound (auditory icon) |
|---|---|---|---|---|
| 1 | Bottle | `bottle` | table/floor | liquid clink/rattle |
| 2 | Cup/mug | `cup` | table/desk | ceramic tap |
| 3 | Cell phone | `cell phone` | desk/table | ringtone/vibrate buzz |
| 4 | Book | `book` | table/shelf | page flip/thump |
| 5 | Chair | `chair` | floor, anywhere | wood scrape/creak |
| 6 | Laptop | `laptop` | desk | keyboard clicks/fan hum |
| 7 | Remote control | `remote` | table/couch | button click |
| 8 | Keyboard | `keyboard` | desk | typing clatter |
| 9 | Clock | `clock` | wall/shelf | ticking |
| 10 | Potted plant | `potted plant` | floor/shelf | leaf rustle |
| 11 | Vase | `vase` | table/shelf | ceramic chime |
| 12 | Backpack | `backpack` | floor/chair | zipper |

The spread across table/desk/floor/wall placements exercises the spatial audio at different directions and heights in the evaluation ([§9](#9-evaluation-plan)).

**Playback behavior.** While a target is active, its sound loops softly (fade-in; no startle); volume scales with distance and distance-appropriate filtering adds a range cue; position renders head-relative so the sound stays anchored to the object as the user turns. **Guidance output is strictly non-verbal** (D10): no spoken directions, object names, or coordinate callouts at any point during guidance — system speech is limited to query-time dialogue (STT confirmation/disambiguation, D7).

**Terminal guidance (open design decision).** Continuous guidance ends at proximity, but the user still must find exactly where to reach. Planned scheme: as the user closes inside ~1 m, the loop gives way to a **discrete closing pulse** whose rate rises as distance falls (NaviSense-style escalation); when the sound is centered and near, a short **"on target" confirmation** tells the user to explore with their hands. Validate in the Phase 5 pilot; exact thresholds are tuned there.

**Re-look prompt.** A distinct non-verbal chime fired by object localization when the target is absent, ambiguous, or moved ([§3](#3-data-flow-and-module-contracts)); non-verbal by scope (D10). Exact design is finalized in Phase 4 with pilot users — recorded as an open design choice.

**Latency budget.** Two paths, two targets ([§2.2](#22-link)):

- **Motion-to-sound** (head pose → audio output): target **≤ ~30 ms** — head-tracker lag is perceptible around ~30 ms (Brungart et al.), and this loop carries the object-anchoring illusion. Pose travels the low-latency UDP channel; Unity's render + output buffer adds ~10–20 ms (tuned via Steam Audio/DSP buffer settings — measured in Phase 4).
- **End-to-end chain** (frame → detection → localization → sound position): target **< 100 ms** (perception→sound lag benchmarked by Sound of Vision, Hoffmann et al. 2018): sensor ~10 ms + link ~20 ms (tether) / ~30–60 ms (Wi-Fi) + perception ~30 ms + localization ~5 ms + audio ~10–20 ms. This loop is latency-tolerant (static objects) but still capped for responsiveness when the target resolves.

## 9. Evaluation plan

- **Design** — single condition: the spatial-audio system as designed ([§8](#8-audio-design)), evaluated with within-subject repeated trials (counterbalanced object order, randomized placements per trial). **No comparison baseline** (D10): guidance is strictly the projected spatial sound — the system never speaks directions or object names during guidance (voice is query input only, D7). Modality-comparison claims are out of scope; prior comparative evidence (Qin et al. 2026) is cited in the thesis rather than re-run.
- **P rarticipants** — mostly **blindfolded sighted** (replicating VI; retained old-concept element; precedented by Qin et al. 2026 and Fei et al. 2024), N ≈ 10–12 for the main study; **plus a small VI pilot** (1–2 actual visually impaired users, if recruited) reported separately as qualitative case studies — never pooled with the main sample (statistics stay on the blindfolded-sighted group). VI pilot logistics: accessible consent materials, recruitment via VI organizations (start early), longer device familiarization.
- **Environment** — one furnished room; objects from the 12 classes placed across table/desk/floor/shelf/wall ([§8](#8-audio-design)).
- **Task** — map-scan → query → walk to the target → touch it. Success = touches the correct object within a time cap (120 s cap precedent: Coughlan et al. 2020).
- **Metrics** — time-to-target, success rate, collisions/bumps, path efficiency (walked vs straight-line), SUS, NASA-TLX; semi-structured interview. Sessions recorded for offline head-trajectory analysis (D8).
- **Evaluation criteria (descriptive)** — H1: participants locate the requested object with the spatial-audio guide (success rate against a target criterion, e.g., ≥ 80%); H2: time-to-target improves with practice (learning effect); H3: workload acceptable (NASA-TLX) and usability adequate (SUS ≥ 70).
- **Safety** — experimenter shadowing; static obstacles during trials; capped audio levels; clear walking lanes (no loose cables on the floor — link routing per [§2.2](#22-link)); practice trials to settle veering and cue interpretation before measurement.
- **Screening** — self-reported normal hearing; no prior experience requirement noted (spatial-audio familiarity recorded as a covariate).

## 10. Risks and mitigations

| Risk                                                                                                                   | Impact                                                                                               | Mitigation                                                                                                                                                                                                                                                                                   |
| ---------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Wi-Fi latency jitter breaks the budget                                                                                 | guidance feels detached from head motion                                                             | dual-channel link ([§2.2](#22-link)) keeps the strict pose path off the heavy frame stream; dedicated 5 GHz hotspot; Phase 0 measurement; USB-tether fallback; Phase 5 decision gate (D2)                                                                                                    |
| iOS session interruption / auto-lock → ARKit world reset                                                               | map frame jumps; anchored objects "teleport"                                                         | keep-awake + Guided Access during sessions; detect ARKit interruption → invalidate map, trigger re-look/re-map                                                                                                                                                                               |
| Clock skew between iPhone and desktop clocks                                                                           | latency measurements and recorder alignment are invalid                                              | sync pings (offset estimation) on link setup; desktop-receipt timestamps for latency accounting; document in the recorder format ([§3](#3-data-flow-and-module-contracts))                                                                                                                   |
| STT misrecognition ("potted plant" vs "plant", room noise)                                                             | wrong or unresolved target                                                                           | vocabulary constrained to the 12 class names; spoken confirmation prompt ("did you say mug?"); experimenter trigger fallback (D7)                                                                                                                                                            |
| Multiple instances of the same class (two chairs)                                                                      | ambiguous target                                                                                     | ambiguity policy: nearest/most-confident candidate, or spoken disambiguation prompt; logged as `ambiguous` in [§3](#3-data-flow-and-module-contracts)                                                                                                                                        |
| Near-field audio (target < ~1 m)                                                                                       | generic far-field HRTF renders "in-head"; distance-scaled loudness uncomfortable at reach            | volume cap near the target; switch to discrete closing-pulse guidance as the user nears (NaviSense-style escalation); near-field HRTF is a Production refinement                                                                                                                             |
| Front–back confusion with generic HRTF (Wenzel 1993)                                                                   | user walks the wrong way                                                                             | head-motion parallax resolves it (users naturally turn); practice trials; re-look prompt on no-progress                                                                                                                                                                                      |
| Occluded / never-seen target after scanning                                                                            | "look around more" without direction frustrates                                                      | scan-coverage awareness from the map (regions with no observations) → directed re-look prompt ("try looking to your left")                                                                                                                                                                   |
| Head-mount instability + camera-to-head offset (strap slip, wobble; camera sits on the forehead, not between the ears) | pose ≠ true head pose → sound anchoring drifts; depth misaligns with the audio reference             | rigid strap/cradle with fit checks each session; one-time calibration of the fixed camera→head-center transform, applied by the audio node; document in [§3](#3-data-flow-and-module-contracts)                                                                                              |
| iPhone power draw during continuous LiDAR + camera + streaming (~1 h)                                                  | session cut short mid-trial                                                                          | Phase 0 decision: battery pack on the strap (weight/heat) vs charging cable (a soft tether — tension with D2, must be routed like [§2.2](#22-link) fallback); monitor battery state in the recorder                                                                                          |
| Face heat / comfort over 30–60 min sessions                                                                            | participant fatigue contaminates later trials                                                        | break schedule; comfort check between trials; session length caps in the protocol                                                                                                                                                                                                            |
| Class confusions among similar pairs (cup/bottle, laptop/keyboard, remote/phone)                                       | confident wrong class → user follows the wrong sound                                                 | Phase 1 confusion-matrix check; per-class confidence thresholds; spoken disambiguation prompt for confusable pairs                                                                                                                                                                           |
| Object-ID churn (detection flicker at gate boundaries splits/merges IDs)                                               | anchors unstable → the sound jumps between positions                                                 | Phase 3 tracking-quality metric (ID survival across frames/seconds); hysteresis on the state machine (candidate → confirmed only after N sightings); recorder logs ID lifetimes for tuning                                                                                                   |
| Terminal-guidance gap (guidance ends at proximity; the last ~30 cm is unsolved)                                        | user is near the object but cannot find exactly where to reach                                       | design decision in [§8](#8-audio-design): closing-pulse escalation → "on target" confirmation when the sound is centered → user's hand exploration; validate in the Phase 5 pilot                                                                                                            |
| Sound-asset indistinctness (similar timbres; loudness mismatch)                                                        | classes not identifiable by ear → the mapping fails                                                  | Phase 4 mini identification pilot (blindfolded listeners name the sound's class; target ≥ 90% correct) before assets are frozen                                                                                                                                                              |
| Single-condition evaluation — no comparison baseline (D10)                                                             | conclusions about spatial audio are descriptive (system performance), not causal vs other modalities | scoped as such; cite prior comparative evidence (Qin et al. 2026) in the thesis discussion; note in limitations                                                                                                                                                                              |
| Blindfolded-sighted results may not transfer to actually VI users                                                      | external validity of the study's conclusion                                                          | main study claims are system-level (non-visual task performance), stated as such; **VI pilot (1–2 participants) reported as qualitative case studies** for feasibility evidence; Qin et al. 2026 / Fei et al. 2024 precedent cited for the replication rationale; note in thesis limitations |
| ARKit depth noise/range (degrades beyond ~4 m, low light)                                                              | poor map, missed objects                                                                             | room-size limit; controlled lighting; confidence gating in mapping                                                                                                                                                                                                                           |
| Head-tracking loss (fast motion)                                                                                       | map/localization breaks                                                                              | re-look prompt doubles as recovery; moderate-motion guidance in the protocol                                                                                                                                                                                                                 |
| Detection misses (small/distant objects)                                                                               | target unresolved                                                                                    | re-look prompt; fine-tune on collected samples; per-class distance caps                                                                                                                                                                                                                      |
| Audio masking by ambient noise                                                                                         | cue inaudible                                                                                        | level calibration; quiet-room protocol; perforated cups preserve localization while leaking less than open air                                                                                                                                                                               |
| Map drift over long sessions                                                                                           | stale object positions                                                                               | ARKit VIO is strong indoors; Phase 2 drift assessment; Production RTAB-Map adds loop closure                                                                                                                                                                                                 |
| Object moved between mapping and search                                                                                | "found" position wrong                                                                               | object state machine re-detects on sight; re-look prompt (the [§1.1](#11-task-loop-one-session) step-3 behavior)                                                                                                                                                                             |
| GPU/CPU contention (detection + STT + Unity scene)                                                                     | frame drops, latency spikes                                                                          | frame throttling; Unity audio DSP is CPU-bound and the debug view is lightweight — measure in Phases 1/5                                                                                                                                                                                     |
| Unity audio output latency on Ubuntu (PulseAudio buffering)                                                            | motion-to-sound budget blown by output buffering                                                     | Unity "best latency" DSP-buffer setting; measure the full Unity path in Phase 4; if unresolvable, the custom Python renderer is the documented last-resort fallback ([§7](#7-software-stack))                                                                                                |
| iPhone thermal throttling on long sessions                                                                             | streaming degradation                                                                                | session time limits; monitor in Phase 0 bench                                                                                                                                                                                                                                                |

## 11. Literature map

Working notes: [`docs/auris-thesis/notes/`](docs/auris-thesis/notes/) (index in its [`README.md`](docs/auris-thesis/notes/README.md)). Module → key studies:

- **Audio simulation** — Qin et al. 2026 (objects speak, head-tracked spatial-audio guidance), Romigh et al. 2015, Wenzel et al. 1993, Gaver 1986 / Blattner et al. 1989
- **Object localization / search** — ObjectFinder (Liu et al. 2024), NaviSense (Sridhar et al. 2025), StereoPilot (Hu et al. 2022), CamIO guidance (Coughlan et al. 2020), VizWiz::LocateIt (Bigham et al. 2010)
- **Room mapping** — Chen et al. 2021 (semantic SLAM wearable), Fei et al. 2024 (ORB-SLAM2 + YOLOv5s), Lee & Medioni 2016, Ou et al. 2022
- **System benchmarks** — Sound of Vision family (Hoffmann et al. 2018: < 100 ms lag, hours-to-cane-parity training; Zvorișteanu et al. 2021)
- **Methodology** — Qin et al. 2026 & Fei et al. 2024 (blindfolded-sighted replication), Coughlan et al. 2020 (120 s cap, within-subject)
- **Context/surveys** — [`docs/auris-thesis/notes/surveys.md`](docs/auris-thesis/notes/surveys.md)

## 12. Change log

- **2026-09-04 (rev. 3)** — **D6 implementation fixed to Unity + Steam Audio**: the audio-simulation module becomes a thin Unity renderer (room-map scene, head-tracked `AudioListener` via NetMQ bridge, SOFA HRTF import, attenuation-based distance encoding) driven by Python decision logic; Phase 0 loopback and Phase 4 reworked around the Unity path (motion-to-sound ≤ ~30 ms through Unity); Ubuntu 22.04+ confirmed to host Unity; new Unity-audio-latency risk row.
- **2026-09-04 (rev. 2)** — Decisions locked: D9 (D435i confirmed), D10 (spatial-audio-only evaluation — non-verbal guidance, no speech-only baseline); desktop OS fixed to Ubuntu 22.04+ LTS; dual-channel link design; near-field/terminal-guidance behavior specified; participant structure set (blindfolded-sighted main study + VI pilot); **Phase 7 restructured into the gated ARKit → ROS 2 transition sub-phases (7a–7c)** with the transition map in [§4.3](#43-the-arkit-to-ros-2-transition); transition hooks added to Phases 0 and 2; expanded risk register.
- **2026-09-04** — New development plan written (this document) from the consolidated concept and the literature consolidation; former plan cleared 2026-09-03.
