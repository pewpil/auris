# Button-only bring-up test — AGENTS.md

Standing instructions for `pointer/firmware/button_test/`. Read with [`../../../AGENTS.md`](../../../AGENTS.md) for the repository-wide sections and with [`../../AGENTS.md`](../../AGENTS.md) for the pointer's hardware context. This directory exists to test **one switch on one pin**, with no IMU, ToF, Wi-Fi or compute unit involved.

> **Scope:** [§2](../../../AGENTS.md) (document every function and class), [§3](../../../AGENTS.md) (the AI never commits), [§4](../../../AGENTS.md) (evidence classes), [§5](../../../AGENTS.md) (never artificially wrap prose), and [§6](../../../AGENTS.md) (side notes) apply here unchanged. This is a bring-up sketch, not the deployable firmware; its output is a development observation.

## Status: isolating the button

The pointer's button has been the least trustworthy part of the bench: it has read stuck-pressed, stuck-released, and correlated with the status LED in ways that turned out to be the firmware's own LED logic. This sketch removes every other subsystem so the only question left is whether the switch changes GPIO18 cleanly.

It is deliberately a separate sketch with its own `button-test` profile. It does not use the VL53L1X library, the MPU6050, Wi-Fi or NVS. Flash it, press the switch, and the lines it prints are the whole diagnosis.

## Parts

| Part | What it is | Note |
|---|---|---|
| ESP32 DevKit V1 (30-pin) | 30-pin ESP32 board with silkscreen labels, CP2102 USB-UART | USB powers it and carries the serial stream; the printed labels are the authority |
| Momentary tactile switch, 4-leg | Does not latch; closes only while held | One leg from each side of the contact is used; the other two stay unconnected |
| 4.7 kΩ–10 kΩ resistor *(optional)* | External pull-up, only if one is wanted | The firmware uses the ESP32's internal pull-up, so the two-wire build needs no resistor |

## Wiring

**Bare 4-leg momentary switch — the intended build.** This is the safe two-wire arrangement; the ESP32's internal pull-up supplies the high level, so the switch never sees VCC.

| From                                                    | To                                                   | Function                                         |
| ------------------------------------------------------- | ---------------------------------------------------- | ------------------------------------------------ |
| `D18` — **GPIO18** (bottom row, between `D5` and `D19`) | switch leg A                                         | Input, `INPUT_PULLUP`; idle HIGH, pressed LOW    |
| `GND` (bottom row, next to `3V3`)                       | switch leg B, on the **opposite side** of the switch | Closes GPIO18 to ground while held               |
| switch legs C and D                                     | *no connection*                                      | The unused pair                                  |
| **No VCC**                                              | —                                                    | A bare switch is a contact, not a powered device |

**3-pin button module labelled VCC / GND / OUT.** If the part is a module rather than a bare switch, use its labels:

| Module pin | To |
|---|---|
| `VCC` | `3V3` (bottom row, next to `GND`) |
| `GND` | `GND` (bottom row, next to `3V3`) |
| `OUT` | `D18` (GPIO18, bottom row between `D5` and `D19`) |

Most such modules present an active-low output, so the internal pull-up is harmless and a press still reads as a press.

**Optional external pull-up.** If an external pull-up is wanted anyway, connect `3V3` → 4.7 kΩ–10 kΩ → GPIO18, and the switch from GPIO18 to GND. The internal pull-up remains in parallel (roughly 45 kΩ), which is fine. VCC/3V3 feeds only the resistor; it never touches a switch leg.

### Why not VCC, GND and signal on three switch legs

A bare 4-leg tactile switch has **no supply terminals**. On the common part, the two legs on one side are internally joined, the two on the other side are joined, and pressing bridges the two pairs. That means:

- VCC on one leg and GND on a leg of the **same side** shorts the 3.3 V rail **permanently**, pressed or not.
- VCC on one leg and GND on a leg of the **opposite side** shorts the rail **while pressed**.

Either is a supply short, not a button circuit. If a "VCC, GND, signal" arrangement appeared to work, it was almost certainly a module with its own pull-up, or the legs were not paired the way they looked. The two-wire signal + GND build above is the one this sketch expects.

The switch's two chosen legs must be on **opposite sides** of the contact. The two legs on one side are internally joined and can never produce a press on their own; wiring across one side gives a permanently closed input.

## What the sketch does

