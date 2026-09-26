# Cane — Purchase list (branch esp32-s3, entire project)

> **The components and materials for the entire project — P2 bench, P3 build kit, P4/P5 phases — itemized for the esp32-s3 selections (research pass 2026-09-26; selection in [README §3.1–3.2](../README.md#31-pointer--electronics)).** Verify prices and stock at checkout; totals serve the co-researcher's purchase-approval sign-off (financial reasons, [README §9](../README.md#9-open-items)). Sourcing scope per decision 2026-09-26: **Philippine-local lanes only** — Shopee PH, Lazada PH, e-Gizmo (Mechatronix Central, Manila), Circuitrocks, Makerlab/Maker Selections-type local shops; no global parts houses. Quantities cover **both devices fully** (pointer + wearable share one parts pool). Tracking hardware included — it is part of the final design ([README §2.5](../README.md#25-pointer-tracking-stack-vision-uwb-imu)).

## 1. Scope rules

- **This list is the whole project's purchase list, not a bench order.** It itemizes every component, material, fixture, tool, and service the project consumes from P2 through P5 — nothing more, nothing missing: §2–§4 are the items bought at/before the bench phase that keep serving the final builds and study; §5 holds the phase-deferred one-time blocks (P3 build/service lanes, P5 hardware), labeled with their consuming phase.
- **No-soldering rule (purchasing constraint).** P2 attaches every component non-permanently and involves no soldering of any kind, so every module is ordered with **headers pre-soldered**; verify the listing before checkout. Unsoldered arrivals wait, unsoldered, for the P3 build.
- **Buy-once rule (purchasing constraint).** Each component category is purchased once — the selection is made *before* the purchase, never corrected by a re-buy afterwards ([README §3](../README.md#3-hardware)). Non-consumable categories (§5 tools) are also bought once; consumables are sized per phase.
- **Local-lane rule (decision 2026-09-26).** All rows are priced against Philippine-local shops and marketplaces; no offshore distributor accounts are opened for this build.
- Every row carries its consuming bench tests or phase, so any cut can be checked against the T0–T8 matrix and the phase structure before it is made.

## 2. Device electronics (ordered for the bench — headers pre-soldered, verify listing; serve P2 → P5)

| #   | Item                                                     | Listing keywords                           | Qty                              | Lane                                                                                     | Price point (₱) | Pre-soldered                                                                                                                    | Consumed by                                           | Notes                                                                                                                                                    |
| --- | -------------------------------------------------------- | ------------------------------------------ | -------------------------------- | ---------------------------------------------------------------------------------------- | --------------- | ------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1   | ESP32-S3-DevKitC-1 **N16R8** (16 MB flash, 8 MB PSRAM)   | "ESP32-S3-DevKitC-1 N16R8"                 | 2 (1 per device)                 | e-Gizmo (in stock, ₱499 ea. as of 2026-09-26); Shopee/Lazada alternates                  | 1,000 total     | yes — pre-headered devkit                                                                                                       | T0–T8 all; final builds                               | dual USB (native USB = debug/JTAG; no separate programmer); 240 MHz dual core + vector ext. wears the renderer + CV budget (risk-8/T7)                   |
| 2   | BNO085 breakout (GY-BNO085 class, I²C)                   | "GY-BNO085 BNO085 IMU"                     | 2 (1 per device)                 | Shopee/Lazada; e-Gizmo/Circuitrocks if stocked                                           | 700–1,100 ea.   | varies — prefer listing photos showing pre-soldered headers                                                                     | T1, T5, T6                                            | same part both devices ([README §3.7](../README.md#37-component-notes)); distinct fixed address from the ToF                                             |
| 3   | VL53L1X ToF with optical cover                           | "VL53L1X TOF 4m"                           | 1                                | Shopee/Lazada; Makerlab PH (TOF400C-class alternates carry the ≤ 4 m spec — check cover) | 150–350         | yes — XSHUT/INT headered                                                                                                        | T2, T5                                                | cover-glass version; 940 nm Class-1 beam = the "invisible laser" ([README §3.1](../README.md#31-pointer--electronics))                                   |
| 4   | MAX98357A I²S Class-D mono amp                           | "MAX98357A I2S amplifier"                  | 2 (1 per ear)                    | Shopee/Lazada                                                                            | 60–120 ea.      | yes — standard module                                                                                                           | T4, T5                                                | one per ear; L/R via the L/R-select pin ([README §3.2](../README.md#32-wearable--electronics))                                                           |
| 5   | OV2640 DVP camera **160° wide-FOV**, 24-pin              | "OV2640 160 degree wide angle 24pin ESP32" | 2 + 1 spare                      | Shopee; Maker Selections-type local stock                                                | 100–200 ea.     | FPC ribbon — plus breakout-to-header adapters so P2 stays solder-free                                                           | T7                                                    | I²C-addr strappable, verify before checkout; two cameras share the S3's single DVP port (alternation default; mux/co-processor still T7's call — risk 8) |
| 6   | DWM3000 UWB module (DW3110-based, ceramic antenna)       | "DWM3000 module DW3110"                    | 2 (tag + anchor)                 | Shopee/Lazada (Qorvo Decawave listings); Circuitrocks if stocked                         | 700–1,500 ea.   | module, castellated — solders onto the carrier header (never bare QFN, [README §3.9](../README.md#39-assembly--outsourcing-p3)) | T8                                                    | SPI + IRQ/reset, the interface the wiring maps resolve ([README §3.8](../README.md#38-pointer-tracking-hardware--notes--contingencies))                  |
| 7   | Momentary push buttons (through-hole, panel-mount class) | "push button momentary 12mm"               | 4 (trigger + re-zero + 2 spares) | Shopee/e-Gizmo                                                                           | 5–15 ea.        | leads for breadboard; P3 panel-caps                                                                                             | trigger (T5 onward), re-zero (T5), fit-check fixtures | one button per device in the design: pointer trigger, wearable re-zero ([README §3.1](../README.md#31-pointer--electronics), §3.2)                       |
| 8   | Wired stereo earphones, 3.5 mm (baseline pair)           | "wired earphones 3.5mm"                    | 2                                | local consumer lanes                                                                     | 100–250 ea.     | n/a                                                                                                                             | T4, T5; P4–P5 standardized pair                       | wired mandatory ([README §2.1](../README.md#21-devices)); one bench pair, one study pair kept identical                                                  |

## 3. Battery, power & charge path (per device; serves P2 → P5)

| # | Item | Listing keywords | Qty | Lane | Price point (₱) | Consumed by | Notes |
|---|---|---|---|---|---|---|---|
| 1 | USB-C charge/protect board (TP4056-C + protection class) | "TP4056 USB-C charge protect" | 2 | Shopee/Lazada | 25–60 ea. | T0 | one per device |
| 2 | 18650 Li-ion cell, protected | "18650 3.7V protected" | 2 | Shopee/Lazada | 120–180 ea. | T0 | wearable rear-station cell — doubles as counterweight ([README §3.3](../README.md#33-structural--mechanical)); the second keeps bench sessions rolling while one charges |
| 3 | 18650 holder (tabbed or spring) | "18650 holder" | 1 | Shopee | 20–40 | T0, P3 | rear-station mount; P2 uses holder + jumpers (solder-free) |
| 4 | Slim Li-ion pouch | "Li-ion 3.7V pouch 1000-2000mAh" | 1 | Shopee/Lazada | 150–250 | T0 | pointer cell (slim, for the shell); capacity class pinned by the T0 current measurement |
| 5 | Slide/toggle switch | "slide switch SPDT panel" | 2 | Shopee | 15–30 ea. | T0 | per-device power switch |
| 6 | 3.5 mm stereo jack (panel or breakout) | "3.5mm stereo jack breakout" | 1 | Shopee | 20–50 | T4, T5 | wired-earphones path ([README §2.1](../README.md#21-devices)) |
| 7 | USB-C wall charger, ≥ 2 A | "USB-C charger 2A" | 1 | local consumer lanes | 150–300 | T0 | charges both devices; power banks cover untethered tests |

## 4. Bench infrastructure (one-time; serves T0–T8 and the P3/P4 sessions)

| # | Item | Listing keywords | Qty | Lane | Price point (₱) | Consumed by | Notes |
|---|---|---|---|---|---|---|---|
| 1 | Solderless breadboard, 830 pt | "breadboard 830" | 2 | Shopee/e-Gizmo | 60–130 ea. | T0–T8 | one per device |
| 2 | Dupont jumper packs (M-M + M-F) | "dupont jumper wire pack" | 2 | Shopee | 80–150 ea. | T0–T8 | |
| 3 | USB power bank | "power bank 10000mAh" | 2 | local consumer lanes | 350–700 ea. | T0 onwards | untethered bench sessions ([bench-tests.md](bench-tests.md) §Bring-up safety) |
| 4 | USB-C data cable | "USB-C data cable" | 2 | Shopee | 60–120 ea. | bring-up, flashing | one per devkit |
| 5 | Hookup wire set (22–26 AWG) | "hookup wire kit" | 1 | Shopee/e-Gizmo | 100–200 | T0–T8 | harness prototypes |
| 6 | Spare header packs (2.54 mm) | "2.54 header pin pack" | 1–2 | Shopee | 30–80 ea. | wiring maps | module remounts / spare rows |
| 7 | Bench attachment kit: zip ties, velcro straps, double-sided foam tape | "zip ties velcro foam tape" | 1 | Shopee/hardware | 60–150 | T0–T8 | the no-solder fasteners of the P2 rule ([bench-tests.md](bench-tests.md) header) |

## 5. One-time blocks — P3 build kit, service lanes, P5 study hardware (phase labels in-row)

| # | Item | Qty | Lane | Price point (₱) | Consuming phase | Notes |
|---|---|---|---|---|---|---|
| 1 | Digital multimeter | 1 | local tool shops/Shopee | 350–700 | P2–P5 | T0 current measurements + incoming inspection ([README §3.4](../README.md#34-assembly--bench-tools-one-time)) |
| 2 | Minimal rework & inspection set — strippers/cutters/pliers, tweezers, adjustable iron + solder + flux + desoldering wick | 1 | e-Gizmo/Shopee | 700–1,200 | P3 rework only | full soldering kit dropped (outsourced build — [README §3.9](../README.md#39-assembly--outsourcing-p3)); helping-hands optional |
| 3 | Printed marker stock — matte high-contrast sticker/print sheets | 1 pack | Shopee/print shops | 50–150 | T7 (test tags) + P3 (the shell-mounted marker) | printed ArUco/AprilTag ([README §3.8](../README.md#38-pointer-tracking-hardware--notes--contingencies)); dictionary/size pinned at T7 |
| 4 | Harness materials kit — JST-XH/PH connectors + crimps, heat-shrink assortment, colored silicone wire (22/26 AWG) | 1 kit | Shopee/Makerlab PH | 400–800 | P3 (the service's handoff kit, [`assembly.md`](assembly.md) §3) | also spares any P4-phase harness repair; keying/colors per our wire schedule |
| 5 | Head-form fixture (T7 setup) | 1 | printed (own filament) or foam head | 0–150 | T7 | camera stations spaced like the strap stations ([bench-tests.md](bench-tests.md) T7) |
| 6 | Bare carrier PCBs, **with ≥ 2 spares/device** | ≥ 2/device (+2 spares) | PCB fab house | per quote | P3 ([README §3.9](../README.md#39-assembly--outsourcing-p3) lane 1) | order at the breadboard exit (~Oct 27) |
| 7 | Local hand-solder service fee | 2 builds | local service | per quote | P3 ([`assembly.md`](assembly.md) §4) | per-build quote, turnaround inside the P3 window |
| 8 | Filament (PLA/HTPLA) or print-service voucher | 1–2 kg / voucher | Shopee/local print service | 700–1,400 | P3 | pointer shell + wearable strap mounts ([README §3.3](../README.md#33-structural--mechanical)); own-printer vs service is the co-researcher's call |
| 9 | Elastic head strap + fasteners/adhesive + heat-set inserts | 1 set | Shopee/local sewing/hardware | 100–250 | P3 | goggle-style band + mount fastening |
| 10 | Evaluation hardware — blindfolds, floor marking tape, measuring tape, obstacle props | 1 set | Shopee/local hardware | 300–600 | P5 ([README §3.5](../README.md#35-evaluation-hardware-one-time)) | timing on the on-hand phone; props partly scavenged (course furniture) |

## 6. Checkout checklist

- [ ] Pre-soldered headers verified on every §2 module row from listing photos **before checkout** (rule §1; unsoldered arrivals wait unsoldered for P3).
- [ ] I²C addresses distinct: BNO085 (0x4a) vs VL53L1X (0x29) — they share the pointer's bus; addr straps left at defaults unless the wiring maps say otherwise.
- [ ] Both OV2640 modules: I²C address strap confirmed and *different* (the two cameras share the wearable's bus in alternation mode) — no-collision check before checkout.
- [ ] DWM3000 row = **module with integrated antenna** (DW3110-based), not a bare QFN chip.
- [ ] DevKitC-1 variant = **N16R8** (16 MB flash / 8 MB PSRAM) — the cheaper N8R2/N4-class clones miss the PSRAM budget.
- [ ] MAX98357A listings: L/R-select pin broken out (per-ear channel strapping needs it).
- [ ] 18650 cell = protected, name-brand; pouch cell = with PCM protection leads (charge board + cell both protected — T0's trip test).
- [ ] Per-quote rows (§5 rows 6–7) quoted **before** the purchase-approval sign-off.
- [ ] Sum the rows into the §7 envelope; record committed totals at [README §3.6](../README.md#36-totals) after sign-off.

## 7. Price envelope (point-in-time, branch esp32-s3, 2026-09-26)

| Block | Low (₱) | High (₱) |
|---|---|---|
| Device electronics (§2, incl. buttons + earphones) | ~4,500 | ~8,000 |
| Battery, power & audio path (§3) | ~650 | ~1,200 |
| Bench infrastructure (§4) | ~1,300 | ~2,800 |
| One-time blocks (§5, rows 1–5, 8–10: tools/kit/prints/P5 — **excluding the two per-quote rows**) | ~2,600 | ~5,350 |
| — bare carrier PCBs + spares (§5 row 6) | per quote | per quote |
| — local hand-solder service, 2 builds (§5 row 7) | per quote | per quote |
| **Total, entire project — fixed-price rows** | **~₱9,100** | **~₱17,350** |

Totals are a researched point-in-time envelope, not committed costs: Shopee prices fluctuate by seller/voucher; specialty-stock rows (DWM3000 especially) carry single-shop risk — verify stock before sign-off. The envelope covers the entire project: the bench order (P2), the P3 build kit and service lanes, and the P5 evaluation hardware. Final committed totals are computed from the co-researcher's approved order and recorded in [README §3.6](../README.md#36-totals).
