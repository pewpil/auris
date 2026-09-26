---
id: hw-comparison
aliases: []
tags: []
---
# Cane — Hardware branch comparison (esp32-s3 / esp32-p4 / raspi-5)

> **Derived from the three branch tips (2026-09-26)** — `esp32-s3` (`b853b5b`), `raspi-5` (`adade7f`), `esp32-p4` (`490a7fc`) — each carrying its own itemized components-and-pricing exercise over the `agnostic` hardware-open baseline (`5b40cb2`). Purpose: give the co-researcher's purchase-approval sign-off ([README §9](../README.md#9-open-items)) a single comparison artifact across **all** decision variables (cost, heat, power, compute, cameras, link, form — the full set of 26 below), with a 1/0/−1 score matrix: **1 = the branch's wearable board performs well on the variable, 0 = neither good nor poor, −1 = performs poorly** — absolute scores, so several boards can earn 1s together on a row. The branch choice itself is part of the sign-off; this matrix is a decision aid, not the decision.

**Comparison boundary.** The pointer and both P5-bound one-time blocks are byte-identical across the three tips (the ESP32-S3 pointer kit — devkit, IMU, time-of-flight module, ultra-wideband tag, buttons, earphones — plus bench infrastructure, harness kits, the printed-marker stock, the carrier-PCB and soldering-service quote lanes). Every difference priced or scored below is the **wearable side**: its compute board, cameras, audio path, and worn power chain.

## 1. Notes table — the facts each matrix score is grounded in