`button_test.ino` configures GPIO18 as `INPUT_PULLUP`, samples it at **1 kHz**, and debounces with a **20 ms** window: a raw level must persist for 20 ms before it becomes the accepted state. It then prints **one column and nothing else** — `btn`, 1 while the button is pressed and 0 while it is not, once every 100 ms. No timing, no counters, no statistics: the sketch exists to answer whether the switch changes GPIO18 cleanly, and the column is that answer. Only the debounced state is printed, so contact chatter inside the window never reaches the output.

The header repeats every five seconds, so a monitor that attaches after boot still learns what the column means. The 100 ms period is a readability choice rather than a measurement — fast enough to see a press register, slow enough not to flood the terminal. It does not affect the debounce, which samples at 1 kHz.

The onboard LED mirrors the debounced state: on while pressed, off while released. It is the same GPIO2 LED the deployable firmware uses, but here it follows only the button.

If the pin already reads LOW at boot, the banner prints a warning: either the switch is held, its legs are on one side of the contact, or something is shorting GPIO18 to ground. The test keeps running so the behaviour can still be observed in the column.

## Expected output

With the switch untouched, the column repeats `0`:

```
# Auris button-only test
# bare switch: GPIO18 -> leg A, GND -> opposite leg B, no VCC
# 3-pin module: VCC -> 3V3, GND -> GND, OUT -> GPIO18
# GPIO18 INPUT_PULLUP, debounce 20 ms
# boot state: btn=0
# btn=1 while pressed, btn=0 while released
btn
0
0
0
```

While the switch is held, it reads `1`:

```
1
1
1
```

A clean switch moves the column between 0 and 1 once per press, and the transition survives the 20 ms debounce. If the column flickers on a single press, the switch is chattering beyond the window or is not momentary.

## Reading the log when it fails

- **`# boot state: btn=1` with the switch untouched** — GPIO18 is being pulled low. Check for a short to ground, legs on the same side, or VCC on a switch leg. The banner warns about this.
- **The column stays 0 when the switch is pressed** — the signal leg missed its row, the switch is defective, or the chosen legs are both on the same side (internally joined). Confirm continuity across the two chosen legs before relying on them.
- **The column stays 1 after release** — the switch is latching or stuck, or the input never returns high. This sketch assumes a momentary part.
- **The column flickers on a single press** — contact chatter longer than 20 ms. It is a switch-quality observation, not a firmware fault; the printed values are still debounced.
- **No output at all, not even the banner** — wrong port or baud. Run `arduino-cli board list`, set 115200, and use the port it reports.
- **The LED never changes** — the LED follows the debounced state, so first confirm the column changes. If it does and the LED stays dark, the board's LED is not on GPIO2.

## Build, flash, and monitor

From this directory:

```bash
arduino-cli compile --profile button-test
arduino-cli upload  --profile button-test --port /dev/ttyUSB0
arduino-cli monitor --port /dev/ttyUSB0 --config baudrate=115200
```

This sketch has **no library dependencies** beyond the ESP32 core; `sketch.yaml` pins the core at 3.3.12 and targets the pointer board, `esp32:esp32:esp32doit-devkit-v1` (30-pin DevKit V1). The deployed firmware's profile is `pointer-sensor-node`, the IMU test's is `imu-test`, and the combined test's is `node-test`; profiles are per sketch, so do not copy one into another directory.

If the monitor reports that the port is missing, the CP2102 has dropped enumeration; replug the USB cable and use the port `arduino-cli board list` reports. If the stream looks like binary garbage, a previous tool left the tty at a different baud; force it with `stty -F /dev/ttyUSB0 115200 raw -echo` before monitoring.

## Notes and limitations

- **This is not the pointer firmware.** It does not exercise the deployed button-gating, the sensors, the link or calibration. Its only claim is whether GPIO18 changes cleanly with the switch.
- **Sub-20 ms taps are ignored by design.** The debounce window is 20 ms; a human press is much longer. If a very fast tap must be caught, lower `DEBOUNCE_MS` and accept more chatter.
- **The test is a development observation.** In [§4](../../../AGENTS.md#4-verification-policy--no-hardware-bench-phase) terms it is not `[SIM]`, `[BUD]`, `[PROP]` or `[CITE]` evidence for the thesis; it is bench bring-up.
- **The deployed firmware uses the same pin and polarity:** GPIO18, `INPUT_PULLUP`, pressed LOW (`btn=1`). Once this test shows clean transitions, the switch wiring can be carried back to `pointer_sensor_node/`.
