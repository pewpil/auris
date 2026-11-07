# Pointer hardware and firmware — AGENTS.md

Standing instructions and hardware specification for the Auris pointer. This file is the single source of truth for how the pointer is wired and what its firmware is expected to do.

> **Scope:** [`../AGENTS.md`](../AGENTS.md) §1–§6 apply here unchanged — the documentation standard (§2), the no-commit rule (§3), the verification policy (§4), the unwrapped-prose rule (§5), and the side-note convention (§6) all govern this directory. §1–§6 are repository-wide and apply inside `pointer/` exactly as they do everywhere else. This file follows §5: every prose paragraph is one unwrapped line.
>
> **Where this sits:** `pointer/` is an ordinary in-repo directory, not a submodule ([`../AGENTS.md` §7](../AGENTS.md)). The compute unit's code lives in the separate `cu/` submodule and is not affected by anything here.

## What the pointer is

The pointer is the handheld device of [§1.5](../AGENTS.md): it carries a laser and its button, an IMU that establishes the pointer's own orientation, and a time-of-flight sensor that gives the range to whatever the laser is aimed at. It does all of its own orientation fusion onboard and sends the result onward to the compute unit. It is not the compute unit — that is the Raspberry Pi, which lives in `cu/`.

**Current state: the deployable firmware exists and the bench harness is populated with both 100 nF capacitors and the optional 100 µF; the final enclosure, power source, and button check are open.** The pointer's **30-pin ESP32 DevKit V1**, the IMU breakout, and the ToF breakout are on a breadboard and the wiring follows the connection list below. The **38-pin NodeMCU-32S** that previously served as the bench pointer is now the compute unit's ESP-NOW bridge, wired to the Pi by USB. The firmware that runs on the pointer itself lives in [`firmware/pointer_sensor_node/`](firmware/pointer_sensor_node/AGENTS.md); the sibling sketches in `firmware/` are the bring-up record, including the button-only [`firmware/button_test/`](firmware/button_test/AGENTS.md). The bench harness is not yet the final pointer assembly, so its numbers stay development observations.

## Components

