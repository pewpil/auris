# Pointer hardware and firmware — AGENTS.md

Standing instructions and hardware specification for the Auris pointer. This file is the single source of truth for how the pointer is wired and what its firmware is expected to do.

> **Scope:** [`../AGENTS.md`](../AGENTS.md) §1–§6 apply here unchanged — the documentation standard (§2), the no-commit rule (§3), the verification policy (§4), the unwrapped-prose rule (§5), and the side-note convention (§6) all govern this directory. §1–§6 are repository-wide and apply inside `pointer/` exactly as they do everywhere else. This file follows §5: every prose paragraph is one unwrapped line.
>
> **Where this sits:** `pointer/` is an ordinary in-repo directory, not a submodule ([`../AGENTS.md` §7](../AGENTS.md)). The compute unit's code lives in the separate `cu/` submodule and is not affected by anything here.

## What the pointer is

The pointer is the handheld device of [§1.5](../AGENTS.md): it carries a laser and its button, an IMU that establishes the pointer's own orientation, and a time-of-flight sensor that gives the range to whatever the laser is aimed at. It does all of its own orientation fusion onboard and sends the result onward to the compute unit. It is not the compute unit — that is the Raspberry Pi, which lives in `cu/`.

**Current state: a breadboard prototype.** The ESP32, the IMU breakout, and the ToF breakout are on a breadboard and the wiring is being established from the connection list below. Nothing in this directory has been built yet; the firmware rewrite follows the wiring, not the other way round.

## Components

| Component | Part | Notes |
|---|---|---|
| MCU | ESP32 NodeMCU-32S (ESP32-D0WD-V3, ESP32-WROOM-32), **38-pin** breadboard layout | 19 pins per row at 2.54 mm. Onboard CP2102 USB-UART, so the laptop's USB port both powers the board and carries the serial link. See the pin table below before touching it — this layout also breaks out six flash pins |
| IMU | MPU6050 on a GY-521 breakout | 3-axis accelerometer and gyroscope, plus die temperature |
| ToF | VL53L0X breakout | Single-zone time-of-flight, non-contact range |
| Input | Tactile push button, momentary, 4-leg | Laser trigger |
| Passives | 2 × 4.7 kΩ resistor, 2 × 100 nF ceramic capacitor | I²C pull-ups and decoupling |
| Common | Half-size breadboard, jumper wires, USB **data** cable | |

## Connection list

This is the wiring. Build it exactly as written, one row at a time, with the USB cable **disconnected** until the last row.

Every ESP32 pin is given by its row and number **and** its GPIO number. Remember that the two rows are numbered differently — `r1` runs 1–19 and `r2` runs 60–42 — so the same column carries two unrelated numbers. Match the GPIO number against the table above; use the position only to find the pin, since positions are easy to misread with the board facing the other way.

| From | To | Function |
|---|---|---|
| `r2 #47` — **GPIO21** | `MPU6050.SDA` and `VL53L0X.SDA` | I²C data — shared by both sensors |
| `r2 #44` — **GPIO22** | `MPU6050.SCL` and `VL53L0X.SCL` | I²C clock — shared by both sensors |
| `r1 #9` — **GPIO27** | `VL53L0X.XSHUT` | ToF enable, actively driven — **not left floating** |
| `r2 #50` — **GPIO18** | `Button.leg A` | Button input, internal pull-up |
| `r1 #19` — `3V3` | breadboard **+** rail, and from there both `VCC` pins | 3.3 V power |
| `r2 #42`, `r1 #2`, `r1 #6`, or `r2 #48` — `GND` | breadboard **−** rail | Common ground |
| `Button.leg B` | breadboard **−** rail | Closes GPIO 18 to ground when held |
| `MPU6050.VCC` | breadboard **+** rail | Power |
| `MPU6050.GND` | breadboard **−** rail | Common ground |
| `MPU6050.AD0` | breadboard **−** rail | Selects I²C address `0x68` |
| `MPU6050.XDA`, `MPU6050.XCL`, `MPU6050.INT` | *no connection* | Auxiliary bus and interrupt, unused |
| `VL53L0X.VCC` | breadboard **+** rail | Power |
| `VL53L0X.GND` | breadboard **−** rail | Common ground |
| `VL53L0X.GPIO1` | *no connection* | Interrupt output, unused |
| `4.7 kΩ` #1 | between the SDA net and the **+** rail | SDA pull-up |
| `4.7 kΩ` #2 | between the SCL net and the **+** rail | SCL pull-up |
| `100 nF` #1 | across `MPU6050.VCC` and `MPU6050.GND` | Decoupling, place at the module |
| `100 nF` #2 | across `VL53L0X.VCC` and `VL53L0X.GND` | Decoupling, place at the module |

