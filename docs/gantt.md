# Auris — Project Gantt

One Gantt chart per semester (plus the completed old-concept work). Within each
chart, the **sections are the project's phases** ([`README.md` §5](../README.md#5-development-phases)),
so each chart reads as a phase axis laid on the calendar.

## Schedule assumptions

- **Development under the new concept starts 2026-09-06** ("tomorrow" at the time of scheduling).
- **Semester 1 ends one week before Christmas** → 2026-12-18 (Friday).
- **Semester break** → 2026-12-19 to 2027-01-03 (no scheduled work).
- **Semester 2** → 2027-01-04 to **mid-April 2027 (2027-04-15)**.
- **Pre-oral defense ≈ 2027-03-22 (tentative)** on a full Chapters 1–5 draft, with Chapter 4 drafted from the prototype evaluations — there is **no proposal defense**. **Final defense** on the last day of Semester 2 (2027-04-15); revisions and final submission complete within April 2027 (to 2027-04-30). Thesis-writing authority: [`auris-thesis/README.md`](auris-thesis/README.md).
- Phase durations are planning estimates; each phase's exit criteria
  ([`README.md` §5](../README.md#5-development-phases)) — not the calendar —
  gate the next phase. Dates shift at the gates, not before.
- Old-concept bars are **approximate** (that work was cleared; only the
  clearing date, 2026-09-03, is exact).
- The Production procurement bar precedes Phase 7a because the D435i (import,
  ≈ ₱24–27k landed, [§6.2](../README.md#62-production-purchases-stage-2)) has lead time.

## Old concept — completed (before Semester 1)

```mermaid
gantt
    title Old concept — completed work (cleared 2026-09-03)
    dateFormat YYYY-MM-DD
    axisFormat %d %b
    todayMarker off

    section Old concept
    Former concept and plan :done, o1, 2026-06-01, 2026-09-02
    New concept consolidation and literature notes :done, o2, 2026-09-03, 2026-09-05
    New development plan — decisions D1-D10 and hardware BOM :done, o3, 2026-09-04, 2026-09-05
```

## Semester 1 — development and thesis (2026-09-06 → 2026-12-18)

```mermaid
gantt
    title Semester 1 — phases on the calendar (2026-09-06 to 2026-12-18)
    dateFormat YYYY-MM-DD
    axisFormat %d %b
    todayMarker off

    section Phase 0 — bench and link validation
    Streaming bench, latency table, session recorder :active, p0, 2026-09-06, 2026-09-26

    section Phase 1 — perception
    12-class YOLOv8 detection pipeline :p1, 2026-09-27, 2026-10-17

    section Phase 2 — room mapping
    Open3D TSDF fusion, map save and load :p2, 2026-10-18, 2026-11-07

    section Phase 3 — object localization
    Map-anchored objects, ID stability, re-look logic :p3, 2026-11-01, 2026-11-21

    section Phase 4 — audio simulation
    Unity + Steam Audio scene, NetMQ bridge, 12 sound assets :p4, 2026-11-15, 2026-12-12

    section Phase 5 — closed-loop integration
    End-to-end find-an-object, latency and link gates :p5, 2026-11-29, 2026-12-18

    section Thesis writing
    Chapter 2 — Review of Related Literature :t1, 2026-09-08, 2026-10-31
    Chapter 1 — The Problem and Its Setting (draft) :t2, 2026-10-19, 2026-11-28
    Chapter 3 — Methodology (system design draft) :t3, 2026-11-16, 2026-12-18
```

## Semester 2 — evaluation, production, thesis (2027-01-04 → 2027-04-30)

```mermaid
gantt
    title Semester 2 — phases on the calendar (2027-01-04 to 2027-04-30)
    dateFormat YYYY-MM-DD
    axisFormat %d %b
    todayMarker off

    section Phase 6 — evaluation study
    6a — protocol, ethics, recruitment :p6a, 2027-01-04, 2027-01-23
    6b — pilot and data collection (blindfolded-sighted + VI pilot) :p6b, 2027-01-18, 2027-02-27
    6c — analysis :p6c, 2027-02-22, 2027-03-13

    section Phase 7 — production iteration
    Procurement (D435i, print wearable) :p7proc, 2027-02-22, 2027-03-06
    7a — production hardware bring-up :p7a, 2027-03-01, 2027-03-13
    7b — ROS 2 and RTAB-Map port :p7b, 2027-03-14, 2027-03-26
    7c — revalidation at parity :p7c, 2027-03-27, 2027-04-07

    section Thesis writing
    Chapter 3 — Methodology (finalize) :t4, 2027-02-22, 2027-03-13
    Chapter 4 — Results and Discussions (from prototype evaluations) :t5, 2027-03-02, 2027-03-20
    Chapter 5 — Conclusions and full draft consolidation :t6, 2027-03-23, 2027-04-09
    IEEE-format mirror (ieee.md) :t7, 2027-04-06, 2027-04-15
    Revisions and final submission :t8, 2027-04-15, 2027-04-30

    section Milestones
    Pre-oral defense (full Chapters 1-5 draft) :milestone, m1, 2027-03-22, 0d
    Final defense :milestone, m2, 2027-04-15, 0d
```

## Reading the charts

- **Sections = phases**, so each chart is a phase axis on the calendar; bars
  within a section are that phase's work packages (sub-phase letters 6a–6c,
  7a–7c per [`README.md` §5](../README.md#5-development-phases)).
- **Deliberate overlaps** (Phases 3/4/5, 6a/6b/6c, thesis chapters) are
  pipelining — the tail of one task overlaps the head of the next, but each
  gate still applies.
- **Cross-chart dependencies**: Semester 1's Phase 5 gate opens Semester 2's
  Phase 6; Phase 6c's data and the procurement bar feed Phase 7a; Phase 7c's
  parity result and the Chapter 4 draft (from the prototype evaluations) feed
  the **pre-oral defense**; the pre-oral feeds the final defense.
- Phase 7 sits after the evaluation data collection so 7b's regression suite
  can replay **recorded Prototype sessions** (D8).
- If a phase gate slips, the buffer is the January–February overlap and the
  thesis-draft tail of April — protect the two milestones first.
