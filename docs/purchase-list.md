# Cane — Purchase list (branch esp32-p4, entire project)

> **The components and materials for the entire project — P2 bench, P3 build kit, P4/P5 phases — itemized for the esp32-p4 selections (research pass 2026-09-26; selection in [README §3.1–3.2](../README.md#31-pointer--electronics)).** The wearable is an **ESP32-P4** on the Function-EV-Board (onboard ESP32-C6 companion = the Wi-Fi AP; single MIPI-CSI port) with two SC2336-class MIPI cameras in alternation and a MAX98357A I²S audio pair; the pointer keeps the **ESP32-S3** kit; the link is **UDP over Wi-Fi via C6**. Verify prices and stock at checkout; totals serve the co-researcher's purchase-approval sign-off (financial reasons, [README §9](../README.md#9-open-items)). Sourcing scope per decision 2026-09-26: **Philippine-local lanes only** — Shopee PH, Lazada PH, e-Gizmo, Circuitrocks, Makerlab PH-type shops; no global parts houses. Quantities cover **both devices fully**. Tracking hardware included — part of the final design ([README §2.5](../README.md#25-pointer-tracking-stack-vision-uwb-imu)).

## 1. Scope rules

- **This list is the whole project's purchase list, not a bench order.** It itemizes every component, material, fixture, tool, and service the project consumes from P2 through P5 — §2–§4 are the items bought at/before the bench phase that keep serving the final builds and study; §5 holds the phase-deferred one-time blocks (P3 build/service lanes, P5 hardware), labeled with their consuming phase.
- **No-soldering rule (purchasing constraint).** P2 attaches every component non-permanently and involves no soldering of any kind, so every module is ordered with **headers pre-soldered**; verify the listing before checkout. Unsoldered arrivals wait, unsoldered, for the P3 build (the Function-EV-Board needs no assembly; its MIPI cameras connect by FPC; the breakout rows carry their own headers).
- **Buy-once rule (purchasing constraint).** Each component category is purchased once — the selection is made *before* the purchase, never corrected by a re-buy afterwards ([README §3](../README.md#3-hardware)). Non-consumables (§5 tools) bought once; consumables sized per phase.
- **Local-lane rule (decision 2026-09-26).** All rows are priced against Philippine-local shops and marketplaces; no offshore distributor accounts are opened for this build.
- Every row carries its consuming bench tests or phase, so any cut can be checked against the T0–T8 matrix and the phase structure before it is made.

## 2. Device electronics (ordered for the bench — headers pre-soldered where applicable; serve P2 → P5)

### 2a. Pointer kit (ESP32-S3, carried-over rows — identical across all three branches)

| # | Item | Listing keywords | Qty | Lane | Price point (₱) | Pre-soldered | Consumed by | Notes |
|---|---|---|---|---|---|---|---|---|
| 1 | ESP32-S3-DevKitC-1 **N16R8** | "ESP32-S3-DevKitC-1 N16R8" | 1 | e-Gizmo (₱499 seen 2026-09-26); Shopee/Lazada alternates | ~500 | yes — pre-headered | all pointer rows; T3 station side | dual USB (native USB = debug); joins the wearable's C6 AP as station — UDP link ([README §2.1](../README.md#21-devices)) |
| 2 | BNO085 breakout (GY-BNO085 class, I²C) | "GY-BNO085 BNO085 IMU" | 2 (pointer + wearable — same part both) | Shopee/Lazada; e-Gizmo/Circuitrocks if stocked | 700–1,100 ea. | prefer listing photos with pre-soldered headers | T1, T5, T6 | same-part pairing ([README §3.7](../README.md#37-component-notes)); distinct fixed address from ToF |
| 3 | VL53L1X ToF with optical cover | "VL53L1X TOF 4m" | 1 | Shopee/Lazada; Makerlab PH (TOF400C-class alternates — check cover) | 150–350 | yes — XSHUT/INT headered | T2, T5 | cover-glass version; 940 nm Class-1 beam = the "invisible laser" ([README §3.1](../README.md#31-pointer--electronics)) |
| 4 | DWM3000 UWB module (DW3110-based, ceramic antenna) | "DWM3000 module DW3110" | 2 (tag + anchor) | Shopee/Lazada (Qorvo Decawave listings); Circuitrocks if stocked | 700–1,500 ea. | module, castellated — solders onto the carrier header (never bare QFN, [README §3.9](../README.md#39-assembly--outsourcing-p3)) | T8 | SPI + IRQ/reset; wearable side rides the P4's SPI via the board headers |
| 5 | Momentary push buttons | "push button momentary 12mm" | 4 (trigger + re-zero + 2 spares) | Shopee/e-Gizmo | 5–15 ea. | leads for breadboard; P3 panel-caps | trigger (T5+), re-zero (T5) | pointer trigger, wearable re-zero |
| 6 | Wired stereo earphones, 3.5 mm (baseline pair) | "wired earphones 3.5mm" | 2 | local consumer lanes | 100–250 ea. | n/a | T4, T5; P4–P5 standardized pair | wired mandatory ([README §2.1](../README.md#21-devices)) |

### 2b. Wearable kit (ESP32-P4)

| # | Item | Listing keywords | Qty | Lane | Price point (₱) | Pre-soldered | Consumed by | Notes |
|---|---|---|---|---|---|---|---|---|
| 1 | **ESP32-P4-Function-EV-Board** (ESP32-P4 + ESP32-C6-MINI-1 onboard) | "ESP32-P4-Function-EV-Board" | 1 | Shopee/Lazada (listed 2026-09-26); Circuitrocks/Makerlab if stocked; Espressif official via local import lanes if local stock is dry | 2,000–3,200 | n/a — connector board, no soldering needed for this build | T0–T8 wearable side | dual-core 400 MHz RISC-V, 32 MB PSRAM class; **C6 companion onboard (the branch's AP)**; MIPI-CSI single port; USB 2.0 host/device; 5 V/2 A-class |
| 2 | **SC2336-class MIPI camera module, 24-pin FPC** (esp_cam_sensor-supported, esp32-p4 functional) | "SC2336 MIPI camera module 24pin" | 2 + 1 spare | Shopee/Lazada (SC2336 24-PIN MIPI listings confirmed local; wide-angle variant preferred — check listing lens variant) | 300–900 ea. | n/a — FPC to the board's CSI connector; FPC cables pre-terminated | T7 | 1/3" CMOS 2 MP; **MIPI-CSI only on P4** — DVP (OV2640) rows from the S3 branch do not exist here; **both cameras share the single CSI port in alternation** — mux/alternate-capture still T7's call (risk 8 ladder) |
| 3 | FPC/CSI cable spares, 24-pin, 100–300 mm | "24pin FPC cable camera" | 2 + 2 spares | Shopee | 25–80 ea. | n/a | T7, P3 | the Function-EV-Board's included FPC is bench-length; strap-mounted cameras need longer runs whose length is determined at P3; spares protect against FPC tears |
| 4 | MAX98357A I²S Class-D mono amp | "MAX98357A I2S amplifier" | 2 (1 per ear) | Shopee/Lazada | 60–120 ea. | yes — standard module | T4, T5 | the branch-identical audio path — I²S is literal on the P4 (no USB-audio detour like the RPi 5); L/R via L/R-select pin ([README §3.2](../README.md#32-wearable--electronics)) |
| 5 | 3.5 mm stereo jack (panel or breakout) | "3.5mm stereo jack breakout" | 1 | Shopee | 20–50 | n/a | T4, T5 | wired-earphones endpoint (mandatory) |

Pointer rows 1–6 and wearable rows 1–5 constitute the two devices' electronics.

## 3. Battery, power & charge path (serves P2 → P5)

| # | Item | Listing keywords | Qty | Lane | Price point (₱) | Consumed by | Notes |
|---|---|---|---|---|---|---|---|
| 1 | USB power bank for the wearable, 5 V/≥ 2 A out, ≥ 10 Ah | "power bank 10000mAh compact" | 1 (spare at P4 optional) | Shopee/local consumer lanes | 400–900 | T0 | the Function-EV-Board is 5 V/2 A-class — far lighter duty than the RPi 5 branch's 45 W PD rail; compact pocket form, rear-station role ([README §3.3](../README.md#33-structural--mechanical)) |
| 2 | TP4056-C USB-C charge/protect board | "TP4056 USB-C charge protect" | 1 | Shopee/Lazada | 25–60 | T0 | **pointer only** (S3 + pouch path, branch-identical) |
| 3 | Slim Li-ion pouch | "Li-ion 3.7V pouch 1000-2000mAh" | 1 | Shopee/Lazada | 150–250 | T0 | pointer cell (slim, shell fit); capacity pinned by T0 current measurement |
| 4 | Slide/toggle switch | "slide switch SPDT panel" | 1 | Shopee | 15–30 | T0 | pointer power switch (the wearable is switched by its power bank) |
| 5 | USB-C wall charger, ≥ 2 A | "USB-C charger 2A" | 1 | local consumer lanes | 150–300 | T0, charging lane | recharges both devices' supplies |

## 4. Bench infrastructure (one-time; serves T0–T8 and P3/P4 sessions)

| # | Item | Listing keywords | Qty | Lane | Price point (₱) | Consumed by | Notes |
|---|---|---|---|---|---|---|---|
| 1 | Solderless breadboard, 830 pt | "breadboard 830" | 2 | Shopee/e-Gizmo | 60–130 ea. | T0–T8 | one per device |
| 2 | Dupont jumper packs (M-M + M-F) | "dupont jumper wire pack" | 2 | Shopee | 80–150 ea. | T0–T8 | |
| 3 | USB-C data cable | "USB-C data cable" | 2 | Shopee | 60–120 ea. | flashing, T3 | one for the S3 pointer, one for the P4 |
| 4 | Hookup wire set (22–26 AWG) | "hookup wire kit" | 1 | Shopee/e-Gizmo | 100–200 | T0–T8 | |
| 5 | Spare header packs (2.54 mm) | "2.54 header pin pack" | 1–2 | Shopee | 30–80 | wiring maps | module remounts |
| 6 | Bench attachment kit — zip ties, velcro straps, foam tape | "zip ties velcro foam tape" | 1 | Shopee/hardware | 60–150 | T0–T8 | the no-solder fasteners of the P2 rule ([bench-tests.md](bench-tests.md)) |

## 5. One-time blocks — P3 build kit, service lanes, P5 study hardware (phase labels in-row)

| # | Item | Qty | Lane | Price point (₱) | Consuming phase | Notes |
|---|---|---|---|---|---|---|
| 1 | Digital multimeter | 1 | local tool shops/Shopee | 350–700 | P2–P5 | T0 + incoming inspection ([README §3.4](../README.md#34-assembly--bench-tools-one-time)) |
| 2 | Minimal rework & inspection set — strippers/cutters/pliers, tweezers, adjustable iron + solder + flux + desoldering wick | 1 | e-Gizmo/Shopee | 700–1,200 | P3 rework only | full soldering kit dropped (outsourced build — [README §3.9](../README.md#39-assembly--outsourcing-p3)) |
| 3 | Printed marker stock — matte high-contrast sticker/print sheets | 1 pack | Shopee/print shops | 50–150 | T7 (test tags) + P3 (shell marker) | printed ArUco/AprilTag ([README §3.8](../README.md#38-pointer-tracking-hardware--notes--contingencies)) |
| 4 | Harness materials kit — JST-XH/PH connectors + crimps, heat-shrink, colored silicone wire (22/26 AWG) | 1 kit | Shopee/Makerlab PH | 400–800 | P3 ([`assembly.md`](assembly.md) §3) | handoff kit wires + P4-phase spares |
| 5 | Head-form fixture (T7 setup) | 1 | printed or foam head | 0–150 | T7 | camera stations like the strap spacing ([bench-tests.md](bench-tests.md) T7) |
| 6 | Bare carrier PCBs, **with ≥ 2 spares/device** | ≥ 2/device (+2 spares) | PCB fab house | per quote | P3 ([README §3.9](../README.md#39-assembly--outsourcing-p3) lane 1) | order at the breadboard exit (~Oct 27) |
| 7 | Local hand-solder service fee | 2 builds | local service | per quote | P3 ([`assembly.md`](assembly.md) §4) | per-build quote, turnaround in-window |
| 8 | Filament (PLA/HTPLA) or print-service voucher | 1–2 kg / voucher | Shopee/local print service | 700–1,400 | P3 | pointer shell + wearable strap mounts ([README §3.3](../README.md#33-structural--mechanical)) |
| 9 | Elastic head strap + fasteners/adhesive + heat-set inserts | 1 set | Shopee/local sewing/hardware | 100–250 | P3 | goggle-style band + mounts |
| 10 | Evaluation hardware — blindfolds, floor marking tape, measuring tape, obstacle props | 1 set | Shopee/local hardware | 300–600 | P5 ([README §3.5](../README.md#35-evaluation-hardware-one-time)) | timing on the on-hand phone |

## 6. Checkout checklist

- [ ] Pointer rows — all pre-soldered-header rules apply (§2a); unsoldered arrivals wait unsoldered for P3.
- [ ] **P4 board = ESP32-P4-Function-EV-Board variant that includes the ESP32-C6-MINI-1 companion module** — listings with only the P4 chip (boardless cores or P4-Module-only) miss the branch's radio row; the C6 is the AP, not optional.
- [ ] **SC2336-class MIPI modules: esp_cam_sensor-listed variant** (fixed-focus 24-pin FPC); check the listing's lens marking — narrow-law ("60°/70°") listings are functional at T7 but under-cover the forward hemisphere; wide-angle variants preferred, otherwise the lens swap is a P3 fit issue.
- [ ] **24-pin FPC cables ordered to the board's connector pitch** (the Function-EV-Board's CSI connector keying); spares included.
- [ ] MAX98357A listings: L/R-select pin broken out (per-ear channel strapping needs it).
- [ ] Power bank: 5 V/≥ 2 A sustained output, not peak-only; compact form for the rear station.
- [ ] DWM3000 = module with integrated antenna (DW3110-based), not a bare QFN chip.
- [ ] BNO085 vs VL53L1X addresses distinct (0x4a / 0x29 — shared pointer bus, no collision).
- [ ] The onboard codec/amp and MIPI-DSI display hardware of the Function-EV-Board are **deliberately unused** — do not buy extra boards to "use the onboard audio."
- [ ] Per-quote rows (§5 rows 6–7) quoted **before** the sign-off.
- [ ] Sum rows into the §7 envelope; committed totals land at [README §3.6](../README.md#36-totals) post-sign-off.

## 7. Price envelope (point-in-time, branch esp32-p4, 2026-09-26)

| Block | Low (₱) | High (₱) |
|---|---|---|
| Pointer kit (§2a: S3 devkit + BNO085 ×2 + VL53L1X + DWM3000 ×2 + buttons + earphones) | ~3,700 | ~6,600 |
| Wearable kit (§2b: Function-EV-Board + SC2336 MIPI cams ×2 + FPC spares + MAX98357A ×2 + jack) | ~4,300 | ~6,700 |
| Battery & power/charge path (§3) | ~600 | ~1,300 |
| Bench infrastructure (§4) | ~600 | ~1,300 |
| One-time blocks (§5, rows 1–5, 8–10 — **excluding the two per-quote rows**) | ~2,600 | ~4,550 |
| — bare carrier PCBs + spares (§5 row 6) | per quote | per quote |
| — local hand-solder service, 2 builds (§5 row 7) | per quote | per quote |
| **Total, entire project — fixed-price rows** | **~₱11,800** | **~₱20,450** |

Totals are a researched point-in-time envelope, not committed costs: Shopee/Lazada prices fluctuate by seller/voucher; the Function-EV-Board and SC2336-class MIPI modules have fewer PH-local stock lanes than the mainstream boards (single-shop risk — check stock before sign-off, and the official-PCB import lane is the fallback). The envelope covers the entire project: the bench order (P2), the P3 build kit and service lanes, and the P5 evaluation hardware. Final committed totals are computed from the co-researcher's approved order and recorded in [README §3.6](../README.md#36-totals).