This board has **four `GND` positions** — `r1 #2`, `r1 #6`, `r2 #48`, and `r2 #42`. Any of them is fine; pick the one nearest your wiring and use it consistently. Do not use the 5 V input at `r1 #1` as a ground.

### Per-module detail

**MPU6050 (GY-521).** `VCC` to the **+** rail, `GND` to the **−** rail, `SDA` to `r2 #47` (GPIO21), `SCL` to `r2 #44` (GPIO22), `AD0` to ground. `XDA`, `XCL`, and `INT` stay unconnected. Tying `AD0` low puts the part at `0x68`; leaving it floating puts it at `0x69`, which nothing here probes for.

**VL53L0X.** `VCC` to the **+** rail, `GND` to the **−** rail, `SDA` and `SCL` shared with the IMU, `XSHUT` to `r1 #9` (GPIO27), `GPIO1` unconnected. **Read the "Verify before first power-up" section before connecting this one** — its supply requirements differ from the IMU's.

**Button.** One leg to `r2 #50` (GPIO18), the opposite leg to ground. The firmware configures GPIO 18 as `INPUT_PULLUP`, so an idle button reads `HIGH` and a pressed one reads `LOW`. On a four-leg tactile switch, pick two legs on **opposite sides** of the switch's electrical contact; the two outer legs on the same side are internally connected and will never close the circuit.

## I²C bus

Both sensors share one bus. There is no address conflict: the MPU6050 answers at `0x68` (or `0x69` if `AD0` is tied high), and the **VL53L0X has no address pins at all** — it is fixed at `0x29`.

**Fit both 4.7 kΩ pull-ups even though the GY-521 usually carries its own.** If your GY-521 already has 4.7 kΩ on each line, the two in parallel give roughly 2.3 kΩ, which is still comfortably within spec for 400 kHz over breadboard-length wiring. Do not stack three sets — that approaches 1.6 kΩ and is needlessly stiff for the bus.

Run the bus at **400 kHz**, which both parts rate well above. Keep SDA and SCL runs short and, where you must, route them as a pair; breadboard capacitance is small but the pull-ups are the only thing keeping the lines from ringing.

## `XSHUT` must be driven, not floated

This is the single most important change from the previous build, and it is a correction rather than a preference.

With `XSHUT` left unconnected, the ToF either never enables or holds the bus in a state that makes **every other device on it unreadable**. In the previous prototype the IMU never produced a valid sample with the ToF attached — readings came back as dropped transfers or as all-zero register dumps — and recovered completely once the ToF was removed. Driving `XSHUT` from `r1 #9` (GPIO27) is what makes it possible to have both sensors on one bus.

The firmware must therefore assert `XSHUT` deliberately rather than assuming the sensor comes up on its own: hold it **low** briefly to force a hardware reset, release it **high**, and wait for the part to become responsive before issuing any I²C transaction to it. The datasheet requires a settling interval after enable; treat anything under about 100 ms as unverified.

## ESP32 header pins

