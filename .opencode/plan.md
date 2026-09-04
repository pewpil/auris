# Auris — Development Plan (summary)

> Canonical plan: the project [`README.md`](../README.md). This file condenses it
> into standing session instructions. Consolidated on 2026-09-04 from the
> concept in [`concept.md`](concept.md) and the literature consolidation
> ([`docs/auris-thesis/notes/`](../docs/auris-thesis/notes/)).

## Concept in one line

A blindfolded/VI user asks for one of 12 everyday object classes; the system —
having mapped the room from head-worn depth — projects that object's
characteristic sound **spatially, head-tracked, as if the object emits it**;
the user follows the sound. If the target is absent/moved, the user is
prompted to look around more.

## Architecture

- **Head unit:** Prototype = LiDAR iPhone (RGB + depth via ARKit `sceneDepth` +
  6-DoF pose via ARKit VIO) + perforated over-ear headphones (wired to desktop).
  Production = 3D-printed head-mounted wearable + RealSense-class RGB-D camera.
- **Link:** wireless-first (Wi-Fi streaming); long-USB tether fallback; decision
  gate in Phase 5.
- **Desktop computing unit (plain-Python processes over ZeroMQ):** sensor
  ingest → YOLOv8 12-class detection (COCO weights) → Open3D depth-fusion
  mapping → object localization (map-anchored object states + re-look prompt) →
  generic-HRTF binaural audio rendered in **Unity (Steam Audio)** — head-tracked,
  per-class auditory icons, distance encoded; decision logic in Python over
  the bus (thin renderer); STT voice query + experimenter-trigger fallback; session
  recorder/replayer (every session recorded — standing requirement).
- **Production:** ROS 2 + RTAB-Map only if the camera swaps off ARKit.

## Phases (detail: README §5)

0. Bench & link validation + build the recorder/replayer
1. Perception (12-class detection)
2. Room mapping (ARKit pose + Open3D TSDF)
3. Object localization (+ re-look prompt logic)
4. Audio simulation (Unity + Steam Audio scene, NetMQ bridge, 12 sound assets)
5. Closed-loop integration (<100 ms end-to-end budget; wireless-vs-tether gate)
6. Evaluation study (blindfolded-sighted; spatial-audio guidance only — D10)
7. Production iteration (7a hardware bring-up · 7b ROS 2 + RTAB-Map port · 7c revalidation)

## Standing rules

- Stage rationale is **financial**: available hardware first, purchases at
  Production.
- End-to-end latency budget **< 100 ms** (Sound of Vision benchmark).
- The 12 classes (all COCO): bottle, cup/mug, cell phone, book, chair, laptop,
  remote, keyboard, clock, potted plant, vase, backpack.
- **Guidance output is strictly non-verbal spatial audio** (D10) — the wearable
  never speaks directions or object names during guidance; voice is query input
  only, and there is **no speech-only comparison baseline** (single-condition
  evaluation).
- Literature notes live in `docs/auris-thesis/notes/` — consult before writing
  thesis sections; thesis draft goes to `docs/auris-thesis/paper.md` only.
