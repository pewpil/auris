# Auris — Concept (consolidated)

> **Canonical, detailed plan: [`README.md`](../README.md)** (end-state vision §1, architecture §2,
> per-subsystem design §3, contracts §4). This file is a session-context summary kept consistent
> with it — the pre-plan early draft (triangulate-depth wording, etc.) is retired.

## What Auris is

A system that helps visually impaired users locate objects in a room through simulated auditory
cues. A camera array tracks every object **and** the user's head as full 6DoF poses in one shared
room/world frame; a headset renders a 3D soundscape in which each object continuously emits a
distinctive earcon anchored to its real position. Turning or walking the user rotates/moves the
sound field with them, so sounds stay fixed to the world. An optional beacon mode lets the user
request a specific object ("find the bottle"), which then pings periodically for directed search.

## The user & goal

The user is visually impaired — or blindfolded (replicating VI for the study) — standing (and
walking) in a room containing objects on the floor, shelves, and furniture at varied heights. The
goal: locate named objects using **only** the projected sounds through the headset.

## System functions

1. **Computer vision (Python — all spatial math lives here):**
   - Determine what each object is and where it is located.
   - Compute the user's real-time 6DoF head pose (position + rotation) from the camera array.
   - Everything is registered into ONE shared world frame; per object, compute head-relative
     azimuth φ, elevation θ, distance r.
2. **Auditory projection (Unity — a "dumb" renderer):**
   - Place each object's assigned earcon at its head-relative position via HRTF binaural
     rendering — simulating the objects emitting their own sounds (a water bottle at the top-left
     sounds from the top-left).
   - Interaction is **hybrid**: always-on earcons for ambient awareness + query/beacon mode
     (keyboard first, then speech) for directed search.

## Project setup

- **Multiple RGB cameras** (array) — object identification/localization and head capture.
  Capture priority ladder: ① RGB primary → ② multi-view RGB triangulation → ③ dedicated depth
  cameras last.
- **Computer unit** — runs the CV pipeline, transforms, and YOLO inference.
- **Headset** — renders the binaural soundscape (HRTF spatializer, earcon library, distance cues).
- **Networking devices** — Python → Unity over UDP; **wired and wireless interconnects are both
  supported at every stage**. Wireless is acceptable if the measured end-to-end latency still
  meets the system's **< 100 ms** standard — an engineering target that is verified, not researched.

## Development stages (split for financial reasons)

1. **Prototype** — the entire stack (CV, audio, integration, performance) on existing +
   individually approved hardware: phone cameras streaming MJPEG/RTSP, **ARUCO markers as object
   stand-ins and a side-of-head marker pair as a rigid body for head tracking (30–60 Hz)**.
   Pinned constraint: **RGB only — no triangulation, no depth**.
2. **Production** — all components purchased on specialized gear: camera array covering the room,
   YOLO on real objects (one-shot registration → multi-view triangulation → depth only if
   insufficient), markerless CV head tracking with a headset-attached IMU fallback for orientation.
   All formal evaluation and the user study happen here.

## Standing rules

- **Contract-first:** `Frame`, `ObjectLocalizer`, `HeadPoseEstimator`, `SceneState` (frozen UDP
  schema in the Prototype), `AudioSink` — defined before implementation so the hardware swap never
  touches pipeline code or Unity.
- **Transition-first:** Prototype code is built against the contracts, not its hardware;
  Prototype-only hacks (BT earbud latency, phone intrinsics) never leak into Production assumptions.
- **Latency < 100 ms** is a verified engineering requirement on both interconnect paths — outside
  thesis research scope.
- Formal evaluation (Production only): CV accuracy, latency verification per path, user study
  (N ≈ 10–15 blindfolded), hybrid-mode comparison, ANOVA/t-tests.
