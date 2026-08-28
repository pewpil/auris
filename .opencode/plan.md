# Auris — Development Plan (summary)

> **Canonical plan:** [`README.md`](../README.md) at the repository root holds the full in-depth development plan. This file is a compact summary kept for opencode's session context; keep the two consistent when either changes.
>
> **Diagrams:** Markdown files with fenced Mermaid blocks live in `docs/` (`architecture.md`, `scenestate-update.md`, `prototype-production-swap.md`, `phases-gantt-m1-m4-prototype.md`, `phases-gantt-m5-m8-production.md`); renderers draw them natively and the source pastes into Lucidchart's Diagram-as-code editor.

## Project overview

Thesis project: a visually impaired (or blindfolded) user stands in a room containing various objects placed around them. A camera array tracks objects and the user's head — full 6DoF (position + rotation), since the user can turn and walk. A headset plays spatialized audio: each object emits a unique earcon anchored to its real position; turning the head rotates the sound field so it stays anchored in world space. A hybrid interaction model adds always-on earcons plus a query/beacon mode ("find the bottle" → that object pings).

## Why two stages — financial reasons

The Prototype/Production split exists **for financial reasons**, not primarily risk isolation:

- **Prototype stage** builds and integrates the *entire* stack — CV pipeline, audio engine, and integration & performance — using only **existing hardware plus individually approved purchases** (approval table: [README §7.2](../README.md#72-prototype-hardware-purchase-approval)). It validates risks (A) head-rotation-tracked spatial-audio intuitiveness and (B) marker-based head tracking at 30–60 Hz, end-to-end.
- **Production stage** is where **all** system components are purchased. The engines are re-platformed behind unchanged contracts onto specialized gear, and every formal evaluation + user study happens here.

## Interconnect policy

- **Wired AND wireless are supported at every stage**; components for both are specified ([README §7](../README.md#7-hardware-requirements--cost-breakdown)).
- End-to-end latency **< 100 ms** remains the verified engineering target. Measured reality: wired USB cameras ≈ negligible link latency; wireless camera links ~80–150 ms tuned MJPEG/raw-UDP, 200–300 ms RTSP/IP-cam, ~200 ms RPi-edge WebRTC floor; audio 2.4 GHz dongle 15–40 ms vs classic BT 150–250 ms.
- A fully-wireless chain measures ~150–400 ms — the gap is mitigated (dedicated WiFi 6 AP, tuned streams, codec choice) and reported per path, never studied as thesis content.

## Prototype compute & escalation ladder

Contributed researcher hardware serves as Prototype compute, with a fixed escalation order. **Standing instruction: whenever YOLO dataset/training/inference stalls on compute during the Prototype stage, remind the user of this ladder and escalate one rung at a time.**

1. **No-swap (primary, tentative):** Desktop PC #1 — Ryzen 5 3500 + RTX 4060 (8 GB) + 64 GB RAM — hosts Python CV, YOLO inference, and Unity simultaneously. YOLO **training is offloaded to Google Colab (free T4)** so it never competes with live sessions; local training is the offline fallback (dataloader workers ≈ 4). Keep live streams at 720 p when all cameras are active.
2. **Two-box split:** move Unity/audio to Desktop PC #2 (i7-10700 + RTX 3050, 16 GB); SceneState UDP shifts from localhost to LAN — contract unchanged.
3. **Parts swap (last resort):** consolidate i7-10700 + RTX 4060 + 64 GB into one chassis; Cooler Master MWE 750 (230 V) verified adequate (~300–350 W system peak vs 750 W capacity).

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
- Phases: 0 literature review (+ ethics filing) → 1 Prototype CV pipeline → 2 Prototype audio engine → 3 Prototype integration & performance → *financial gate (components ordered during Phase 3)* → 4 Production CV pipeline → 5 Production audio & integration → 6 user study & final thesis integration. **Total: 8 months full-time, Aug 31 2026 – Apr 30 2027** — Prototype = months 1–4, Production Dec–Feb (Phase 4 stretches across the PH December holidays), study+final-writing Mar–Apr, final two weeks are defense buffer. **Thesis writing is distributed**: §1–§3 (and most appendices) are drafted in Phases 0–5 as each section's inputs exist; only §4.3–§4.5, §5, and the Abstract wait for the study/measurements (Phase 6). Full mapping in `docs/auris-thesis/thesis.md`.
- Prototype purchases require researcher approval: [README §7.2](../README.md#72-prototype-hardware-purchase-approval) table columns are Component | Subsystem | Qty | Price | Cost | Requirement | Researcher 1/2/3 (each marks *yes*); networking items marked wired/wireless/either.
- Evaluation (Production only): CV accuracy vs ground truth, latency verification per path, user study N ≈ 10–15 (angular/distance error, time-to-locate, path efficiency), hybrid-mode comparison, ANOVA/t-tests.
