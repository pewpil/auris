---
id: rs-sourcing
aliases: []
tags: []
---

# Cane — RS Philippines component list (both devices)

> **Superseded 2026-09-29 (second hardware clearing).** The esp32-s3 rows this file re-sourced are cleared with the selection they belong to ([README §9](../README.md#9-open-items)): the co-researcher re-opened the hardware record to source components and materials **compatible with both esp32-s3 and raspi-5**, and the compatible list is not researched yet — the RS re-source re-runs against that list once it exists. This file is kept as the 2026-09-28 live-search record: the evidence log ([§9](#9-search-log-and-provenance)), the "not carried on RS" classifications, and the compatibility-gate checks carry over to the next pass; the RS prices below are stale for any re-sourced row and valid only as lane-cost baselines for the superseded list.

> **The device electronics and bench order for both devices (pointer + wearable, esp32-s3 selections — [`purchase-list.md`](purchase-list.md) §2–§4), re-sourced against RS Components Philippines only (<https://ph.rs-online.com/>; researched 2026-09-28, live site search).** Original quantities kept; prices are RS list prices in PHP (exc. VAT, verify at checkout). The MCU rows are **pinned**: RS PH does not carry the exact part, so they stay with the pinned lane and nothing is substituted. Every other row either carries an RS stock number (same part or a compatible alternative) or is stated as **not carried on RS** rather than force-matched with something incompatible. Query-by-query evidence behind every row is listed in [§9](#9-search-log-and-provenance).

## 1. Sourcing rules

- **RS PH only.** Every purchasable row below is priced on ph.rs-online.com. Rows marked **not carried on RS** stay with their pinned original lanes ([`purchase-list.md`](purchase-list.md)); no incompatible substitute is invented for them.
- **Pinned MCUs (co-researcher's instruction).** Both devices use the ESP32-S3-DevKitC-1 **N16R8** (16 MB flash / 8 MB PSRAM — the PSRAM budget the dual QVGA buffers need, [`bench-materials.md`](bench-materials.md) §7). These are recorded here as **buy at the pinned lane** (e-Gizmo, ₱499 ea., in stock 2026-09-26; Shopee/Lazada alternates) — the only rows allowed off-RS.
- **Compatibility gates for RS alternatives.** An RS alternative must preserve: the interface the wiring maps resolve (I²C addresses 0x4a / 0x29 on the pointer's shared bus; two OV2640 address straps different; SPI + IRQ/reset for UWB; I²S into the amp pair with an L/R-select pin), the **pre-soldered / no-solder rule** of P2 (fine-pitch bare ICs are excluded even when cheap), and the battery-path protection requirement of T0 ([`bench-tests.md`](bench-tests.md)).
- **Availability badges** (RS search/product pages): *In Stock* = orderable now; *Temporarily out of stock / Sourced on demand / Back order / Last RS stock / Stocked by manufacturer* = orderable with lead-time risk; *Currently unavailable / Supply shortage* = not purchasable today.

## 2. Pinned MCUs (both devices) — not carried on RS

| # | Item (pinned) | Qty | RS PH result (2026-09-28) | Action |
|---|---|---|---|---|
| 1 | ESP32-S3-DevKitC-1 **N16R8** (16 MB flash, 8 MB PSRAM) | 2 (1 per device) | **Not carried.** No Espressif brand exists on RS PH — brand search returns 0 products | **Buy at pinned lane** — e-Gizmo ₱499 ea. (in stock 2026-09-26), Shopee/Lazada alternates |

Closest RS PH items found for completeness — **none is a DevKitC-1 substitute** (no N16R8 variant, different pin maps, and the C6/relay/display boards miss the S3's DVP camera port entirely):

- Seeit ESP32-DEV-38UE / 38VE (870-184 / 870-185, temporarily out of stock, ₱1,266.16 / ₱1,419.39) — generic ESP32-class dev boards.
- Seeed Studio XIAO ESP32S3 voice board (830-759, temporarily out of stock, ₱3,472.40) — S3 silicon, voice-expansion form factor.
- Arduino Nano ESP32, headers-less (268-6963, ABX00092, in stock, ₱1,630.62) — ESP32-S3-based (16 MB flash / 8 MB PSRAM) but a Nano footprint with a different pin map and flashing path; adopting it would rewrite the wiring maps ([`bench-tests.md`](bench-tests.md) §Wiring maps) for no cost gain over the ₱499 devkit.
- Seeit ESP32-C6-DEV28P / DEV30P (870-189 / 870-190, temporarily out of stock) — C6, wrong chip class.
- 4D Systems GEN4-ESP32 starter kits (649-…, in stock, ₱13,300–16,900) — display-integrated kits, wrong class entirely.

## 3. Device electronics (one parts pool — quantities cover both devices)

| # | Item (auris spec) | Qty | RS PH row | RS Stock No. | Status | Unit (₱) | Fit notes |
|---|---|---|---|---|---|---|---|
| 1 | ESP32-S3-DevKitC-1 N16R8 | 2 | — | — | — | — | **Pinned MCU — not on RS** ([§2](#2-pinned-mcus-both-devices--not-carried-on-rs)) |
| 2 | BNO085 breakout (I²C, 9-DoF fusion) | 2 | MikroElektronika Smart DOF 4 Click — BNO085 (MIKROE-6369) | 391-617 | In Stock | 3,757.68 | The pinned sensor on a factory-assembled board: I²C + INT/RST on mikroBUS rows — no-solder rule holds; address family unchanged (0x4a default). Only RS PH listing carrying the BNO085. Alternative if cost matters: Bosch Shuttle Board 3.0 BNO055 (245-7094, in stock, ₱3,370.76) — same role but different sensor + firmware API, so it is a fallback, not the default |
| 3 | VL53L1X ToF with optical cover | 1 | ST VL53L1X-SATEL breakout (VL53L1X-SATEL) | 182-7794 | In Stock | 1,465.18 | Official ST carrier: sensor + cover glass integrated, 10-pin header (I²C + XSHUT/INT) — address 0x29, bench-compatible. The cheaper bare IC VL53L1CXV0FY/1 (175-1110, ₱361.89) is a fine-pitch LGA — **excluded by the no-solder rule** |
| 4 | MAX98357A I²S Class-D mono amp (L/R-select) | 2 | **Not carried on RS PH** | — | — | — | No I²S-input amplifier exists on RS PH: "MAX98357" returns nothing real, and the "I2S amplifier" field is only analog-input amps (LM386 536-1366P ₱88.96, LM4766 534-3318). An analog amp cannot replace the literal I²S path ([README §3.2](../README.md#32-wearable--electronics)); the SPI/I²C-controlled audio DACs on RS (PCM1792A 662-1717, PCM4104 662-1509, MikroE DAC 2 Click 923-5987) are also not I²S-input drop-ins. **Buy at pinned lane** (₱60–120 ea., Shopee/Lazada) |
| 5 | OV2640 DVP camera, 160° wide-FOV, 24-pin (+ FPC breakout adapters) | 2 + 1 spare | **Not carried on RS PH** | — | — | — | No ESP32-DVP-compatible camera modules: "OV2640"/"Arducam" return fuzzy junk, and RS PH's camera-module shelf is Raspberry Pi CSI, USB, or I²C interface classes — wrong capture path for the pinned alternation design ([`bench-tests.md`](bench-tests.md) T7). **Buy at pinned lane** (₱100–200 ea. + adapters) |
| 6 | DWM3000 UWB module (DW3110-based, integrated antenna) | 2 | **Not carried on RS PH** — all RS UWB rows are older DW1000-class silicon | — | — | — | RS PH UWB rows: MikroElektronika UWB Click (**DWM1000**, MIKROE-4199, 216-2643, in stock, ₱6,163.31); M5Stack UWB Unit (U100, 230-5148, in stock, ₱3,801.56 — Ai-Thinker BU01, DW1000-class, confirmed on the product page); Murata LBUA0VG2BP-EVK-P (**Type 2BP** = DW3110, 863-595, in stock, ₱16,924.07 — an EVK, not an integrable module). None matches the pinned DWM3000 — the DWM1000 parts would force a different driver stack and channel plan at the T8 bring-up, so they are **not treated as compatible**. **Buy at pinned lane** (₱700–1,500 ea.) |
| 7 | Momentary push buttons (through-hole, panel class) | 4 | RS PRO Miniature Push Button Switch, Momentary, Panel, 13.6 mm cutout | 734-6704 | In Stock | 212.42 | Panel-mount momentary with solder leads; alternate RS PRO PCB momentary SPDT 6.35 mm (734-6788, ₱226.42) |
| 8 | Wired stereo earphones, 3.5 mm | 2 | **Not carried on RS PH** (wired-3.5 mm class) | — | — | — | RS PH's earphone/headphone shelf is USB and Bluetooth only (EPOS USB 218-205, Panasonic BT 267-5484, 3M ear-defenders 184-8313) — all violate the wired-mandatory rule ([README §2.1](../README.md#21-devices)). **Buy at the pinned consumer lane** (₱100–250 ea.) |

## 4. Battery, power & charge path

| # | Item (auris spec) | Qty | RS PH row | RS Stock No. | Status | Unit (₱) | Fit notes |
|---|---|---|---|---|---|---|---|
| 1 | USB-C charge/protect board (TP4056-C class, protection trips on T0) | 2 | **Not carried on RS PH** | — | — | — | "TP4056" returns fuzzy junk; the charge field is mains chargers and bare charger ICs only (MCP73812T-420I/OT 738-6613P, ₱223.95 — SOT-23-5 fine-pitch, excluded; ST BMS EVKs ₱5k–15k — wrong class). **Buy at pinned lane** (₱25–60 ea.) |
| 2 | 18650 Li-ion cell, protected, name-brand | 2 | **Not carried on RS PH** | — | — | — | No bare 18650 lithium cells on RS PH — only chargers for them (Ansmann 146-6770, ₱1,632.80) and storage boxes. **Buy at pinned lane** (₱120–180 ea.) |
| 3 | 18650 holder (tabbed or spring) | 1 | **Not carried on RS PH** | — | — | — | "18650 cell holder" returns storage boxes (Storacell 915-9785/915-9776) and compartment cases (Bopla 254-3394), not electrical 18650 cradles; RS's battery-holder shelf stops at AA/CR2032/9V/N sizes. **Buy at pinned lane** (₱20–40) |
| 4 | Slim Li-ion pouch (1S, 1000–2000 mAh, protection leads) | 1 | **Not carried on RS PH** | — | — | — | No Li-ion pouch/pack cells — the "lithium polymer" field is Nichicon SLB micro-cells (2.4 V coin-class). **Buy at pinned lane** (₱150–250) |
| 5 | Slide/toggle switch (SPDT, panel class) | 2 | RS PRO PCB Slide Switch SPDT Latching 5 A | 734-7296 | In Stock | 231.91 | Through-hole SPDT slide — panel-mountable power switch; cheaper RS PRO 3 A Nylon alternate 734-7343 (₱161.59); C&K OS102011MA1QN1C (257-0670, ₱252.01) equivalent |
| 6 | 3.5 mm stereo jack (panel/breakout, female) | 1 | **Not clearly carried on RS PH** | — | — | — | RS PH carries 3.5 mm **male** solder plugs (RS PRO 395-1119 ₱98.77 / 395-1131 ₱141.11) and 1/4″ panel females (588-685, wrong size); no clean 3.5 mm female panel/breakout socket at a sane price (Tasker TK146SS 555-032 ₱2,484.44 is unlabelled on RS; Switchcraft C12BX 878-6910 is male). **Buy the breakout at the pinned lane** (₱20–50); if a plug-terminated harness is acceptable instead, 395-1119 is the on-RS fallback |
| 7 | USB-C wall charger ≥ 2 A | 1 | Sanwa Supply USB-C charger, 20 W, 5/9/12 V (ACA-PD102W) | 763-640 | In Stock | 1,226.21 | 5 V @ 3 A on the 20 W profile — charges both devices; replaces the generic consumer charger. 65 W Sanwa (763-646, ₱3,416.17) overkill |

## 5. Bench infrastructure (one-time; serves T0–T8 and the P3/P4 sessions)

| # | Item (auris spec) | Qty | RS PH row | RS Stock No. | Status | Unit (₱) | Fit notes |
|---|---|---|---|---|---|---|---|
| 1 | Solderless breadboard, 830 pt | 2 | Kitronik 2444 Breadboard Prototyping Board, 56.5 × 165.5 mm | 215-3175 | In Stock | 541.77 | The 830-point class (830-pt boards are ~165 × 55 mm); RS PRO 54 × 165 mm (144-718, ₱1,359.93) as alternate |
| 2 | Dupont jumper packs (M-M + M-F) | 2 | Kitronik 4128-40 / 4110-40 200 mm Jumper Wire | 204-8243 / 204-8241 | In Stock | 369.83 / 359.80 | Breadboard jumper wires (M-M class); RS sets also exist (555-912 ₱1,031.83; 350-pc RS PRO set 105-9873 ₱2,986.08). **M-F dupont packs are not clearly carried on RS PH** — if M-F leads are needed for the M-F half of the rule, source that half at the pinned lane or use jumpers + header rows |
| 3 | USB power bank (10 000 mAh class, 5 V USB out) | 2 | SKROSS RELOAD 20, 10 000 mAh, USB-A + USB-C (SKPBR10TRA01CN) | 802-787 | In Stock | 4,362.16 | USB-output bank in the right capacity class — **but RS's spec table lists a 12 V output attribute; verify the 5 V USB profile at checkout before buying** (the Ansmann 1700 series, 859-241/859-239, is a true 12 V-output bank and is **rejected**). The USB-A→C data cables below feed the devkits from it |
| 4 | USB-C data cable | 2 | RS PRO USB-A → USB-C cable | 186-3053 | In Stock | 1,377.92 | Data-capable A-to-C — the flashing/serial path for the DevKitC-1's USB-C ports |
| 5 | Hookup wire set (22–26 AWG, multi-colour) | 1 kit | RS PRO 24 AWG (0.2 mm²) hook-up wire, 100 m reel, per colour | 873-8961 | In Stock | 2,368.04/reel | RS sells per-colour reels, not an assortment kit — buy the colours the harness colour standard needs (20 AWG 2491X series, 361-579 ₱4,104.93, as the heavier alternate) |
| 6 | Spare header packs (2.54 mm) | 1–2 | RS PRO straight through-hole pin header, 36-contact, 2.54 mm | 251-8317 | In Stock | 553.05 | 2.54 mm pitch through-hole; TE AMPMODU 26-way vertical (279-8758, ₱277.20) as alternate |
| 7 | Bench attachment kit (zip ties, velcro, foam tape) | 1 | Velcro hook tape 20 mm × 5 m (618-8869, ₱1,096.00); RS PRO releasable zip ties 150 mm (811-1612, ₱924.47); RS PRO double-sided foam tape 19 mm × 5.5 m (227-901, ₱1,718.49) | 618-8869 / 811-1612 / 227-901 | In Stock | 1,096.00 / 924.47 / 1,718.49 | Industrial pack sizes — same items the P2 rule names, at industrial quantity |
| 8 | Digital multimeter (inline mA for T0 + P3 inspection) | 1 | Fluke 15B | 281-5818 | In Stock | 8,984.00 | Has the mA range the bring-up rule needs; the cheaper Fluke 101 (790-4097, ₱3,823.00) has **no current ranges — rejected** |

## 6. Bench fixtures & consumables (bench-materials §4 ◆ rows)

| Item | RS PH row | RS Stock No. | Status | Unit (₱) | Notes |
|---|---|---|---|---|---|
| Pointer clamp/stand (camera tripod) | Hama 00004105 camera tripod, 1 065 mm | 143-8983 | In Stock | 3,615.40 | T2's clamp/stand; the 125 mm Hama 4175 (656-4853, ₱4,269.01) is the compact alternate |
| Measuring tape, 5 m | RS PRO 5 m tape measure | 254-6241 | In Stock | 468.83 | Ground truth for T2/T6/T8 and the P5 course |
| Digital caliper (optional, T6) | RS PRO 150 mm digital caliper | 243-6615 | In Stock | 8,030.86 | Optional row (₱0–250 on the pinned list) — cheapest decent unit on RS |
| White foam board (T2 surface matrix) | **Not carried** — RS PH's foam is industrial sheet stock (black PE 733-6700 ₱6,484.47) | — | — | — | Buy locally (print/hardware shop, ₱80–150) |
| Dark fabric remnant (T2 surface matrix) | **Not carried** (textile lane) | — | — | — | Buy locally (₱50–150) |
| Head-form fixture (T7) | **Not carried** ("mannequin head" returns unrelated junk) | — | — | — | Printed jig (own filament) or foam head locally, as the bench plan already allows |
| Printed-marker stock (ArUco/AprilTag sheets) | Avery white adhesive label sheets, 20/pack (L4775-20) | 484-7027 | In Stock | 4,762.72 | Printable matte white stock on RS — industrial-priced; the ₱50–150 print-shop lane remains the cheaper default |
| Solder (P3 rework set) | Weller solder wire 0.8 mm (T0051401399) | 244-1545 | In Stock | 1,626.75 | With the iron kit below and the wick/flux lines, this completes the minimal rework set |
| Desoldering wick (P3 rework set) | Super Wick no-clean braid 2.5 mm, 1.5 m | 193-2131 | In Stock | 376.20 | 1.5 mm width alternate 193-2126 (₱331.65) |
| Soldering iron (P3 rework set) | Weller 30 W soldering iron kit | 238-6592 | In Stock | 3,528.64 | Minimal rework/inspection set (entry iron); 80 W kit 238-6597 (₱4,515.36) if headroom wanted |

## 7. What RS PH does not carry (rows that stay at their pinned lanes)

1. **ESP32-S3-DevKitC-1 N16R8** — no Espressif devkits at all ([§2](#2-pinned-mcus-both-devices--not-carried-on-rs)).
2. **MAX98357A I²S amplifier** — no I²S-input amplifier class exists on RS PH; DACs on RS are SPI/I²C-controlled, not I²S-input drop-ins.
3. **OV2640 DVP camera modules (24-pin, 160°) + FPC breakout adapters** — no ESP32-DVP-compatible cameras (RS's camera shelf is RPi-CSI / USB / I²C classes).
4. **DWM3000 / DW3110 UWB modules** — only DW1000-class modules (MikroE UWB Click, M5Stack U100) and a Murata Type 2BP EVK; none matches the pinned DWM3000 driver/channel plan (T8).
5. **TP4056-class USB-C charge/protect boards** — charger ICs (fine-pitch) and mains chargers only.
6. **Bare lithium cells and holders** — no protected 18650 cells, no slim Li-ion pouches, no 18650 cradle holders on RS PH.
7. **Wired 3.5 mm stereo earphones** — RS PH's audio accessories are USB/Bluetooth/defender classes only (wired-mandatory rule violated).

For all seven, the pinned purchase-list lanes (e-Gizmo, Circuitrocks, Shopee/Lazada, consumer lanes) remain the source of record; those rows' totals stay in [`purchase-list.md`](purchase-list.md) §7 unchanged — roughly **₱3,500–6,200** of the original envelope stays off-RS (MCU pair, two camera modules + adapter set, UWB pair, amp pair, charge boards, cells + holder + pouch, earphones, jack breakout).

## 8. RS PH totals for the sourced rows (point-in-time, 2026-09-28)

| Row | Unit (₱) | Qty | Ext (₱) |
|---|---|---|---|
| MikroE Smart DOF 4 Click (BNO085) — 391-617 | 3,757.68 | 2 | 7,515.36 |
| ST VL53L1X-SATEL — 182-7794 | 1,465.18 | 1 | 1,465.18 |
| RS PRO panel push button, momentary — 734-6704 | 212.42 | 4 | 849.68 |
| RS PRO SPDT slide switch — 734-7296 | 231.91 | 2 | 463.82 |
| Sanwa USB-C 20 W charger — 763-640 | 1,226.21 | 1 | 1,226.21 |
| Kitronik 2444 breadboard — 215-3175 | 541.77 | 2 | 1,083.54 |
| Kitronik 200 mm jumper wire — 204-8243 | 369.83 | 2 | 739.66 |
| SKROSS RELOAD 20 power bank — 802-787 | 4,362.16 | 2 | 8,724.32 |
| RS PRO USB-A→C cable — 186-3053 | 1,377.92 | 2 | 2,755.84 |
| RS PRO 24 AWG hookup wire reel (per colour) — 873-8961 | 2,368.04 | 2 | 4,736.08 |
| RS PRO 36-way pin header — 251-8317 | 553.05 | 2 | 1,106.10 |
| Velcro hook tape — 618-8869 | 1,096.00 | 1 | 1,096.00 |
| RS PRO releasable zip ties — 811-1612 | 924.47 | 1 | 924.47 |
| RS PRO double-sided foam tape — 227-901 | 1,718.49 | 1 | 1,718.49 |
| Fluke 15B multimeter — 281-5818 | 8,984.00 | 1 | 8,984.00 |
| Hama camera tripod — 143-8983 | 3,615.40 | 1 | 3,615.40 |
| RS PRO 5 m tape measure — 254-6241 | 468.83 | 1 | 468.83 |
| Weller solder 0.8 mm — 244-1545 | 1,626.75 | 1 | 1,626.75 |
| Super Wick braid — 193-2131 | 376.20 | 1 | 376.20 |
| Weller 30 W iron kit — 238-6592 | 3,528.64 | 1 | 3,528.64 |
| **Total, RS-sourced fixed rows** | | | **≈ ₱53,005** |

Notes on the total:

- Two hookup-wire colours counted (the harness colour standard needs ≥ 2); each additional colour adds ₱2,368.04.
- The optional RS label-sheet row for printed tags (484-7027, ₱4,762.72) and the optional caliper (243-6615, ₱8,030.86) are **excluded** — the print-shop and on-hand lanes cover them cheaper; add them only if the co-researcher wants everything on one RS order.
- This RS-only exercise is a sourcing lane, not a substitute for the purchasing record: the RS rows above are **3–30× the Shopee/e-Gizmo prices** on several rows (the BNO085 Click alone is ~3–5× the pinned-lane pair; power banks, tape, and tape measures are industrial-priced). The blended purchase still follows [`purchase-list.md`](purchase-list.md) — this file tells the co-researcher exactly what an all-RS lane would cost and carry.

## 9. Search log and provenance

- Live-searched on ph.rs-online.com on 2026-09-28, term-by-term in a real browser session (Brave via CDP automation profile). Search terms used, with outcomes: `ESP32-S3`, `ESP32-S3-DevKitC`, `ESP32`, `Espressif` (0), `BNO085`, `BNO055`, `VL53L1X`, `MAX98357`, `I2S amplifier`, `audio DAC module`, `PCM5102` (fuzzy junk), `OV2640` (junk), `Arducam` (junk), `camera module`, `DWM3000` (junk), `UWB`, `Decawave` (junk), `momentary push button`, `stereo earphones`, `TP4056` (junk), `LiPo charger`, `single cell lithium charger`, `battery protection board`, `18650`, `18650 protected`, `18650 holder`, `18650 cell holder`, `lithium polymer battery`, `battery holder`, `slide switch SPDT`, `3.5mm stereo jack`, `3.5 mm jack socket`, `3.5 mm audio socket`, `jack socket`, `USB-C wall charger`, `solderless breadboard`, `jumper wire kit`, `female jumper wire`, `power bank`, `power bank 5V`, `USB-C data cable`, `hookup wire`, `wire assortment`, `pin header 2.54`, `digital multimeter`, `soldering iron kit`, `solder wire`, `desoldering wick`, `velcro tape`, `zip ties`, `double sided foam tape`, `foam board`, `mannequin head` (junk), `sticker paper`, `digital caliper`, `measuring tape`, `camera tripod`.
- Product pages opened to confirm part identity where it mattered: MIKROE-6369 (BNO085), VL53L1X-SATEL, M5Stack U100 (Ai-Thinker BU01), MIKROE-4199 (DWM1000), Kitronik 4128-40/4110-40, Tasker TK146SS, RS PRO hook-up wire (100 m reel), Ansmann/SKROSS power-bank spec tables (the 12 V catch).
- Prices are RS list prices (PHP, exc. VAT) from the search tiles; RS shows quantity breaks — verify at checkout. Search pages are bot-protected, so plain scripted fetches do not see this data — the browser session is the evidence chain for any re-verification.
- If a row changes (stock, price, discontinuation), re-run the listed query on RS PH, update the row and the date here, and log the verified price into [`purchase-list.md`](purchase-list.md) §7 at sign-off. The pinned-lane rows and their prices remain governed by [`purchase-list.md`](purchase-list.md) / [`bench-materials.md`](bench-materials.md).
