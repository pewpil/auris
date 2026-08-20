# Auris — Development Plan

## Purpose

This document is the canonical development plan for the Auris project. It is stored at the repository root. It is consumed by opencode as development context (via `.opencode/context.md`), and it is written so that the AI working on the project keeps the Prototype → Production transition in mind at every decision. Read it together with `.opencode/concept.md`, which describes the project concept.

## Project overview

The system aids a visually impaired (or blindfolded) user in locating objects on a table using simulated auditory cues. The user sits at a table with several objects. Cameras track the objects and the user's head. A headset plays spatialized audio: each object emits a unique earcon anchored to its real position. Turning the head rotates the sound field correctly so the field stays anchored in world space. A hybrid interaction model provides always-on earcons plus a query/beacon mode ("find the bottle" → that object pings periodically).

## Terminology

- **Prototype** — the early, throwaway feasibility rig using phones as cameras and Bluetooth earbuds. Purpose: validate the core concept. It is not part of the thesis evaluation.
- **Production system** — the real deliverable built with specialized gear (specialized cameras, low-latency headset). The interconnect may be wired or wireless; the choice is decided by measured end-to-end latency against the < 100 ms standard. All formal evaluation and user studies happen on this system.
- The plan favors the Production system. The Prototype is a thin vertical slice, not a parallel implementation.

## End-state vision (how the project turns out)

A seated, blindfolded user at a table with objects. A camera array tracks the table and the user's head. A headset plays a 3D soundscape; objects stay anchored in space as the user moves their head. Voice or button selects a target for beacon mode.

Measured outcomes:

- End-to-end latency < 100 ms (verified, not a research topic)
- Object localization < 15° angular error, < 10 cm reach error
- Blindfolded users locate a named object in < 30 s
- User study with statistical analysis (ANOVA / t-tests)

## System architecture

```
[Pipeline: camera array → (object detection + head pose) → coordinate
transform → UDP localhost → Unity audio engine → headset]
```

All spatial math lives in Python. Unity is a "dumb" audio renderer receiving head-relative object positions; it never computes poses. Audio renders on the host and plays through the headset.

## Contract-first interfaces (critical for the transition)

Data schemas are defined BEFORE writing either implementation. Both Prototype and Production implementations conform to the same contracts, so the hardware swap never touches the rest of the system.

1. `Frame` — pixels + timestamp + camera intrinsics (capture abstraction)
2. `ObjectLocalizer` — interface with two implementations:
   - `PlaneProjectionLocalizer` (mono camera, ray-plane intersection) — Prototype/simple
   - `DepthLocalizer` (depth/stereo triangulation) — Production
3. `HeadPoseEstimator` — MediaPipe solvePnP 6DoF (camera-agnostic; unchanged between Prototype and Production)
4. `SceneState` — UDP message schema (object id/class/azimuth/elevation/distance). Written once in the Prototype, never changed afterwards, so the Unity engine is untouched by the hardware swap.
5. `AudioSink` — audio output device abstraction (BT earbuds in Prototype, production headset in Production; both are output devices to Unity)

## What carries over vs. what is rebuilt (transition strategy)

**Carries over cleanly (kept):**

- Coordinate transform math (table frame → head-relative azimuth/elevation/distance)
- Unity audio scene, 3D Tune-In HRTF renderer, earcon library, hybrid sonification
- Head pose via MediaPipe solvePnP
- YOLO weights/object model, object tracking logic
- UDP Python→Unity protocol and SceneState schema
- Dataset, calibration procedure, evaluation harness

**Must be re-implemented for Production:**

- Camera capture layer (phone RTSP/MJPEG → specialized native/depth capture)
- Camera calibration (phone unknown intrinsics → measured intrinsics, with or without depth)
- Localization backend (PlaneProjectionLocalizer → DepthLocalizer)
- Replace phone-specific workarounds (rolling shutter, low-res tuning); WiFi jitter handling is re-evaluated in Phase 4 if wireless remains the Production interconnect

