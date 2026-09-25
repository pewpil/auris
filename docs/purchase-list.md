# Cane — Purchase list (full-project order)

> **Selected 2026-09-25 by the co-researcher** — the itemized order below covers the **entire thesis project** (instructed 2026-09-25): the two devices' electronics, the pointer-tracking hardware, the battery/power path, the bench infrastructure, the one-time tools, the P3 build materials, and the P5 evaluation hardware, priced in **Philippine peso**. Prices are local-retail estimates (Lazada/Shopee/OL-class listings, Sept 2026, ₱57 ≈ USD 1, ±20% typical spread); checkout-verified figures supersede them ([README §3.6](../README.md#36-totals)). Every row carries the bench tests that consume it ([`bench-tests.md`](bench-tests.md)); P3-only and study-only rows are flagged by phase.

## 1. Scope and the no-soldering rule

- **No-soldering rule (purchasing constraint).** P2 attaches every component non-permanently and involves no soldering of any kind, so every module must be ordered with **headers pre-soldered** — including the Raspberry Pi 5 (official SKU with pre-soldered 40-pin header); verify the listing before checkout. Unsoldered arrivals are set aside for the P3 build, never soldered during P2.
- **Buy-once rule (purchasing constraint).** Each component category is purchased once — the selection is made *before* the purchase, never corrected by a re-buy afterwards ([README §3](../README.md#3-hardware)).
- The Raspberry Pi 5's compute selection consumes the capture-strategy question (dual native CSI — no camera mux is purchased) and resolves the link medium to Wi-Fi UDP (no link hardware is purchased) ([README §3.7](../README.md#37-component-notes)).

## 2. A — Device electronics: pointer (P2)

| Item | Spec | Qty | Unit ₱ | Subtotal ₱ | Consumed by |
|---|---|---|---|---|---|
| ESP32-S3 dev board | DevKitC-1-class, 8 MB flash, pre-soldered headers | 1 | 650 | 650 | T1, T2, T3, T5, T6, T8 |
| BNO085 IMU module | 9-DoF, I²C 0x4A, pre-soldered (same part as wearable's) | 1 | 1,100 | 1,100 | T1 |
| VL53L1X ToF module | room-scale ~4 m class, I²C 0x29, pre-soldered | 1 | 550 | 550 | T2 |
| Trigger button | panel tactile + cap | 1 | 25 | 25 | T2, T5 |
| TP4056-C module | USB-C charge + DW01/FS8205-class protection | 1 | 40 | 40 | T0 |
| 18650 Li-ion cell | 3.4 Ah protected-class | 1 | 300 | 300 | T0 |
| 18650 holder | 1-cell, terminal type | 1 | 35 | 35 | T0 |
| DW3000 UWB module | SPI + IRQ/reset, pre-soldered (tag side) | 1 | 1,000 | 1,000 | T8 |

**A subtotal = ₱3,700**

## 3. B — Device electronics: wearable (P2)

| Item | Spec | Qty | Unit ₱ | Subtotal ₱ | Consumed by |
|---|---|---|---|---|---|
| Raspberry Pi 5 (4 GB) | official SKU, **pre-soldered 40-pin header** | 1 | 4,100 | 4,100 | T0, T1, T4, T5, T7, T8 |
| BNO085 IMU module | 9-DoF, I²C (same part as pointer's) | 1 | 1,100 | 1,100 | T1 |
| MAX98357A amp module | I²S Class-D mono, L/R channel strap | 2 | 200 | 400 | T4, T5 |
| 3.5 mm TRS jack | panel/PCB, stereo | 1 | 30 | 30 | T4, T5 |
| TP4056-C module | charge + protection (bench front-end) | 1 | 40 | 40 | T0 |
| 18650 Li-ion cells | 3.4 Ah ×2 (2S pack) | 2 | 300 | 600 | T0 |
| 18650 holders | 1-cell ×2 (2S assembly) | 2 | 35 | 70 | T0 |
| 5 V/5 A buck module | 7.4 V → 5 V, 5 A-class | 1 | 250 | 250 | T0 |
| 2S balance/protection board | 2S charge + protection | 1 | 180 | 180 | T0 |
| Re-zero button | panel tactile + cap | 1 | 25 | 25 | T5 |

**B subtotal = ₱6,795**

## 4. C — Pointer-tracking hardware (P2)

| Item | Spec | Qty | Unit ₱ | Subtotal ₱ | Consumed by |
|---|---|---|---|---|---|
| Camera Module 3 Wide | 12 MP, ~120° DFOV, native CSI (Pi 5 CAM + CAM0) | 2 | 1,850 | 3,700 | T7 |
| DW3000 UWB module | SPI + IRQ/reset, pre-soldered (anchor side) | 1 | 1,000 | 1,000 | T8 |
| Head-form fixture materials | foam head/stand + friction mounts (velcro, tape, zip ties) | 1 | 150 | 150 | T7 |

**C subtotal = ₱4,850**

## 5. D — Bench infrastructure (P2)

| Item | Spec | Qty | Unit ₱ | Subtotal ₱ | Consumed by |
|---|---|---|---|---|---|
| Breadboards | full-size | 3 | 110 | 330 | T0–T8 |
| Dupont jumper sets | M-M, M-F, F-F | 3 | 120 | 360 | T0–T8 |
| MicroSD card | 32 GB, A1-class (Pi OS image) | 1 | 450 | 450 | all Pi sessions |
| MicroSD reader | USB | 1 | 150 | 150 | image provisioning |
| USB data cables | USB-C + micro-USB, data-capable | 3 | 85 | 255 | flash/debug |
| 65 W USB-PD adapter | bench USB for the Pi side | 1 | 1,500 | 1,500 | T0 soak |
| USB V/I tester | inline (T0 logging support) | 1 | 400 | 400 | T0 |
| Velcro / zip ties / foam tape | friction-mount kit | 1 | 350 | 350 | T0–T8 |
| FFC cables (spare) | 22-pin, 200 mm + 300 mm | 2 | 100 | 200 | T7 (spares) |
| Lux meter | budget digital | 1 | 650 | 650 | T7(d) low-light check |

**D subtotal = ₱4,645**

## 6. E — One-time tools (P2 bench + P3 build)

| Item                             | Spec                        | Qty | Unit ₱ | Subtotal ₱ | Phase |
| -------------------------------- | --------------------------- | --- | ------ | ---------- | ----- |
| Digital multimeter               | inline current/voltage (T0) | 1   | 850    | 850        | P2    |
| Pliers / strippers / cutters set | basic 3-piece               | 1   | 450    | 450        | P2    |
| Soldering iron kit               | adjustable temp + tips      | 1   | 1,400  | 1,400      | P3    |
| Helping-hands / PCB holder       | —                           | 1   | 350    | 350        | P3    |
| Desoldering wick + flux          | —                           | 1   | 220    | 220        | P3    |
| Solder spool                     | 60/40, 0.6 mm               | 1   | 180    | 180        | P3    |

**E subtotal = ₱3,450**

## 7. F — Build materials (P3)

| Item | Spec | Qty | Unit ₱ | Subtotal ₱ | Consumed by |
|---|---|---|---|---|---|
| Print service | pointer shell + wearable camera brackets, IMU mount, battery cradle, printed ArUco/AprilTag markers, spares | 1 | 1,200 | 1,200 | P3 housing |
| Elastic head strap | goggle-style band | 1 | 250 | 250 | P3 strap |
| Fasteners | M2/M3 screws + nyloc + washers | 1 | 250 | 250 | P3 mounts |
| Adhesive | double-sided foam tape + epoxy putty | 1 | 180 | 180 | P3 mounting |

**F subtotal = ₱1,880**

## 8. G — Evaluation hardware (P5)

| Item | Spec | Qty | Unit ₱ | Subtotal ₱ | Consumed by |
|---|---|---|---|---|---|
| Blindfolds | — | 2 | 100 | 200 | P5 study |
| Floor marking tape + measuring tape | course layout + ranging ground truth | 1 | 350 | 350 | P5 study |
| Obstacle props | tables/chairs on hand + misc furnishing | 1 | 800 | 800 | P5 course |
| Timing | on-hand phone/smartwatch | — | 0 | 0 | P5 study |

**G subtotal = ₱1,350**

## 9. Totals

| Block | Subtotal ₱ |
|---|---|
| A — Pointer electronics (P2) | 3,700 |
| B — Wearable electronics (P2) | 6,795 |
| C — Tracking hardware (P2) | 4,850 |
| D — Bench infrastructure (P2) | 4,645 |
| **P2 bench order** | **19,990** |
| E — One-time tools (P2/P3) | 3,450 |
| F — Build materials (P3) | 1,880 |
| G — Evaluation hardware (P5) | 1,350 |
| **Grand total** | **26,670** |
| **+15% contingency** | **≈ 30,670** |

The contingency prices the buy-once rule (a board-class miss at T7/T8 is absorbed by design, not re-buy) and the ±20% local spread.

## 10. Checkout checklist

1. Every module listing verified **headers pre-soldered** — including the Pi 5's 40-pin header SKU.
2. Buy-once check: each category ordered once; no duplicate categories.
3. Camera pair confirmed **Camera Module 3 Wide** with FFC cables (spares in D).
4. 2S power path confirmed (2 cells + balance/protection + 5 V/5 A buck) before the T0 battery test is scheduled.
5. Verified local prices supersede the estimates above; totals re-recorded in [`README §3.6`](../README.md#36-totals) at checkout.
