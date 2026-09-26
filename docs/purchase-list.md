# Cane — Purchase list (branch raspi-4, entire project)

> **The components and materials for the entire project — P2 bench, P3 build kit, P4/P5 phases — itemized for the raspi-4 selections (research pass 2026-09-26; selection in [README §3.1–3.2](../README.md#31-pointer--electronics)).** The wearable is a **Raspberry Pi 4 Model B (4 GB)** with two IMX219 160° MIPI cameras alternating on its single 15-pin CSI port, USB-audio output, and a simple 5 V/3 A power rail; the pointer keeps the **ESP32-S3** kit; the link is **UDP over Wi-Fi** (Pi 4 hosts the AP). Verify prices and stock at checkout; totals serve the co-researcher's purchase-approval sign-off (financial reasons, [README §9](../README.md#9-open-items)). Sourcing scope per decision 2026-09-26: **Philippine-local lanes only** — Shopee PH, Lazada PH, e-Gizmo, Circuitrocks, Makerlab PH-type shops; no global parts houses. Quantities cover **both devices fully**. Tracking hardware included — part of the final design ([README §2.5](../README.md#25-pointer-tracking-stack-vision-uwb-imu)).

## 1. Scope rules

- **This list is the whole project's purchase list, not a bench order.** It itemizes every component, material, fixture, tool, and service the project consumes from P2 through P5 — §2–§4 are the items bought at/before the bench phase that keep serving the final builds and study; §5 holds the phase-deferred one-time blocks (P3 build/service lanes, P5 hardware), labeled with their consuming phase.
- **No-soldering rule (purchasing constraint).** P2 attaches every component non-permanently and involves no soldering of any kind, so every module is ordered with **headers pre-soldered**; verify the listing before checkout. Unsoldered arrivals wait, unsoldered, for the P3 build (the Pi 4 needs no assembly; its cameras connect by ribbon cable; the pointer rows are all headered modules).
- **Buy-once rule (purchasing constraint).** Each component category is purchased once — the selection is made *before* the purchase, never corrected by a re-buy afterwards ([README §3](../README.md#3-hardware)). Non-consumables (§5 tools) bought once; consumables sized per phase.
- **Local-lane rule (decision 2026-09-26).** All rows are priced against Philippine-local shops and marketplaces; no offshore distributor accounts are opened for this build.
- Every row carries its consuming bench tests or phase, so any cut can be checked against the T0–T8 matrix and the phase structure before it is made.

## 2. Device electronics (ordered for the bench — headers pre-soldered where applicable; serve P2 → P5)

### 2a. Pointer kit (ESP32-S3, carried-over rows — identical across all four branches)

| # | Item | Listing keywords | Qty | Lane | Price point (₱) | Pre-soldered | Consumed by | Notes |
|---|---|---|---|---|---|---|---|---|
| 1 | ESP32-S3-DevKitC-1 **N16R8** | "ESP32-S3-DevKitC-1 N16R8" | 1 | e-Gizmo (₱499 seen 2026-09-26); Shopee/Lazada alternates | ~500 | yes — pre-headered | all pointer rows; T3 station side | dual USB (native USB = debug); joins the Pi 4's Wi-Fi AP as station — UDP link ([README §2.1](../README.md#21-devices)) |
| 2 | BNO085 breakout (GY-BNO085 class, I²C) | "GY-BNO085 BNO085 IMU" | 2 (pointer + wearable — same part both) | Shopee/Lazada; e-Gizmo/Circuitrocks if stocked | 700–1,100 ea. | prefer listing photos with pre-soldered headers | T1, T5, T6 | same-part pairing ([README §3.7](../README.md#37-component-notes)); distinct fixed address from ToF |
| 3 | VL53L1X ToF with optical cover | "VL53L1X TOF 4m" | 1 | Shopee/Lazada; Makerlab PH (TOF400C-class alternates — check cover) | 150–350 | yes — XSHUT/INT headered | T2, T5 | cover-glass version; 940 nm Class-1 beam = the "invisible laser" ([README §3.1](../README.md#31-pointer--electronics)) |
| 4 | DWM3000 UWB module (DW3110-based, ceramic antenna) | "DWM3000 module DW3110" | 2 (tag + anchor) | Shopee/Lazada (Qorvo Decawave listings); Circuitrocks if stocked | 700–1,500 ea. | module, castellated — solders onto the carrier header (never bare QFN, [README §3.9](../README.md#39-assembly--outsourcing-p3)) | T8 | SPI + IRQ/reset; wearable side rides the Pi 4's SPI bus via the GPIO header |
| 5 | Momentary push buttons | "push button momentary 12mm" | 4 (trigger + re-zero + 2 spares) | Shopee/e-Gizmo | 5–15 ea. | leads for breadboard; P3 panel-caps | trigger (T5+), re-zero (T5) | pointer trigger, wearable re-zero |
| 6 | Wired stereo earphones, 3.5 mm (baseline pair) | "wired earphones 3.5mm" | 2 | local consumer lanes | 100–250 ea. | n/a | T4, T5; P4–P5 standardized pair | wired mandatory ([README §2.1](../README.md#21-devices)); plugs into the §2b jack row |

### 2b. Wearable kit (Raspberry Pi 4)

| # | Item | Listing keywords | Qty | Lane | Price point (₱) | Pre-soldered | Consumed by | Notes |
|---|---|---|---|---|---|---|---|---|
| 1 | **Raspberry Pi 4 Model B, 4 GB** | "Raspberry Pi 4 Model B 4GB" | 1 | Circuitrocks (same-day Metro Manila; board-only and kit forms listed) / Makerlab PH / Shopee/Lazada (kit bundles incl. PSU+SD ~₱5.5–6.5k — buy board-only and pin our own PSU/SD rows) | 4,000–5,000 | n/a — no soldering | all wearable rows, T0–T8 | quad A72 @ 1.5 GHz + 4 GB; single 15-pin CSI; **global supply advisory appears on listings — verify before sign-off**; **heatsink row below is mandatory** ([README §3.2](../README.md#32-wearable--electronics)) |
| 2 | **IMX219 camera module, 160° wide-FOV lens, 15-pin** (visible-light) | "IMX219 160 degree wide angle camera" | 2 + 1 spare | Shopee/Lazada (multiple local listings confirmed 2026-09-26) | 500–900 ea. | n/a — 15-pin ribbon to the CSI connector | T7 | 8 MP fixed focus — acceptable across the 0.5–3 m tag band; **visible-light variant only — NoIR/night-vision listings rejected** ([README §3.7](../README.md#37-component-notes)); **both cameras share the single CSI port in alternation** — mux/alternate-capture still T7's call (risk 8 ladder) |
| 3 | CSI ribbon cables, 15-pin, 200–300 mm | "raspberry pi camera cable 15pin" | 2 + 2 spares | Shopee/Circuitrocks | 50–120 ea. | n/a | T7, P3 | the Pi 4's connector is **15-pin** — not the Pi 5's 22-pin (common mixed-listing trap); spares protect against FPC tears at P3 |
| 4 | **Heatsink for the Pi 4** (passive; + optional low-profile fan) | "raspberry pi 4 heatsink" | 1 set | Shopee/Circuitrocks | 80–250 (+ fan 100–250 optional) | adhesive or screw-on | T0 thermal soak, P3 housing | **mandatory** — the A72 throttles under sustained all-core load; the printed build must carry a thermal path ([README §3.2](../README.md#32-wearable--electronics)) |
| 5 | microSD card **A2-class, 64 GB** | "microSD 64GB A2" | 2 (boot + imaging spare) | Shopee (SanDisk Ultra/Samsung Evo-lineage; official RPi-brand A2 cards listed) | 400–700 ea. | n/a | OS bring-up, all rows after | A2-rating matters for Linux app load; second card = reflashing insurance mid-P2 |
| 6 | USB audio dongle, class-compliant (3.5 mm stereo out) | "USB audio adapter 3.5mm" | 1 + 1 spare | Shopee/Lazada (generic USB-audio sticks ~₱150–400) | 150–450 ea. | n/a | T4, T5 | the branch **declines the Pi 4's native analog jack** (PWM-derived, hiss/limited SNR) — the dongle *is* the audio path ([README §3.2](../README.md#32-wearable--electronics)); also keeps the audio stack identical to the raspi-5 branch |
| 7 | 3.5 mm stereo jack (panel or breakout) | "3.5mm stereo jack breakout" | 1 | Shopee | 20–50 | n/a | T4, T5 | between the dongle and the wired earphones (wired-mandatory endpoint) |
| 8 | USB-C male→female zip cord, short (bank→board run) | "USB-C data cable" | 2 | Shopee | 60–120 ea. | n/a | T0, P4 | plain 3 A-class cable is sufficient here — **no 5 A/e-marked requirement exists on this branch** (contrast with raspi-5) |

Pointer rows 1–6 and wearable rows 1–8 constitute both devices' electronics.

## 3. Battery, power & charge path (serves P2 → P5)

| # | Item | Listing keywords | Qty | Lane | Price point (₱) | Consumed by | Notes |
|---|---|---|---|---|---|---|---|
| 1 | USB power bank for the wearable, **5 V/≥ 3 A out, ≥ 10 Ah** | "power bank 10000mAh 3A" | 1 (spare at P4 optional) | Shopee/local consumer lanes | 700–1,500 | T0 | the Pi 4's 15 W-class contract — verify the bank's **sustained 3 A** spec, not marketing peak; rear-station role ([README §3.3](../README.md#33-structural--mechanical)); **no PD-trigger row exists on this branch** |
| 2 | **Official 15 W USB-C PSU (5.1 V/3 A)** | "raspberry pi 4 official power supply" | 1 | Circuitrocks/Makerlab PH (listed); Shopee (official PSU listings) | 650–800 | T0 bench only | bench/development PSU ([README §3.2](../README.md#32-wearable--electronics)); avoided-on-head by design |
| 3 | TP4056-C USB-C charge/protect board | "TP4056 USB-C charge protect" | 1 | Shopee/Lazada | 25–60 | T0 | **pointer only** (S3 + pouch path, branch-identical) |
| 4 | Slim Li-ion pouch | "Li-ion 3.7V pouch 1000-2000mAh" | 1 | Shopee/Lazada | 150–250 | T0 | pointer cell (slim, shell fit); capacity pinned by T0 current measurement |
| 5 | Slide/toggle switch | "slide switch SPDT panel" | 1 | Shopee | 15–30 | T0 | pointer power switch (the Pi 4 is switched by its bank) |
| 6 | USB-C wall charger, ≥ 3 A | "USB-C charger 3A" | 1 | local consumer lanes | 150–300 | T0, charging lane | recharges both supplies |

## 4. Bench infrastructure (one-time; serves T0–T8 and P3/P4 sessions)

| # | Item | Listing keywords | Qty | Lane | Price point (₱) | Consumed by | Notes |
|---|---|---|---|---|---|---|---|
| 1 | Solderless breadboard, 830 pt | "breadboard 830" | 2 | Shopee/e-Gizmo | 60–130 ea. | T0–T8 | one per device |
| 2 | Dupont jumper packs (M-M + M-F) | "dupont jumper wire pack" | 2 | Shopee | 80–150 ea. | T0–T8 | |
| 3 | USB-C data cable | "USB-C data cable" | 2 | Shopee | 60–120 ea. | flashing, T3 | one for the S3 pointer, one for the Pi 4 |
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
- [ ] **Pi 4 variant = Model B, 4 GB, board-only listing** (not a kit concealing a third-party PSU/SD — the PSU and SD are pinned rows here).
- [ ] **IMX219 modules: visible-light variant with the 160° lens** — NoIR/night-vision listings rejected; lens marking checked per listing.
- [ ] **CSI ribbon = 15-pin type** — not the Pi 5's 22-pin (mixed-listing trap); two + two spares.
- [ ] **Heatsink row present in the cart** — mandatory, not decorative (A72 throttling, [README §3.2](../README.md#32-wearable--electronics)).
- [ ] microSD = A2-rated, ≥ 64 GB; second card for reflash insurance.
- [ ] USB audio dongle: class-compliant (UAC2) stereo-out — verified by an actual ALSA listing, not a digital-only variant.
- [ ] Power bank: **sustained 5 V/3 A**, not peak-only.
- [ ] DWM3000 = module with integrated antenna (DW3110-based), not a bare QFN chip.
- [ ] BNO085 vs VL53L1X addresses distinct (0x4a / 0x29 — shared pointer bus, no collision).
- [ ] Per-quote rows (§5 rows 6–7) quoted **before** the sign-off.
- [ ] Sum rows into the §7 envelope; committed totals land at [README §3.6](../README.md#36-totals) post-sign-off.

## 7. Price envelope (point-in-time, branch raspi-4, 2026-09-26)

| Block | Low (₱) | High (₱) |
|---|---|---|
| Pointer kit (§2a: S3 devkit + BNO085 ×2 + VL53L1X + DWM3000 ×2 + buttons + earphones) | ~3,700 | ~6,600 |
| Wearable kit (§2b: Pi 4 4 GB + IMX219 160° ×2 + FPC spares + **heatsink** + microSD ×2 + USB-audio dongles ×2 + jack + zip cords) | ~6,800 | ~10,500 |
| Battery & power/charge path (§3) | ~1,700 | ~2,900 |
| Bench infrastructure (§4) | ~600 | ~1,300 |
| One-time blocks (§5, rows 1–5, 8–10 — **excluding the two per-quote rows**) | ~2,600 | ~5,300 |
| — bare carrier PCBs + spares (§5 row 6) | per quote | per quote |
| — local hand-solder service, 2 builds (§5 row 7) | per quote | per quote |
| **Total, entire project — fixed-price rows** | **~₱15,400** | **~₱26,600** |

Totals are a researched point-in-time envelope, not committed costs: Shopee prices fluctuate; the same global Raspberry Pi supply advisory appears on Pi 4 listings as on Pi 5 — verify stock the day you buy; specialty-stock rows (DWM3000) carry single-shop risk. The envelope covers the entire project: the bench order (P2), the P3 build kit and service lanes, and the P5 evaluation hardware. Final committed totals are computed from the co-researcher's approved order and recorded in [README §3.6](../README.md#36-totals).
