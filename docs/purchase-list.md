# Cane — Purchase list (branch raspi-5, entire project)

> **The components and materials for the entire project — P2 bench, P3 build kit, P4/P5 phases — itemized for the raspi-5 selections (research pass 2026-09-26; selection in [README §3.1–3.2](../README.md#31-pointer--electronics)).** The wearable is a **Raspberry Pi 5 (8 GB)** with two native MIPI cameras (Camera Module 3 Wide ×2), USB-audio output, and a USB-PD power rail; the pointer keeps the **ESP32-S3** kit; the link is **UDP over Wi-Fi** (RPi 5 hosts the AP). Verify prices and stock at checkout; totals serve the co-researcher's purchase-approval sign-off (financial reasons, [README §9](../README.md#9-open-items)). Sourcing scope per decision 2026-09-26: **Philippine-local lanes only** — Shopee PH, Lazada PH, e-Gizmo, Circuitrocks, Makerlab PH (official Pi distributor), Maker PH-type local shops; no global parts houses. Quantities cover **both devices fully**. Tracking hardware included — part of the final design ([README §2.5](../README.md#25-pointer-tracking-stack-vision-uwb-imu)).

## 1. Scope rules

- **This list is the whole project's purchase list, not a bench order.** It itemizes every component, material, fixture, tool, and service the project consumes from P2 through P5 — §2–§4 are the items bought at/before the bench phase that keep serving the final builds and study; §5 holds the phase-deferred one-time blocks (P3 build/service lanes, P5 hardware), labeled with their consuming phase.
- **No-soldering rule (purchasing constraint).** P2 attaches every component non-permanently and involves no soldering of any kind, so every module is ordered with **headers pre-soldered**; verify the listing before checkout. Unsoldered arrivals wait, unsoldered, for the P3 build (the S3 kit rows below are all headered devkits/modules — the RPi 5 needs no soldering anywhere).
- **Buy-once rule (purchasing constraint).** Each component category is purchased once — the selection is made *before* the purchase, never corrected by a re-buy afterwards ([README §3](../README.md#3-hardware)). Non-consumables (§5 tools) bought once; consumables sized per phase.
- **Local-lane rule (decision 2026-09-26).** All rows are priced against Philippine-local shops and marketplaces; no offshore distributor accounts are opened for this build.
- Every row carries its consuming bench tests or phase, so any cut can be checked against the T0–T8 matrix and the phase structure before it is made.

## 2. Device electronics (ordered for the bench — headers pre-soldered where applicable; serve P2 → P5)

### 2a. Pointer kit (ESP32-S3, unchanged from the pointer role of the esp32-s3 branch)

| # | Item | Listing keywords | Qty | Lane | Price point (₱) | Pre-soldered | Consumed by | Notes |
|---|---|---|---|---|---|---|---|---|
| 1 | ESP32-S3-DevKitC-1 **N16R8** | "ESP32-S3-DevKitC-1 N16R8" | 1 | e-Gizmo (₱499 seen 2026-09-26); Shopee/Lazada alternates | ~500 | yes — pre-headered | all pointer rows, T3 station side | dual USB (native USB = debug); joins the wearable's Wi-Fi AP as station — UDP link ([README §2.1](../README.md#21-devices)) |
| 2 | BNO085 breakout (GY-BNO085 class, I²C) | "GY-BNO085 BNO085 IMU" | 2 (pointer + wearable — same part both) | Shopee/Lazada; e-Gizmo/Circuitrocks if stocked | 700–1,100 ea. | prefer listing photos with pre-soldered headers | T1, T5, T6 | same-part pairing ([README §3.7](../README.md#37-component-notes)); distinct fixed address from ToF |
| 3 | VL53L1X ToF with optical cover | "VL53L1X TOF 4m" | 1 | Shopee/Lazada; Makerlab PH (TOF400C-class alternates — check cover) | 150–350 | yes — XSHUT/INT headered | T2, T5 | cover-glass version; 940 nm Class-1 beam = the "invisible laser" ([README §3.1](../README.md#31-pointer--electronics)) |
| 4 | DWM3000 UWB module (DW3110-based, ceramic antenna) | "DWM3000 module DW3110" | 2 (tag + anchor) | Shopee/Lazada (Qorvo Decawave listings); Circuitrocks if stocked | 700–1,500 ea. | module, castellated — solders onto the carrier header (never bare QFN, [README §3.9](../README.md#39-assembly--outsourcing-p3)) | T8 | SPI + IRQ/reset; wearable side rides the RPi 5's SPI bus |
| 5 | Momentary push buttons | "push button momentary 12mm" | 4 (trigger + re-zero + 2 spares) | Shopee/e-Gizmo | 5–15 ea. | leads for breadboard; P3 panel-caps | trigger (T5+), re-zero (T5) | pointer trigger, wearable re-zero |
| 6 | Wired stereo earphones, 3.5 mm (baseline pair) | "wired earphones 3.5mm" | 2 | local consumer lanes | 100–250 ea. | n/a | T4, T5; P4–P5 standardized pair | wired mandatory ([README §2.1](../README.md#21-devices)); connects at the §2b jack row |

### 2b. Wearable kit (Raspberry Pi 5)

| # | Item | Listing keywords | Qty | Lane | Price point (₱) | Pre-soldered | Consumed by | Notes |
|---|---|---|---|---|---|---|---|---|
| 1 | **Raspberry Pi 5, 8 GB** | "Raspberry Pi 5 8GB" | 1 | Circuitrocks / Makerlab PH (official distributor; **MbP price seen ₱11,449 8 GB** — currently "Sold out" on their web store, restock-watch; Circuitrocks lists genuine boards same-day) / Lazada / Shopee (kit bundles incl. PSU+SD at ~₱10–13k) | 9,500–12,500 | n/a — no soldering | all wearable rows, T0–T8 | 5 V/5 A-class device; dual MIPI CSI is the branch's decisive fact ([README §3.2](../README.md#32-wearable--electronics)); **stock/price currently unstable globally — verify before sign-off** |
| 2 | **Camera Module 3 Wide** (IMX708, 120°, IR-cut) | "Raspberry Pi Camera Module 3 Wide" | 2 + 1 spare? (2 only if budget-tight — see note) | Circuitrocks / Makerlab PH (official; **MbP variant seen ₱2,990**, Wide/NoIR variants listed) / Lazada / Shopee | 1,500–3,000 ea. (bundle pricing pending checkout) | n/a — CSI ribbon, no soldering | T7 | PDAF + HDR; **Wide** (IR-cut) variant — NoIR rejected ([README §3.7](../README.md#37-component-notes)); each camera takes its own CSI port — no mux ([README §3.2](../README.md#32-wearable--electronics)) |
| 3 | CSI ribbon cable, 22-pin 0.5 mm, 200–300 mm | "raspberry pi 5 camera cable 22pin" | 2 + 2 spares | Shopee/Circuitrocks | 50–120 ea. | n/a | T7, P3 | Pi 5 uses the 22-pin fine-pitch connectors — **not** the Pi 4 15-pin; buy the correct type, spare set protects against FPC tears at P3 |
| 4 | microSD card **A2-class, 64 GB** | "microSD 64GB A2" | 2 (boot + imaging spare) | Shopee (SanDisk Ultra/Samsung Evo-lineage, official RPi-brand A2 cards listed) | 400–700 ea. | n/a | OS bring-up, all rows after | A2-rating matters for Linux app load; second card = reflashing insurance mid-P2 |
| 5 | USB audio dongle, class-compliant (3.5 mm stereo out) | "USB audio adapter 3.5mm" | 1 + 1 spare | Shopee/Lazada (Logitech H340/H390-class USB headsets listed; generic USB-audio sticks ~₱150–400) | 150–450 ea. | n/a | T4, T5 | the Pi 5 has **no analog audio out** — this dongle *is* the audio path ([README §3.2](../README.md#32-wearable--electronics)); feeds the §3 jack; small-buffer ALSA-checked at T4 |
| 6 | 3.5 mm stereo jack (panel or breakout) | "3.5mm stereo jack breakout" | 1 | Shopee | 20–50 | n/a | T4, T5 | between the USB dongle and the wired earphones (wired-mandatory endpoint) |
| 7 | USB-PD trigger board, **5 V/5 A profile** | "USB-C PD trigger board 5A" | 1 (+ 1 spare) | Shopee/Lazada (PD/QC decoy boards abound, keypad or DIP types ₱25–60) | ~50–120 | n/a | T0 | decodes the mobile bank's 5 V/5 A PD contract into a clean 5 V rail for the Pi; cheap, carry spares |
| 8 | USB-C male→female zip cord, short (bank→trigger/dongle runs) | "USB-C PD cable 5A 22AWG" | 2 | Shopee | 80–180 ea. | n/a | T0, P4 | **5 A-rated (e-marked) cable** — a 3 A cable throttles the Pi 5; carry spares |

Pointer rows 1–6 and wearable rows 1–8 constitute both devices' electronics.

## 3. Battery, power & charge path (serves P2 → P5)

| #   | Item                                                           | Listing keywords                 | Qty                         | Lane                                                                                                      | Price point (₱) | Consumed by       | Notes                                                                                                                                                                                                               |
| --- | -------------------------------------------------------------- | -------------------------------- | --------------------------- | --------------------------------------------------------------------------------------------------------- | --------------- | ----------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1   | **USB-PD power bank, ≥ 20 Ah / ≥ 45 W output (2A-port class)** | "power bank 20000mAh 45W PD"     | 1 (spare recommended at P4) | Shopee/Lazada (Vention/Philips-class 20 Ah 45 W PD banks listed ~₱1,500–2,300)                            | 1,200–2,300     | T0                | the wearable's mobile rail — Pi 5 + cameras + UWB at the rear station, doubles as counterweight ([README §3.3](../README.md#33-structural--mechanical)); must sustain ≥ 5 A continuous — verify spec, not marketing |
| 2   | USB-C PD **wall charger ≥ 45 W**                               | "USB-C PD charger 45W"           | 1                           | local consumer lanes                                                                                      | 400–800         | T0, charging lane | recharges the bank + bench bench sessions                                                                                                                                                                           |
| 3   | **Official 27 W USB-C PSU (5.1 V/5 A)**                        | "official Raspberry Pi 27W PSU"  | 1                           | Circuitrocks / Makerlab PH (official; seen ₱1,080–₱1,250 listed) / e-Gizmo (VAT-exclusive ₱1,213 ceiling) | 1,000–1,400     | T0 bench only     | bench/development PSU per the power-path note ([README §3.2](../README.md#32-wearable--electronics)); avoided-on-head by design                                                                                     |
| 4   | TP4056-C USB-C charge/protect board                            | "TP4056 USB-C charge protect"    | 1                           | Shopee/Lazada                                                                                             | 25–60           | T0                | **pointer only** — the S3 + pouch path (unchanged from the S3-branch policy)                                                                                                                                        |
| 5   | Slim Li-ion pouch                                              | "Li-ion 3.7V pouch 1000-2000mAh" | 1                           | Shopee/Lazada                                                                                             | 150–250         | T0                | pointer cell (slim, shell fit); capacity pinned by the T0 current measurement                                                                                                                                       |
| 6   | Slide/toggle switch                                            | "slide switch SPDT panel"        | 1                           | Shopee                                                                                                    | 15–30           | T0                | pointer power switch (the Pi 5 is switched by the bank)                                                                                                                                                             |
| 7   | 18650 spare (optional reserve)                                 | "18650 3.7V protected"           | 0–1                         | Shopee/Lazada                                                                                             | 120–180         | —                 | only if the PD bank is dropped for a 2S/18650-class custom pack in a later iteration — **not** the branch's plan                                                                                                    |

## 4. Bench infrastructure (one-time; serves T0–T8 and P3/P4 sessions)

| # | Item | Listing keywords | Qty | Lane | Price point (₱) | Consumed by | Notes |
|---|---|---|---|---|---|---|---|
| 1 | Solderless breadboard, 830 pt | "breadboard 830" | 2 | Shopee/e-Gizmo | 60–130 ea. | T0–T8 | one per device |
| 2 | Dupont jumper packs (M-M + M-F) | "dupont jumper wire pack" | 2 | Shopee | 80–150 ea. | T0–T8 | |
| 3 | USB-C data cable, 5 A-class(e-marked) | "USB-C data cable 5A" | 2 | Shopee | 60–180 ea. | flashing, T3 | one for the RPi5 dev link, one for the S3 |
| 4 | Hookup wire set (22–26 AWG) | "hookup wire kit" | 1 | Shopee/e-Gizmo | 100–200 | T0–T8 | |
| 5 | Spare header packs (2.54 mm) | "2.54 header pin pack" | 1–2 | Shopee | 30–80 | wiring maps | module remounts |
| 6 | Bench attachment kit — zip ties, velcro straps, foam tape | "zip ties velcro foam tape" | 1 | Shopee/hardware | 60–150 | T0–T8 | the no-solder fasteners of the P2 rule ([bench-tests.md](bench-tests.md)) |
| 7 | HDMI micro-adapter + monitor pass-through (development only) | "micro HDMI to HDMI adapter" | 1 | Shopee | 100–200 | bring-up | headless-first policy anyway (SSH from the laptop) — this is the fallback |

## 5. One-time blocks — P3 build kit, service lanes, P5 study hardware (phase labels in-row)

| # | Item | Qty | Lane | Price point (₱) | Consuming phase | Notes |
|---|---|---|---|---|---|---|
| 1 | Digital multimeter | 1 | local tool shops/Shopee | 350–700 | P2–P5 | T0 + incoming inspection ([README §3.4](../README.md#34-assembly--bench-tools-one-time)) |
| 2 | Minimal rework & inspection set — strippers/cutters/pliers, tweezers, adjustable iron + solder + flux + desoldering wick | 1 | e-Gizmo/Shopee | 700–1,200 | P3 rework only | full soldering kit dropped (outsourced build — [README §3.9](../README.md#39-assembly--outsourcing-p3)) |
| 3 | Printed marker stock — matte high-contrast sticker/print sheets | 1 pack | Shopee/print shops | 50–150 | T7 (test tags) + P3 (shell marker) | printed ArUco/AprilTag ([README §3.8](../README.md#38-pointer-tracking-hardware--notes--contingencies)) |
| 4 | Harness materials kit — JST-XH/PH connectors + crimps, heat-shrink, colored silicone wire (22/26 AWG) | 1 kit | Shopee/Makerlab PH | 400–800 | P3 ([`assembly.md`](assembly.md) §3) | the handoff kit's wires + spares for P4 repair |
| 5 | Head-form fixture (T7 setup) | 1 | printed or foam head | 0–150 | T7 | camera stations like the strap spacing ([bench-tests.md](bench-tests.md) T7) |
| 6 | Bare carrier PCBs, **with ≥ 2 spares/device** | ≥ 2/device (+2 spares) | PCB fab house | per quote | P3 ([README §3.9](../README.md#39-assembly--outsourcing-p3) lane 1) | order at the breadboard exit (~Oct 27) |
| 7 | Local hand-solder service fee | 2 builds | local service | per quote | P3 ([`assembly.md`](assembly.md) §4) | per-build quote, turnaround in-window |
| 8 | Filament (PLA/HTPLA) or print-service voucher | 1–2 kg / voucher | Shopee/local print service | 700–1,400 | P3 | pointer shell + wearable strap mounts ([README §3.3](../README.md#33-structural--mechanical)) |
| 9 | Elastic head strap + fasteners/adhesive + heat-set inserts | 1 set | Shopee/local sewing/hardware | 100–250 | P3 | goggle-style band + mounts |
| 10 | Evaluation hardware — blindfolds, floor marking tape, measuring tape, obstacle props | 1 set | Shopee/local hardware | 300–600 | P5 ([README §3.5](../README.md#35-evaluation-hardware-one-time)) | timing on the on-hand phone |

## 6. Checkout checklist

- [ ] Pointer rows — all pre-soldered-header rules still apply to the S3-side modules (§2a); unsoldered arrivals wait unsoldered.
- [ ] **RPi 5 variant = 8 GB, board-only listing** (not a full kit concealing a 3rd-party PSU/SD); Circuitrocks/Makerlab authenticity preferred.
- [ ] **Camera Module 3 Wide (standard IR-cut)** — not NoIR, not the standard-FOV version; two units, each with its own cable.
- [ ] **CSI cables = 22-pin, 0.5 mm Pi 5 type** — not the Pi 4 15-pin type (common mixed-listing trap).
- [ ] microSD = A2-rated, ≥ 64 GB; second card for reflash insurance.
- [ ] USB audio dongle: class-compliant (UAC2) **stereo-out** tested by actual ALSA listing — not a TOSLINK/digital-only variant.
- [ ] PD trigger board: supports **5 V/5 A** profile (mass-market decoy boards top out at 3 A on 5 V; check the spec table, not the headline wattage).
- [ ] Power bank: **sustained ≥ 45 W PD output**, not peak-only; 5 V/5 A continuous confirmed.
- [ ] DWM3000 = module with integrated antenna (DW3110), not bare QFN.
- [ ] BNO085 vs VL53L1X addresses distinct (0x4a / 0x29 — shared bus on the pointer, no collision).
- [ ] I²S convention rows (MAX98357A L/R select etc.) do **not** apply on this branch — the audio path is USB-only; the §2.4-style L/R-pin checkout line of the S3 list is N/A here.
- [ ] Per-quote rows (§5 rows 6–7) quoted **before** the sign-off.
- [ ] Sum rows into the §7 envelope; committed totals land at [README §3.6](../README.md#36-totals) post-sign-off.

## 7. Price envelope (point-in-time, branch raspi-5, 2026-09-26)

| Block | Low (₱) | High (₱) |
|---|---|---|
| Pointer kit (§2a: S3 devkit + BNO085 ×2 + VL53L1X + DWM3000 ×2 + buttons + earphones) | ~3,700 | ~6,600 |
| Wearable kit (§2b: RPi5 8 GB + Cam3Wide ×2 + CSI cables ×4 + microSD ×2 + USB-audio dongles ×2 + jack + PD trigger ×2 + USB-C zip cords) | ~14,000 | ~20,800 |
| Battery & power/charge path (§3) | ~2,800 | ~4,700 |
| Bench infrastructure (§4) | ~700 | ~1,800 |
| One-time blocks (§5, rows 1–5, 8–10 — **excluding the two per-quote rows**) | ~2,600 | ~4,550 |
| — bare carrier PCBs + spares (§5 row 6) | per quote | per quote |
| — local hand-solder service, 2 builds (§5 row 7) | per quote | per quote |
| **Total, entire project — fixed-price rows** | **~₱23,800** | **~₱38,500** |

Totals are a researched point-in-time envelope, not committed costs: Shopee prices fluctuate; RPi 5 stock/price is globally unstable right now (the official distributor's store currently shows items "Sold out" at ₱11,449 — 8 GB variant), so Circuitrocks/Makerlab should be checked the day you buy; specialty-stock rows (DWM3000) carry single-shop risk. The envelope covers the entire project: the bench order (P2), the P3 build kit and service lanes, and the P5 evaluation hardware. Final committed totals are computed from the co-researcher's approved order and recorded in [README §3.6](../README.md#36-totals).