This is a **38-pin NodeMCU-32S** (Ai-Thinker layout, 2.54 mm pitch). The board is named for the 32 usable GPIOs it breaks out, but it also exposes pins wired to the module's own SPI flash, and those must never be used. This board labels them by their flash function rather than by GPIO number, so they read like harmless signal names.

**The two rows are numbered differently, and this is deliberate.** `r1` is the top row, numbered **1 to 19**. `r2` is the bottom row, numbered **60 down to 42** across those same columns. Each physical column therefore has two different numbers, and a column is identified by both: the top pin as `r1 #n`, the bottom pin as `r2 #n`. Do not assume a number carries over between rows — `r1 #9` and `r2 #50` are in the same column but are unrelated numbers.

**The GPIO number is the authority, not the position.** Board revisions differ in how the headers are numbered, and positions are easy to misread with the board facing the other way. The position tells you where to look; the GPIO number is what tells you what the pin is.

| Column | `r1` # | `r1` — top row                              | `r2` # | `r2` — bottom row                          |
| ------ | ------ | ------------------------------------------- | ------ | ------------------------------------------ |
| **1**  | **1**  | **5V** — labelled `su`, power input         | **60** | `clk` — **GPIO6** — **flash**              |
| **2**  | **2**  | `GND`                                       | **59** | `sdd` / `SD0` — **GPIO7** — **flash**      |
| **3**  | **3**  | `sd3` / `SD3` — **GPIO10** — **flash**      | **58** | `sd1` / `SD1` — **GPIO8** — **flash**      |
| **4**  | **4**  | `sd2` / `SD2` — **GPIO9** — **flash**       | **57** | `p15` — **GPIO15** — strapping             |
| **5**  | **5**  | `p13` — **GPIO13** — strapping              | **56** | `p2` — **GPIO2** — strapping, on-board LED |
| **6**  | **6**  | `GND`                                       | **55** | `p0` — **GPIO0** — strapping, BOOT button  |
| **7**  | **7**  | `p12` — **GPIO12** — strapping              | **54** | `p4` — **GPIO4** — free                    |
| **8**  | **8**  | `p14` — **GPIO14** — strapping              | **53** | `p16` — **GPIO16** — free                  |
| **9**  | **9**  | `p27` — **GPIO27** — free — **ToF `XSHUT`** | **52** | `p17` — **GPIO17** — free                  |
| **10** | **10** | `p26` — **GPIO26** — free                   | **51** | `p5` — **GPIO5** — strapping               |
| **11** | **11** | `p25` — **GPIO25** — free                   | **50** | `p18` — **GPIO18** — free — **button**     |
| **12** | **12** | `p33` — **GPIO33** — free                   | **49** | `p19` — **GPIO19** — free                  |
| **13** | **13** | `p32` — **GPIO32** — free                   | **48** | `GND`                                      |
| **14** | **14** | `p35` — **GPIO35** — input only             | **47** | `p21` — **GPIO21** — free — **I²C SDA**    |
| **15** | **15** | `p34` — **GPIO34** — input only             | **46** | `px` / `RX` — **GPIO3** — UART0 serial     |
| **16** | **16** | `svn` / `SVN` — **GPIO39** — input only     | **45** | `tx` / `TX` — **GPIO1** — UART0 serial     |
| **17** | **17** | `svp` / `SVP` — **GPIO36** — input only     | **44** | `p22` — **GPIO22** — free — **I²C SCL**    |
| **18** | **18** | `EN` — chip enable / reset, active **LOW**  | **43** | `p23` — **GPIO23** — free                  |
| **19** | **19** | `3V3` — 3.3 V output rail                   | **42** | `GND`                                      |

All four pins this build uses fall in the free category: **`r1 #9`** (GPIO27, ToF `XSHUT`), **`r2 #50`** (GPIO18, button), **`r2 #47`** (GPIO21, SDA), and **`r2 #44`** (GPIO22, SCL).

