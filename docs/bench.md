# Cane — First-power-up bring-up & incoming inspection (the retired bench slice, retained)

> **Decision 2026-10-01: there is no hardware bench phase.** The breadboard/fixture/prop rows that used to be the bench-phase material slice are **not bought**. This file keeps them recorded (nothing deleted — see §3) so the reasoning survives and so the slice can be priced again cheaply if the decision is ever revisited, but the operative content is now §1: the **bring-up and incoming-inspection checklist**, which under [`bench-tests.md`](bench-tests.md) is the project's *only* hardware-safety gate before P4 calibration. The **device components are already itemized in [`purchase-list.md`](purchase-list.md)** (boards, IMUs, ToF, UWB pair, DAC, cameras, jack, buttons, batteries, power bank) — not duplicated here. Prices are Philippine-local ballparks, **verified at checkout**; the purchase-approval sign-off ([README §9](../README.md#9-open-items)) governs.

## 1. The bring-up & inspection checklist (operative — no new spend)

Applied at the P3 incoming inspection and again at first power-up ([`bench-tests.md`](bench-tests.md) "Bring-up safety"):

1. **Visual** — solder joints, connectors seated, no stray strands or debris; nothing from the service powered before inspection passes.
2. **Per-net continuity and no-short rails** — each rail checked against ground before any module attaches.
3. **Battery-path polarity** — checked twice; a bare cell is never parked on metal.
4. **Current-limited first power-up** — multimeter in-line (mA range) or a current-limited USB source; brownout-type resets on a weak source are source behaviour, not device failure.
5. **Rail verification with the multimeter** *before* modules attach — on the raspi-5 wearable also the boot/shutdown discipline (never cut power to a writing filesystem).
6. **One firmware-flash / logger-visibility check** — `bench-log` stream live on both devices.
7. **Then the P3 functional smoke check** ([`bench-tests.md`](bench-tests.md)) — one end-to-end audible placement run. This is build acceptance, not threshold evidence.

Tools and leads for all of it come from the main cart: the digital multimeter and USB data/power cables ([`purchase-list.md`](purchase-list.md) §6) and the current-limited source = the wearable's USB-PD power bank (§4). **Optional add-on (still unbought):** inline USB power meter, ₱200–400 — excluded from the envelope; the multimeter covers the rule.

## 2. Rules this slice binds itself to

- **Buy-once rule** — the cart is one purchase; genuinely consumable rows (tape, zip ties) may be re-bought as they deplete, at trivial cost.
- **Connector-ready rule (unchanged in substance, re-purposed)** — every module arrives with headers/connectors pre-fitted (or consumer-ready); nothing is hand-soldered in-house and no fine-pitch bench-solderable part is bought ([README §3](../README.md#3-hardware) constraint 2). The old "non-permanent bench attach, zero soldering" rationale is retired with the bench; the purchasing half survives for the P3 service build.
- **Nothing on the device-critical path** — the P3 soldered build, P4 calibration, and P5 study never needed the breadboards; tools in [`purchase-list.md`](purchase-list.md) §6 and attachment/print materials in §7 are shared and stay in the main cart.
- **Harness software is a build item, not a material** — `bench/capture.py`, `bench/runbooks/<test>.yaml`, and `bench/reduce.py` are written at P2 entry, not bought. (`reduce.py --parity` retires with the parity re-run.)

## 3. Retired rows — recorded, not bought (no deletions; the decision is reversible)

These were the bench-phase-only material slice. Under the no-bench decision they stay **unbought**; the table is kept so the ≈ ₱1,600–3,500 envelope and the reasoning behind it survive in the record.

| Section | Item(s) (consumed by, formerly) | Low ₱ | High ₱ |
|---|---|---|---|
| Breadboard & wiring infrastructure (`P2-path-A`) | solderless breadboards ×2 (both device builds); M–M/M–F dupont packs; 22–26 AWG hookup wire; 2.54 mm pin-header pack; M–F jumper set for the Pi 5 GPIO header | 1,010 | 1,950 |
| Test fixtures (`P2-path-A`) | camera tripod/clamp stand (T2); printed head-form jig (T7 geometry); printed marker-variant sheets (T7 dictionary/size pin) | 450 | 1,250 |
| Test props & ground truth (`P2-path-A`) | white foam board / cardboard / dark fabric for the T2 surface matrix; 5 m tape (T2/T6/T8 — shared, already in the cart §8) | 100 | 300 |
| **Retired bench-slice total** | | **1,560** | **3,500** |

Two rows had non-bench afterlife and are therefore **not** retired: the printed **marker** (now pinned at P2 as a config decision, printed in the P3 print lane) and the 5 m **tape** (already in the cart; reused by the P5 course). Everything else above is unbought.

## 4. On-hand inventory (no spend — the selection-independent portion)

- **Laptop** — flashing both boards, serial monitoring, and the per-session CSV telemetry pipeline ([`bench-tests.md`](bench-tests.md) "Telemetry format": `bench/capture.py` → `bench/reduce.py`), now serving P4/P5 telemetry.
- **Phone** — inclinometer reference (P4), lux reading, timing.
- **Room furniture** — a level surface, a steel table or rebar piece as the ferromagnetic disturbance source, a known-pose obstacle.
- **Space** — a ≥ 10 m clear run for the P4 distance walks; a dimmable room corner for the P4 low-light check.

## 5. Checkout reminders

- **Do not buy** the §3 breadboard/fixture/prop rows.
- Connector-ready check unchanged from the main cart (headers pre-fitted; no fine-pitch bench-solderable parts).
- If the P2 evidence gate pins a marker dictionary/size different from the pre-printed sheet, the marker re-print is a trivial consumable, not a buy-once violation.
