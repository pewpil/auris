# Cane — Purchase list (P2 bench order)

> The approved-now purchase subset for the purchase-approval sign-off: everything needed to perform the bench tests T0–T8 of [`docs/bench-tests.md`](bench-tests.md), built 2026-09-17 from the surveyed BOM of [README §3](../README.md#3-hardware) (Philippine-market prices, ₱, surveyed 2026-09-08 with the 2026-09-10/09-15 revisions; *(est)* items are pinned at checkout). Ordering window Sep 17–20, 2026; parts arrive ~Oct 1–5; the breadboard bench exits Oct 24 ([`docs/schedule.md`](schedule.md) §2). This order is **not** the full [README §3.6](../README.md#36-totals) total — P3-only and study-only blocks are explicitly deferred (§13).

## 1. Scope and the no-soldering rule

- The list buys every component, fixture, and tool the test matrix and wiring maps of [`bench-tests.md`](bench-tests.md) consume — frozen-core electronics for both devices, the pointer-tracking ✚ hardware (T7–T8 validate on the breadboards), battery-path hardware (T0), and the no-solder bench infrastructure.
- **Pre-soldered rule (purchasing constraint).** P2 attaches every component non-permanently and involves no soldering of any kind, so every module — both MCUs, the IMUs, the ToF, the two cameras, the UWB pair — must be ordered with **headers pre-soldered**; verify "pre-soldered / headers attached" on the listing before checkout. Unsoldered arrivals are set aside for the P3 build, never soldered during P2.
- Every row carries the bench tests that consume it, so any cut can be checked against the test matrix before it is made.

## 2. Section A — Pointer electronics (frozen core)

| # | Item | Qty | Unit ₱ | Subtotal ₱ | Consumed by | Source |
|---|---|---|---|---|---|---|
| A1 | ESP32-C3 SuperMini (BLE MCU) — **pre-soldered headers** | 1 | 355 | 355 | all pointer tests | [Circuitrocks](https://circuit.rocks/products/esp32-c3-super-mini-development-board); ₱151 on [Lazada PH](https://h5.lazada.com.ph/products/esp32-c3-development-board-esp32-c3-supermini-wifi-bluetooth-for-arduino-i4393598793.html) |
| A2 | VL53L1X ToF rangefinder module (940 nm, ~4 m) — **pre-soldered** | 1 | 525 | 525 | T2, T5, T6, T8 | [Shopee PH](https://shopee.ph/COD-VL53L1X-laser-sensor-module-TOF-time-of-flight-4-meter-ranging-i.1804393363.53909554436) |
| A3 | GY-9250 (MPU-9250 9-DoF IMU) — **pre-soldered** | 1 | 400 | 400 | T1, T5, T6, T8 | [Lazada PH](https://www.lazada.com.ph/products/mpu9250-mpu6500-9-9-dof-16-bit-gyroscope-acceleration-magnetic-sensor-accelerator-module-iicspi-i15524063344.html) |
| A4 | TP4056 USB-C charge board **with protection** | 1 | 30 | 30 | T0 battery path | [Makerlab PH](https://makerlab.ph/products/type-c-micro-usb-5v-1a-18650-tp4056-lithium-battery-charger-module-charging-board-with-protection) |
| A5 | Tactile button assortment (6×6 mm, breadboard-friendly) | 1 kit | 60 *(est)* | 60 | pointer trigger (T2–T5) + wearable re-zero (T1) + spares | Lazada/Shopee PH |
| | **Subtotal A** | | | **1,370** | | |

## 3. Section B — Wearable electronics (frozen core)

| # | Item | Qty | Unit ₱ | Subtotal ₱ | Consumed by | Source |
|---|---|---|---|---|---|---|
| B1 | Seeed XIAO ESP32-S3 — **pre-soldered headers** | 1 | 499 | 499 | T1, T3, T4, T5 | [Makerlab PH](https://makerlab.ph/products/seeed-xiao-esp32-s3-113991114) |
| B2 | GY-9250 (MPU-9250 9-DoF IMU) — **pre-soldered**, same part as A3 | 1 | 400 | 400 | T1 | [Lazada PH](https://www.lazada.com.ph/products/mpu9250-mpu6500-9-9-dof-16-bit-gyroscope-acceleration-magnetic-sensor-accelerator-module-iicspi-i15524063344.html) |
| B3 | MAX98357A I²S 3 W Class-D amp — mono, one per ear | 2 | 499 | 998 | T4, T5 | [Circuitrocks (Adafruit breakout)](https://circuit.rocks/products/i2s-3w-class-d-amplifier-breakout-max98357a-adafruit); generic Lazada/Shopee clones ₱150–250 each → subtotal ~₱300–500 |
| B4 | BAVIN HX820 wired earphones, 3.5 mm | 1 | 118 | 118 | T4, T5 | [Lazada PH](https://www.lazada.com.ph/products/pdp-i3057481002.html) |
| B5 | TP4056 USB-C charge board **with protection** | 1 | 30 | 30 | T0 battery path | [Makerlab PH](https://makerlab.ph/products/type-c-micro-usb-5v-1a-18650-tp4056-lithium-battery-charger-module-charging-board-with-protection) |
| B6 | 3.5 mm stereo jack breakout | 1 | 50 *(est)* | 50 | T4, T5 (audio wiring map) | Lazada/Shopee PH |
| | **Subtotal B** | | | **2,095** | | (₱1,400–1,600 if generic amp clones are chosen) |

## 4. Section C — Battery & power path (T0)

| # | Item | Qty | Unit ₱ | Subtotal ₱ | Consumed by | Source |
|---|---|---|---|---|---|---|
| C1 | 18650 Li-ion 2600 mAh cell — Kaizen **2-pc pack** (one cell per device, no separate spares line needed) | 1 pack | 369 | 369 | T0 battery-path validation, both devices | [Kaizen PH](https://kaizenphilippines.com/products/kaizen-3-7v-18650-2600mah-15a-rechargeable-battery-2pc-lithium-ion-battery) |
| C2 | 18650 battery holder **with leads** (solderless; leads to dupont/breadboard) | 2 | 15 *(est)* | 30 | T0 | Lazada/Shopee PH |
| C3 | Slide switch assortment (SS12D10-class) | 1 kit | 20 *(est)* | 20 | T0 battery-path on/off, both devices | Lazada/Shopee PH |
| | **Subtotal C** | | | **419** | | |

## 5. Section D — Pointer-tracking hardware ✚ (T7–T8)

| # | Item | Qty | Unit ₱ | Subtotal ₱ | Consumed by | Source |
|---|---|---|---|---|---|---|
| D1 | OV5640 camera module, **24-pin DVP, wide-FOV lens ≥ 160° diagonal**, fixed focus, QVGA @ 120 fps capability — **pre-soldered** | 2 | 400–700 *(est)* | 800–1,400 | T7 | Lazada/Shopee PH / AliExpress — **wide-lens variant only**; the 68°-lens dev-board bundles do NOT qualify |
| D2 | UWB module, DW1000 class (e.g., DWM1000; DW3000-class alternate) — tag + anchor pair, **pre-soldered** | 2 | 1,200–2,500 *(est)* | 2,400–5,000 | T8 | Lazada/Shopee PH — pin exact module, range, and update rate at checkout |
| D3 | Tracking beacon: 2× bright visible-red 5 mm LEDs + dropper resistors (lit only while the button is held) | 1 set | 30–80 *(est)* | 30–80 | T7 | Lazada/Shopee PH |
| D4 | Camera mounts, flex/PD cables, wiring | — | 150 *(est)* | 150 | T7 fixture | Lazada/Shopee PH |
| | **Subtotal D** | | | **3,380–6,630** | | matches the ✚ block in [README §3.6](../README.md#36-totals) |

## 6. Section E — Board-class hedge boards (T7/T8 gates)

| # | Item | Qty | Unit ₱ | Subtotal ₱ | Consumed by | Source |
|---|---|---|---|---|---|---|
| E1 | ESP32-S3 DevKitC-class dev board (DVP-capable camera pins, 44-pin) | 1 | 300–700 *(est)* | 300–700 | T7 — the XIAO cannot host even one DVP camera; this board runs the two-camera co-residence test while the gate decides the P3 board class | Lazada/Shopee PH |
| E2 | ESP32-C3 DevKit-class dev board (free SPI + IRQ pins) | 1 | 200–400 *(est)* | 200–400 | T8 — UWB SPI bus on the pointer side while the SuperMini stays pin-tight | Lazada/Shopee PH |
| | **Subtotal E** | | | **500–1,100** | | |

Both hedge boards run the same firmware as the reference boards and both stay inside the board-class swap range of [README §3.7](../README.md#37-component-notes); the reference boards (A1, B1) remain primary for the frozen-core bring-up and T1–T6. The hedges exist so the gate decisions — camera mux / alternate-frame capture / DevKit-class board / co-processor, and SPI remap vs. DevKit — can be tested immediately, without a second shipping cycle eating the Oct 5–24 bench window.

## 7. Section F — Bench infrastructure & consumables (no-solder bench, not in the BOM)

| # | Item | Qty | Unit ₱ | Subtotal ₱ | Consumed by | Source |
|---|---|---|---|---|---|---|
| F1 | Solderless breadboard, full-size 830-point | 2 | 100–150 *(est)* | 200–300 | one per device + overflow | Lazada/Shopee PH |
| F2 | Dupont jumper bundles (M-M + M-F) | 2 | 75–125 *(est)* | 150–250 | all wiring maps | Lazada/Shopee PH |
| F3 | Resistor assortment kit | 1 kit | 100–150 *(est)* | 100–150 | beacon droppers, MAX98357A SD-pin channel-select dividers, I²C pull-ups | Lazada/Shopee PH |
| F4 | Capacitor assortment kit (100 nF / 10 µF) | 1 kit | 100–150 *(est)* | 100–150 | rail decoupling for the T0 ±3 % stability check | Lazada/Shopee PH |
| F5 | 24-pin DVP FPC-to-DIP breakout adapter | 2 | 50–100 *(est)* | 100–200 | breadboarding the OV5640s — the no-solder path for the camera connector | Lazada/Shopee PH / AliExpress |
| F6 | T2 test surfaces: white foam board, dark fabric, cardboard box | — | 100–200 *(est)* | 100–200 | three reflectances for the ToF surface matrix | Lazada/Shopee PH / on-hand scraps |
| F7 | 5 m measuring tape + floor marking tape | — | 200–300 *(est)* | 200–300 | ground truth T2/T6/T7/T8; per-pose marking | Lazada/Shopee PH |
| F8 | Flat grip fixture + clamps (board + spring clamps, or phone tripod with clamp) | 1 | 0–200 *(est)* | 0–200 | T2 pointer stand, T6 grip fixture | on-hand tripod preferred; buy only if missing |
| F9 | Zip ties, velcro, double-sided tape (head-form fixture build) | — | 100 *(est)* | 100 | T7 camera + beacon mounting on the fixture | Lazada/Shopee PH |
| | **Subtotal F** | | | **1,050–1,850** | | (min assumes an on-hand tripod/clamps) |

## 8. Section G — One-time bench tools

| # | Item | Qty | Unit ₱ | Subtotal ₱ | Consumed by | Source |
|---|---|---|---|---|---|---|
| G1 | Digital multimeter (DT830-class) | 1 | 250 *(est)* | 250 | T0 inline current + rail checks; bring-up safety on first power-up | [Lazada PH](https://www.lazada.com.ph/products/dt830-digital-multimeter-multi-tester-holdpeak-manual-ranging-multi-tester-i4454326879.html) |
| G2 | USB data cables (USB-C plug) | 2 | 35 | 70 | flashing + serial telemetry, both devices | [Circuitrocks](https://circuit.rocks/products/circuitrocks-usb-cable-type-c-type-b-to-type-a-male-for-arduino-uno-mega) |
| | **Subtotal G** | | | **320** | | |

## 9. Section H — Bench USB power banks

| # | Item | Qty | Unit ₱ | Subtotal ₱ | Consumed by | Source |
|---|---|---|---|---|---|---|
| H1 | USB power bank (small, ≥ 5 V / 2 A output) | 2 | 300–500 *(est)* | 600–1,000 | untethered bench sessions T3–T8 (bench sessions run from USB until T0 passes) | Lazada/Shopee PH |
| | **Subtotal H** | | | **600–1,000** | | |

## 10. Section S — Spares

| # | Item | Qty | Unit ₱ | Subtotal ₱ | Consumed by | Source |
|---|---|---|---|---|---|---|
| S1 | MAX98357A I²S amp — generic clone, **pre-soldered** | 1 | 150–250 *(est)* | 150–250 | spare amp — the one bench part with no same-day local substitute; the 2-pc 18650 pack already carries the battery spare | Lazada/Shopee PH |
| | **Subtotal S** | | | **150–250** | | |

## 11. On hand — no purchase

- Phone with an inclinometer app (T1 static-reference) and a laptop (flashing, serial telemetry, CSV logging) — both on hand.
- Desk lamp or phone flash for the T7 bright-light rejection check — on hand.
- USB-C wall charger for charging the cells through the TP4056 boards — on hand.

## 12. Totals

| Block | ₱ |
|---|---|
| A — Pointer electronics (frozen core) | 1,370 |
| B — Wearable electronics (frozen core, Adafruit amps) | 2,095 |
| C — Battery & power path | 419 |
| D — Pointer-tracking hardware ✚ | 3,380–6,630 |
| E — Board-class hedge boards | 500–1,100 |
| F — Bench infrastructure & consumables | 1,050–1,850 |
| G — One-time bench tools | 320 |
| H — Bench USB power banks | 600–1,000 |
| S — Spares | 150–250 |
| **Subtotal — P2 bench order** | **9,884–15,034** |
| Contingency 20 % (shipping, promo drift, re-orders) | 1,980–3,010 |
| **Total — P2 bench order** | **≈ 11,900–18,100** |

Choosing generic MAX98357A clones instead of the Adafruit breakouts (B3) lowers the subtotal by ~₱500–800 before contingency. The [README §3.6](../README.md#36-totals) project total remains the sign-off context; this order is its bench-scoped subset (§13).

## 13. Deferred from this order (P3 / P5 blocks of the project budget)

- **3D printer** (Ender 3 V3 SE, ₱11,199) **or** print-service housings (~₱1,000 *(est)*) and **PLA filament** (₱700 *(est)*) — the P3 soldered build's shells and strap mounts; the own-printer vs. print-service decision stays open in [README §9](../README.md#9-open-items).
- **Soldering iron kit, desoldering wick/flux, helping hands, wire strippers/cutters/pliers** (~₱750 combined *(est)*) — needed only at P3, where soldering is allowed.
- **Perfboard** (the permanent-assembly portion of the BOM misc rows) — P3.
- **Elastic head strap + fasteners + padding** — P3; the T7 cameras mount on an improvised head-form fixture built from F9 (no-solder rule), spaced like the future strap stations.
- **Blindfolds and the full evaluation hardware block** (₱500) — P5 study hardware; only the tape measure and marking tape (F7) are pulled forward here as T0–T8 ground truth.

## 14. Checkout checklist

1. **Pre-soldered verification** — every module listing (A1–A3, B1, B2, D1, D2, S1) must state "pre-soldered / headers attached" before checkout; unsoldered arrivals go to the P3 tray.
2. **OV5640 (D1)** — 24-pin DVP, wide-FOV ≥ 160° diagonal, fixed focus, RGB565/YUV output; reject 68°-lens dev-board bundles; verify real FOV, low-light behavior, and DVP pinout on arrival.
3. **UWB (D2)** — pin the exact module at checkout: DWM1000-class preferred for library maturity (ESP-IDF / arduino-DW1000 support), DW3000-class alternate; record the listing's range and update rate in this file's notes for T8.
4. **MAX98357A breakout (B3, S1)** — verify the SD-pin channel-select strapping scheme (resistor levels for L/R/(L+R)/2/shutdown) against the purchased breakout's documentation before the T4 wiring.
5. **TP4056 (A4, B5)** — USB-C version **with** protection IC (over-discharge protection is required for 18650 use); Makerlab listing pinned.
6. **18650 (C1)** — Kaizen 2-pc pack; check the listing's terminal type (flat-top/button-top) against the C2 holders.
7. **Bring-up discipline on arrival** — first power-up of each device through the multimeter inline (mA range) or a current-limited USB source, per the [`bench-tests.md`](bench-tests.md) bring-up safety rules.
8. **Order window** — sign-off lands Sep 17–20, 2026; parts arrive ~Oct 1–5; the breadboard bench runs Oct 5–24 ([`docs/schedule.md`](schedule.md) §2).

## 15. Provenance

- **BOM-derived rows** — [README §3.1](../README.md#31-pointer--electronics) / [§3.2](../README.md#32-wearable--electronics) device electronics (A, B, D); [§3.3](../README.md#33-structural--mechanical) rows 4–6 pulled forward where the bench needs them (battery holders, zip ties/velcro/tape); [§3.4](../README.md#34-assembly--bench-tools-one-time) rows 2 and 5 (multimeter, USB cables); [§3.5](../README.md#35-evaluation-hardware-one-time) row 2's tape measure.
- **Bench-only additions (not in the BOM)** — solderless breadboards, dupont jumpers, DVP FPC-to-DIP breakouts, resistor/capacitor assortments, USB power banks, spare amp: required by the no-soldering rule and the T0–T8 fixtures.
- **Reconciling with [README §3.6](../README.md#36-totals)** — the frozen-core + ✚ electronics here match the README subtotals; the README total additionally contains the P3-deferred blocks (printer/housings/filament, soldering tools, strap), so the two totals bracket each other rather than add.
