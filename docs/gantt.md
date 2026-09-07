# Auris — Project Gantt

One Gantt chart per semester (plus the completed old-concept work). Within each
chart, the **sections are the project's phases** ([`README.md` §5](../README.md#5-development-phases)),
so each chart reads as a phase axis laid on the calendar.

## Schedule assumptions

- **Development under the new concept starts 2026-09-06** ("tomorrow" at the time of scheduling).
- **Semester 1 ends 2026-12-14** (Monday), per the school calendar — not the
  previously assumed one-week-before-Christmas date.
- **Semester break** → 2026-12-15 to 2027-01-03 (no scheduled work).
- **Semester 2** → 2027-01-04 to **mid-April 2027 (2027-04-15)**.
- **Pre-oral defense ≈ 2026-12-11 (tentative, Friday)** — **before Semester 1
  ends** — on a full Chapters 1–5 draft whose **Chapters 4–5 draw on the
  prototype evaluation**: the evaluation study (Phase 6) runs in Semester 1,
  immediately after the Phase 5 gate. There is **no proposal defense**. The
  **Production iteration (Phase 7) runs entirely in Semester 2**; the **final
  defense 2027-04-15** falls before that semester ends; revisions and final
  submission complete within April 2027 (to 2027-04-30). Thesis-writing
  authority: [`auris-thesis/README.md`](auris-thesis/README.md).
- **Semester 1 is the critical path**: Phases 0–5 are compressed (2–3 weeks
  each, deliberately overlapped) so the evaluation and the Chapters 4–5 draft
  fit before the pre-oral. Phase 6a (protocol, ethics, recruitment) runs in
  parallel from late September and must be ready before the Phase 5 gate
  opens 6b.
