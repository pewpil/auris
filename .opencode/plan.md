# Auris — Development Plan (summary)

> **Canonical plan:** [`README.md`](../README.md) at the repository root holds the full in-depth development plan. This file is a compact summary kept for opencode's session context; keep the two consistent when either changes.
>
> **Diagrams:** Mermaid sources live in `docs/*.mmd` (`architecture.mmd`, `scenestate-update.mmd`, `prototype-production-swap.mmd`) and paste into Lucidchart's Diagram-as-code editor.

## Project overview

Thesis project: a visually impaired (or blindfolded) user stands in a room containing various objects placed around them. A camera array tracks objects and the user's head — full 6DoF (position + rotation), since the user can turn and walk. A headset plays spatialized audio: each object emits a unique earcon anchored to its real position; turning the head rotates the sound field so it stays anchored in world space. A hybrid interaction model adds always-on earcons plus a query/beacon mode ("find the bottle" → that object pings).

## Terminology

- **Prototype** — throwaway feasibility rig (phones as cameras, ARUCO markers for objects and on the sides of the head, BT earbuds). Validates only: (A) intuitiveness of head-rotation-tracked spatial audio, (B) marker-based head tracking at 30–60 Hz. Not part of thesis evaluation.
- **Production system** — the deliverable on specialized gear (RGB/depth camera array, real-object YOLO + depth/stereo localization, low-latency headset). All formal evaluation and user studies happen here.
- Interconnect (wired vs wireless) is decided by measured end-to-end latency against the < 100 ms standard.

## Transition-first development (non-negotiable)

Development in the **Prototype stage must be considerate of transitioning to the Production stage as seamlessly as possible**:

- Data contracts (`Frame`, `ObjectLocalizer`, `HeadPoseEstimator`, `SceneState`, `AudioSink`) are defined before implementation and shared by both stages.
- The SceneState UDP schema is frozen in the Prototype; Unity is never touched by the hardware swap.
- Prototype code is built against the contracts, not against its hardware; only algorithms, contracts, and data survive the transition. Prototype-only hacks (MJPEG, phone intrinsics, BT behavior) must never leak into Production assumptions.

## Key facts

- One shared room/world frame via multi-camera extrinsic calibration; no dominant-plane shortcut.
- All spatial math in Python; Unity renders audio only; UDP localhost at 30–60 Hz.
- Head-relative azimuth/elevation/distance per object is what Unity receives.
- Latency < 100 ms is an engineering target verified in Phase 4 — not a research topic.
- Phases: 0 literature review → 1 prototype slice → 2 production CV → 3 production audio → 4 integration/performance → 5 user study & writing. Total ~9–12 months (part-time roughly doubles).
- Evaluation (Production only): CV accuracy vs ground truth, latency verification, user study N ≈ 10–15 (angular/distance error, time-to-locate, path efficiency), hybrid-mode comparison, ANOVA/t-tests.
