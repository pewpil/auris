# T0 — Power rails: background and bench procedure

> **Test-under-description:** [`docs/bench-tests.md` T0](../../docs/bench-tests.md#t0--power-rails) — the first test of the P2 matrix and the gate every other test depends on. **Circuit:** [`t0-power-rails.kicad_sch`](t0-power-rails.kicad_sch) in this folder (export: [`export/t0-power-rails.pdf`](export/t0-power-rails.pdf)). Metrics logged: `v_rail`, `i_load`, `t_soak`, `temp_c`.

## 1. What T0 decides

T0 asks one question about each device — *is the electrical foundation sound?* — and answers it with four pieces of evidence: the **sensor rail** (3.3 V) holds within **±3 % under full load**; **no brownout resets** occur across a **30-minute soak** at tracking duty; the **charge-board protection** actually trips on a forced short; and the assembly runs the soak **without thermal runaway** (a by-touch temperature check, logged). Everything downstream — IMU drift curves (T1), latency distributions (T3–T5), tracking rates (T7–T8) — inherits the rail quality established here, which is why T0 runs first: a rail that sags mid-measurement corrupts every dataset collected after it.

## 2. Background

**The 1S lithium-ion cell.** Both devices run a single Li-ion cell: a slim pouch cell on the pointer, a protected 18650 on the wearable. A 1S cell operates from **4.2 V (full) down to ~3.0 V (practically empty)**, and its voltage under load also dips with current (internal resistance — tens of milliohms for a healthy cell, more for a pouch with long jumper leads). This 3.0–4.2 V window is the raw material the rest of the power path works with.

**Charge: constant-current then constant-voltage.** The TP4056-class charge/protect board charges the cell in two phases: **constant current** (~1 A, the board's fixed program) until the cell reaches 4.2 V, then **constant voltage** at 4.2 V while the current naturally tapers; charging ends near a C/10 fraction of cell capacity. During constant-current charging the board dissipates noticeable heat — a warm (not hot) TP4056 during charging is normal physics, not a defect. These boards also carry a **battery-protection stage** (a DW01-class supervisor with a dual MOSFET in the negative lead) that disconnects the cell on overcharge (~4.25–4.3 V), over-discharge (~2.4–2.5 V), or over-current (~3 A class) — all typical datasheet values, exact figures per the board's parts. The pointer's pouch cell has **no** protection of its own and relies entirely on the board; the wearable's 18650 is **protected at the cell** and wears the board's protection as a second layer.

**Discharge: the load chain.** From the charge board's protected output (OUT+/OUT−), current flows through the **slide switch**, through the **inline multimeter** (mA range), into the DevKitC-1's **5 V pin**, and onward: the DevKitC-1's onboard regulator produces the **3.3 V sensor rail** that feeds every module — BNO085 IMU, VL53L1X ToF and DWM3000 UWB tag on the pointer; head IMU, UWB anchor, both cameras and (from the switched battery rail directly, which suits the amp's 2.5–5.5 V input) the two MAX98357A I²S amplifiers on the wearable. The regulator is a **linear** one: it burns the difference between battery and 3.3 V as heat, and — the part that matters at this bench — it needs **headroom**: below roughly 3.3–3.5 V of input its output begins to follow the input down. A cell sagging toward end-of-charge therefore shows up first as a sagging sensor rail, not as a device failure — T0 is designed to observe exactly this honestly (see §9).

**Brownouts.** The ESP32-S3 watches its own supply with a **brownout detector** (threshold in the ≈2.4–2.9 V class, fixed at boot): if the 3.3 V rail dips below it, the chip resets deliberately rather than misbehave, and the reset reason is readable in software. Our firmware logs it (`esp_reset_reason()` through the `bench-log` stream — [README §4.1](../../README.md#41-firmware-modules)), so "no brownout resets logged" is a machine-checked acceptance row, not an operator impression.

**Why the ammeter reading bounces.** The radio transmitter does not draw smoothly: an ESP-NOW transmission pulls a **burst** of hundreds of milliamps for a fraction of a millisecond, then drops back to idle tens of milliamps. A handheld multimeter integrates these bursts into a jumpy average — expect the display to dance between tens and a few hundred mA during full-load stages. The bursts also travel: thin dupont jumpers and breadboard rails add resistance precisely where the burst current flows, sagging the rail at the exact instants the brownout detector watches. This is why the acceptance is written on rail **stability** (±3 %) rather than on any single current figure.

**Why a 30-minute soak.** Rail faults that matter are thermal and cumulative: a marginal connection heats and drifts, a pouch cell with long leads sags as it warms, a charge board still plugged in cycles its top-off. A spot-check passes these; a soak under the heaviest duty (both cameras streaming on the shared DVP bus + UWB ranging + radio TX + audio) catches them — and simultaneously re-validates the **battery budget** with the tracking tier's real draw included (camera streaming is the largest single addition).

## 3. Apparatus

- Both devices breadboarded per the wiring maps ([`docs/bench-tests.md`](../../docs/bench-tests.md)): USB-C charge/protect board, cell (pouch / protected 18650 in holder), slide switch, DevKitC-1, and the sensor modules of each device.
- Digital multimeter (mA range, in series on the switched battery rail; second meter or re-probing for rail voltages).
- Bench USB source for the first power-up (current-limited if available), charger for the protection test.
- The logging harness: laptop running `bench/capture.py` with both devices' USB serial ports — operator entries ride the same stream (`start` / `stop` / `mark` / `set`), per [`docs/bench-tests.md` §Logging format](../../docs/bench-tests.md#logging-format).
- Optional: a second thermometer (or IR no-contact) to corroborate the by-touch temperature estimate.

## 4. The circuit

One sheet, two variants — same topology, different cells (schematic in this folder; the net names carry the device suffix `_P`/`_W` because the two devices never share a ground):

| Element | Pointer | Wearable | Role in T0 |
|---|---|---|---|
| `J` USB-C in | charger port | charger port | 5 V into the charge board (`CHG5`) |
| `U` TP4056-C | charge + protection | charge + protection | CC/CV charge into `VBAT`; protected output `OUT` |
| `BT` cell | 1S Li-ion pouch | 1S protected 18650 + holder | the rail under test (`VBAT` → test point `TP1/5`) |
| `SW` slide switch | load on/off | load on/off | bench master switch |
| `M` multimeter | inline, mA | inline, mA | `i_load` (series, switched rail → DevKitC 5 V) |
| `U` DevKitC-1 | pointer MCU | wearable MCU | makes `P3V3` from the switched rail (`VBAT_SW` → test point `TP2/6`) |
| loads | IMU, ToF, UWB tag | head IMU, UWB anchor, 2× amp, 2× cam | the full-load stage; all grounds per-device (`GND_P`/`GND_W`) |
| `TP` test points | cell / switched / 3V3 / GND | same | where the probes live (`TP3/7` = `v_rail`) |

Note the node the ammeter sits in: switched-battery → 5 V pin. The DevKitC-1's USB port stays **unplugged** during cell-powered stages — power enters through one path at a time.

## 5. Safety

- Check **cell polarity twice** before each insertion; never park a bare cell on metal (it shorts through the breadboard's plate).
- **First power-up of each device through the multimeter inline** (mA range) — or a current-limited USB source — never directly from a bare cell.
- **Power off before any wiring change**; hot-plugging I²C/I²S is how modules die.
- All P2 bench rules hold: no soldering, modules pre-soldered, bench USB (not the battery rail) until T0 passes ([`docs/bench-tests.md` §Bring-up safety](../../docs/bench-tests.md#bring-up-safety)).

## 6. Procedure

Run once per device, pointer first. Every stage's readings go through the harness — the quoted commands are sent from `bench/capture.py`; stage marks make the reducer's derivations (`t_soak`, per-stage stats) possible.

1. **Preflight (device off).** Visual check of the breadboard against the wiring map; cell polarity confirmed twice; switch off; multimeter in series in the switched-battery rail (mA range); voltmeter across the 3.3 V rail at the sensor end (not at the regulator) — the far end is where sag shows.
2. **First power-up, current-limited.** Bench USB into the DevKitC's USB port, current limit ~250 mA, switch off. Power on: the board boots (firmware banner), idle current in the tens of mA. Any large or rising current → power off, find the short before proceeding.
3. **Cell bring-up.** Switch off; connect the charged cell (≥ 3.9 V — start the soak high on the discharge curve); move the meter to the battery path as in §4; switch on. Log: `start T0 <pointer|wearable>`, then `set v_rail=<V>` for the battery rail (`TP1/5`) and the 3.3 V rail (`TP3/7`), `set i_load=<mA>` for idle. This is the **idle** row.
4. **Full-load, stage by stage.** Trigger the aid's own load stages in dependency order: (a) radio TX + amp playing (trigger-gated playback); (b) — once the tracking extras are fitted — **tracking duty**: both cameras capturing on the shared DVP bus + UWB ranging. After each stage: `mark <stage>` and `set v_rail=… i_load=…` at the sensor-rail test point. Expect the ammeter to bounce with TX bursts — record the visible range in the `set` notes rather than chasing one number.
5. **The 30-minute soak.** Leave the device at tracking duty. `mark soak_start`; at ~10-minute intervals, `set v_rail=… i_load=…`; at 30 min, `mark soak_end`. Watch the meter for *trends* (rising baseline current = thermal or software trouble brewing) and the device for **brownout resets** — the firmware logs them; zero is the gate.
6. **Protection short test.** Cell-connected device, switch off, and the short made **on the switched output only**: with the switch off, bridge the load side of the switch to ground through the ammeter's leads briefly — the protection stage must cut the output in milliseconds (the meter reading collapses to zero). **Never** short the cell terminals themselves. Restore by re-applying the charger briefly (the DW01-class latch releases on charge voltage). Log `mark protection_trip` with the observed behaviour.
7. **Thermal check.** Immediately after the soak: touch the charge board, the regulator, the cell — warm is fine, painful is a failure. Log `set temp_c=<estimate>` (and any corroborating thermometer reading in the notes). The wearable's temp is a first-class metric under tracking duty.
8. **Budget check.** From the soak's logged currents, confirm the battery budget with tracking active — camera streaming is the largest addition, and this is where its real number is captured. `stop` ends the session; the reducer emits the CSV and the pass/fail summary.

## 7. Data recording

The session lands in the mandated schema — one CSV per session: `session,test_id,timestamp_ms,metric,value,unit,notes` ([`docs/bench-tests.md` §Logging format](../../docs/bench-tests.md#logging-format)). Device-side rows (rail ADC, reset reasons) arrive automatically; operator rows come from the `set` commands above; the reducer derives `t_soak` from the soak marks and prints the acceptance check. A clean transcript reads like:

```
> start T0 pointer
> set v_rail=4.13 v_rail_pos=battery_tp unit_check=ok
> set i_load=38 notes=idle, radio off
> mark full_load_stage_b
> set i_load=120..310 notes=tx_bursts+amp
> mark soak_start
> set v_rail=3.28 i_load=140..520 notes=tracking_duty
> mark soak_end
> mark protection_trip notes=cut_in_<1s, released_by_charger
> set temp_c=38 notes=regulator_warm, board_cool
> stop
```

## 8. Acceptance criteria

| Row | Gate | Source |
|---|---|---|
| Sensor rail stability | `v_rail` (3.3 V rail) within **±3 %** of nominal under full load | [`bench-tests.md` T0](../../docs/bench-tests.md#t0--power-rails) |
| Brownouts | **no brownout resets logged** after the soak | same |
| Protection | charge-board protection **trips on short test** | same |
| Thermal | no thermal runaway; by-touch `temp_c` logged | same |
| Soak | 30 min at tracking duty (cameras + UWB + TX + amp) | same |

## 9. Troubleshooting

- **Brownout resets during TX bursts** — the classic first failure: burst current × breadboard/jumper resistance sags the rail below the detector. Shorten the high-current path (battery → switch → 5 V pin), use thicker jumpers there, keep the sensor modules' jumpers as-is; re-run. If it persists at a healthy cell voltage, the protection board's over-current trip may be nudging — check whether the reset coincides with its re-lock behaviour.
- **3.3 V rail sags with a healthy load** — look at the *battery* voltage simultaneously: if the cell is below ~3.4 V, the DevKitC's regulator is out of headroom and simply passing the sag through. Recharge, re-run; if it sags at 4.0 V+, that is a real finding — record it, it belongs in the T0 data, not in a workaround.
- **Meter reading implausibly high at idle** — a module's idle state changed (a camera powered but not streaming, the UWB module in a chatty mode); check the firmware's duty state before blaming the rail.
- **TP4056 hot to the touch during charging** — expected during the constant-current phase; hot *at idle with no charger* is not, and points at the protection stage — swap the board.
- **Protection will not re-arm after the trip test** — some boards need the charger applied for several seconds; if it never re-arms, the board took the short personally — replace it (spares are cheap; the buy-once rule budgets for this).

## 10. References

- [`docs/bench-tests.md`](../../docs/bench-tests.md) — the P2 protocol this walkthrough executes (T0 rows, bring-up safety, logging format).
- [`README.md` §3.1/§3.2](../../README.md#31-pointer--electronics) — the settled electronics of both devices; [§4.2](../../README.md#42-latency-budget-motion-to-sound-target--100-ms) — the budget the rails make measurable.
- [`docs/purchase-list.md`](../../docs/purchase-list.md) — the ordered parts this test consumes (charge boards, cells, switch, meters).
- TP4056 / DW01-class charge+protect board datasheets, the DevKitC-1 power tree, and the ESP32-S3 brownout documentation — datasheet-grade, per component note [README §3.7](../../README.md#37-component-notes).