- Phase durations are planning estimates; each phase's exit criteria
  ([`README.md` §5](../README.md#5-development-phases)) — not the calendar —
  gate the next phase. Dates shift at the gates, not before.
- Old-concept bars are **approximate** (that work was cleared; only the
  clearing date, 2026-09-03, is exact).
- The Production procurement bar precedes Phase 7a because the D435i (import,
  ≈ ₱24–27k landed, [§6.2](../README.md#62-production-purchases-stage-2)) has lead time.
- Rendering: every chart sets `topAxis: true` (date ticks on **top and
  bottom**) and a red `todayMarker` line via frontmatter config.

## Old concept — completed (before Semester 1)

```mermaid
---
config:
  gantt:
    topAxis: true
    todayMarker: "stroke-width:2px,stroke:#c62828"
---
gantt
    title Old concept — completed work (cleared 2026-09-03)
    dateFormat YYYY-MM-DD
    axisFormat %d %b

    section Old concept
    Former concept and plan :done, o1, 2026-08-23, 2026-09-02
    New concept consolidation and literature notes :done, o2, 2026-09-03, 2026-09-05
    New development plan — decisions D1-D10 and hardware BOM :done, o3, 2026-09-04, 2026-09-05
```

## Semester 1 — development, prototype evaluation, thesis (2026-09-06 → 2026-12-14)

```mermaid
---
config:
  gantt:
    topAxis: true
    todayMarker: "stroke-width:2px,stroke:#c62828"
---
gantt
    title Semester 1 — phases on the calendar (2026-09-06 to 2026-12-14)
    dateFormat YYYY-MM-DD
    axisFormat %d %b

    section Phase 0 — bench and link validation
    Streaming bench, latency table, session recorder :active, p0, 2026-09-07, 2026-09-19

    section Phase 1 — perception
    12-class YOLOv8 detection pipeline :p1, 2026-09-20, 2026-10-03

    section Phase 2 — room mapping
    Open3D TSDF fusion, map save and load :p2, 2026-09-28, 2026-10-11

    section Phase 3 — object localization
    Map-anchored objects, ID stability, re-look logic :p3, 2026-10-05, 2026-10-18

    section Phase 4 — audio simulation
    Unity + Steam Audio scene, NetMQ bridge, 12 sound assets :p4, 2026-10-12, 2026-11-01

    section Phase 5 — closed-loop integration
    End-to-end find-an-object, latency and link gates :p5, 2026-10-26, 2026-11-08

    section Phase 6 — evaluation study (prototype)
    6a — protocol, ethics, recruitment :p6a, 2026-09-28, 2026-10-25
    6b — pilot and data collection (blindfolded-sighted + VI pilot) :p6b, 2026-11-09, 2026-11-22
    6c — analysis :p6c, 2026-11-16, 2026-11-29

    section Thesis writing
    Chapter 2 — Review of Related Literature :t1, 2026-09-07, 2026-10-10
    Chapter 1 — The Problem and Its Setting :t2, 2026-10-12, 2026-11-07
    Chapter 3 — Methodology (system design draft) :t3, 2026-10-19, 2026-11-21
    Chapter 4 — Results (from the prototype evaluations) :t4, 2026-11-23, 2026-12-06
    Chapter 5 — Conclusions and Recommendations (draft) :t5, 2026-11-30, 2026-12-10
    Full-draft consolidation and pre-oral prep :t6, 2026-12-07, 2026-12-10

    section Milestones
    Pre-oral defense (full Chapters 1-5 draft) :milestone, m1, 2026-12-11, 0d
```

## Semester 2 — production, thesis, defense (2027-01-04 → 2027-04-30)

```mermaid
---
config:
  gantt:
    topAxis: true
    todayMarker: "stroke-width:2px,stroke:#c62828"
---
gantt
    title Semester 2 — phases on the calendar (2027-01-04 to 2027-04-30)
    dateFormat YYYY-MM-DD
    axisFormat %d %b

    section Phase 7 — production iteration
    Procurement (D435i, print wearable) :p7proc, 2027-01-04, 2027-01-15
    7a — production hardware bring-up :p7a, 2027-01-18, 2027-02-05
    7b — ROS 2 and RTAB-Map port :p7b, 2027-02-01, 2027-02-26
    7c — revalidation at parity :p7c, 2027-03-01, 2027-03-12

    section Thesis writing
    Chapter 3 — Methodology finalize (3.3, 3.4 + production components) :t7, 2027-01-04, 2027-01-29
    Chapter 4 — Production revalidation update :t8, 2027-03-01, 2027-03-19
    Chapter 5 — finalize (recommendations from the port) :t9, 2027-03-08, 2027-03-26
    IEEE-format mirror (ieee.md) :t10, 2027-03-29, 2027-04-12
    Absctract, ToC/LoT/LoF, final-draft consolidation :t11, 2027-03-29, 2027-04-13

    section Milestones
    Final defense :milestone, m2, 2027-04-15, 0d
```

## Reading the charts

- **Sections = phases**, so each chart is a phase axis on the calendar; bars
  within a section are that phase's work packages (sub-phase letters 6a–6c,
  7a–7c per [`README.md` §5](../README.md#5-development-phases)).
- **Deliberate overlaps** (Phases 2/3/4/5, 6b/6c, thesis chapters) are
  pipelining — the tail of one task overlaps the head of the next, but each
  gate still applies.
- **Within Semester 1**: Phase 5's gate opens 6b (6a is already complete);
  6c's analysis feeds Chapter 4; the Chapters 4–5 draft and consolidation feed
  the **pre-oral defense, before the semester ends**.
- **Within Semester 2**: procurement feeds 7a; 7b regression-tests replayed
  Prototype **and evaluation** sessions (D8); 7c's parity result feeds the
  Chapter 4 production-revalidation update and the **final defense, before
  the semester ends**.
- If a phase gate slips, the buffer is the 6b/6c overlap and the
  Chapter 4/Chapter 5 tail — protect the pre-oral first, the final defense
  second.
