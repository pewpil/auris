# Auris — Development Processes (list form)

This document lists the development processes as a checklist, mirroring the Gantt charts
([`phases-gantt-m1-m4-prototype.md`](phases-gantt-m1-m4-prototype.md) / [`phases-gantt-m5-m8-production.md`](phases-gantt-m5-m8-production.md))
but in list form for tracking. The canonical phase definitions and exit criteria live in [`README.md` §5](../README.md#5-development-phases).
Thesis-drafting milestones are summarized per phase and detailed in [`.opencode/man.md`](../.opencode/man.md).

Check boxes are for progress tracking; they do not change the schedule.

## Phase 0 — Literature review & ethics (3 wks)

- [x] Literature review (drafted with PRISMA 2020 apparatus in thesis.md §2 + Appendix E; institutional re-runs owed — see `.opencode/man.md`)
- [x] Gap analysis → justifies the design choices (design-choice justification matrix, Table 2.1, in thesis.md §2.5)
- [ ] Concept consolidation
- [ ] Ethics/IRB application prepared & filed
- [ ] Stand up the contract-first repo skeleton (`Frame`, `SceneState`, `AudioSink`)
- Thesis draft (concurrent): [§1 The Problem & Its Setting](auris-thesis/thesis.md#1-the-problem-and-its-setting); [§2 Review of Related Literature](auris-thesis/thesis.md#2-review-of-related-literature); Appendix D (ethics/IRB); front-matter skeleton

## Phase 1 — Prototype CV pipeline (4 wks)

- [ ] WiFi phone capture behind `CameraSource` (Android/iPhone MJPEG/RTSP)
- [ ] `MarkerLocalizer` (ARUCO solvePnP)
- [ ] `ObjectDetector` (YOLO v8/v11) bring-up — training offloaded to Google Colab (free T4), inference local on the RTX 4060
- [ ] ARUCO side-of-head `HeadPoseEstimator` (6DoF, 30–60 Hz)
- [ ] Calibration tooling establishing the room/world frame
- Constraint: **RGB only — no triangulation, no depth**
- Thesis draft (concurrent): [§3.1 Research Study](auris-thesis/thesis.md#31-research-study); [§3.2 System Overview](auris-thesis/thesis.md#32-system-overview); [§3.5 System Design](auris-thesis/thesis.md#35-system-design) (contracts, transform); [§3.6 Camera](auris-thesis/thesis.md#361-camera-capture-and-calibration) / [Object-perception](auris-thesis/thesis.md#362-object-perception) / [Head-tracking](auris-thesis/thesis.md#363-head-tracking-6dof) write-ups (proto); Appendix A & B (proto)

## Phase 2 — Prototype audio engine (3 wks)

- [ ] Unity scene + HRTF spatializer + `AudioSink`
- [ ] Earcon library + always-on / beacon modes (keyboard first)
- [ ] Distance cues (gain, low-pass, reverb)
- Thesis draft (concurrent): [§3.6 Spatial audio engine](auris-thesis/thesis.md#364-spatial-audio-engine) (proto); Appendix C (proto earcon library)

## Phase 3 — Prototype integration & performance (3 wks)

- [ ] `SceneState` UDP contract defined and **frozen**
- [ ] Threaded/async pipeline
- [ ] End-to-end latency measured & mitigated on wired and wireless paths
- [ ] Feasibility test — risk A (head-rotation-tracked spatial audio intuitiveness) & risk B (marker-based head tracking at 30–60 Hz)
- Constraint: built against the contracts, not the hardware
- Thesis draft (concurrent): [§3.3 Setting & Participant](auris-thesis/thesis.md#33-setting-and-participant) (proto rig); [§3.4 Description of Experiments](auris-thesis/thesis.md#34-description-of-experiments) (protocol); [§3.6 Networking & interconnect](auris-thesis/thesis.md#365-networking-and-interconnect) (proto); [§3.7](auris-thesis/thesis.md#37-block-diagram)/[§3.8](auris-thesis/thesis.md#38-system-flowcharts) Block diagram & flowcharts (proto); [§4.1](auris-thesis/thesis.md#41-cv-accuracy-results) & [§4.2](auris-thesis/thesis.md#42-system-performance-results) results drafted from proto measurements

## Financial gate

- [ ] Order all Production components (lead times run in parallel; ordered during Phase 3 so they arrive before Phase 4)

## Transition

- [ ] Gear arrival & assembly (components ordered Nov 16)

## Phase 4 — Production CV pipeline (4 wks, holiday-stretched)

- [ ] Multi-camera room-scale world-frame calibration
- [ ] YOLO re-training on production captures + `TriangulatedLocalizer`
- [ ] IMU-fused 6DoF head pose + handoff stability
- [ ] CV accuracy validation vs tape-measure / ARUCO ground truth
- Thesis draft (concurrent): [§3.6 Camera](auris-thesis/thesis.md#361-camera-capture-and-calibration) / [Object-perception](auris-thesis/thesis.md#362-object-perception) / [Head-tracking](auris-thesis/thesis.md#363-head-tracking-6dof) write-ups (production); [§4.1 CV-accuracy results](auris-thesis/thesis.md#41-cv-accuracy-results) (production); Appendix A & B (production)

## Phase 5 — Production audio & integration (3 wks)

- [ ] Engines ported behind unchanged contracts onto production gear
- [ ] Full-room deployment
- [ ] End-to-end latency verified per interconnect path
- Thesis draft (concurrent): [§3.3](auris-thesis/thesis.md#33-setting-and-participant)/[§3.6](auris-thesis/thesis.md#365-networking-and-interconnect) audio & networking (production); [§3.7](auris-thesis/thesis.md#37-block-diagram)/[§3.8](auris-thesis/thesis.md#38-system-flowcharts) final diagrams; [§4.2 latency results](auris-thesis/thesis.md#42-system-performance-results) (production); Appendix C (production)

## Phase 6 — User study & final thesis integration (8 wks)

- [ ] Pilot sessions + participant scheduling (ethics cleared since Sep)
- [ ] Study data collection
- [ ] Statistical analysis (ANOVA / t-tests)
- [ ] Final thesis integration & revisions
- [ ] Buffer — defense preparation
- Thesis draft (concurrent): [§4.3](auris-thesis/thesis.md#43-user-study-results)–[§4.4](auris-thesis/thesis.md#44-hybrid-mode-comparison)–[§4.5](auris-thesis/thesis.md#45-interpretation-and-threats-to-validity) user-study results / hybrid-mode / interpretation; [§5 Conclusions](auris-thesis/thesis.md#51-conclusion); Abstract; TOC / Lists of Tables & Figures; front-matter finalization; link-strip & polish