One discrepancy worth recording: this board's headers do not appear to expose `CMD`, the sixth flash line (GPIO11). The five flash pins that are present sit at `r2 #60` (`clk`), `r2 #59` (`sdd`), `r2 #58` (`sd1`), `r1 #3` (`sd3`), and `r1 #4` (`sd2`).

### Pins to treat with care

Only four of the 38 header pins are unconditionally free. The rest fall into one of these categories, and the distinction is worth keeping before assigning anything new:

| Category | Pins | What it means |
|---|---|---|
| **Never use** | `GPIO6`–`GPIO11` — pins 4, 5, 6, 10, 11, 12, labelled `clk`, `sdd`, `sd1`, `sd2`, `sd3`, `cmd` | Wired to the module's internal SPI flash. Driving one cuts the chip off from its own program memory: crashes, boot loops, or a board that looks dead |
| **Input only** | `GPIO36`, `GPIO39`, `GPIO34`, `GPIO35` — pins 22, 23, 37, 38, labelled `svp`, `svn`, `34`, `35` | No output driver, so `pinMode(OUTPUT)` does nothing. Also no internal pull-up, so the button cannot use them |
| **Strapping** | `GPIO0`, `GPIO2`, `GPIO5`, `GPIO12`, `GPIO13`, `GPIO14`, `GPIO15` — pins 13, 18, 29, 30, 31, 32, 36 | State at reset selects boot mode or peripheral behaviour. `GPIO0` low at reset enters the bootloader; `GPIO12` high at reset selects 1.8 V flash and the board browns out |
| **Serial** | `GPIO1`, `GPIO3` — pins 15, 14, labelled `tx`, `px` | UART0, the link back to the laptop. Taking these over breaks flashing and the monitor |
| **Free** | `GPIO4`, `GPIO16`, `GPIO17`, `GPIO18`, `GPIO19`, `GPIO21`, `GPIO22`, `GPIO23`, `GPIO25`, `GPIO26`, `GPIO27`, `GPIO32`, `GPIO33` | No flash, strapping, serial, or input-only constraint. Start here |

The flash pins deserve restating because their **labels give no hint of the hazard**. `clk` and `sd1` read like ordinary signal names; only the GPIO number — 6, 8 — reveals they are wired to the chip's own program memory. They sit at the ends of the rows, which is exactly where wires naturally run, so it is easy to grab one by accident. The fault is not permanent: disconnect it and reflash, and the board recovers.

The pins this build uses — **16, 17, 21, and 28** (`GPIO21`, `GPIO22`, `GPIO18`, `GPIO27`) — are all in the free category.

## Verify before first power-up

Check these with the USB cable disconnected and the breadboard fully populated. Each one is a way to damage a module or waste an afternoon.

1. **`VL53L0X.VREF` must be 2.8 V.** The VL53L0X is a 2.8 V part and its `VREF` pins must be held at 2.8 V, never at 3.3 V or 5 V. Most breakouts supply this from an on-board regulator, but cheap clones vary and some tie `VREF` directly to `VCC`. Identify how your module sets `VREF`, and confirm with a multimeter that the pin sits at 2.8 V **before** powering the board. If your breakout ties `VREF` straight to `VCC`, then feeding it 3.3 V from the ESP32 rail will overdrive it — stop and get a breakout that regulates.
2. **Every ground is joined.** `ESP32.GND`, `MPU6050.GND`, `VL53L0X.GND`, and the button's second leg must all share the breadboard's `−` rail. I²C needs a real common ground, not a shared 3V3 connection.
3. **No rail short.** Confirm the `+` and `−` rails are not bridged before applying power.
4. **Continuity on SDA and SCL.** Each pull-up resistor must reach 3V3 and the correct data line; a resistor bridging the two data lines together is the classic breadboard error and shows up only as a completely dead bus.
5. **Button legs are opposite sides.** Confirm continuity between the two legs you chose before relying on them.
6. **No wire is on a flash pin.** Check that nothing is connected to pins 4, 5, 6, 10, 11, or 12 — the ones labelled `cmd`, `sd3`, `sd2`, `sd1`, `sdd`, `clk`. A board that boots now and not after you finish wiring is almost always this.

