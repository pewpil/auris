# Cane — Bench materials (the bench-phase slice)

> The physical materials the **hardware bench path** ([`bench-tests.md`](bench-tests.md) §1, "Path A") needs to stand up the T0–T8 matrix on breadboards. **Conditional**: these rows are needed only if the thesis executes the physical bench phase; under the software/data-sheet path ([`bench-tests.md`](bench-tests.md) §2) they stay unbought. The **device components the bench consumes are already itemized in [`purchase-list.md`](purchase-list.md)** (boards, IMUs, ToF, UWB pair, DAC, cameras, jack, buttons, batteries, power bank) — not duplicated here. This file is the bench-only delta: breadboard infrastructure, fixtures, test props, printed marker variants, and the on-hand inventory. Prices are Philippine-local ballparks, **verified at checkout**; the purchase-approval sign-off ([README §9](../README.md#9-open-items)) governs both files.

## 1. Rules this slice binds itself to

- **Buy-once rule** — the slice is one purchase; genuinely consumable rows (tape, zip ties) may be re-bought as they deplete, at trivial cost.
- **Connector-ready rule (unchanged)** — every module attached at the bench arrives with headers/connectors pre-fitted; the bench attach stays **non-permanent** (breadboards, jumpers, ties, tape, friction mounts) and involves no soldering of any kind ([bench-tests.md](bench-tests.md) §1).
- **Nothing here is on the device-critical path** — the P3 soldered build, P4 calibration, and P5 study never need the breadboards; the tools in [`purchase-list.md`](purchase-list.md) §6 and the attachment/print materials in §7 are shared and stay in the main cart.
- **Harness software is a build item, not a material** — `bench/capture.py`, `bench/runbooks/<test>.yaml`, and `bench/reduce.py` are written at P2 entry, not bought.

## 2. Breadboard & wiring infrastructure (`P2-path-A`)

| # | Item | Consumed by | Qty | Unit ₱ | Subtotal ₱ |
|---|---|---|---|---|---|
| 1 | Solderless breadboard, 830-pt class | both device breadboards | 2 | 150–250 | 300–500 |
| 2 | Dupont jumper packs (M–M and M–F combined) | all breadboard wiring | 2 | 120–250 | 240–500 |
| 3 | Hookup wire assortment (multi-colour, 22–26 AWG) | wiring revisions | 1 | 250–500 | 250–500 |
| 4 | Pin header pack, 2.54 mm | module rows, breakouts | 1 | 100–200 | 100–200 |
| 5 | M–F jumper set for the Pi 5's GPIO header (cameras, I²S DAC, UWB SPI rows) | wearable breadboard | 1 | 120–250 | 120–250 |
| | | | | **Subtotal** | **1,010–1,950** |

## 3. Test fixtures (`P2-path-A`)

| # | Item | Consumed by | Qty | Unit ₱ | Subtotal ₱ |
|---|---|---|---|---|---|
| 1 | Camera tripod / clamp stand (pointer ToF fixture) | T2 | 1 | 300–800 | 300–800 |
| 2 | Head-form fixture — printed jig (charged to the print lane in [`purchase-list.md`](purchase-list.md) §7) | T7 camera geometry | 1 | 100–300 | 100–300 |
| 3 | Printed marker variants — candidate dictionary/size sheets for the T7 pin (print shop) | T7 | 1 | 50–150 | 50–150 |
| | | | | **Subtotal** | **450–1,250** |

## 4. Test props & ground truth (`P2-path-A`)

| # | Item | Consumed by | Qty | Unit ₱ | Subtotal ₱ |
|---|---|---|---|---|---|
| 1 | White foam board (reflectant surface) | T2 surface matrix | 1 | 50–150 | 50–150 |
| 2 | Dark fabric remnant (dark surface) | T2 surface matrix | 1 | 50–150 | 50–150 |
| 3 | Cardboard surface (mid-reflectance) | T2 surface matrix | 1 | 0 | on-hand |
| 4 | Measuring tape, 5 m | T2/T6/T8 ground truth | — | *shared* | bought in [`purchase-list.md`](purchase-list.md) §8 (the P5 course reuses it) |
| | | | | **Subtotal** | **100–300** |

## 5. Bench power & bring-up (cross-reference — no new spend)

- **Current-limited bench source** = the wearable's USB-PD power bank ([`purchase-list.md`](purchase-list.md) §4) — the same bank charged at the strap station is the bench's safe rail for T0-style bring-up and flashing sessions.
- **Inline mA measurement** = the digital multimeter ([`purchase-list.md`](purchase-list.md) §6).
- **Data + power leads** = the USB-A→USB-C data cable set ([`purchase-list.md`](purchase-list.md) §6; the third cable added for the Pi's simultaneous power-while-logging case).
- *Optional add-on:* inline USB power meter, ₱200–400 (excluded from the envelope — the multimeter covers the bring-up rule).

## 6. On-hand inventory (no spend — the fixed, selection-independent portion)

- **Laptop** — flashing both boards, serial monitoring, and the per-session CSV logging harness ([`bench-tests.md`](bench-tests.md) § Logging format: `bench/capture.py` → `bench/reduce.py`).
- **Phone** — inclinometer reference (T1), lux reading (T7's low-light record), timing.
- **Room furniture** — a level surface (T1), a steel table or rebar piece as the ferromagnetic disturbance source (T1), a known-pose obstacle (T5).
- **Space** — a ≥ 10 m clear run for the T3 distance walks; a dimmable room corner for the T7 low-light sweep.

## 7. Bench-slice envelope (only if Path A runs)

| Section | Low ₱ | High ₱ |
|---|---|---|
| Breadboard & wiring infrastructure | 1,010 | 1,950 |
| Test fixtures | 450 | 1,250 |
| Test props & ground truth | 100 | 300 |
| **Bench-slice total** | **1,560** | **3,500** |

Indicative envelope **≈ ₱1,600–3,500**, held separately from the project cart in [`purchase-list.md`](purchase-list.md) (which the bench slice's shared rows — tools, print lane, tape, power bank — already count). Optional power meter extra on top.

## 8. Checkout reminders

- **Hold the slice** until the Path A / Path B decision lands; the bench is not on the device-critical path.
- Connector-ready check unchanged from the main cart (headers pre-fitted; no fine-pitch bench-solderable parts).
- Confirm the breadboard point count (830-pt class, ~165 × 55 mm) and that the M–F jumper half exists for the Pi header row.
- Route the head-form jig and the marker-variant sheets through the same print/print-shop lane as the housings ([`purchase-list.md`](purchase-list.md) §7).
- If the T7 gate later pins a dictionary/size that the pre-printed variants don't cover, the marker re-print is a trivial consumable, not a buy-once violation.