| # | Fact | ESP32-S3 branch | ESP32-P4 branch | Raspberry Pi 5 branch |
|---|---|---|---|---|
| N1 | Wearable compute chip | ESP32-S3-DevKitC-1, N16R8: 2 cores at 240 MHz (LX7) + vector instructions, 8 MB PSRAM, radio on-chip | ESP32-P4 Function-EV-Board: 2 cores at 400 MHz (RISC-V) + vector instructions, up to 32 MB PSRAM, ESP32-C6 companion module on-board | Raspberry Pi 5, 8 GB RAM: 4 cores at 2.4 GHz (Cortex-A76), runs Linux |
| N2 | Software class | microcontroller, no operating system | microcontroller, no operating system | full Linux single-board computer |
| N3 | Whole-project fixed-price envelope (each branch's committed totals) | ₱9,100–17,350 | ₱11,800–20,450 | ₱23,800–38,500 |
| N4 | Wearable-core cost inside that envelope (board + cameras + audio) | ~₱940–1,590 (second devkit + three OV2640 modules + amp pair) | ~₱4,300–6,700 (evaluation board + three MIPI camera modules + flat-cable spares + amp pair) | ~₱14,000–20,800 (Pi + two Camera Modules 3 Wide + cables + cards + audio sticks) |
| N5 | Mobile power chain (worn) | protected 18650 cell on a charging/protection board (rear station) | compact 10,000 mAh power bank, 5 V / 2 A out | USB power-delivery bank ≥ 20,000 mAh / 45 W + a 5 V/5 A trigger board + 5 A-rated cables; official 27 W mains supply for the bench only |
| N6 | Typical sustained draw, render + tracking active | under ~1.5 W | roughly 2–4 W | roughly 5–12 W |
| N7 | Runtime per charge (worst-case math from each branch's own rows) | 9 Wh cell at ~1 W, duty-cycled → many hours | 37 Wh bank at ~3 W → 10 h + | 74 Wh bank at ~8 W → ~8 h, on the branch's biggest bank |
| N8 | Thermal class at the head | cool, no heatsink | warm, no heatsink (larger board area spreads it) | wants a heatsink/active cooler near skin |
| N9 | Boot to first sound | well under a second | well under a second | tens of seconds (Linux boot) plus crash/freeze modes the branch itself flags |
| N10 | Camera ports on the board | one (parallel DVP) → cameras alternate; multiplexer question open at the tracking gate | one (MIPI serial lane) → cameras alternate; multiplexer question reopened at the tracking gate | two native MIPI lanes → mux question closed in hardware |
| N11 | Pinned cameras | OV2640, 160° field of view, ~₱100–200 each, stocked everywhere local | SC2336-class MIPI modules, ~₱300–900 each, fewer local stockists, lens-variant checkout caution | Camera Module 3 Wide official, 120°, autofocus — ₱1,500–3,000 each; distributor currently showing sold-out under a global supply advisory |
| N12 | Radio / link | native Wi-Fi on-chip; link = ESP-NOW (native, no accessory) | no radio on the chip — the on-board C6 companion hosts the access point; data crosses an esp_hosted hop | native Wi-Fi; hosts the access point through Linux networking |
| N13 | Audio path | two I²S class-D amplifiers → wired jack (literal hardware path) | identical literal I²S path | USB dongle → jack (kernel audio stack in the path) |
| N14 | Board size / on-head mass class | devkit ~25×55 mm; wearable well under ~200 g all-in | evaluation board ~100×70 mm + bank → ~300 g class | 85×56 mm + active cooling + biggest bank → ~450 g+ class |
| N15 | Supply-lane health (each branch's own words) | confirmed in stock locally (₱499 devkit) | listed locally but single-shop risk flagged | distributor currently sold-out at ₱11,449 (8 GB), price/lead-time instability advisory |

## 2. The score matrix (1 performs well / 0 neither / −1 performs poorly)

| # | Variable | ESP32-S3 | ESP32-P4 | RPi 5 |
|---|---|---|---|---|
| 1 | Whole-project cost envelope | 1 | 0 | −1 |
| 2 | Bench-phase outlay (parts must land before the breadboard phase) | 1 | 0 | −1 |
| 3 | Wearable-core part cost | 1 | 0 | −1 |
| 4 | Supply stability on branch-critical rows | 1 | 0 | −1 |
| 5 | Compute headroom at the render + tag-detection co-residence gate | 0 | 1 | 1 |
| 6 | Motion-to-sound latency certainty (the 100 ms budget's audio path) | 0 | 1 | −1 |
| 7 | Render underrun risk (≤ 5 ms per audio buffer) | 0 | 1 | −1 |
| 8 | Boot-to-first-sound time and run reliability | 1 | 1 | −1 |
| 9 | Heat near the user's head at sustained load | 1 | 1 | −1 |
| 10 | Thermal-design burden on the printed build | 1 | 0 | −1 |
| 11 | Mobile-rail weight and bulk on the strap | 1 | 0 | −1 |
| 12 | Session runtime between charges | 1 | 1 | 0 |
| 13 | Power-rail complexity | 1 | 1 | −1 |
| 14 | Recharge turnaround and spare-supply logistics | 1 | 0 | −1 |
| 15 | Camera capture architecture (ports; mux exposure) | −1 | −1 | 1 |
| 16 | Camera module price and local availability | 1 | 0 | −1 |
| 17 | Camera field-of-view fit for the vision tier | 1 | 0 | 1 |
| 18 | Camera software and driver maturity | 1 | 0 | 1 |
| 19 | Audio path simplicity and fidelity | 1 | 1 | −1 |
| 20 | Link implementation risk on the branch's own stack | 1 | −1 | 0 |
| 21 | Link throughput headroom | 0 | 1 | 1 |
| 22 | Total on-head weight | 1 | 0 | −1 |
| 23 | Breadboard-friendliness under the no-solder rule | 1 | −1 | 0 |
| 24 | Hand-solder carrier fit for the outsourced soldered build | 1 | 0 | 0 |
| 25 | Fit in the strap-mounted wearable form | 1 | 0 | −1 |
| 26 | HRTF rendering capacity and quality-building headroom | 0 | 1 | 1 |
| | **Column sums** | **+19** | **+8** | **−9** |

Gate-weighted view — doubling the four rows where a bench gate can actually fail (rows 5, 6, 7, 15): **ESP32-S3 +18, ESP32-P4 +10, RPi 5 −9.** The ordering is stable under both weightings.

## 3. Explanation of each variable

1. **Whole-project cost envelope** — each branch's committed fixed-price total from its purchase list (notes row N3). Scored by where the envelope sits, not by a relative race: S3's sits comfortably low (+1), the P4's is a reasonable middle (0), the Pi's is more than double the S3's (−1).
2. **Bench-phase outlay** — what must be spent up front for the breadboard phase (the first gate of the schedule). The bench proves the same functions on all three, so paying for a 45 W supply and bank-class power before that proof is a poor property (−1), a mid-size evaluation board is neutral (0), a ₱500 devkit is well (+1).
3. **Wearable-core part cost** — the only part of the bill that differs by branch (notes N4). Same scoring logic.
4. **Supply stability** — how risky the branch-critical rows are to buy today under the plan's local-lane purchasing rule: S3 confirmed in stock locally (+1); P4 listed but its own document flags single-shop risk (0); the Pi's official distributor is currently showing sold-out with an explicit supply advisory (−1).
5. **Compute headroom at the co-residence gate** — the Phase 2 bench test 7 makes the audio renderer and the camera-vision pointer tracker share one chip. S3: expected to pass, but the margin is exactly what the plan's risk 8 worries about → 0. P4 (400 MHz, up to 32 MB PSRAM) and R5 (Linux + OpenCV-class compute) both carry genuinely comfortable margins (+1).
6. **Motion-to-sound latency certainty** — the plan's absolute 100 ms motion-to-sound target, of which ≤ 5 ms is the per-audio-buffer render row. S3: literal hardware path, but the margin must be proven on the bench — scored 0 (unguaranteed, unthreatened). P4: literal path and more margin → 1. R5: a kernel scheduler sits in the audio path; the branch itself names Linux audio jitter as its real exposure → −1.
7. **Render underrun risk** — the same render row from the failure side: buffer underruns make the sound stutter. Scoring mirrors row 6.
8. **Boot and run reliability** — participants in the Phase 5 study put this on their heads: S3 and P4 power on into the sound in under a second with no operating system to freeze (+1 each); the Pi takes tens of seconds and introduces crash/silent-failure modes → −1.
9. **Heat at the head** — sustained render + camera load next to skin. Both microcontroller-class boards run cool without heatsinks (+1 each); a quad-core 2.4 GHz Linux board wants a heatsink and airflow near the user's face → −1.
10. **Thermal-design burden on the printed build** — how much the enclosed, service-soldered Phase 3 housing must engineer airflow: none needed for S3 (+1); mild vent/area planning for the bigger P4 board (0); deliberate thermal design for the Pi → −1.
11. **Mobile-rail weight and bulk** — the worn supply's mass: small protected cell (+1) vs pocket bank (0) vs a delivery-grade 45 W bank plus trigger board and heavy-rated cables (−1).
12. **Session runtime** — stored energy versus draw: S3's small cell with duty-cycled draw lasts long (+1); P4's bank dwarfs its gentle draw (+1); the Pi's very large bank against a heavy draw nets out ordinary (0).
13. **Power-rail complexity** — how many negotiated-contract/delicate-cable mechanisms sit between battery and chip: S3's protected-cell chain (+1), P4's plain 5 V bank feed (+1), the Pi's 5 V/5 A contract — which fails into throttling and needs a power-delivery trigger board and e-marked 5 A cables → −1.
14. **Recharge turnaround** — S3's small cell recharges quickly and cheaply to duplicate (+1); P4's swappable bank (0); the Pi's multi-hour 20,000 mAh bank recharge (−1).
15. **Camera capture architecture** — how many native camera ports the board has: S3 has one (−1) and P4 has one (−1) → both leave the multiplex/alternation decision to the tracking gate at bench test 7, while the Pi's two native lanes close the question in hardware (+1). This is the row where the P4's compute surplus is spent buying back a problem the S3 branch also carries.
16. **Camera price and availability** — OV2640 modules cost ~₱100–200 each everywhere (+1); SC2336-class is available with fewer stockists and a lens-variant caution (0); the official Camera Module 3 Wide is 3–4× the price with shaky distributor stock → −1.
17. **Field-of-view fit** — 160° modules on S3 (+1); the P4's pinned sensor class lists narrow lenses by default, wide variants needing checkout care (0); the Pi's official 120° wide camera is a clean fit (+1).
18. **Camera software maturity** — the S3's camera driver and detection examples are the most trodden path in the ecosystem (+1); the P4's sensor support is in-tree but newer (0); the Pi's camera stack is mature and officially maintained (+1).
19. **Audio path simplicity** — S3 and P4 both take the plan-native literal digital-audio-into-amplifier route into the mandatory wired jack (+1 each); the Pi branch's USB-dongle detour exists only because that board has no analog output (−1).
20. **Link implementation risk** — S3's ESP-NOW is native and abundant in examples (+1); the P4's answer — an on-board companion chip bridged by esp_hosted glue — is functional but is exactly the hop its own document flags for verification (−1); the Linux access-point pattern is standard, unremarkable (0).
21. **Link throughput headroom** — ESP-NOW suits today's small payloads but little more (0); both the Wi-Fi-6 companion (+1) and the full Linux network stack (+1) leave room to grow.
22. **Total on-head weight** — the wearer carries everything: S3 well under ~200 g (+1); P4 lands in the ~300 g class with its bank (0); R5 goes past ~450 g with its cooler and biggest bank (−1).
23. **Breadboard-friendliness** — the Phase 2 rule forbids soldering: the S3 devkit is all header rows (+1); the P4 evaluation board is connector-saturated (serial-camera and flat-flex interfaces dominate) → −1; the Pi's 40-pin header is workable with common cobbler adapters (0).
24. **Hand-solder carrier fit** — the Phase 3 build is outsourced to a local hand-solder service with carrier boards designed through-hole-dominant ([`assembly.md`](assembly.md) binds this): the S3 fits that bind perfectly (+1); on P4 and R5 the compute board itself is a connector-saturated module that mounts as a unit, and the flat-flex camera world sits at the edge of what a hand-solder shop wants — the harness work still applies to all three, but the carrier story is simplest on S3 (0, 0).
25. **Strap-form fit** — the resolved form factor is an elastic strap with printed mounts, not a helmet: a credit-card-plus devkit vanishes into a mount (+1); the evaluation board rides mid-strap, bulky but practical (0); an 85×56 mm board plus cooler plus the biggest bank is the least consistent with the form (−1).
26. **HRTF rendering capacity and quality-building headroom** — spatialized rendering is the product (`renderer`: generic HRTF azimuth, 48 kHz buffers under the ≤ 5 ms-per-buffer row), and the plan deliberately keeps the workload modest (elevation is carried by the carrier-pitch cue, not spectral HRTF — the plan's risk 3). All three run the planned HRTF; this variable scores what the chip can afford **beyond** that while sharing duty with vision: longer impulse responses and denser azimuth grids (the real levers on rendering quality), and eventually per-participant measured HRTF sets. S3: planned route fits inside the budget, but upgrade headroom while co-residing with detection is bounded, and measured-HRTF work is out of its class → 0. P4: vector instructions plus PSRAM tables afford longer filters and denser grids alongside CV duty (+1). R5: CPU to burn for research-grade binaural work (+1). This is the one row where branch choice most directly touches the study's perceptual outcomes (the localization metrics) — a genuine plus for the two high-compute branches even though the thesis plan specifies the modest, robust route; worth putting before the co-researcher at sign-off rather than burying.

## 4. Reading the result

- **Unweighted: ESP32-S3 +19, ESP32-P4 +8, Raspberry Pi 5 −9.** Gate-weighted (rows 5, 6, 7, 15 doubled): **S3 +18, P4 +10, R5 −9** — the ordering survives both weightings, so extra compute genuinely de-risks the tracking gate while everything else holds.
- The corrected scoring semantics sharpen the story: the S3 leads not because the P4 is bad — its co-residence, heat, runtime, audio, power-simplicity, and HRTF-headroom scores are all genuinely +1 — but because the S3 wins or ties on nearly everything while being the cheapest at the head and in the pocket. The P4's honest deficit list is short and specific: one camera port, the companion-chip link hop, flat-cable-heavy bench ergonomics, thin supply lanes. The Raspberry Pi 5's deficits — power, heat on the head, weight, boot/latency exposure, price — are structural to the computing-class, which is exactly what a head-mounted device cannot buy back with headroom.
- The matrix is a prior, not a verdict: the bench gates (render tests and the tracking co-residence test) are the referee on whatever board the co-researcher's sign-off picks. Prices were point-in-time (2026-09-26) and the ultra-wideband module's single-shop risk applies to all three equally, so it scores nowhere.

## 5. Provenance and upkeep

Derived read-only from the committed state of the three branch tips on 2026-09-26 (`esp32-s3` @ `b853b5b`, `raspi-5` @ `adade7f`, `esp32-p4` @ `490a7fc`, baseline `agnostic` @ `5b40cb2`); scores are judgments over those tips' committed purchase lists and README selections, not new facts. If a branch tip changes (prices re-verified, a part swapped), re-derive the affected rows from that tip and bump the date here; the hardware-open plan on `agnostic`/`master` is unaffected by this comparison either way.
