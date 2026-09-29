# Cane — P3 assembly protocol (outsourced soldered build)

> The operational counterpart of the P2 bench protocol ([`bench-tests.md`](bench-tests.md)) for the soldered build. Decision 2026-09-26 ([README §3.9](../README.md#39-assembly--outsourcing-p3)): the P3 build is **designed in-house, soldered externally** — the schematics, carrier-PCB layouts, harness/wiring design, and assembly drawings are produced in this project, the soldering/assembly is executed by an **external local hand-solder service**, and the bare carrier PCBs come from a PCB **fab** house. The P3 window (Nov 2–21, 2026), exit criterion (parity — [`bench-tests.md`](bench-tests.md) §P3 parity re-run), and phase structure are unchanged ([README §6](../README.md#6-development-phases)). The builds are the evaluated devices — the units the human study runs on ([README §5](../README.md#5-evaluation-plan)) — so the handoff package and the incoming inspection below are **gates, not courtesies**. The [hand-solderability binding](#2-hand-solderability-binding-the-designs-must-fit-the-service) is the design constraint; the [handoff package](#3-handoff-package-what-the-service-receives) is the service's build instruction; the [incoming inspection](#5-incoming-inspection-in-house-gate-before-any-power-up) is the return gate.

## 1. Division of labor

| Task                                                                                                                                       | Owner                              |
| ------------------------------------------------------------------------------------------------------------------------------------------ | ---------------------------------- |
| Schematics, carrier-PCB layouts in KiCad, human GUI review before fabrication ([README §4.3](../README.md#43-eda-toolchain-kicad-and-mcp)) | in-house                           |
| Harness/wiring design: connector selection + keying, wire gauge/color/length schedule, strain relief, labeling                             | in-house                           |
| Assembly drawings + wiring diagrams (the service-facing build instructions)                                                                | in-house                           |
| Bare carrier-PCB fabrication, ordered **with spares**                                                                                      | external PCB fab house             |
| Soldering + assembly of boards and harness into the permanent builds                                                                       | external local hand-solder service |
| Incoming inspection, trivial rework, T0–T8 parity re-run                                                                                   | in-house                           |

## 2. Hand-solderability binding (the designs must fit the service)

The service is a hand-solder shop, not an SMT assembly line — so the KiCad designs are constrained accordingly ([README §3.9](../README.md#39-assembly--outsourcing-p3)):

- The carrier boards are **through-hole-dominant**: module header rows, wiring points, keyed/polarized through-hole connectors. **No fine-pitch SMT is designed onto our boards** — all fine-pitch electronics stays on the pre-assembled modules, extending the P2 pre-soldered rule ([README §3](../README.md#3-hardware) purchasing constraints) to our own boards.
- Generous clearances and pad sizing for hand work; single-sided component placement preferred (cheaper assembly, easier inspection).
- Every connector keyed or polarized where misinsertion is possible (I²C/I²S/IMU, amplifiers, battery path, audio jack); two connectors whose meanings could be swapped never share a silently-mirrored footprint.
- The boards self-explain at the bench: silkscreen carries net function and polarity on every connector and on the battery/jack paths.

## 3. Handoff package (what the service receives)

The service builds strictly from the package — nothing from memory, no improvisation; deviations are flagged on delivery, not discovered at power-up:

1. **Schematic set** per device (PDF exports from the KiCad projects), annotated.
2. **Bare carrier PCBs** from the fab order.
3. **The kit** — every module (P2-validated, headers pre-soldered), through-hole part, connector, wire, fastener, and consumable the boards and harness install, matched to BOM rows with exact manufacturer part numbers.
4. **Assembly drawings** per device: module placement and orientation on the board, connector pinouts and keying, the wire-by-wire harness schedule (color/gauge/length/label), and the battery, switch, and 3.5 mm jack wiring with polarity flags on every entry.
5. **Handling notes**: ESD caution for the modules, no hot-plugging of I²C/I²S, battery-path polarity double-checked before soldering.

## 4. Order lanes

1. **PCB fab (bare boards)** — manufacturing exports from the KiCad projects ([README §4.3](../README.md#43-eda-toolchain-kicad-and-mcp)); the order is placed at the breadboard exit (~Oct 27), after the P2 gate validates the designs, and **with ≥ 2 spares per device** — a defective board then costs a re-solder, not a re-fab.
2. **Local hand-solder service** — quotes and capability vetting inside the P2 window, booked before P3 entry ([`schedule.md`](schedule.md)). Selection criteria: demonstrable through-hole hand-solder workmanship (references or a small trial board), turnaround quoted inside the P3 window, local drop-off/pick-up (a fix loop without shipping), builds exactly from the package, per-build quote. Selected **once, from the quotes, before P3 entry** (the buy-once rule extended to services, [README §3](../README.md#3-hardware)); the quote folds into the co-researcher's purchase-approval sign-off lane ([README §9](../README.md#9-open-items)).

## 5. Incoming inspection (in-house gate, before any power-up)

1. **Visual** — every joint checked against the assembly drawings: cold joints, bridges, misses, wire terminations; polarity re-checked on the battery path, amplifiers, and jack.
2. **Continuity** — per-net buzzer check from module headers through harness ends, plus a no-short check across the rails to GND.
3. **First power-up, current-limited** — multimeter inline or current-limited USB source, the same [bring-up safety rule as P2](bench-tests.md#bring-up-safety).
4. Only after all three: the full **T0–T8 parity re-run** ([§ P3 parity re-run](bench-tests.md#p3-parity-re-run)) — the P3 exit criterion ([README §6](../README.md#6-development-phases)).

Systematic workmanship faults route back to the service for rework (on spare boards where relevant); the in-house rework kit covers trivial fixes only.

## 6. Rework kit ([README §3.4](../README.md#34-assembly--bench-tools-one-time))

Minimal by decision 2026-09-26 — the one-time full soldering kit is dropped, because no in-house production soldering remains: digital multimeter, wire strippers/cutters/pliers, USB data cables, tweezers, one adjustable soldering iron + solder + flux + desoldering wick for trivial fixes on returned assemblies and harness repairs later in P4/P5; helping-hands/PCB holder optional. *Selection pending the co-researcher.*
