# Cane — Hardware components & cost breakdown (pointer + wearable)

> **Board class decided 2026-10-01, purchased and finalised 2026-10-03 (co-researcher): the wearable is a Raspberry Pi 5, 8 GB, and the board is bought (₱6,000).** The purchased SKU is 8 GB; the operative reason was availability at purchase, and the engineering justification is that the resident workload — one or two QVGA CV streams, the renderer, the Wi-Fi AP, and batch-written logs — was already shown to fit **4 GB** with room to spare (~1 GB), so **8 GB is a strict superset and no budget, threshold, risk, or evidence class in this plan changes**. No swap is configured either way (flash endurance on the log card). The board-class question is closed and **no alternative class is carried** ([§4](#4-board-class-closed--no-alternative-class-carried)); the pointer keeps its own Wi-Fi-capable MCU class — an esp32-s3-class devkit — because the two devices differ in power, size, and workload, not because the wearable's class is reversible: it joins the Pi 5's access point over IP ([README §3](../README.md#3-hardware)). The vision tier's **camera count is 1 or 2, still undecided** — settled at P2 from the FOV-geometry and co-residence analysis, confirmed at P4, and free to reduce to one because neither module has been purchased yet.
>
> **Prices:** one row is now an **actual** (the Raspberry Pi 5, 8 GB, ₱6,000, purchased 2026-10-03 — recorded against the bare board; if that purchase later proves to have included the PSU or case, the value is re-split across those rows rather than left double-counted). **Every other row remains an indicative Philippine-local ballpark** (Shopee/Lazada/e-Gizmo/Circuitrocks-class lanes) to be verified when ordered, under the purchase-approval sign-off ([README §9](../README.md#9-open-items)). Totals are therefore **mixed actual-and-estimate ranges**, split spent-vs-to-buy in [§10](#10-totals--contingency-guidance). The itemized cart form of this file (with phase/path consumption tags) lives in [`purchase-list.md`](purchase-list.md); the two files share the same row set and totals.

## 1. Pointer — electronics (its own MCU class, by device design)

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

## 3. Wearable — the selected class rows (raspi-5, 8 GB; board purchased 2026-10-03)

| # | Component & pick | Role (README cross-ref) | Key interface | Qty | Unit ₱ | Subtotal ₱ |
|---|---|---|---|---|---|---|
| 1 | **Raspberry Pi 5 — 8 GB RAM** — ✅ **PURCHASED 2026-10-03, ₱6,000** | wearable compute board — renderer, CV tracking co-resident, link AP | 2× CSI-2, I²S on GPIO, Wi-Fi AP, 5 V rails | 1 | **6,000** *(paid)* | **6,000** *(paid)* |
| 2 | Active cooler (official class) or passive heatsink | head-worn thermal control | FPC thermal mount | 1 | 500–900 | 500–900 |
| 3 | microSD A2-class, 64–128 GB (boot + flash-buffered logs) | OS + data | microSD | 1 | 600–1,000 | 600–1,000 |
| 4 | CSI camera module, wide angle 120° FOV, RGB (Camera Module 3 Wide class, IMX708) | vision tier — **1 or 2, count still undecided**; settled at P2 from the FOV-geometry and co-residence analysis. Two are carried in the cart so either count is buildable, and because **neither has been purchased yet, adopting one camera instead of two costs ₱0** rather than stranding a bought part (dropping to 1 before ordering saves ₱1,700–2,400 + one printed bracket) | CSI-2, **mini 22-pin** (Pi 5 uses the 22-pin connector, not the older 15-pin — the Standard-Mini cable is required; third-party modules may ship the wrong cable) | 2 | 1,700–2,400 | 3,400–4,800 |
| 5 | USB-C PD power bank ≥ 20,000 mAh, **5 V/5 A (25 W+) out profile** | the worn power rail (batteries + charging self-contained) | USB-C out → Pi 5 | 1 | 2,500–5,500 | 2,500–5,500 |
| | **Selected class subtotal** | | | | | **13,000–18,200** |
| 6 | *Optional* — Raspberry Pi 27 W USB-C PSU (current-limited source for first power-up instead of the bank) | bring-up convenience | USB-C PD | 0–1 | 1,000–1,500 | *(not in totals)* |
| 7 | *Optional* — USB-UART dongle (CP2102-class) | serial console debug on the Pi | USB↔UART | 0–1 | 150–300 | *(not in totals)* |

## 4. Board class closed — no alternative class carried

**The wearable's compute class is the purchased Raspberry Pi 5, 8 GB (§3 row 1, bought 2026-10-03 at ₱6,000), and no alternative board class is itemized, priced, or held.** The esp32-s3-class wearable that earlier occupied this section has been retired along with the flip rule it served ([README §3](../README.md#3-hardware) constraint 4). The scored comparison across the four candidate classes remains in [`hw-comparison.md`](hw-comparison.md) as the decision's dated research record — a record of why this class was chosen, not a menu of what else could be bought.

What replaces the retired flip path as the response to a late compute or audio-path failure is **[README §7](../README.md#7-risks-and-limitations) risk 12**: a failure that would once have been answered by changing board class is now answered inside the chosen class, by architectural margin, by feature-level re-scoping (one camera, the detect-then-track cadence, or the USB-audio-dongle audio fallback — all purchase-free, and the camera change ₱0 while neither module is bought), and by the ≥ 2 spare carrier PCBs that absorb a re-fab inside the P3 window ([`assembly.md`](assembly.md) §4).

## 5. Retired bench materials (recorded, not bought — [`bench.md`](bench.md) §3)

With **no hardware bench phase** (decided 2026-10-01), the breadboard infrastructure, test fixtures, T2 surface props, and marker-variant prints are **not bought** and not part of this file's totals. Their former envelope (≈ ₱1,600–3,500) is recorded in [`bench.md`](bench.md) §3, which also holds the bring-up and incoming-inspection checklist — the only hardware-safety gate before P4, served by the tools, print lane, 5 m measuring tape, and PD bank that remain in §§6–8 below. Two former bench rows survive in the P3 lanes: the **printed marker** (dictionary and size pinned at P2, printed with the housings) and the **measuring tape** (§8).

## 6. One-time tools (inspection lanes and P3 rework)

| # | Item | Consumed by | Unit ₱ |
|---|---|---|---|
| 1 | Digital multimeter (must measure current — inline mA for first power-up + no-short rails checks) | P3 incoming inspection + first power-up | 800–1,600 |
| 2 | Adjustable soldering iron kit | P3/P4/P5 trivial rework (per the outsourcing decision) | 800–1,500 |
| 3 | Solder wire (0.8 mm) | P3 rework | 120–250 |
| 4 | Flux pen/jar | P3 rework | 80–150 |
| 5 | Desoldering wick | P3 rework | 60–120 |
| 6 | Hand tool set — strippers / cutters / pliers | P3 harness work | 330–650 |
| 7 | Tweezers | P3 inspection/rework | 80–150 |
| 8 | USB-A→USB-C data cables ×3, data-capable (flashing/serial/logging; the 3rd covers the Pi's power-while-logging case) | flashing + in-study telemetry capture | 450–1,050 |
| | **Tools subtotal** | | **2,720–5,470** |
| 9 | *Optional* — helping-hands/PCB holder | rework convenience | *(not in totals)* |

## 7. Structural & materials

| # | Item | Role | Unit ₱ |
|---|---|---|---|
| 1 | Elastic head strap band (goggle-style, adjustable) | the wearable's carrier | 50–150 |
| 2 | 3D printing — filament or print-service voucher (camera brackets, IMU station, battery/bank cradle, pointer shell, printed ArUco/AprilTag marker) — **bracket count follows the camera-count decision (1 or 2), still open** | all structural parts | 800–2,000 |
| 3 | Fastener set (screws, heat-sets or nuts, in kits) | mounts | 100–250 |
| 4 | Attachment kit — velcro ties, zip ties, double-sided foam tape | cable management + bench + strap stations | 250–500 |
| | **Materials subtotal** | | **1,200–2,900** |

## 8. Evaluation hardware (P5)

| # | Item | Role | Unit ₱ |
|---|---|---|---|
| 1 | Blindfolds (×3: participant + spare + practice) | aid-off condition + localization test | 100–250 |
| 2 | Floor marking tape (course layouts) | P5 course geometry | 120–300 |
| 3 | Measuring tape, 5 m (also the P4 ground truth) | course + P4 | 120–250 |
| 4 | Obstacle props (as-needed; largely on-hand room furniture) | course obstacles | 0–500 |
| | **Evaluation subtotal** | | **340–1,300** |

## 9. P3-folded lanes (quoted at P3 entry, folded into the same sign-off — not cart rows here)

- **Bare carrier PCBs** from a PCB fab house, ordered **with ≥ 2 spares per device** (`assembly.md §§3–4`) — quote and place at the **P2 software-verification exit** (the gate that now clears the design, since there is no breadboard exit).
- **Local hand-solder service** — per-build quote for both devices, booked before P3 entry (`assembly.md §4`).

## 10. Totals & contingency guidance

| Section | Low ₱ | High ₱ |
|---|---|---|
| Pointer electronics | 2,615 | 5,430 |
| Wearable board-agnostic | 1,965 | 4,010 |
| Wearable — selected class (raspi-5, 8 GB; board row is an actual) | 13,000 | 18,200 |
| **Device electronics total** | **17,580** | **27,640** |
| One-time tools | 2,720 | 5,470 |
| Structural & materials | 1,200 | 2,900 |
| Evaluation hardware | 340 | 1,300 |
| **Cart grand total** | **21,840** | **37,310** |

**Cart envelope: ≈ ₱21,800–37,300** as itemized, **≈ ₱25,100–42,900 with a 15 % contingency** — the contingency is guidance for the sign-off, not a committed line. Optional rows (marked *not in totals* in §§2–3) are extra on top, and the retired bench-phase slice ([`bench.md`](bench.md) §3, ≈ ₱1,600–3,500) is outside this envelope **because it is not bought**.

**Spent vs to buy (2026-10-03):**

| | Amount | Rows |
|---|---|---|
| **Spent** | **₱6,000** | the Raspberry Pi 5, 8 GB (§3 row 1) |
| **Still to buy** | **≈ ₱15,800–31,300** | pointer electronics, wearable agnostic rows, and the remaining selected-class rows (cooler, microSD, cameras, PD bank) plus tools, materials, and evaluation hardware |
| Itemized total | ≈ ₱21,800–37,300 | |

**Two procurement notes carried forward.** (i) The board was bought at ₱6,000, consistent with a **grey import carrying no manufacturer warranty** — so the dead-on-arrival lane is *"replace from a local seller,"* not an RMA claim, and it is a logged deviation rather than a warranty recovery ([README §3](../README.md#3-hardware) constraint 3, [§7](../README.md#7-risks-and-limitations) risk 12). (ii) Because only the board is bought, the remaining **Pi-5-pinned rows (cooler, cameras + Standard-Mini cables) are still substitutable at zero sunk cost** until they are ordered — which is what keeps the camera-count decision free.

## 11. Lane & checkout notes

- **Verify at checkout, every row:** current price, stock, and (for modules) that headers/connectors arrive pre-fitted or consumer-ready — no fine-pitch solderable-on-bench parts ([purchase-list.md](purchase-list.md) §1).
- **PD bank:** confirm the output profile does **5 V at 5 A (25 W+)** on USB-C PD in its spec table. The Pi 5's recommended supply is the 27 W / 5.1 V / 5.0 A unit; below 5 A the Pi 5 restricts downstream USB power to 600 mA and flags the current limit at boot, which is unacceptable with the camera + UWB + Wi-Fi load at tracking duty. Note that few ≥ 20 Ah banks deliver 5 A — if none qualifies at order time, the recorded fallback is a protected-cell (18650-class) rail with a 5 V/5 A buck stage — heavier at the head than reusing the PD bank, and the only alternative rail left now that the lighter board-class option is retired (risk 11).
- **Camera cable:** Pi 5 uses the **mini 22-pin** CSI connector, not the 15-pin used on earlier boards — confirm each camera module ships (or is bought with) a Standard-Mini cable; official Camera Module 3 variants do, third-party IMX708 modules often do not.
- **Cameras:** RGB (visible-light) wide-FOV — NoIR variants are wrong for the printed-tag detection; cables ship with the modules but confirm Raspberry Pi 5 cable compatibility.
- **DWM3000 stock** is the spottiest row — if unavailable at checkout time, the DW1000-class module family (e.g. Ai-Thinker BU01 / M5Stack UWB Unit) is the recorded substitute; it works per the SPI+IRQ interface but pins a different driver/channel plan at the tracking gate — flag it in the sign-off if substituted.
- **IMU pair** (pointer + wearable) must be the same part and module revision — buy both from the same listing.
- **Pointer stays esp32-s3-class** even though the wearable is now raspi-5 — the IP-over-Wi-Fi link needs only a 2.4 GHz radio on the pointer side.
- **PD bank doubles as the bench source** for T0-style bring-up and flashing sessions; a separate bench bank is unnecessary.
