# Cane — Purchase list (the complete project cart: every component and material)

> This is the **complete itemized order** — every hardware component, material, tool, and fixture the project buys across the device electronics, the build, and the study. The selections and per-row rationale live in [`hardware.md`](hardware.md); this cart follows it row for row and shares its totals. Built on the 2026-10-01 decision: **wearable = Raspberry Pi 5, 4 GB** — a **single cart**, the board class decided, with the priced esp32-s3-class flip path kept in [`hardware.md`](hardware.md) §4 (₱0 in this cart until a tripwire forces it). The **bench-phase-only material slice** (breadboard infrastructure, fixtures, surface props) is itemized separately in [`bench.md`](bench.md) and is **not** counted below. Prices are Philippine-local ballparks, **verified at checkout**; the co-researcher's **purchase-approval sign-off** ([README §9](../README.md#9-open-items)) precedes any checkout.

## 1. Rules the cart binds itself to

- **Buy-once rule** — each component category is purchased once; the selection is fixed before purchase, never corrected by a re-buy. Substitute-flagged rows (e.g. the UWB module) are replacements chosen *at checkout time when a row is unstocked*, not re-buy decisions.
- **Connector-ready rule (pre-soldered, adapted)** — P2/P3 attach everything non-permanently or through keyed connectors; every module is ordered with headers/connectors **pre-fitted** (or consumer-ready where no header applies, e.g. the Pi kit, the power bank, earphones). Anything arriving as a fine-pitch bare IC or an unsoldered header board is returned or set aside — never made bench-solderable out of necessity.
- **Every row carries its consuming phase** — the *Consumed by* column maps each row to the phase that uses it (`P2/P3/P4/P5` per [README §6](../README.md#6-development-phases)); bench-only materials live in [`bench.md`](bench.md).
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

## 4. Wearable — the selected class rows: raspi-5, 4 GB (`P2/P3/P4/P5`)

| # | Item | Consumed by | Qty | Unit ₱ | Subtotal ₱ |
|---|---|---|---|---|---|
| 1 | **Raspberry Pi 5, 4 GB RAM** | all phases | 1 | 8,500–10,500 | 8,500–10,500 |
| 2 | Active cooler (official class) or heatsink | thermal | 1 | 500–900 | 500–900 |
| 3 | microSD A2-class, 64–128 GB | OS + logs | 1 | 600–1,000 | 600–1,000 |
| 4 | CSI camera, wide-angle 120°, RGB (IMX708 class) — **1 or 2 adopted at the T7 placement gate; two in the cart** (fixing the count at 1 before checkout saves ₱1,700–2,400 + one printed bracket) | vision tier | 2 | 1,700–2,400 | 3,400–4,800 |
| 5 | USB-C PD power bank ≥ 20,000 mAh, **5 V/3 A out** | worn power rail + bench source | 1 | 1,800–3,200 | 1,800–3,200 |
| | | | | **Subtotal** | **14,800–20,400** |
| *(opt.)* | Raspberry Pi 27 W USB-C PSU (bench supply if the bank is otherwise engaged) | bench | 0–1 | 1,000–1,500 | *(excluded)* |
| *(opt.)* | USB-UART dongle (CP2102-class) | Pi serial console | 0–1 | 150–300 | *(excluded)* |

## 5. Wearable — recorded alternative, esp32-s3-class flavor (`₱0 in this cart`)

Itemized in [`hardware.md`](hardware.md) §4 — kept in sync with that file; summarized here so the cart records the flip cost without duplicating the rows.

| Item | When bought | Indicative price if ever re-activated |
|---|---|---|
| esp32-s3-class devkit (N16R8) + 2× OV2640-class DVP 160° cameras (2 + spare) + FPC-to-header adapter sets | only if the class decision is revisited after a bring-up tripwire | ≈ 900–1,600 (base) |
| DVP camera-mux module (only if the T7 gate forces simultaneous two-camera capture) | same trigger, gate-dependent | +150–600 |
| Camera brackets + M–F leads for the DVP modules (print lane) | same trigger | +0–300 |
| Protected 18650 + cradle rail — only if the lighter head-worn rail is chosen over reusing the PD bank | same trigger | +300–600 (or ₱0 by reusing the bank) |
| *Stereo audio (the shared PCM5102A module), thermal (none needed), every board-agnostic row, and the entire pointer survive the flip at ₱0.* | — | — |
| **Worst-case cost of reversal** | | **≈ ₱2,800–4,100** |

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
| 2 | 3D printing lane — filament or print-service voucher (brackets, IMU station, bank cradle, pointer shell, head-form jig) | structural parts | 800–2,000 |
| 3 | Fastener set | mounts | 100–250 |
| 4 | Attachment kit (velcro, zip ties, foam tape) | stations + management | 250–500 |
| | | | **1,200–2,900** |

## 8. Evaluation hardware (`P5`)

| # | Item | Consumed by | Unit ₱ |
|---|---|---|---|
| 1 | Blindfolds ×3 (participant + spare + practice) | study | 100–250 |
| 2 | Floor marking tape | course | 120–300 |
| 3 | Measuring tape, 5 m (shared with the bench ground truth, [`bench.md`](bench.md) §4) | course + T2/T6/T8 | 120–250 |
| 4 | Obstacle props (as-needed, largely on-hand furniture) | course | 0–500 |
| | | | **340–1,300** |

## 9. Totals

| Section | Low ₱ | High ₱ |
|---|---|---|
| Pointer electronics | 2,615 | 5,430 |
| Wearable board-agnostic | 1,965 | 4,010 |
| Wearable — selected class (raspi-5, 4 GB) | 14,800 | 20,400 |
| **Device electronics** | **19,380** | **29,840** |
| One-time tools | 2,720 | 5,470 |
| Structural & materials | 1,200 | 2,900 |
| Evaluation hardware | 340 | 1,300 |
| **Cart grand total** | **23,640** | **39,510** |

**Indicative envelope ≈ ₱23,600–39,500 itemized; ≈ ₱27,200–45,400 with 15 % contingency guidance.** Optional rows are extra on top of the envelope. The bench-phase slice ([`bench.md`](bench.md), ≈ ₱1,600–3,500) is **not** included in these totals. Nothing is committed before the purchase-approval sign-off.

## 10. Checkout checklist

1. **Prices and stock re-verified the day of checkout** — every subtotal above is a point-in-time estimate.
2. **Connector-ready rule re-checked on each module page** (headers/connectors pre-fitted; no fine-pitch solderable-on-bench parts).
3. **PD bank profile** — output table must show **5 V at ≥ 3 A** on USB-C PD in its spec table; banks that fall back to 2 A underpower a Pi 5 with cameras.
4. **Cameras RGB wide-FOV on CSI-2** — visible-light, 120°-class, Raspberry Pi 5-compatible cable; confirm ×2 interchangeable.
5. **IMU pair from one listing** (same sensor and module revision); UWB pair likewise.
6. **DWM3000 stock check** — if out of stock everywhere, substitute a DW1000-class module family (Ai-Thinker BU01 / M5Stack UWB Unit) and flag it in the sign-off: interface-compatible (SPI + IRQ), different driver/channel plan at the tracking gate.
7. **Wired-only earphones** — Bluetooth/USB earphones are excluded by the audio rule regardless of cost.
8. **microSD A2-class** (random-write endurance for the flash-buffered logs).
9. If any price moves more than the envelope's headroom after re-verification, the changed rows go back through the sign-off lane before checkout — the buy-once rule permits buying once, not buying blind.
