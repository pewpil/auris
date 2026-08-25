# Auris — Development Plan (summary)

> **Canonical plan:** [`README.md`](../README.md) at the repository root holds the full in-depth development plan. This file is a compact summary kept for opencode's session context; keep the two consistent when either changes.
>
> **Diagrams:** Mermaid sources live in `docs/*.mmd` (`architecture.mmd`, `scenestate-update.mmd`, `prototype-production-swap.mmd`, `phases-gantt.mmd`) and paste into Lucidchart's Diagram-as-code editor.

## Project overview

Thesis project: a visually impaired (or blindfolded) user stands in a room containing various objects placed around them. A camera array tracks objects and the user's head — full 6DoF (position + rotation), since the user can turn and walk. A headset plays spatialized audio: each object emits a unique earcon anchored to its real position; turning the head rotates the sound field so it stays anchored in world space. A hybrid interaction model adds always-on earcons plus a query/beacon mode ("find the bottle" → that object pings).

## Why two stages — financial reasons

The Prototype/Production split exists **for financial reasons**, not primarily risk isolation:

- **Prototype stage** builds and integrates the *entire* stack — CV pipeline, audio engine, and integration & performance — using only **existing hardware plus individually approved purchases** (approval table: README §7.2). It validates risks (A) head-rotation-tracked spatial-audio intuitiveness and (B) marker-based head tracking at 30–60 Hz, end-to-end.
- **Production stage** is where **all** system components are purchased. The engines are re-platformed behind unchanged contracts onto specialized gear, and every formal evaluation + user study happens here.

## Interconnect policy

- **Wired AND wireless are supported at every stage**; components for both are specified (README §7).
- End-to-end latency **< 100 ms** remains the verified engineering target. Measured reality: wired USB cameras ≈ negligible link latency; wireless camera links ~80–150 ms tuned MJPEG/raw-UDP, 200–300 ms RTSP/IP-cam, ~200 ms RPi-edge WebRTC floor; audio 2.4 GHz dongle 15–40 ms vs classic BT 150–250 ms.
- A fully-wireless chain measures ~150–400 ms — the gap is mitigated (dedicated WiFi 6 AP, tuned streams, codec choice) and reported per path, never studied as thesis content.

## Transition-first development (non-negotiable)

Development in the **Prototype stage must be considerate of transitioning to the Production stage as seamlessly as possible**:

- Data contracts (`Frame`, `ObjectLocalizer`, `HeadPoseEstimator`, `SceneState`, `AudioSink`) are defined before implementation and shared by both stages.
- The SceneState UDP schema is frozen in the Prototype; Unity is never touched by the hardware swap.
- Prototype code is built against the contracts, not against its hardware; only algorithms, contracts, and data survive the transition. Prototype-only hacks (phone intrinsics, handheld rigging, classic-BT audio) must never leak into Production assumptions.

## Key facts

- One shared room/world frame via multi-camera extrinsic calibration; no dominant-plane shortcut.
- Capture priority: ① RGB (+ one-shot registration) → ② multi-view RGB triangulation → ③ dedicated depth cameras. The Prototype stage is pinned to ① only (RGB-only, ARUCO solvePnP; no triangulation, no depth).
- All spatial math in Python; Unity renders audio only; UDP localhost at 30–60 Hz.
- Head-relative azimuth/elevation/distance per object is what Unity receives.
- Latency < 100 ms is an engineering target verified on both interconnect paths — not a research topic.
- Phases: 0 literature review → 1 Prototype CV pipeline → 2 Prototype audio engine → 3 Prototype integration & performance → *financial gate: purchase all Production components* → 4 Production CV pipeline → 5 Production audio & integration → 6 user study & writing. Total ~9–12 months (part-time roughly doubles).
- Prototype purchases require researcher approval: README §7.2 table columns are Component | Subsystem | Qty | Price | Cost | Requirement | Researcher 1/2/3 (each marks *yes*); networking items marked wired/wireless/either.
- Evaluation (Production only): CV accuracy vs ground truth, latency verification per path, user study N ≈ 10–15 (angular/distance error, time-to-locate, path efficiency), hybrid-mode comparison, ANOVA/t-tests.