| Component | Part | Notes |
|---|---|---|
| MCU (pointer) | **30-pin ESP32 DevKit V1** (ESP32-D0WD-V3, ESP32-WROOM-32), silkscreen-labelled pins | 15 pins per row; the printed labels are the authority, not a row/column number. USB powers it and carries the serial link during provisioning. See [the pointer pin map](#pointer-board-pin-map-30-pin-devkit-v1) for the labels and their neighbours. |
| Compute-unit bridge | **38-pin NodeMCU-32S** (the previous bench pointer), USB to the Pi | Runs [`cu/espnow_bridge/`](../../cu/espnow_bridge/AGENTS.md); its pin table is [below](#compute-unit-bridge-pin-table-38-pin-nodemcu-32s). |
| IMU | MPU6050 on a GY-521 breakout | 3-axis accelerometer and gyroscope, plus die temperature |
| ToF | VL53L1X breakout (CJMCU-531, sold mislabeled "VL53L0X") | Single-zone time-of-flight, non-contact range; identity `EA CC 10 FF` (`0xEACC`) at 16-bit index `0x010F` |
| Input | Tactile push button, momentary, 4-leg | Laser trigger |
| Passives | 2 × 4.7 kΩ resistor, 2 × 100 nF ceramic capacitor | I²C pull-ups and decoupling |
| Common | Half-size breadboard, jumper wires, USB **data** cable | |

## Connection list

This is the wiring. Build it exactly as written, one row at a time, with the USB cable **disconnected** until the last row.

The pointer's board prints its pin names on the silkscreen, so the labels are the authority; the 38-pin bridge's row/column numbering does not apply here. The bottom row runs `3V3, GND, D15, D2, D4, RX2, TX2, D5, D18, D19, D21, RX0, TX0, D22, D23`; the top row is `VN, GND, D13, D12, D14, D27, D26, D25, D33, D32, D35, D34, VN, VP, EN` as printed. The used pins and their neighbours are in [the pointer pin map](#pointer-board-pin-map-30-pin-devkit-v1).

| From | To | Function |
|---|---|---|
| `D21` — **GPIO21** (bottom, between `D19` and `RX0`) | `MPU6050.SDA` and `VL53L1X.SDA` | I²C data — shared by both sensors |
| `D22` — **GPIO22** (bottom, between `TX0` and `D23`) | `MPU6050.SCL` and `VL53L1X.SCL` | I²C clock — shared by both sensors |
| `D27` — **GPIO27** (top, between `D14` and `D26`) | `VL53L1X.XSHUT` | ToF enable, actively driven — **not left floating** |
| `D18` — **GPIO18** (bottom, between `D5` and `D19`) | `Button.leg A` | Button input, internal pull-up |
| `3V3` (bottom, next to `GND`) | breadboard **+** rail, and from there both `VCC` pins | 3.3 V power |
| `GND` (bottom, next to `3V3`; top row also has `GND`) | breadboard **−** rail | Common ground |
| `Button.leg B` | breadboard **−** rail | Closes GPIO 18 to ground when held |
| `MPU6050.VCC` | breadboard **+** rail | Power |
| `MPU6050.GND` | breadboard **−** rail | Common ground |
| `MPU6050.AD0` | breadboard **−** rail | Selects I²C address `0x68` |
| `MPU6050.XDA`, `MPU6050.XCL`, `MPU6050.INT` | *no connection* | Auxiliary bus and interrupt, unused |
| `VL53L1X.VCC` | breadboard **+** rail | Power |
| `VL53L1X.GND` | breadboard **−** rail | Common ground |
| `VL53L1X.GPIO1` | *no connection* | Interrupt output, unused |
| `4.7 kΩ` #1 | between the SDA net and the **+** rail | SDA pull-up |
| `4.7 kΩ` #2 | between the SCL net and the **+** rail | SCL pull-up |
| `100 nF` #1 | across `MPU6050.VCC` and `MPU6050.GND` | Decoupling, place at the module |
| `100 nF` #2 | across `VL53L1X.VCC` and `VL53L1X.GND` | Decoupling, place at the module |

The pointer board exposes `GND` on both rows (bottom next to `3V3`, and one on the top row). Pick the bottom one next to the 3V3 feed and keep the sensor returns on the same rail. Do not use the `VIN`/5 V pin as a ground.

### Per-module detail

**MPU6050 (GY-521).** `VCC` to the **+** rail, `GND` to the **−** rail, `SDA` to `D21` (GPIO21), `SCL` to `D22` (GPIO22), `AD0` to ground. `XDA`, `XCL`, and `INT` stay unconnected. Tying `AD0` low puts the part at `0x68`; leaving it floating puts it at `0x69`, which nothing here probes for.

**VL53L1X (CJMCU-531).** `VCC` to the **+** rail, `GND` to the **−** rail, `SDA` and `SCL` shared with the IMU, `XSHUT` to `D27` (GPIO27), `GPIO1` unconnected. The CJMCU-531 regulates its own 2.8 V core from the 3.3 V rail; feed it from 3V3 only and never put 5 V on its `VCC`. **Read the "Verify before first power-up" section before connecting this one** — its supply requirements differ from the IMU's.

**Button.** One leg to `D18` (GPIO18), the opposite leg to ground. The firmware configures GPIO 18 as `INPUT_PULLUP`, so an idle button reads `HIGH` and a pressed one reads `LOW`. On a four-leg tactile switch, pick two legs on **opposite sides** of the switch's electrical contact; the two outer legs on the same side are internally connected, so a button wired across one side is permanently closed and reads pressed all the time.

## I²C bus

Both sensors share one bus. There is no address conflict: the MPU6050 answers at `0x68` (or `0x69` if `AD0` is tied high), and the **VL53L1X has no address pins at all** — it is fixed at `0x29`.

**Fit both 4.7 kΩ pull-ups even though the GY-521 usually carries its own.** If your GY-521 already has 4.7 kΩ on each line, the two in parallel give roughly 2.3 kΩ, which is still comfortably within spec for 400 kHz over breadboard-length wiring. Do not stack three sets — that approaches 1.6 kΩ and is needlessly stiff for the bus.

Run the bus at **400 kHz**, which both parts rate well above. Keep SDA and SCL runs short and, where you must, route them as a pair; breadboard capacitance is small but the pull-ups are the only thing keeping the lines from ringing.

## `XSHUT` must be driven, not floated

This is the single most important change from the previous build, and it is a correction rather than a preference.

With `XSHUT` left unconnected, the ToF either never enables or holds the bus in a state that makes **every other device on it unreadable**. In the previous prototype the IMU never produced a valid sample with the ToF attached — readings came back as dropped transfers or as all-zero register dumps — and recovered completely once the ToF was removed. Driving `XSHUT` from `D27` (GPIO27) is what makes it possible to have both sensors on one bus.

The firmware must therefore assert `XSHUT` deliberately rather than assuming the sensor comes up on its own: hold it **low** briefly to force a hardware reset, release it **high**, and wait for the part to become responsive before issuing any I²C transaction to it. The datasheet requires a settling interval after enable; treat anything under about 100 ms as unverified.

## Pointer board pin map (30-pin DevKit V1)

The pointer uses the **30-pin ESP32 DevKit V1**. Its pins are labelled on the silkscreen, and those labels are the authority; there is no row/column number to translate. The two rows as reported are:

| Row | Labels, left to right |
|---|---|
| Top | `VN`, `GND`, `D13`, `D12`, `D14`, `D27`, `D26`, `D25`, `D33`, `D32`, `D35`, `D34`, `VN`, `VP`, `EN` |
| Bottom | `3V3`, `GND`, `D15`, `D2`, `D4`, `RX2`, `TX2`, `D5`, `D18`, `D19`, `D21`, `RX0`, `TX0`, `D22`, `D23` |

The top row has 15 labels and two `VN`s; `D14` sits between `D12` and `D27`, and `D27` sits between `D14` and `D26`. The printed labels are the authority; the signals this build uses are unambiguous: `D27` is the ToF enable, and `D18`, `D21`, `D22`, `3V3` and `GND` are on the bottom row as listed.

| Signal | Pin | Neighbours / note |
|---|---|---|
| I²C SDA | `D21` (GPIO21) | bottom row, between `D19` and `RX0` |
| I²C SCL | `D22` (GPIO22) | bottom row, between `TX0` and `D23` |
| ToF XSHUT | `D27` (GPIO27) | top row, between `D14` and `D26`; driven, never floated |
| Button | `D18` (GPIO18) | bottom row, between `D5` and `D19`; `INPUT_PULLUP`, pressed LOW |
| Onboard LED | `D2` (GPIO2) | bottom row, between `D15` and `D4`; no external wire |
| 3V3 rail | `3V3` | bottom row, next to `GND` |
| Ground rail | `GND` | bottom row, next to `3V3`; the top row also exposes `GND` |

The 30-pin board does not break out the SPI-flash pins, so the flash-pin warning below applies only to the 38-pin bridge.

## Compute-unit bridge pin table (38-pin NodeMCU-32S)

This table is for the **bridge**, not the pointer; the pointer's labels are above. This is a **38-pin NodeMCU-32S** (Ai-Thinker layout, 2.54 mm pitch). The board is named for the 32 usable GPIOs it breaks out, but it also exposes pins wired to the module's own SPI flash, and those must never be used. This board labels them by their flash function rather than by GPIO number, so they read like harmless signal names.

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

The pointer's four signals are **`D18`**, **`D21`**, **`D22`** and **`D27`** (GPIO18, 21, 22, 27), with the onboard LED on **`D2`** (GPIO2). The bridge uses only its USB connection to the Pi; the 38-pin table above is retained for the bridge hardware, not the pointer.

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

1. **The ToF breakout must regulate its own 2.8 V core supply.** The VL53L1X's laser and core run at 2.8 V; the CJMCU-531 provides that regulation from the 3V3 rail. Feed the module from the 3.3 V rail only — never 5 V on its `VCC` — and do not substitute a bare VL53L1X die without its regulator. The CJMCU-531 has been used on this bench across several sessions and is the supported module.
2. **Every ground is joined.** `ESP32.GND`, `MPU6050.GND`, `VL53L1X.GND`, and the button's second leg must all share the breadboard's `−` rail. I²C needs a real common ground, not a shared 3V3 connection.
3. **No rail short.** Confirm the `+` and `−` rails are not bridged before applying power.
4. **Continuity on SDA and SCL.** Each pull-up resistor must reach 3V3 and the correct data line; a resistor bridging the two data lines together is the classic breadboard error and shows up only as a completely dead bus.
5. **Button legs are opposite sides.** Confirm continuity between the two legs you chose before relying on them.
6. **No wire is on a flash pin.** On the 38-pin bridge, check that nothing is connected to pins 4, 5, 6, 10, 11, or 12 — the ones labelled `cmd`, `sd3`, `sd2`, `sd1`, `sdd`, `clk`. The pointer's 30-pin board does not break those pins out; check its labels against [the pointer pin map](#pointer-board-pin-map-30-pin-devkit-v1).

Once the above hold, connect the USB cable. The board powers the sensors through the 3V3 regulator, so there is no separate supply to sequence.

### Breadboard notes

- **The DevKit is wider than the breadboard's centre channel**, so its pins will not seat directly into the board. Use male-to-female jumpers from the ESP32's header pins into the breadboard, rather than trying to straddle it.
- **Place each 100 nF capacitor at its own module**, bridging that module's `VCC` and `GND` rows. Decoupling that has been stretched across the board does not decouple anything.
- **Power budget is not a concern here.** The two sensors draw a few tens of milliamps between them, well within the DevKit's 3V3 regulator. Do not add a laser or anything else powered from that rail without revisiting this.
- **Leave the sensor breakouts on their own rows.** Sharing a `VCC` row between both modules makes it ambiguous later when one of them misbehaves and you need to isolate which.

## Firmware contract

The deployable firmware in [`firmware/pointer_sensor_node/`](firmware/pointer_sensor_node/AGENTS.md) now implements the following contract. The bring-up sketches in `firmware/` remain a record, not the deployment.

- **Read the MPU6050 directly over `Wire`.** No third-party library is required: a `WHO_AM_I` probe, four configuration writes, and one 14-byte burst per sample cover it. The VL53L1X driver is the one legitimate exception, because that part's init sequence is long and must be correct.
- **Emit a compact fixed-width table on the USB serial port**, one line per sample, narrow enough for one 80-column terminal row: the fused orientation, acceleration in g, range in millimetres, range status, range age and flags, with units in the column headers. Include an explicit status column rather than folding failure into the value. The link sends the full data as a versioned `P1` CSV over ESP-NOW while the button is held; the table is the human view, the CSV is the machine schema, and the quaternion, gyro, temperature, sequence and reset reason live in the packet.
- **While the button is held, send the fused orientation and range onward to the compute unit.** The link is **ESP-NOW**, a direct peer-to-peer radio link between the pointer and the compute unit with no access point, router, DHCP server or IP addressing in the path — see [The link is ESP-NOW, not Wi-Fi UDP](#the-link-is-esp-now-not-wi-fi-udp). Release stops the stream, and the compute unit times out so a lost release cannot leave sound running. The current bench build sets `LINK_BUTTON_GATED` false and streams continuously so the link can be exercised without a button; the gated behaviour above is the deployment target.
- **Persist per-device calibration.** The GY-521 on this bench has a zero-g offset far outside the MPU6050 datasheet, so the firmware stores accelerometer and gyro offsets in NVS through the `@CAL` procedure rather than trusting raw registers.
- **Never emit a plausible-looking zero for a failed read.** A dropped transfer must be visibly missing data, so a disconnection is diagnosable from the stream alone rather than looking like a sensor reading zero.
- **Drive `XSHUT` through the startup sequence** described above, and gate all ToF traffic behind that.
- **Report presence, not just data.** The firmware should say at boot which addresses acknowledged, so a miswired sensor is named outright instead of surfacing later as an unexplained column of zeros.
- **Document every function and class**, per [§2](../AGENTS.md). The `.ino` is no exception.

### The link is ESP-NOW, not Wi-Fi UDP

**The pointer and the compute unit must be the only two devices in the link.** This is a hard constraint rather than a preference: the connection may not depend on a router, an access point, or any other device that neither side owns.

**The firmware in `firmware/pointer_sensor_node/` now implements it.** `src/link.h` no longer includes `WiFiUdp.h`; `src/link.cpp` starts the Wi-Fi driver in station mode with no SSID, calls `WiFi.disconnect()`, pins channel 1, initialises ESP-NOW and registers the bridge's MAC as an unencrypted peer. `cfg::LINK_PORT`, `@DEST` and the stored unicast override are gone. The transmitter is compiled but not yet flashed, because the pointer board was not connected to the build machine.

**ESP-NOW replaces it.** ESP-NOW is a peer-to-peer radio link with no access point, no router, no DHCP and no IP addressing, so it involves exactly the two boards. The ESP32 speaks it natively. The Raspberry Pi's radio cannot, so the compute unit carries a second ESP32 as its ESP-NOW interface, already built and flashed as [`cu/espnow_bridge/`](../../cu/espnow_bridge/AGENTS.md), which relays received payloads verbatim onto USB serial for `pointer_link` to read. The resulting path is:

```
pointer ESP32  --ESP-NOW-->  bridge ESP32  --USB serial-->  Raspberry Pi
```

**The bridge board is the 38-pin NodeMCU-32S previously used as the bench pointer; the pointer is now the 30-pin DevKit V1.** Both use the ESP32-D0WD-V3, but the board layouts differ. Its identity was read off the hardware rather than assumed, because the only thing that reliably tells the two boards apart is the MAC address:

| | |
|---|---|
| Chip | **ESP32-D0WD-V3, revision v3.1**, dual core + LP core, 240 MHz |
| Flash | 4 MB (manufacturer `0x68`, device `0x4016`) |
| Crystal | 40 MHz |
| USB-UART bridge | CP2102 (`10c4:ea60`), presents as `/dev/ttyUSB0` |
| **MAC (station)** | **`28:05:A5:E2:F2:78`** |

That MAC is confirmed two independent ways: `esptool` reads it from eFuse, and the bridge firmware prints it at boot. It was re-read on 31 Oct 2026 after the boards were reassigned: the pointer on the laptop is `B0:CB:D8:CF:70:1C`, and the bridge on the Pi is `28:05:A5:E2:F2:78`. The MAC, not the board appearance, is the identity. They agree. This matters because reading the MAC before the Wi-Fi driver starts returns an unset value that looks entirely plausible — an early build of the bridge reported `CC:09:FC:3F:03:00` — so a board identified that way could have been mistaken for the pointer.

These link facts were established on that hardware, so the pointer-side swap can be written against them:

| | |
|---|---|
| ESP-NOW channel | **1** — both ends must agree; a mismatch receives nothing while looking perfectly healthy |
| Peer address | the bridge's MAC above, for unicast; broadcast (`FF:FF:FF:FF:FF:FF`) needs no peer address |
| Payload limit | 250 bytes per ESP-NOW packet |
| `P1` worst case | **179 bytes**, measured against maximum field widths — 71 bytes of headroom |
| Packet schema | unchanged; ESP-NOW carries the same `P1` CSV as opaque bytes |

**The swap is complete.** `src/link.h` and `src/link.cpp` are the entire transport, and `Link`'s interface is what the rest of the firmware calls, so the sensor, fusion and packet code did not change. `cfg::LINK_PORT` is gone, `@DEST` and the stored unicast override are gone, and `@STATUS` now prints the peer MAC, channel and readiness instead of an IP, destination and RSSI.

**Both ends start the Wi-Fi driver without joining anything.** ESP-NOW requires the driver to be running, so each board is put into station mode with no SSID configured and `WiFi.disconnect()` called. Neither associates with a network, which is what keeps the link two-node while still using the radio. The compute unit's side already behaves this way and is verified to boot with no association; the pointer side now does the same in `Link::begin()`.

## Build, flash, monitor

The toolchain is arduino-cli with the `esp32:esp32` core pinned in the sketch's `sketch.yaml`. The pointer is **`esp32:esp32:esp32doit-devkit-v1`** (30-pin DevKit V1); the 38-pin bridge is `esp32:esp32:nodemcu-32s` and has its own firmware in `cu/espnow_bridge/`.

The USB-UART device is owned by `root:uucp`, so the user account must be in the `uucp` group on Arch (`dialout` elsewhere) before the port can be opened at all:

```bash
sudo usermod -aG uucp "$USER"   # then log out and back in
```

Commands, from the deployed sketch directory [`firmware/pointer_sensor_node/`](firmware/pointer_sensor_node/AGENTS.md):

```bash
arduino-cli compile --profile pointer-sensor-node
arduino-cli upload  --profile pointer-sensor-node --port /dev/ttyUSB0
arduino-cli monitor --port /dev/ttyUSB0 --config baudrate=115200
```

That directory's `build.sh` writes the flashable images into `dist/`. The pointer firmware now uses ESP-NOW: it has no `wifi_secrets.h`, no SSID and no association. It is compiled but not yet flashed. The sibling bring-up sketches keep their own profiles (`imu-test`, `node-test`, `button-test`, …) and are not the deployment.

**Pin no port in `sketch.yaml`.** arduino-cli aborts a `compile` outright when a `default_port` names a device that is not currently plugged in, which silently breaks offline builds, and the CP2102 renumbers between `/dev/ttyUSB0` and `/dev/ttyUSB1` across replugs anyway. Pass `--port` explicitly on every command that needs one.

Note that a monitor which attaches after the chip has already booted sees only the sample stream; the boot banner is emitted once, inside `setup()`. Prefer diagnostics that repeat periodically over relying on catching that one moment.

## A note on verification

[§4](../AGENTS.md) rules out a hardware bench phase, and that rule governs **thesis verification claims** — what may be asserted as evidence in the write-up. Breadboard bring-up is development, not verification, and reading a sensor off a board held in your hand is not a measurement of the kind §4 is restricting. The distinction to keep in mind: a number that came off this breadboard is a development observation, and promoting it to an evidence claim in the thesis still requires the §4 fallback order — simulation, then budget, then propagation, then citation.


