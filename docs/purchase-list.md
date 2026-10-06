# Cane — Purchase list (the complete project cart: every component and material)

> This is the **complete cart** — every hardware component, material, tool, and fixture the project buys across the device electronics, the build, and the study, with each row marked **supplied or to buy**. The selections and per-row rationale live in [`hardware.md`](hardware.md); this cart follows it row for row and shares its totals.
>
> **Purchase state as of 2026-10-03:** the **wearable is a Raspberry Pi 5, 8 GB and the board is bought — ₱6,000** (recorded against the bare board; if that purchase later proves to have included the PSU or case, the value is re-split across those rows rather than left double-counted). **Every other row in this cart is still to buy** (≈ ₱15,800–31,300), and the purchase-approval sign-off ([README §9](../README.md#9-open-items)) precedes that remaining checkout. A **single cart**, the board class closed with **no alternative class carried** ([`hardware.md`](hardware.md) §4). With no hardware bench phase (2026-10-01), the former bench-phase-only slice (breadboard infrastructure, fixtures, surface props, ≈ ₱1,600–3,500) is **not bought**; it is recorded-but-retired in [`bench.md`](bench.md) §3, which also holds the bring-up and incoming-inspection checklist that this cart's tools, leads, and PD bank serve. It is **not** counted below.
>
> The 8 GB SKU required no re-derivation of anything: the resident workload was already shown to fit **4 GB**, so 8 GB is a strict superset and every budget, threshold, and evidence class stands ([README §3](../README.md#3-hardware) preamble). Remaining prices are indicative Philippine-local ballparks, verified when ordered. **§10** is the received-goods inspection that runs on the board and on every later delivery.

## 1. Rules the cart binds itself to

- **Buy-once rule (partly spent on 2026-10-03)** — the rule's purpose is *"the selection is fixed before purchase, never corrected by a re-buy,"* and for the compute board that is now **executed**: the Pi 5, 8 GB is bought and the class is closed. Rows still to buy are bought once on the same basis, and substitute-flagged rows (e.g. the UWB module) are replacements chosen *at order time when a row is unstocked*, not re-buy decisions. **Recorded consequence:** any spend on electronics **after** this purchase is a re-buy by definition and needs an explicit exception; and because only the board is bought, a part swapped later is a logged deviation rather than a covered re-buy ([README §3](../README.md#3-hardware) constraint 3).
- **Connector-ready rule (pre-soldered, adapted)** — with no bench phase the rule serves the P3 service build: every module is ordered with headers/connectors **pre-fitted** (or consumer-ready where no header applies, e.g. the Pi kit, the power bank, earphones). Anything arriving as a fine-pitch bare IC or an unsoldered header board is returned or set aside — nothing is hand-soldered in-house.
- **Every row carries its consuming phase** — the *Consumed by* column maps each row to the phase that uses it (`P2/P3/P4/P5` per [README §6](../README.md#6-development-phases)); the retired bench-only materials are recorded unbought in [`bench.md`](bench.md) §3.
- **Same-part rule** — the two 9-DoF IMUs and the two UWB modules come from one listing each (matching revisions).
- **P3-folded lanes** (carrier PCBs with ≥ 2 spares per device; local hand-solder service) are **quoted at P3 entry** and not itemized here — they join this sign-off lane when quoted ([hardware.md §9](hardware.md#9-p3-folded-lanes-quoted-at-p3-entry-folded-into-the-same-sign-off--not-cart-rows-here)).

## 2. Pointer electronics (`P2/P3/P4/P5`)

| # | Item | Consumed by | Qty | Unit ₱ | Subtotal ₱ |
|---|---|---|---|---|---|
| 1 | ESP32-S3-class devkit (16 MB flash / 8 MB PSRAM class) | all phases | 1 | 500–700 | 500–700 |
| 2 | BNO085 9-DoF IMU breakout (I²C) | all phases | 1 | 600–1,100 | 600–1,100 |
| 3 | VL53L1X-class ToF module (optical cover) | all phases | 1 | 300–600 | 300–600 |
| 4 | DWM3000-class UWB tag module | tracking tiers | 1 | 900–1,800 | 900–1,800 |
| 5 | Momentary push button | trigger | 1 | 15–50 | 15–50 |
| 6 | Slim Li-ion pouch 1S (1,500–2,000 mAh, protection leads) | battery | 1 | 180–350 | 180–350 |
| 7 | USB-C charge/protect board (TP4056-C class) | power | 1 | 40–100 | 40–100 |
| 8 | Slide/toggle switch | power | 1 | 30–80 | 30–80 |
| 9 | Printed ArUco/AprilTag marker (print-shop) | vision tier | 1 | 50–150 | 50–150 |
| | | | | **Subtotal** | **2,615–5,430** |

## 3. Wearable — board-agnostic rows (`P2/P3/P4/P5`)

These rows survive a class flip with no re-buy; the [selected class rows](#4-wearable--the-selected-class-rows-raspi-5-4-gb-p2p3p4p5) below are class-pinned to the decided board.

| # | Item | Consumed by | Qty | Unit ₱ | Subtotal ₱ |
|---|---|---|---|---|---|
| 1 | BNO085 9-DoF IMU breakout (same listing as the pointer's) | all phases | 1 | 600–1,100 | 600–1,100 |
| 2 | UWB anchor module (pair with the tag) | tracking tiers | 1 | 900–1,800 | 900–1,800 |
| 3 | Momentary push button | re-zero | 1 | 15–50 | 15–50 |
| 4 | 3.5 mm stereo jack breakout, female | audio terminus | 2 | 30–80 | 60–160 |
| 5 | PCM5102A stereo I²S DAC module (1 + 1 spare) | audio path | 2 | 120–250 | 240–500 |
| 6 | Wired stereo earphones, 3.5 mm (wired-mandatory) | audio | 1 | 150–400 | 150–400 |
| | | | | **Subtotal** | **1,965–4,010** |
| *(opt.)* | Headphone amp mini-board — add only if line-out drive proves weak | audio headroom | 0–1 | 80–200 | *(excluded)* |

## 4. Wearable — the selected class rows: raspi-5, 8 GB (`P2/P3/P4/P5`)

| # | Item | State | Consumed by | Qty | Unit ₱ | Subtotal ₱ |
|---|---|---|---|---|---|---|
| 1 | **Raspberry Pi 5, 8 GB RAM** | ✅ **supplied 2026-10-03** | all phases | 1 | **6,000** *(paid)* | **6,000** *(paid)* |
| 2 | Active cooler (official class) or heatsink | to buy | thermal | 1 | 500–900 | 500–900 |
| 3 | microSD A2-class, 64–128 GB | to buy | OS + logs | 1 | 600–1,000 | 600–1,000 |
| 4 | CSI camera, wide-angle 120°, RGB (IMX708 class) — **count 1 or 2, still undecided**; settled at P2 from the FOV-geometry and co-residence analysis. Two are carried so either build is possible, and because **neither is bought yet, adopting one camera costs ₱0** rather than stranding a part (but one camera forfeits the redundancy/occlusion coverage and pushes more updates to tier 2) | to buy | vision tier | 2 | 1,700–2,400 | 3,400–4,800 |
| 5 | USB-C PD power bank ≥ 20,000 mAh, **5 V/5 A (25 W+) out** | to buy | worn power rail + first-power-up source | 1 | 2,500–5,500 | 2,500–5,500 |
| | | | | | **Subtotal** | **13,000–18,200** |
| *(opt.)* | Raspberry Pi 27 W USB-C PSU (current-limited source for first power-up if the bank is otherwise engaged) | bring-up | 0–1 | 1,000–1,500 | *(excluded)* |
| *(opt.)* | USB-UART dongle (CP2102-class) | Pi serial console | 0–1 | 150–300 | *(excluded)* |

## 5. Board class closed — no alternative class carried

**There is no alternative wearable board class in this cart.** The Raspberry Pi 5, 8 GB (§4 row 1) is purchased, the class question is closed ([README §3](../README.md#3-hardware) constraint 4), and the esp32-s3-class flavor previously recorded here has been retired with the flip rule it served. Nothing in this cart is held "in reserve for a class change."

The scored comparison across the four candidate classes remains in [`hw-comparison.md`](hw-comparison.md) as the decision's dated research record — why this class was chosen, not a menu of alternatives. What absorbs a late compute or audio-path failure is now **feature-level re-scoping and the spare-PCB margin** rather than a different board: **[README §7](../README.md#7-risks-and-limitations) risk 12** — one camera with a wider FOV requirement, the detect-then-track cadence, or the USB-audio-dongle audio fallback, all purchase-free (the camera change ₱0 while neither module is bought), plus ≥ 2 spare carrier PCBs per device against a re-fab ([`assembly.md`](assembly.md) §4).

## 6. One-time tools (`both paths`; solder kit = P3 rework per the outsourcing decision)

| #        | Item                                                                                 | Consumed by                  | Unit ₱          |
| -------- | ------------------------------------------------------------------------------------ | ---------------------------- | --------------- |
| 1        | Digital multimeter (with current ranges — inline mA)                                 | bring-up + P3 inspection     | 800–1,600       |
| 2        | Adjustable soldering iron kit                                                        | P3/P4/P5 rework              | 800–1,500       |
| 3        | Solder wire 0.8 mm                                                                   | P3 rework                    | 120–250         |
| 4        | Flux                                                                                 | P3 rework                    | 80–150          |
| 5        | Desoldering wick                                                                     | P3 rework                    | 60–120          |
| 6        | Hand tool set (strippers/cutters/pliers)                                             | P2/P3 harness                | 330–650         |
| 7        | Tweezers                                                                             | P3 inspection                | 80–150          |
| 8        | USB-A→USB-C data cables ×3 (data-capable; 3rd for the Pi's power-while-logging case) | flashing/logging, both paths | 450–1,050       |
|          |                                                                                      |                              | **2,720–5,470** |
| *(opt.)* | Helping-hands/PCB holder                                                             | rework                       | *(excluded)*    |

## 7. Structural & materials (`P2–P5`)

| # | Item | Consumed by | Unit ₱ |
|---|---|---|---|
| 1 | Elastic head strap band (goggle-style, adjustable) | wearable carrier | 50–150 |
| 2 | 3D printing lane — filament or print-service voucher (brackets, IMU station, bank cradle, pointer shell, printed ArUco/AprilTag marker) | structural parts | 800–2,000 |
| 3 | Fastener set | mounts | 100–250 |
| 4 | Attachment kit (velcro, zip ties, foam tape) | stations + management | 250–500 |
| | | | **1,200–2,900** |

## 8. Evaluation hardware (`P5`)

| # | Item | Consumed by | Unit ₱ |
|---|---|---|---|
| 1 | Blindfolds ×3 (participant + spare + practice) | study | 100–250 |
| 2 | Floor marking tape | course | 120–300 |
| 3 | Measuring tape, 5 m (also serves the P4 ground truth, [`bench.md`](bench.md) §4) | course + P4 | 120–250 |
| 4 | Obstacle props (as-needed, largely on-hand furniture) | course | 0–500 |
| | | | **340–1,300** |

## 9. Totals

| Section | Low ₱ | High ₱ |
|---|---|---|
| Pointer electronics | 2,615 | 5,430 |
| Wearable board-agnostic | 1,965 | 4,010 |
| Wearable — selected class (raspi-5, 8 GB; board row is an actual) | 13,000 | 18,200 |
| **Device electronics** | **17,580** | **27,640** |
| One-time tools | 2,720 | 5,470 |
| Structural & materials | 1,200 | 2,900 |
| Evaluation hardware | 340 | 1,300 |
| **Cart grand total** | **21,840** | **37,310** |

**Envelope ≈ ₱21,800–37,300 itemized; ≈ ₱25,100–42,900 with 15 % contingency guidance.** Optional rows are extra on top of the envelope. The retired bench-phase slice ([`bench.md`](bench.md) §3, ≈ ₱1,600–3,500) is **not bought and not** included in these totals.

**Spent vs to buy (2026-10-03):** **₱6,000 spent** on the Raspberry Pi 5, 8 GB (§4 row 1); **≈ ₱15,800–31,300 still to buy** across every other row, pending the purchase-approval sign-off ([README §9](../README.md#9-open-items)). The board row is an **actual**, all others remain indicative estimates.

## 10. Received-goods inspection & order checklist

Two lanes, because only one row has been bought. **Lane A (already done): the compute board** — items A1–A2 below. **Lane B (every remaining delivery): the pre-order and on-receipt checks** — items B1–B9.

**Lane A — compute board (Raspberry Pi 5, 8 GB, received 2026-10-03, ₱6,000).**

- **A1. Bare board or kit?** recorded against the **bare board**. *Open:* if the purchase in fact included the 27 W PSU or a case, say so and the value is **re-split** across those cart rows (§4 optional rows) rather than left double-counted.
- **A2. Warranty provenance.** ₱6,000 is consistent with a **grey import carrying no manufacturer warranty**, so the dead-on-arrival lane is *"replace from a local seller,"* not an RMA claim — and it is a **logged deviation**, since the buy-once rule no longer shields the board ([README §3](../README.md#3-hardware) constraint 3, [§7](../README.md#7-risks-and-limitations) risk 12). Visually inspect the board on arrival: no bent PCB, no damaged connector, both CSI connectors and the 40-pin header present and straight.

**Lane B — the remaining cart (pre-order, then on receipt).**

1. **B1. Prices and stock re-verified on the order day** — every remaining subtotal is a point-in-time estimate.
2. **B2. Connector-ready rule re-checked on each module page** (headers/connectors pre-fitted or consumer-ready; no fine-pitch parts, nothing soldered in-house).
3. **B3. PD bank profile** — output table must show **5 V at 5 A (25 W+)** on USB-C PD. The Pi 5's recommended supply is 27 W / 5.1 V / 5.0 A; a 3 A bank is the minimum only, restricts downstream USB to 600 mA, and flags the current limit at boot — inadequate for the camera + UWB + Wi-Fi tracking load. Few ≥ 20 Ah banks do 5 A; if none qualifies, the recorded fallback is a protected-cell (18650-class) rail with a 5 V/5 A buck stage. *This is the single least-certain row in the cart.*
4. **B4. Camera cable — the sharpest procurement hazard.** Pi 5 uses the **mini 22-pin** CSI connector, not the 15-pin of earlier boards: confirm each module ships or is bought with a **Standard-Mini** cable (official Camera Module 3 does; third-party IMX708 often does not). Also settle the **camera count (1 or 2)** here — still undecided, and ₱0 to reduce while nothing is bought.
5. **B5. Cameras RGB wide-FOV on CSI-2** — visible-light, 120°-class, Raspberry Pi 5-compatible; confirm ×2 interchangeable.
6. **B6. IMU pair from one listing** (same sensor and module revision); UWB pair likewise.
7. **B7. DWM3000 stock check** — if out of stock everywhere, substitute a DW1000-class module family (Ai-Thinker BU01 / M5Stack UWB Unit) and flag it in the sign-off: interface-compatible (SPI + IRQ), different driver/channel plan at the tracking gate.
8. **B8. Wired-only earphones** — Bluetooth/USB earphones are excluded by the audio rule regardless of cost.
9. **B9. microSD A2-class** (random-write endurance for the flash-buffered logs).
10. **B10. If any price moves more than the envelope's headroom after re-verification, the changed rows go back through the sign-off lane before ordering** — and any spend beyond this cart is a **re-buy needing an explicit exception**, not a same-lane top-up.
