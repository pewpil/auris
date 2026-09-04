# Auris — Concept

> Consolidated on 2026-09-04 from the working draft (the former `new.md`) and the
> literature consolidation ([`docs/auris-thesis/notes/`](../docs/auris-thesis/notes/)).
> Kept consistent with the development plan in the project
> [`README.md`](../README.md).

## The user

The user is visually impaired — or blindfolded (replicating VI for the study).

## Environment

A single indoor room with obstacles (furniture, walls) and various everyday
objects.

## The goal

The user locates their desired object.

## Task loop

1. **Map** — the user looks around; depth frames + 6-DoF head pose accumulate
   into a room map (scanning phase).
2. **Query** — the user names the target object by voice (one of 12 everyday
   classes) or the experimenter triggers it.
3. **Localize** — the system resolves the target's position from the map. If
   the object is absent, ambiguous, or has moved, the user is prompted to look
   around more.
4. **Guide** — the target's characteristic sound is projected spatially at the
   object's position, head-tracked and distance-encoded; the user follows the
   sound and reaches the object.

## Navigation aid principle

Sounds are projected by the head-mounted wearable according to the desired
object's position and the user's head position and orientation — rendered **as
if the object itself emits the sound** (e.g., an object at the user's left is
heard at the left). Each object class carries its own characteristic sound
(auditory-icon mapping).

## System modules

### Head-mounted wearable

- **Head position and orientation** — 6-DoF head pose (Prototype: ARKit VIO on
  a LiDAR iPhone; Production: RealSense-class camera + RTAB-Map).
- **Vision** — depth (obstacle/room sensing) + real-time RGB imagery.
- **Sound output** — perforated over-ear headphones.

### Computing unit (desktop)

1. **Computer vision** — identifies objects (12 everyday classes, YOLOv8) from
   the mapped-out room.
2. **Audio simulation** — projects each object's characteristic sound at its
   position (generic-HRTF binaural, head-tracked); encodes distance.
3. **Mapped-out room** — depth + head pose accumulate into a room map
   (Prototype: Open3D fusion; Production: RTAB-Map with relocalization).
4. **Object localization** — anchors detections in the map and resolves the
   target; if the object is not where identified (absent/moved), the user is
   prompted to look around more.
5. **Query handling** — voice input (speech-to-text) with an experimenter
   trigger fallback.
6. **Session recorder** — every session recorded for offline analysis.

Hardware and development detail (stages, phases, evaluation) live in the
project [`README.md`](../README.md).
