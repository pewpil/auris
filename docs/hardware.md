# Cane — Hardware components & cost breakdown (pointer + wearable)

> **Board class decided 2026-09-29 (co-researcher): the wearable is a Raspberry Pi 5, 8 GB.** The dual-compatible screening criterion and the deferred-class option are settled by this selection; the esp32-s3-class flavor stays recorded in [§4](#4-wearable--recorded-alternative-esp32-s3-class-flavor) as the validated alternative (₱0 in the cart, re-priced only if the class decision is ever revisited after a bring-up tripwire). The pointer keeps its own Wi-Fi-capable MCU class — an esp32-s3-class devkit, which reaches either class over the IP-over-Wi-Fi link ([README §3](../README.md#3-hardware)).
>
> **Prices are Philippine-local ballparks** (Shopee/Lazada/e-Gizmo/Circuitrocks-class lanes), point-in-time estimates to be **verified at checkout**; the purchase-approval sign-off ([README §9](../README.md#9-open-items)) precedes every checkout. Totals are ranges — nothing here is an approved spend until that sign-off. The itemized cart form of this file (with phase/path consumption tags) lives in [`purchase-list.md`](purchase-list.md); the two files share the same row set and totals.

## 1. Pointer — electronics (single tranche; no dual-class requirement)

| # | Component & pick | Role (README cross-ref) | Key interface | Qty | Unit ₱ | Subtotal ₱ |
|---|---|---|---|---|---|---|
| 1 | ESP32-S3-class devkit (16 MB flash / 8 MB PSRAM class, e.g. DevKitC-1 N16R8) | pointer MCU — fusion + ToF driver + link sender | 2.4 GHz Wi-Fi, I²C, SPI + IRQ, USB CDC | 1 | 500–700 | 500–700 |
| 2 | BNO085 9-DoF IMU breakout (I²C; AHRS-capable, hall orientation fused onboard) | pointer orientation | I²C (shared bus) | 1 | 600–1,100 | 600–1,100 |
| 3 | VL53L1X-class ToF module (with optical cover glass) | hit distance $d$ (~4 m room-scale class) | I²C (shared bus, addr 0x29 class) | 1 | 300–600 | 300–600 |
| 4 | DWM3000-class UWB module (DW3110-based, integrated antenna) | tracking tag (pair, fallback tier) | SPI + IRQ/reset | 1 | 900–1,800 | 900–1,800 |
| 5 | Momentary push button (panel class, through-hole) | press-and-hold trigger | GPIO | 1 | 15–50 | 15–50 |
| 6 | Slim Li-ion pouch 1S (1,500–2,000 mAh, with protection leads) | pointer battery | USB-C charge lane | 1 | 180–350 | 180–350 |
| 7 | USB-C charge/protect board (TP4056-C class) | pointer charge + protection | USB-C | 1 | 40–100 | 40–100 |
| 8 | Slide/toggle switch (SPST/SPDT panel class) | pointer power switch | panel wiring | 1 | 30–80 | 30–80 |
| 9 | Printed ArUco/AprilTag marker (print-shop sheet) | vision tier's passive target | none (passive) | 1 | 50–150 | 50–150 |
| | **Pointer subtotal** | | | | | **2,615–5,430** |

## 2. Wearable — board-agnostic rows (serve either class; unaffected by the board decision)

| # | Component & pick | Role (README cross-ref) | Key interface | Qty | Unit ₱ | Subtotal ₱ |
|---|---|---|---|---|---|---|
| 1 | BNO085 9-DoF IMU breakout (same part as the pointer's) | head orientation | I²C at 3.3 V | 1 | 600–1,100 | 600–1,100 |
| 2 | UWB anchor module (same DWM3000-class family as the tag; pair) | tracking anchor (fallback tier) | SPI + IRQ/reset | 1 | 900–1,800 | 900–1,800 |
| 3 | Momentary push button (panel class) | per-doning re-zero | GPIO | 1 | 15–50 | 15–50 |
| 4 | 3.5 mm stereo jack — panel/breakout, female | wired earphone terminus | passive audio | 2 | 30–80 | 60–160 |
| 5 | PCM5102A stereo I²S DAC module (1 + 1 spare) | spatial-audio output path | I²S | 2 | 120–250 | 240–500 |
| 6 | Wired stereo earphones, 3.5 mm plug | user's audio apparatus | 3.5 mm | 1 | 150–400 | 150–400 |
| | **Board-agnostic subtotal** | | | | | **1,965–4,010** |
| 7 | *Optional* — headphone amp mini-board (opamp/LDO class) | drive headroom behind the DAC if line-out proves weak | analog | 0–1 | 80–200 | *(not in totals)* |

## 3. Wearable — raspi-5, 8 GB class rows (the selected tranche)

| # | Component & pick | Role (README cross-ref) | Key interface | Qty | Unit ₱ | Subtotal ₱ |
|---|---|---|---|---|---|---|
| 1 | **Raspberry Pi 5 — 8 GB RAM** | wearable compute board — renderer, CV tracking co-resident, link AP | 2× CSI-2, I²S on GPIO, Wi-Fi AP, 5 V rails | 1 | 10,000–12,500 | 10,000–12,500 |
| 2 | Active cooler (official class) or passive heatsink | head-worn thermal control | FPC thermal mount | 1 | 500–900 | 500–900 |
| 3 | microSD A2-class, 64–128 GB (boot + flash-buffered logs) | OS + data | microSD | 1 | 600–1,000 | 600–1,000 |
| 4 | CSI camera module, wide angle 120° FOV, RGB (Camera Module 3 Wide class, IMX708) | vision tier ×2 (head-flanking stations) | CSI-2 (cable ships with module) | 2 | 1,700–2,400 | 3,400–4,800 |
| 5 | USB-C PD power bank ≥ 20,000 mAh, **5 V/3 A-capable out profile** | the worn power rail (batteries + charging self-contained) | USB-C out → Pi 5 | 1 | 1,800–3,200 | 1,800–3,200 |
| | **raspi-5 class subtotal** | | | | | **16,300–22,400** |
| 6 | *Optional* — Raspberry Pi 27 W USB-C PSU (bench-only supply for flashing/soak instead of the bank) | bench convenience | USB-C PD | 0–1 | 1,000–1,500 | *(not in totals)* |
| 7 | *Optional* — USB-UART dongle (CP2102-class) | serial console debug on the Pi | USB↔UART | 0–1 | 150–300 | *(not in totals)* |

## 4. Wearable — recorded alternative: esp32-s3-class flavor (₱0 in this cart)

Kept for provenance and fast reversal. Never purchased under the current decision; re-priced only if a bring-up tripwire forces the class back to esp32-s3.git

| Component | Flavor spec (repo-recorded alternative) | Indicative price if re-activated |
|---|---|---|
| Wearable compute board | esp32-s3-class devkit (16 MB flash / 8 MB PSRAM) | 500–700 |
| 2× wide-FOV cameras | OV2640-class DVP modules, 160° FOV + FPC breakout adapters (alternate-frame/mux capture at the tracking gate) | 100–200 ea + adapters |
| Worn power rail | protected 18650 cell + cradle holder rail (or the same PD bank — both serve) | 300–600 |

## 5. Bench-conditional infrastructure (consumed only by the hardware bench path)

| # | Component & pick | Consumed by | Qty | Unit ₱ | Subtotal ₱ |
|---|---|---|---|---|---|
| 1 | Solderless breadboard, 830-pt class | T0–T8 breadboard assembly | 2 | 150–250 | 300–500 |
| 2 | Dupont jumper packs (M–M and M–F) | all breadboard wiring | 2 | 120–250 | 240–500 |
| 3 | Hookup wire assortment kit (multi-colour, 22–26 AWG) | harness revisions | 1 | 250–500 | 250–500 |
| 4 | Pin header pack, 2.54 mm | module rows | 1 | 100–200 | 100–200 |
| 5 | Camera tripod / clamp stand (pointer ToF fixture) | T2 ranging fixture | 1 | 300–800 | 300–800 |
| 6 | Measuring tape 5 m | T2/T6/T8 ground truth; P5 course | 1 | 120–250 | 120–250 |
| 7 | T2 surface set — white foam board + dark fabric remnant | T2 surface matrix | 1 | 100–250 | 100–250 |
| 8 | Head-form fixture for T7 (printed jig via the P5 print lane) | T7 camera geometry | 1 | 100–300 | 100–300 |
| | **Bench-conditional subtotal** | | | | **1,560–3,450** |

## 6. One-time tools (both paths keep the inspection lanes; solder kit serves P3 rework)

| # | Item | Consumed by | Unit ₱ |
|---|---|---|---|
| 1 | Digital multimeter (must measure current — inline mA for bring-up + no-short rails checks) | both paths (P2 bench / P3 incoming inspection) | 800–1,600 |
| 2 | Adjustable soldering iron kit | P3/P4/P5 trivial rework (per the outsourcing decision) | 800–1,500 |
| 3 | Solder wire (0.8 mm) | P3 rework | 120–250 |
| 4 | Flux pen/jar | P3 rework | 80–150 |
| 5 | Desoldering wick | P3 rework | 60–120 |
| 6 | Hand tool set — strippers / cutters / pliers | P2/P3 harness work | 330–650 |
| 7 | Tweezers | P3 inspection/rework | 80–150 |
| 8 | USB-A→USB-C data cables, data-capable (flashing/serial/logging) | both paths | 150–350 ×2 = 300–700 |
| | **Tools subtotal** | | **2,570–5,120** |
| 9 | *Optional* — helping-hands/PCB holder | rework convenience | *(not in totals)* |

## 7. Structural & materials

| # | Item | Role | Unit ₱ |
|---|---|---|---|
| 1 | Elastic head strap band (goggle-style, adjustable) | the wearable's carrier | 50–150 |
| 2 | 3D printing — filament or print-service voucher (camera brackets, IMU station, battery/bank cradle, pointer shell, head-form jig) | all structural parts | 800–2,000 |
| 3 | Fastener set (screws, heat-sets or nuts, in kits) | mounts | 100–250 |
| 4 | Attachment kit — velcro ties, zip ties, double-sided foam tape | cable management + bench + strap stations | 250–500 |
| | **Materials subtotal** | | **1,200–2,900** |

## 8. Evaluation hardware (P5)

| # | Item | Role | Unit ₱ |
|---|---|---|---|
| 1 | Blindfolds (×3: participant + spare + practice) | aid-off condition + localization test | 100–250 |
| 2 | Floor marking tape (course layouts) | P5 course geometry | 120–300 |
| 3 | Obstacle props (as-needed; largely on-hand room furniture) | course obstacles | 0–500 |
| | **Evaluation subtotal** | | **220–1,050** |

## 9. P3-folded lanes (quoted at P3 entry, folded into the same sign-off — not cart rows here)

- **Bare carrier PCBs** from a PCB fab house, ordered **with ≥ 2 spares per device** (`assembly.md §§3–4`) — quote at the breadboard-equivalent exit of whichever bench path runs.
- **Local hand-solder service** — per-build quote for both devices, booked before P3 entry (`assembly.md §4`).

## 10. Totals & contingency guidance

| Section | Low ₱ | High ₱ |
|---|---|---|
| Pointer electronics | 2,615 | 5,430 |
| Wearable board-agnostic | 1,965 | 4,010 |
| Wearable raspi-5 8 GB class | 16,300 | 22,400 |
| **Device electronics total** | **20,880** | **31,840** |
| Bench-conditional infrastructure | 1,560 | 3,450 |
| One-time tools | 2,570 | 5,120 |
| Structural & materials | 1,200 | 2,900 |
| Evaluation hardware | 220 | 1,050 |
| **Cart grand total** | **26,430** | **44,360** |

**Indicative cart envelope: ≈ ₱26,400–44,400** (as itemized), **≈ ₱30,400–51,000 with a 15 % contingency** — a contingency allowance is guidance for the sign-off, not a committed line. Optional rows ([§2.7](#2-wearable--board-agnostic-rows-either-class-unaffected-by-the-board-decision), [§3.6–7](#3-wearable--raspi-5-8-gb-class-rows-the-selected-tranche), [§6.9](#6-one-time-tools-both-paths-keep-the-inspection-lanes-solder-kit-serves-p3-rework)) are extra on top.

## 11. Lane & checkout notes

- **Verify at checkout, every row:** current price, stock, and (for modules) that headers/connectors arrive pre-fitted or consumer-ready — no fine-pitch solderable-on-bench parts ([purchase-list.md](purchase-list.md) §1).
- **PD bank:** confirm the output profile does 5 V at ≥ 3 A on USB-C PD in its spec table (many banks fall back to 2 A output — that underpowers a Pi 5 with cameras; a ≥ 3 A profile runs the Pi 5 headless-class compliantly for our draw).
- **Cameras:** RGB (visible-light) wide-FOV — NoIR variants are wrong for the printed-tag detection; cables ship with the modules but confirm Raspberry Pi 5 cable compatibility.
- **DWM3000 stock** is the spottiest row — if unavailable at checkout time, the DW1000-class module family (e.g. Ai-Thinker BU01 / M5Stack UWB Unit) is the recorded substitute; it works per the SPI+IRQ interface but pins a different driver/channel plan at the tracking gate — flag it in the sign-off if substituted.
- **IMU pair** (pointer + wearable) must be the same part and module revision — buy both from the same listing.
- **Pointer stays esp32-s3-class** even though the wearable is now raspi-5 — the IP-over-Wi-Fi link needs only a 2.4 GHz radio on the pointer side.
- **PD bank doubles as the bench source** for T0-style bring-up and flashing sessions; a separate bench bank is unnecessary.
