# Cane — Bench materials list (everything the P2 bench tests consume)

> **Every component, fixture, consumable, and on-hand item the Phase 2 bench phase consumes** — the bring-up safety rules, the wiring maps, and the T0–T8 test matrix of [`bench-tests.md`](bench-tests.md) — itemized against the **chosen esp32-s3 selection** (decided 2026-09-26, [README §3](../README.md#3-hardware): ESP32-S3 on both devices, ESP-NOW link). This is the bench-keyed view of the entire-project order in [`purchase-list.md`](purchase-list.md); its envelope math rolls up there. Prices are point-in-time Philippine-local ballparks (Shopee/Lazada/e-Gizmo-class lanes — verify at checkout). Two standing rules bind the cart: **every module arrives with headers pre-soldered** (the no-soldering rule — verify the listing photos before checkout), and **buy-once** — the selection is fixed, the purchase corrects nothing.

## 1. Compute & sensors (both devices)

| # | Item | Listing keywords | Qty | Consumed by | Est (₱) | Notes |
|---|---|---|---|---|---|---|
| 1 | ESP32-S3-DevKitC-1 **N16R8** | "ESP32-S3-DevKitC-1 N16R8" | 2 (1 per device) | all of T0–T8 | ~500 ea. | dual USB — the native port is the JTAG/debug path (no separate programmer); strapping pins stay reserved per the wiring-map rules |
| 2 | BNO085 breakout (I²C) | "GY-BNO085 BNO085 IMU" | 2 (1 per device) | T1, T5, T6 | 700–1,100 ea. | same part both devices (fusion parity); on the pointer it shares the I²C bus with the ToF at a distinct fixed address (0x4a vs 0x29) |
| 3 | VL53L1X ToF, optical cover | "VL53L1X TOF 4m" | 1 | T2, T5 | 150–350 | the pointer's ranger; its Class-1 940 nm beam is the gated "invisible laser" ([README §3.1](../README.md#31-pointer--electronics)) |
| 4 | DWM3000 UWB module (DW3110-based) | "DWM3000 module DW3110" | 2 (tag + anchor) | T8 | 700–1,500 ea. | SPI + IRQ/reset on both breadboards; module with integrated antenna — never a bare QFN chip; **single-shop risk — confirm stock before checkout** |
| 5 | MAX98357A I²S Class-D amp | "MAX98357A I2S amplifier" | 2 (1 per ear) | T4, T5 | 60–120 ea. | L/R channel-select strapping per amp; T4's gain sweep may invoke the recorded PCM5102-DAC alternate ([README §3.7](../README.md#37-component-notes)) |
| 6 | OV2640 DVP camera, **160° wide-FOV**, 24-pin | "OV2640 160 degree wide angle 24pin ESP32" | 2 + 1 spare | T7 | 100–200 ea. | the two cameras share the S3's single DVP port in alternation — **I²C address straps must differ**; verify per listing |
| 7 | 24-pin FPC breakout adapters | "OV2640 FPC to DIP adapter" | 2 + 2 spares | T7 wiring | 25–60 ea. | camera ribbon → header rows; keeps the bench solder-free |
| 8 | 3.5 mm stereo jack breakout | "3.5mm stereo jack breakout" | 1 | T4, T5 | 20–50 | the wired-earphones endpoint (wired mandatory — [README §2.1](../README.md#21-devices)) |
| 9 | Momentary push buttons | "push button momentary 12mm" | 4 | T5 onward | 5–15 ea. | pointer trigger + wearable re-zero + 2 spares |

## 2. Power & charge path (T0's subjects)

| # | Item | Listing keywords | Qty | Consumed by | Est (₱) | Notes |
|---|---|---|---|---|---|---|
| 1 | TP4056-C USB-C charge/protect board | "TP4056 USB-C charge protect" | 2 | T0 | 25–60 ea. | one per device; the protection IC must trip on T0's short test |
| 2 | 18650 Li-ion cell, protected | "18650 3.7V protected" | 2 | T0 | 120–180 ea. | wearable rear-station cell; the second keeps sessions rolling while one charges |
| 3 | 18650 holder | "18650 holder" | 1 | T0 | 20–40 | bench use is holder + jumpers (solder-free) |
| 4 | Slim Li-ion pouch | "Li-ion 3.7V pouch 1000-2000mAh" | 1 | T0 | 150–250 | pointer cell; capacity class pinned by the T0 current measurement |
| 5 | Slide/toggle switch | "slide switch SPDT panel" | 2 | T0 | 15–30 ea. | per-device power switch |
| 6 | USB-C wall charger, ≥ 2 A | "USB-C charger 2A" | 1 | T0, charging | 150–300 | recharges both supplies |

## 3. Wiring & breadboard infrastructure

| # | Item | Listing keywords | Qty | Consumed by | Est (₱) | Notes |
|---|---|---|---|---|---|---|
| 1 | Solderless breadboard, 830 pt | "breadboard 830" | 2 | T0–T8 | 60–130 ea. | one per device |
| 2 | Dupont jumper packs (M-M + M-F) | "dupont jumper wire pack" | 2 | T0–T8 | 80–150 ea. | the wiring maps are drawn in dupont |
| 3 | Hookup wire set (22–26 AWG, multi-color) | "hookup wire kit" | 1 | wiring maps | 100–200 | color coding starts here — it becomes the harness color standard at the soldered build |
| 4 | Spare header packs (2.54 mm) | "2.54 header pin pack" | 1–2 | wiring maps | 30–80 ea. | module remounts as the maps iterate |
| 5 | Bench attachment kit — zip ties, velcro straps, double-sided foam tape | "zip ties velcro foam tape" | 1 | T0–T8 | 60–150 | the no-solder fasteners the P2 rule names |
| 6 | USB-C data cable | "USB-C data cable" | 2 | flashing, debug logging | 60–120 ea. | one per devkit |

## 4. Test-specific fixtures & consumables (keyed to the test that consumes them)

| Test | Distinctive items | Qty | Est (₱) | Notes |
|---|---|---|---|---|
| **T0 — power rails** | digital multimeter (inline current measurement + the protection trip check) | 1 | 350–700 | the bring-up rule's first-power-up instrument; also the P3 incoming-inspection meter ([README §3.4](../README.md#34-assembly--bench-tools-one-time)) |
| **T0 — power rails** | USB power banks | 2 | 350–700 ea. | the bring-up rule: bench sessions run from banks/bench USB, not the battery rail, until T0 passes |
| **T1 — IMU orientation & drift** | flat level reference (tabletop/floor) | — | on-site ₱0 | static pitch/roll at 5 poses against a level surface |
| **T1 — IMU orientation & drift** | phone inclinometer app (reference) | — | on-hand ₱0 | the second reference for the static poses |
| **T1 — IMU disturbance** | ferromagnetic items (steel table, rebar corner) | — | on-site ₱0 | the characterization setup; no purchase |
| **T2 — ToF accuracy & cadence** | pointer clamp/stand (camera tripod or lab clamp) ◆ | 1 | 150–300 | the breadboard is clamped while tape-measured distances are ranged |
| **T2 — ToF accuracy & cadence** | measuring tape, 5 m | 1 | 60–150 | ground truth here; reused for T6/T8 and the Phase 5 course |
| **T2 — surface matrix** | white foam board ◆ | 1 sheet | 80–150 | the high-reflectance surface |
| **T2 — surface matrix** | cardboard box | 1 | on-hand ₱0 | the mid-reflectance surface |
| **T2 — surface matrix** | dark fabric remnant ◆ | 1 | 50–150 | the low-reflectance surface (risk 5's setup) |
| **T3 — device-to-device link** | (breadboards + power banks + data cables, above) | — | — | range markers = the measuring tape; the body-blocked run needs a person, not a part |
| **T4 — renderer load** | wired stereo earphones, 3.5 mm | 2 | 100–250 ea. | the amps' load; one bench pair, one standardized Phase 5 pair |
| **T5 — end-to-end placement** | obstacle at a known pose | 1 | on-hand ₱0 | course furnishing reused from the room |
| **T6 — offset calibration** | flat grip fixture (a flat block/jig) ◆ | 1 | 0–100 | any flat rigid block that fixes the pointer pose; printed scrap qualifies |
| **T6 — offset calibration** | ruler (the tape covers it; calipers optional) ◆ | 0–1 | 0–250 | repeatability ±2 cm per component is the gate |
| **T7 — vision tracking** | head-form fixture (foam head or printed jig) | 1 | 0–150 | camera stations spaced like the wearable's strap stations ([bench-tests.md](bench-tests.md) T7 setup) |
| **T7 — vision tracking** | printed ArUco/AprilTag test tags — matte high-contrast sticker/print stock | 1 pack | 50–150 | printed at several sizes for the dictionary/size pinning; the low-light/contrast check runs under scene lighting |
| **T7 — low-light check** | dimmable room lighting / evening window; phone lux-meter app for the minimum-illumination record | — | on-hand ₱0 | a dedicated lux meter is optional (₱300–600) if the phone reading feels untrustworthy |
| **T8 — UWB ranging & fallback** | (the DWM3000 pair + breadboards + tape, above) | — | — | tape-measured baselines at 0.5–4 m; body-blocked/NLOS profiles need a person |

◆ = a bench fixture not yet itemized in [`purchase-list.md`](purchase-list.md) — fold these rows into the checkout cart; they add roughly **₱400–1,300** all-in.

## 5. On-hand items (₱0 — no purchase)

- A laptop for flashing, serial monitoring, and the per-session CSV logging (`session,test_id,timestamp_ms,metric,value,unit,notes` — the logging format the protocol fixes).
- A phone for the inclinometer reference (T1), the lux reading (T7's low-light record), and timing.
- Room furniture: the level surface (T1), the ferromagnetic disturbance source (T1), the known-pose obstacle (T5).

## 6. What is deliberately NOT on this list

- **No soldering consumables for the bench** — the Phase 2 rule forbids soldering entirely; the minimal rework & inspection set (entry iron, solder, flux, wick, tweezers) is a **Phase 3** item ([README §3.4](../README.md#34-assembly--bench-tools-one-time), [`purchase-list.md`](purchase-list.md) §5) and waits for its phase.
- **No spares beyond the pinned ones** (the third OV2640, the second 18650, spare buttons/headers/adapters) — the buy-once rule prices exactly what the tests consume.
- **No Phase 3 build materials** (filament, strap, fasteners, harness kit) or Phase 5 course hardware — those live in [`purchase-list.md`](purchase-list.md) §5, phase-deferred.

## 7. Checkout reminders (the failure modes that matter)

- Pre-soldered headers verified on **every** module row (listing photos, not descriptions).
- I²C addresses distinct: BNO085 (0x4a) vs VL53L1X (0x29) on the shared pointer bus; the two OV2640 address straps **different from each other** (shared DVP bus in alternation).
- DWM3000 = module with integrated antenna (DW3110-based), never a bare QFN chip.
- DevKitC-1 variant = **N16R8** (16 MB flash / 8 MB PSRAM) — the cheaper N8-class clones miss the PSRAM budget the dual QVGA buffers need.
- MAX98357A listings expose the L/R-select pin (per-ear channel strapping).
- Cells: protected 18650 + pouch with protection leads — T0 trips the protection deliberately.

## 8. Bench-slice envelope

| Block | Low (₱) | High (₱) |
|---|---|---|
| Compute & sensors (§1) | ~3,100 | ~7,250 |
| Power & charge path (§2) | ~1,400 | ~2,300 |
| Wiring & breadboard infrastructure (§3) | ~600 | ~1,300 |
| Test fixtures & consumables (§4, incl. the ◆ rows) | ~600 | ~2,000 |
| **Bench slice total** | **~₱5,700** | **~₱12,850** |

This slice is the Phase 2 order portion of the entire-project envelope (the full-project fixed-price envelope, ₱9,750–18,150, additionally carries the pointer-kit items counted here plus the Phase 3/Phase 5 blocks in [`purchase-list.md`](purchase-list.md) §5 — the overlap is intentional; this file is the test-keyed view, that file remains the purchasing record). Fold the ◆ rows into the checkout cart when the order goes out, and log the verified prices back into [`purchase-list.md`](purchase-list.md) §7 at sign-off.
