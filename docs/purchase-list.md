# Cane — Purchase list (P2 bench order)

> **To be finalized by the co-researcher** — the selection of every component and material is pending, for financial reasons ([README §3](../README.md#3-hardware)). The itemized bench order — the two devices' electronics (tracking hardware included, it is part of the final design), the battery/power path, bench infrastructure, and one-time bench tools — is built from that selection, everything the T0–T8 tests of [`docs/bench-tests.md`](bench-tests.md) consume. P3-only and study-only blocks (print-service lane/housings, soldering tools, strap, evaluation hardware) are deferred to their phases. Re-opened 2026-09-29 (second clearing — the researched esp32-s3 itemization of 2026-09-26 lives in master's history): the re-selection criterion has changed — the components and materials are to be **compatible with both esp32-s3 and raspi-5** ([README §3](../README.md#3-hardware) constraint 4).
>
> The prior itemization (branch esp32-s3, 2026-09-26, ₱9,750–18,150 fixed-price envelope) remains in git history as research input for the compatible-list exercise: most rows were already board-class-agnostic (the 9-DoF IMU breakout, the ToF module on shared I²C, the SPI + IRQ/reset UWB module pair, buttons, earphones, battery path, bench infrastructure); the expected re-sourcing rows are the DVP-connection cameras, the audio path, and the link medium (each board class carries its own radio set). The four board-class branches (esp32-s3, esp32-p4, raspi-5, raspi-4) and the comparison matrix in [`docs/hw-comparison.md`](hw-comparison.md) give the compatibility screen.

## 1. Scope and the no-soldering rule

- The order buys every component, fixture, and tool the test matrix and wiring maps of [`bench-tests.md`](bench-tests.md) consume — nothing more.
- **No-soldering rule (purchasing constraint).** P2 attaches every component non-permanently and involves no soldering of any kind, so every module must be ordered with **headers pre-soldered**; verify the listing before checkout. Unsoldered arrivals are set aside for the P3 build, never soldered during P2.
- **Buy-once rule (purchasing constraint).** Each component category is purchased once — the selection is made *before* the purchase, never corrected by a re-buy afterwards ([README §3](../README.md#3-hardware)).
- **Dual-compatible criterion (added 2026-09-29).** Components and materials are selected to work with **both** the esp32-s3 and raspi-5 board classes; rows that cannot serve both are re-sourced per [`hw-comparison.md`](hw-comparison.md).
- Every row carries its consuming bench tests or phase, so any cut can be checked against the T0–T8 matrix and the phase structure before it is made.

## 2. Status

The itemized sections (device electronics, battery/power path, bench infrastructure, one-time tools), the totals, and the checkout checklist are recorded here once the co-researcher's component selection and the purchase-approval sign-off are made.