Once the above hold, connect the USB cable. The board powers the sensors through the 3V3 regulator, so there is no separate supply to sequence.

### Breadboard notes

- **The DevKit is wider than the breadboard's centre channel**, so its pins will not seat directly into the board. Use male-to-female jumpers from the ESP32's header pins into the breadboard, rather than trying to straddle it.
- **Place each 100 nF capacitor at its own module**, bridging that module's `VCC` and `GND` rows. Decoupling that has been stretched across the board does not decouple anything.
- **Power budget is not a concern here.** The two sensors draw a few tens of milliamps between them, well within the DevKit's 3V3 regulator. Do not add a laser or anything else powered from that rail without revisiting this.
- **Leave the sensor breakouts on their own rows.** Sharing a `VCC` row between both modules makes it ambiguous later when one of them misbehaves and you need to isolate which.

## Firmware contract

The firmware is being rewritten to match this wiring. When it exists, it is expected to meet the following. Until it does, treat `pointer/README.md` as unverified or absent rather than as documentation.

- **Read the MPU6050 directly over `Wire`.** No third-party library is required: a `WHO_AM_I` probe, four configuration writes, and one 14-byte burst per sample cover it. The VL53L0X driver is the one legitimate exception, because that part's init sequence is long and must be correct.
- **Emit CSV on the USB serial port**, one line per sample, with units named in the column headers — acceleration in g, angular rate in deg/s, temperature in degC, range in millimetres. Include an explicit status column rather than folding failure into the value.
- **Never emit a plausible-looking zero for a failed read.** A dropped transfer must be visibly missing data, so a disconnection is diagnosable from the stream alone rather than looking like a sensor reading zero.
- **Drive `XSHUT` through the startup sequence** described above, and gate all ToF traffic behind that.
- **Report presence, not just data.** The firmware should say at boot which addresses acknowledged, so a miswired sensor is named outright instead of surfacing later as an unexplained column of zeros.
- **Document every function and class**, per [§2](../AGENTS.md). The `.ino` is no exception.

## Build, flash, monitor

The toolchain is arduino-cli with the `esp32:esp32` core pinned in the sketch's `sketch.yaml`; the board is `esp32:esp32:nodemcu-32s`, the variant Espressif ships for this part.

The USB-UART device is owned by `root:uucp`, so the user account must be in the `uucp` group on Arch (`dialout` elsewhere) before the port can be opened at all:

```bash
sudo usermod -aG uucp "$USER"   # then log out and back in
```

Commands, from the sketch directory:

```bash
arduino-cli compile --profile pointer-sensor-node
arduino-cli upload  --profile pointer-sensor-node --port /dev/ttyUSB0
arduino-cli monitor --port /dev/ttyUSB0 --config baudrate=115200
```

**Pin no port in `sketch.yaml`.** arduino-cli aborts a `compile` outright when a `default_port` names a device that is not currently plugged in, which silently breaks offline builds, and the CP2102 renumbers between `/dev/ttyUSB0` and `/dev/ttyUSB1` across replugs anyway. Pass `--port` explicitly on every command that needs one.

Note that a monitor which attaches after the chip has already booted sees only the sample stream; the boot banner is emitted once, inside `setup()`. Prefer diagnostics that repeat periodically over relying on catching that one moment.

## A note on verification

[§4](../AGENTS.md) rules out a hardware bench phase, and that rule governs **thesis verification claims** — what may be asserted as evidence in the write-up. Breadboard bring-up is development, not verification, and reading a sensor off a board held in your hand is not a measurement of the kind §4 is restricting. The distinction to keep in mind: a number that came off this breadboard is a development observation, and promoting it to an evidence claim in the thesis still requires the §4 fallback order — simulation, then budget, then propagation, then citation.


