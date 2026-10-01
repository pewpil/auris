# Cane — Hardware components & cost breakdown (pointer + wearable)

> **Board class decided 2026-10-01 (co-researcher): the wearable is a Raspberry Pi 5, 4 GB.** 4 GB is ample for the wearable's resident workload — two QVGA CV streams, the renderer, the Wi-Fi AP, and batch-written logs sit well under ~1 GB — and no swap is configured (flash endurance on the log card); the 8 GB variant was not required and stocks less reliably. The board-class question is closed; the esp32-s3-class wearable stays recorded in [§4](#4-wearable--recorded-alternative-esp32-s3-class-flavor-itemized--₱0-in-this-cart) as the priced flip path (₱0 in the cart, ordered only if a bench tripwire forces the class back). The pointer keeps its own Wi-Fi-capable MCU class — an esp32-s3-class devkit — which is what makes the class reversible: it joins the Pi 5's access point over IP ([README §3](../README.md#3-hardware)). The vision tier's **camera count is 1 or 2**, adopted at the T7 placement gate; both modules sit in the cart so either count is buildable without a re-buy.
>
> **Prices are Philippine-local ballparks** (Shopee/Lazada/e-Gizmo/Circuitrocks-class lanes), point-in-time estimates to be **verified at checkout**; the purchase-approval sign-off ([README §9](../README.md#9-open-items)) precedes every checkout. Totals are ranges — nothing here is an approved spend until that sign-off. The itemized cart form of this file (with phase/path consumption tags) lives in [`purchase-list.md`](purchase-list.md); the two files share the same row set and totals.

## 1. Pointer — electronics (its own MCU class; no flip contingency)

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

## 2. Wearable — board-agnostic rows (survive a class flip at no re-buy)

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

## 3. Wearable — the selected class rows (raspi-5, 4 GB)

| # | Component & pick | Role (README cross-ref) | Key interface | Qty | Unit ₱ | Subtotal ₱ |
|---|---|---|---|---|---|---|
| 1 | **Raspberry Pi 5 — 4 GB RAM** | wearable compute board — renderer, CV tracking co-resident, link AP | 2× CSI-2, I²S on GPIO, Wi-Fi AP, 5 V rails | 1 | 8,500–10,500 | 8,500–10,500 |
| 2 | Active cooler (official class) or passive heatsink | head-worn thermal control | FPC thermal mount | 1 | 500–900 | 500–900 |
| 3 | microSD A2-class, 64–128 GB (boot + flash-buffered logs) | OS + data | microSD | 1 | 600–1,000 | 600–1,000 |
| 4 | CSI camera module, wide angle 120° FOV, RGB (Camera Module 3 Wide class, IMX708) | vision tier — **1 or 2** modules; the count is adopted at the T7 placement gate, and two sit in the cart so either count is buildable without a re-buy (fixing it at 1 before checkout saves ₱1,700–2,400 + one printed bracket) | CSI-2, **mini 22-pin** (Pi 5 uses the 22-pin connector, not the older 15-pin — the Standard-Mini cable is required; third-party modules may ship the wrong cable) | 2 | 1,700–2,400 | 3,400–4,800 |
| 5 | USB-C PD power bank ≥ 20,000 mAh, **5 V/5 A (25 W+) out profile** | the worn power rail (batteries + charging self-contained) | USB-C out → Pi 5 | 1 | 2,500–5,500 | 2,500–5,500 |
| | **Selected class subtotal** | | | | | **15,500–22,700** |
| 6 | *Optional* — Raspberry Pi 27 W USB-C PSU (bench-only supply for flashing/soak instead of the bank) | bench convenience | USB-C PD | 0–1 | 1,000–1,500 | *(not in totals)* |
| 7 | *Optional* — USB-UART dongle (CP2102-class) | serial console debug on the Pi | USB↔UART | 0–1 | 150–300 | *(not in totals)* |

## 4. Wearable — recorded alternative: esp32-s3-class flavor (itemized; ₱0 in this cart)

The complete esp32-s3-class wearable, filled in so the flip is a **priced, executable path** rather than a stub: never purchased under the current raspi-5 decision; bought only if a bring-up tripwire forces the class back. Everything in the pointer (§1) and the board-agnostic rows (§2) carries over untouched — so the flip is a software-risk question, not a procurement one.

| #   | Row                                                  | Function                                                                                                         | esp32-s3-class flavor                                                                                                               | Qty | Unit ₱  | Subtotal ₱      |
| --- | ---------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------- | --- | ------- | --------------- |
| 1   | Wearable compute board — the alternative's class row | renders the audio; runs the CV tracking; hosts the 2.4 GHz link radio; provides I²C/SPI/USB to the agnostic rows | ESP32-S3-class devkit **N16R8** (16 MB flash / 8 MB PSRAM — the PSRAM is what makes the dual QVGA frame buffers possible)           | 1   | 500–700 | 500–700         |
| 2   | 2× wide-FOV RGB cameras (B)                          | the vision tier                                                                                                  | OV2640-class **DVP** modules, 160° FOV (2 + 1 spare)                                                                                | 3   | 100–200 | 300–600         |
| 3   | Camera interface adapters (B)                        | the DVP modules arrive as 24-pin FPC; the devkit's single parallel port takes headers                            | FPC-to-header breakout adapter sets (one per camera + spare)                                                                        | 2–3 | 50–150  | 100–300         |
| 4   | Capture contingency (B, conditional)                 | only if the T7 gate forces *simultaneous* two-camera capture on the single DVP port (risk 8)                     | DVP 2:1 camera mux / dual-capture switch module                                                                                     | 0–1 | 150–600 | *(optional)*    |
| 5   | Stereo audio output (B)                              | spatial audio to the wired 3.5 mm jack                                                                           | the **same PCM5102A module** (§2 row 5) rides the native I²S peripheral — no new hardware, different wiring only                    | —   | —       | **0**           |
| 6   | Worn power rail (B)                                  | the worn power                                                                                                   | either **reuse the PD bank** (§3 row 5 — ₱0, oversized at the head) or a **protected 18650 + cradle holder** rail (lighter to wear) | 0–1 | 300–600 | *(alternative)* |
| 7   | Camera mounts & wiring (B)                           | stable extrinsics + DVP leads                                                                                    | printed brackets per the DVP module shape (charged to the §7 print lane) + M–F leads to the devkit header                           | 1   | 0–300   | 0–300           |
| 8   | Thermal                                              | head-worn heat                                                                                                   | **none** — the S3 class is cool to skin; no heatsink, no cooler (a mass advantage over the raspi-5 class)                           | —   | —       | **0**           |

**Cost of reversal:** base path (rows 1–3) ≈ **₱900–1,600**; with mounts/wiring (row 7) ≈ **₱900–1,900**; add the mux contingency (row 4) and the slimmer 18650 rail (row 6) only if those gates demand them, for a worst case of ≈ **₱2,800–4,100** of new hardware. The audio module, thermal, and every agnostic/pointer row survive the flip at ₱0.

**Engineering burden carried by this path** (why the T7 gate decides before the permanent build):

- **Two LX7 cores turn co-residence into a scheduling problem:** the renderer pins to core 0 (≈ 0.2–0.5 ms per 128-sample hop at 48 kHz — comfortably inside the audio budget), the vision pipeline to core 1; Wi-Fi/link and logging ride below both.
- **The honest compute boundary:** AprilTag-class QVGA detection runs ≈ 30–100 ms per frame *per camera* on this silicon, so **two cameras detecting every frame at 15–30 Hz do not fit** — and a single camera, the S3-friendly configuration (also the one the T7 count gate may adopt here), is the only one that leaves real headroom. The plan's **detect-then-track** scheme (detect every Nth frame, track a small ROI between) is mandatory on this class, not a fallback — and the T7 co-residence gate (risk 8) settles it, together with the capture strategy that decides whether row 4's mux is needed.
- **Memory plan:** the 8 MB PSRAM holds the DVP DMA'd QVGA buffers in double-buffering; the 512 KB internal SRAM must cover Wi-Fi + the audio ring + task stacks.
- **Frame-budget reading:** at 48 kHz / 128 samples the buffer period is 2.67 ms, so the plan's render gate is **≤ 2 ms per 128-sample hop** ([README §4.2](../README.md#42-latency-budget-motion-to-sound-target--100-ms)); the S3 render core fits with margin.
- **Ergonomics upside:** no cooler and an optional slimmer rail make this the lighter wearable build — the flip's main argument for the user's own comfort, against the compute headroom the raspi-5 class buys.

## 5. Bench-conditional materials (itemized in [`bench.md`](bench.md))

The breadboard infrastructure, test fixtures, T2 surface props, and marker-variant prints that the **hardware bench path** consumes are no longer part of this file's totals — they are itemized with their own envelope (≈ ₱1,600–3,500) in [`bench.md`](bench.md), held conditional on the bench-phase decision ([`bench-tests.md`](bench-tests.md) §1). The shared rows the bench reuses (tools, print lane, 5 m measuring tape, power bank) remain in §§6–8 below.

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
| 8 | USB-A→USB-C data cables ×3, data-capable (flashing/serial/logging; the 3rd covers the Pi's power-while-logging case) | both paths | 450–1,050 |
| | **Tools subtotal** | | **2,720–5,470** |
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
| 3 | Measuring tape, 5 m (shared with the bench ground truth — [`bench.md`](bench.md) §4) | course + T2/T6/T8 | 120–250 |
| 4 | Obstacle props (as-needed; largely on-hand room furniture) | course obstacles | 0–500 |
| | **Evaluation subtotal** | | **340–1,300** |

## 9. P3-folded lanes (quoted at P3 entry, folded into the same sign-off — not cart rows here)

- **Bare carrier PCBs** from a PCB fab house, ordered **with ≥ 2 spares per device** (`assembly.md §§3–4`) — quote at the breadboard-equivalent exit of whichever bench path runs.
- **Local hand-solder service** — per-build quote for both devices, booked before P3 entry (`assembly.md §4`).

## 10. Totals & contingency guidance

| Section | Low ₱ | High ₱ |
|---|---|---|
| Pointer electronics | 2,615 | 5,430 |
| Wearable board-agnostic | 1,965 | 4,010 |
| Wearable — selected class (raspi-5, 4 GB) | 15,500 | 22,700 |
| **Device electronics total** | **20,080** | **32,140** |
| One-time tools | 2,720 | 5,470 |
| Structural & materials | 1,200 | 2,900 |
| Evaluation hardware | 340 | 1,300 |
| **Cart grand total** | **24,340** | **41,810** |

**Indicative cart envelope: ≈ ₱24,300–41,800** (as itemized), **≈ ₱28,000–48,100 with a 15 % contingency** — a contingency allowance is guidance for the sign-off, not a committed line. Optional rows (marked *not in totals* in §§2–3) are extra on top, and the bench-phase slice ([`bench.md`](bench.md), ≈ ₱1,600–3,500) is outside this envelope by design.

## 11. Lane & checkout notes

- **Verify at checkout, every row:** current price, stock, and (for modules) that headers/connectors arrive pre-fitted or consumer-ready — no fine-pitch solderable-on-bench parts ([purchase-list.md](purchase-list.md) §1).
- **PD bank:** confirm the output profile does **5 V at 5 A (25 W+)** on USB-C PD in its spec table. The Pi 5's recommended supply is the 27 W / 5.1 V / 5.0 A unit; below 5 A the Pi 5 restricts downstream USB power to 600 mA and flags the current limit at boot, which is unacceptable with the camera + UWB + Wi-Fi load at tracking duty. Note that few ≥ 20 Ah banks deliver 5 A — if none qualifies at checkout, the recorded fallback is a protected-cell (18650-class) rail with a 5 V/5 A buck stage (heavier, and the S3 flip path's rail is the lighter option).
- **Camera cable:** Pi 5 uses the **mini 22-pin** CSI connector, not the 15-pin used on earlier boards — confirm each camera module ships (or is bought with) a Standard-Mini cable; official Camera Module 3 variants do, third-party IMX708 modules often do not.
- **Cameras:** RGB (visible-light) wide-FOV — NoIR variants are wrong for the printed-tag detection; cables ship with the modules but confirm Raspberry Pi 5 cable compatibility.
- **DWM3000 stock** is the spottiest row — if unavailable at checkout time, the DW1000-class module family (e.g. Ai-Thinker BU01 / M5Stack UWB Unit) is the recorded substitute; it works per the SPI+IRQ interface but pins a different driver/channel plan at the tracking gate — flag it in the sign-off if substituted.
- **IMU pair** (pointer + wearable) must be the same part and module revision — buy both from the same listing.
- **Pointer stays esp32-s3-class** even though the wearable is now raspi-5 — the IP-over-Wi-Fi link needs only a 2.4 GHz radio on the pointer side.
- **PD bank doubles as the bench source** for T0-style bring-up and flashing sessions; a separate bench bank is unnecessary.