**Rule:** Prototype-only hacks must never leak into Production assumptions. Prototype code is throwaway; only validated algorithms, contracts, and data survive.

## Development phases

| Phase | Duration | Exit criterion |
|---|---|---|
| 0. Literature review | 2–3 wks | Related work (HRTF/binaural, sonification/earcons, assistive object-locating systems). Gap analysis. |
| 1. Prototype (thin vertical slice) | ~3 wks | Validate ONLY the two risky unknowns: (A) is head-rotation-tracked spatial audio intuitive enough to locate objects? (B) does solvePnP head pose track well enough at 30–60 Hz with an ordinary camera? Scope: 1 phone camera + MediaPipe head pose + manually placed object + one spatialized loop in Unity + BT earbuds. NO YOLO, NO speech, NO hybrid modes, NO calibration. Define the SceneState/UDP contract here. |
| 2. Production CV pipeline | 4–6 wks | Specialized cameras, calibration + table plane, trained YOLO, object tracking, head pose with smoothing, head-relative transform. Validate CV accuracy vs ARUCO/ruler ground truth. |
| 3. Production audio engine | 3–4 wks | Unity scene, 3D Tune-In integration, earcon library, hybrid always-on + beacon mode (keyboard first, then speech), UDP protocol, distance/occlusion cues. |
| 4. Integration & performance | 3–4 wks | Threaded/async pipeline; tune to end-to-end < 100 ms. Decide the Production interconnect (wired vs wireless) from measured latency. Latency is an engineering target, not a thesis chapter. |
| 5. User study & thesis writing | 6–10 wks | See Evaluation. |

Total: ~9–12 months (part-time roughly doubles).

## Prototype → Production handoff checklist

- [ ] SceneState UDP schema finalized in Prototype, frozen
- [ ] AudioSink abstraction validated with both BT earbuds and production headset
- [ ] List of "kept" items (algorithms, contracts, data, earcons) documented
- [ ] List of "rebuilt" items (capture, calibration, localization) documented
- [ ] Prototype-specific hacks isolated behind the CameraSource interface
- [ ] Production interconnect (wired vs wireless) decided from measured end-to-end latency
- [ ] No Production code depends on MJPEG, phone intrinsics, or BT earbud-specific behavior

## Evaluation plan (thesis)

1. CV accuracy: detection precision/recall; object-position error (cm) vs ground truth; head-pose error (°) vs ARUCO/IMU ground truth.
2. System performance (verification only): end-to-end latency < 100 ms — this measurement also decides wired vs wireless for Production; audio-position stability during head turns.
3. User study (N ≈ 10–15, blindfolded sighted + optionally VI): named-object localization. Metrics: angular error, reach error, time-to-locate, path efficiency. Questionnaires: SUS, NASA-TLX.
4. Hybrid-mode comparison: always-on vs beacon vs both.
5. Statistics: ANOVA / t-tests.
- Check ethics/IRB approval early if human participants are included.

## Hardware roadmap

| Component | Prototype | Production |
|---|---|---|
| Table camera | Phone (RTSP/MJPEG, WiFi) | Specialized camera (e.g., RealSense D435) |
| Face camera | Phone | 1–2 fixed specialized cameras or headset-IMU fusion |
| Audio | Bluetooth earbuds | Low-latency headset (host-side HRTF) |
| Compute | Laptop | Same (GPU for YOLO) |
| Interconnect | WiFi | Wired or wireless; chosen by measured latency vs. the < 100 ms standard |

## Risks & open questions

- Generic HRTF accuracy → use a good generic HRTF; individual HRTF = future work.
- YOLO dataset effort → keep object set small (≤ 5–8 classes).
- Head pose drift when the face turns away → IMU fusion in Production.
- BT earbud latency → Prototype-only; irrelevant to the production headset.
- Wireless links (camera streaming, audio) add latency/jitter → measured in Phase 4; wireless stays in Production only if end-to-end latency stays < 100 ms, otherwise the system goes wired.
- Open: object set/count, speech query language, whether the study recruits real VI participants (ethics).