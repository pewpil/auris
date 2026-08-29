# Auris — Development Processes (list form)

This document lists the development processes as a checklist, mirroring the Gantt charts
([`phases-gantt-m1-m4-prototype.md`](phases-gantt-m1-m4-prototype.md) / [`phases-gantt-m5-m8-production.md`](phases-gantt-m5-m8-production.md))
but in list form for tracking. The canonical phase definitions and exit criteria live in [`README.md` §5](../README.md#5-development-phases).
Thesis-drafting milestones are summarized per phase and detailed in [`.opencode/man.md`](../.opencode/man.md).

Check boxes are for progress tracking; they do not change the schedule.

## Phase 0 — Literature review & ethics (3 wks)

- [ ] Literature review — HRTF/binaural rendering, sonification & earcons, assistive object-localization systems
- [ ] Gap analysis → justifies the design choices
- [ ] Concept consolidation
- [ ] Ethics/IRB application prepared & filed
- [ ] Stand up the contract-first repo skeleton (`Frame`, `SceneState`, `AudioSink`)
- Thesis draft (concurrent): §1 The Problem & Its Setting; §2 Review of Related Literature; Appendix D (ethics/IRB); front-matter skeleton

## Phase 1 — Prototype CV pipeline (4 wks)

- [ ] WiFi phone capture behind `CameraSource` (Android/iPhone MJPEG/RTSP)
- [ ] `MarkerLocalizer` (ARUCO solvePnP)
- [ ] `ObjectDetector` (YOLO v8/v11) bring-up — training offloaded to Google Colab (free T4), inference local on the RTX 4060
- [ ] ARUCO side-of-head `HeadPoseEstimator` (6DoF, 30–60 Hz)
- [ ] Calibration tooling establishing the room/world frame
- Constraint: **RGB only — no triangulation, no depth**
- Thesis draft (concurrent): §3.1 Research Study; §3.2 System Overview; §3.5 System Design (contracts, transform); §3.6 Camera / Object-perception / Head-tracking write-ups (proto); Appendix A & B (proto)

## Phase 2 — Prototype audio engine (3 wks)

- [ ] Unity scene + HRTF spatializer + `AudioSink`
- [ ] Earcon library + always-on / beacon modes (keyboard first)
- [ ] Distance cues (gain, low-pass, reverb)
- Thesis draft (concurrent): §3.6 Spatial audio engine (proto); Appendix C (proto earcon library)

## Phase 3 — Prototype integration & performance (3 wks)

- [ ] `SceneState` UDP contract defined and **frozen**
- [ ] Threaded/async pipeline
- [ ] End-to-end latency measured & mitigated on wired and wireless paths
- [ ] Feasibility test — risk A (head-rotation-tracked spatial audio intuitiveness) & risk B (marker-based head tracking at 30–60 Hz)
- Constraint: built against the contracts, not the hardware
- Thesis draft (concurrent): §3.3 Setting & Participant (proto rig); §3.4 Description of Experiments (protocol); §3.6 Networking & interconnect (proto); §3.7/§3.8 Block diagram & flowcharts (proto); §4.1 & §4.2 results drafted from proto measurements

## Financial gate

- [ ] Order all Production components (lead times run in parallel; ordered during Phase 3 so they arrive before Phase 4)

## Transition

- [ ] Gear arrival & assembly (components ordered Nov 16)

## Phase 4 — Production CV pipeline (4 wks, holiday-stretched)

- [ ] Multi-camera room-scale world-frame calibration
- [ ] YOLO re-training on production captures + `TriangulatedLocalizer`
- [ ] IMU-fused 6DoF head pose + handoff stability
- [ ] CV accuracy validation vs tape-measure / ARUCO ground truth
- Thesis draft (concurrent): §3.6 Camera / Object-perception / Head-tracking write-ups (production); §4.1 CV-accuracy results (production); Appendix A & B (production)

## Phase 5 — Production audio & integration (3 wks)

- [ ] Engines ported behind unchanged contracts onto production gear
- [ ] Full-room deployment
- [ ] End-to-end latency verified per interconnect path
- Thesis draft (concurrent): §3.3/§3.6 audio & networking (production); §3.7/§3.8 final diagrams; §4.2 latency results (production); Appendix C (production)

## Phase 6 — User study & final thesis integration (8 wks)

- [ ] Pilot sessions + participant scheduling (ethics cleared since Sep)
- [ ] Study data collection
- [ ] Statistical analysis (ANOVA / t-tests)
- [ ] Final thesis integration & revisions
- [ ] Buffer — defense preparation
- Thesis draft (concurrent): §4.3–§4.5 user-study results / hybrid-mode / interpretation; §5 Conclusions; Abstract; TOC / Lists of Tables & Figures; front-matter finalization; link-strip & polish
