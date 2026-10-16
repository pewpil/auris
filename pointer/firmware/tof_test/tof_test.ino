/*
 * Auris pointer — VL53L0X time-of-flight bring-up test.
 *
 * TOF-ONLY TEST WIRING  (MPU6050 deliberately NOT connected)
 *
 *   The two rows are numbered differently: r1 is the top row numbered 1-19, r2 is
 *   the bottom row numbered 60-42 across the same columns.  A column therefore has
 *   two unrelated numbers - r1 #9 and r2 #50 sit side by side.
 *
 *   ESP32 r1 #19  3V3    --  VL53L0X  VCC
 *   ESP32 r2 #42  GND    --  VL53L0X  GND
 *   ESP32 r2 #47  GPIO21 --  VL53L0X  SDA
 *   ESP32 r2 #44  GPIO22 --  VL53L0X  SCL
 *   ESP32 r1 #9   GPIO27 --  VL53L0X  XSHUT    (driven, never left floating)
 *   ESP32 r2 #50  GPIO18 --  button   leg A
 *   ESP32 r2 #42  GND    --  button   leg B
 *
 *   VL53L0X  GPIO1  --  not connected
 *
 *   Ground: r2 #42, or r1 #2 / r1 #6 / r2 #48 -- any of them.
 *   PULL-UPS ARE MANDATORY HERE: this breakout carries none, so fit 4.7k from SDA
 *   to +3V3 and 4.7k from SCL to +3V3.  The GY-521 brings its own; the VL53L0X
 *   does not, and with the GY-521 absent this test has nothing else supplying them.
 *
 * Purpose: stream the VL53L0X range as CSV on the USB serial port, continuously,
 *          until the board is reset; and, WHILE THE BUTTON IS HELD, broadcast the
 *          same rows to the compute unit over UDP.
 *
 *          The button is not a data column. It gates the wireless link: held, samples
 *          are transmitted; released, transmission stops. The local USB stream runs
 *          either way.
 *
 *          Unlike the IMU, the line rate here is bounded by the RANGING PERIOD, not by
 *          LOOP_PERIOD_MS: each iteration blocks waiting for the next measurement, so
 *          the stream runs at the measurement rate set below and no faster. Raising
 *          TOF_PERIOD_MS lowers latency per sample but not the rate; lowering it is
 *          what raises the rate, at the cost of ranging accuracy.
 *
 * Inputs:  VL53L0X on I2C address 0x29 (fixed, the part has no address pins);
 *          momentary button pulling GPIO 18 to GND; Wi-Fi credentials from the
 *          untracked wifi_secrets.h.
 * Outputs: CSV on the USB serial port at 115200 baud; the same rows broadcast to the
 *          compute unit over UDP port 5005 while the button is held.
 *
 * Build:   arduino-cli compile --profile tof-test
 * Flash:   arduino-cli upload  --profile tof-test --port /dev/ttyUSB0
 */

#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>   // core 3.x spells it this way; it typedefs WiFiUDP
#include <VL53L0X.h>

// Credentials live in an untracked header so the password never reaches git. The
// sketch still builds without it, falling back to USB serial only.
#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define HAVE_WIFI_CREDENTIALS 1
#endif

// ---------------------------------------------------------------------------
// Pin map — matches the wiring comment above.
// ---------------------------------------------------------------------------

/** ESP32 I2C SDA line, at r2 #47. */
static constexpr uint8_t PIN_I2C_SDA = 21;
/** ESP32 I2C SCL line, at r2 #44. */
static constexpr uint8_t PIN_I2C_SCL = 22;
/** VL53L0X XSHUT line, at r1 #9. Driven, never left floating. */
static constexpr uint8_t PIN_TOF_XSHUT = 27;
/** Button input, at r2 #50; configured INPUT_PULLUP so a press reads LOW. */
static constexpr uint8_t PIN_BUTTON = 18;

/** The VL53L0X has no address pins; this is fixed in the part. */
static constexpr uint8_t ADDR_VL53L0X = 0x29;

/**
 * I2C bus speed in Hz.
 *
 * Deliberately far below the 400 kHz the parts rate, because this bus is on a breadboard
 * with jumper leads and the data phase was returning nondeterministic bytes: the address
 * phase acknowledged correctly, so the sensor was found and reached, while register
 * reads returned different garbage every time. That is a rise-time/bus-capacitance
 * signature rather than a wrong address — slow enough and the timing closes up.
 *
 * Raise it only once a short, properly-pulled-up bus reads reliably.
 */
static constexpr uint32_t I2C_CLOCK_HZ = 100000;

// ---------------------------------------------------------------------------
// Pointer -> compute unit link. Identical to the IMU test; see that sketch.
// ---------------------------------------------------------------------------

/** UDP port the compute unit listens on. Must match --port on the cu side. */
static constexpr uint16_t LINK_PORT = 5005;

/** Milliseconds allowed for the Wi-Fi association to complete at boot. */
static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;

/** Optional unicast override; empty means broadcast to the subnet. */
static const char *LINK_UNICAST_HOST = "";

/** Whether the link came up and datagrams may be sent. */
static bool linkUp = false;

/** Set on the first send failure, so repeated failures do not spam the serial log. */
static bool linkFaultReported = false;

/** Last observed button state, so press and release are reported once each. */
static bool wasButtonHeld = false;

/** Timestamp of the last periodic sensor probe. */
static uint32_t lastProbeMs = 0;

// ---------------------------------------------------------------------------
// VL53L0X timings and modes.
// ---------------------------------------------------------------------------

/**
 * Per-measurement ranging budget in microseconds.
 *
 * The datasheet's floor is 19.5 ms. Longer budgets buy accuracy and cost rate; the
 * measurement period below has to exceed this whatever it is set to.
 */
static constexpr uint32_t TOF_TIMING_BUDGET_US = 33000;

/** Inter-measurement period in milliseconds, giving about 20 ranges per second. */
static constexpr uint32_t TOF_PERIOD_MS = 50;

/**
 * Driver I2C transaction timeout in milliseconds.
 *
 * The Pololu driver defaults this to zero, which DISABLES the timeout, leaving
 * readRangeContinuousMillimeters() spinning forever on a sensor that stops
 * reporting. On the board that presents as a frozen stream, not as an error, so it
 * is set explicitly here.
 */
static constexpr uint16_t TOF_IO_TIMEOUT_MS = 200;

/** How long XSHUT is held low to force a hard reset, in milliseconds. */
static constexpr uint32_t XSHUT_RESET_MS = 20;

/**
 * How long to wait after releasing XSHUT before talking to the sensor, in
 * milliseconds. The part needs its oscillator to start and its boot sequence to run;
 * addressing it too early is the classic "it never responds" failure. 200 ms is
 * comfortably above the datasheet's requirement.
 */
static constexpr uint32_t XSHUT_SETTLE_MS = 200;

/**
 * Ranges discarded at the start of continuous ranging.
 *
 * The first measurement after startContinuous() can reflect a partially settled
 * state and is not worth reporting.
 */
static constexpr uint8_t TOF_WARMUP_READINGS = 3;

/**
 * Whether init() switches the sensor's I/O pads into 2.8 V mode.
 *
 * Most VL53L0X breakouts feed VREF from an on-board 2.8 V regulator and are meant to
 * be initialised this way, which is also the driver's own default. It only has to be
 * false for a module that ties VREF straight to VCC.
 *
 * This is a precondition the firmware cannot verify. Confirm VREF sits at 2.8 V with
 * a meter BEFORE powering the board; feeding a part whose VREF is tied to VCC from
 * the 3V3 rail overdrives it.
 */
static constexpr bool TOF_IO_2V8 = true;

/** Sensor return code meaning the echo was too weak to measure. */
static constexpr uint16_t TOF_SIGNAL_FAIL = 8190;

/** Sensor return code meaning no target was in range. */
static constexpr uint16_t TOF_OUT_OF_RANGE = 8191;

/** Sensor return code the driver substitutes when its I2C transaction times out. */
static constexpr uint16_t TOF_TIMEOUT = 65535;

/** Why a range reading is or is not usable, as written into the status column. */
enum class ToFStatus : uint8_t {
  Valid = 0,        ///< dist_mm holds a real measurement.
  SignalFail = 1,   ///< Sensor returned 8190; target too dark or too far.
  OutOfRange = 2,   ///< Sensor returned 8191; no target within its range.
  Timeout = 3,      ///< No fresh measurement arrived within TOF_IO_TIMEOUT_MS.
  Unavailable = 4,  ///< Sensor did not come up at boot, so no reading was attempted.
};

/**
 * One decoded range sample.
 *
 * @c raw_mm is carried alongside the decoded value on purpose. Whether the driver
 * returns a bare millimetre count or leaves any status bits encoded in the same word
 * cannot be settled without the sensor on the bench, and the two possibilities are
 * indistinguishable in the decoded column alone: an encoding that shifted the range
 * would still print a plausible millimetre figure. Keeping the untouched value
 * visible makes any such mismatch obvious on the first sample instead of surfacing
 * later as a calibration error.
 */
struct ToFSample {
  uint16_t raw_mm;   ///< Driver's return value verbatim, before any decoding.
  uint16_t dist_mm;  ///< Distance in millimetres; 0 unless status is Valid.
  ToFStatus status;  ///< Why the reading is or is not usable.
  bool fresh;        ///< False when the sensor is not up, so nothing was attempted.
};

/** Pololu driver instance owning the VL53L0X; its state lives across the loop. */
static VL53L0X tof;

/** Whether the sensor came up at boot; gates every range read in loop(). */
static bool tofReady = false;

/**
 * Identity registers captured once at boot and re-reported on every probe.
 *
 * Held rather than re-read each time because they are the evidence for *why* init
 * failed, and a reader that attaches after setup() has finished would otherwise never
 * see it. The boot banner is emitted once and cannot be relied on to catch.
 */
static uint8_t tofModelByte = 0xFF;
static uint8_t tofRevisionByte = 0xFF;

/**
 * Short token naming why the sensor failed to initialise, or nullptr when it came up.
 *
 * Deliberately a token and not a sentence. The full explanation belongs in the boot
 * banner, which is printed once and read by eye; this one is reprinted every probe, and
 * a sentence here overflows the 256-byte serial TX buffer and truncates the line
 * mid-word — which cost more diagnosis time than it saved.
 */
static const char *tofInitFailure = nullptr;

/**
 * A control address that nothing on this bus should ever answer to.
 *
 * Probed alongside the sensor to separate two causes that otherwise look identical: a
 * genuine VL53L0X acknowledging at 0x29, versus a bus whose SDA is held low, in which
 * case *every* address acknowledges because the master reads a low bit as an ACK and
 * no register read ever completes. A breadboard short of SDA to ground is the ordinary
 * way to get the second one, and it is indistinguishable from a real sensor unless a
 * deliberately unused address is probed too.
 */
static constexpr uint8_t ADDR_CONTROL_PROBE = 0x62;

/**
 * Work out where datagrams should be sent.
 *
 * @return LINK_UNICAST_HOST when set, otherwise the subnet's directed broadcast
 *     address derived from the address DHCP handed out.
 */
static IPAddress linkDestination() {
  if (strlen(LINK_UNICAST_HOST) > 0) {
    return IPAddress(LINK_UNICAST_HOST);
  }
  const IPAddress ip = WiFi.localIP();
  const IPAddress subnet = WiFi.subnetMask();
  return IPAddress(static_cast<uint8_t>(ip[0] | ~subnet[0]),
                   static_cast<uint8_t>(ip[1] | ~subnet[1]),
                   static_cast<uint8_t>(ip[2] | ~subnet[2]),
                   static_cast<uint8_t>(ip[3] | ~subnet[3]));
}

/**
 * Model identification register. ST specifies 0xEE for the VL53L0X.
 */
static constexpr uint8_t REG_MODEL_ID = 0xC0;

/**
 * Silicon revision register, read alongside the model byte to tell a genuine part
 * from a module whose ID does not match what this driver expects.
 */
static constexpr uint8_t REG_REVISION_ID = 0xC2;

/**
 * Model byte ST specifies for a VL53L0X.
 */
static constexpr uint8_t MODEL_ID_VL53L0X = 0xEE;

/**
 /**
 * Result of the most recent endTransmission() inside readByte().
 *
 * Kept because a register read that returns the wrong bytes and one that returns the
 * right bytes for the wrong reason look identical from the value alone: 0 on success,
 * 2 for NACK, 1 for a data error. Printing it next to the value is what separates "the
 * sensor answered with something unexpected" from "the transfer did not complete".
 */
static uint8_t gLastWireStatus = 0xFF;

/**
 * Read one byte from a sensor register over I2C.
 *
 * @param addr 7-bit device address.
 * @param reg Register offset.
 * @return The register value, or 0xFF if the read did not complete.
 */
static uint8_t readByte(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  gLastWireStatus = Wire.endTransmission();
  if (gLastWireStatus != 0) {
    return 0xFF;
  }
  if (Wire.requestFrom(addr, static_cast<uint8_t>(1)) != 1) {
    return 0xFF;
  }
  return static_cast<uint8_t>(Wire.read());
}

/**
 * Read the electrical level of each bus line while the bus is idle.
 *
 * Both lines are pulled up by the external 4.7k resistors, so at rest — no I2C traffic,
 * nobody driving — SDA and SCL must both sit at the positive rail. Reading them back as
 * plain inputs, with no internal pull-up enabled, measures the real line rather than
 * what the pad would be forced to, and so separates two faults that look identical from
 * the application layer: a pull-up that is not working leaves the line low, while a
 * healthy idle bus with a device that still will not answer points at the module.
 *
 * The lines are restored to the I2C peripheral immediately afterwards, so this can run
 * between transactions without disturbing the bus.
 *
 * @param[out] sdaIdle Set true when SDA reads high while the bus is idle.
 * @param[out] sclIdle Set true when SCL reads high while the bus is idle.
 */
static void readIdleBusLevels(bool &sdaIdle, bool &sclIdle) {
  pinMode(PIN_I2C_SDA, INPUT);
  pinMode(PIN_I2C_SCL, INPUT);
  delayMicroseconds(200);  // let the line settle after the mode change

  sdaIdle = digitalRead(PIN_I2C_SDA) == HIGH;
  sclIdle = digitalRead(PIN_I2C_SCL) == HIGH;

  Wire.end();
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(I2C_CLOCK_HZ);
}

/**
 * Report whether a single I2C address acknowledges.
 *
 * @param addr 7-bit address to probe.
 * @return true when something acknowledged.
 */
static bool addressResponds(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

/**
 * Print the sensor's identity registers.
 *
 * Printed before init() so that a failure can be attributed to a cause rather than
 * guessed at. The driver's init() returns a bare false from three different places,
 * and they have entirely different remedies: a model byte that is not 0xEE means the
 * module is not the part this driver expects, while a correct model byte with init()
 * still failing points at the SPAD map read or the reference calibration, which is
 * where an unsettled XSHUT or a VREF that is not at 2.8 V shows up.
 */
static void reportSensorIdentity() {
  const uint8_t model = readByte(ADDR_VL53L0X, REG_MODEL_ID);
  const uint8_t revision = readByte(ADDR_VL53L0X, REG_REVISION_ID);
  Serial.printf("VL53L0X: model 0x%02X at 0x%02X %s, revision 0x%02X\n",
                model, REG_MODEL_ID, model == MODEL_ID_VL53L0X ? "(expected)" : "(NOT 0xEE!)",
                revision);
  if (model == 0xFF || revision == 0xFF) {
    Serial.println("          0xFF means the register did not read back. The device is");
    Serial.println("          acknowledging its address but is not completing transactions --");
    Serial.println("          check the pull-ups, and that SDA/SCL are not swapped.");
  }
}

/**
 * Drive XSHUT through a hard reset and bring the sensor up.
 *
 * This is the whole point of the exercise. With XSHUT floating, the previous
 * prototype's ToF either never enabled or left the shared I2C bus in a state that
 * made every other device on it unreadable — the IMU returned dropped transfers and
 * all-zero register dumps, and recovered completely only once the ToF was removed.
 * Owning the line makes the sensor's power-on state a decision rather than a hope.
 *
 * @return true when the sensor acknowledged and continuous ranging was started;
 *         false leaves @c tofReady false and every range column unavailable.
 */
static bool tofInit() {
  pinMode(PIN_TOF_XSHUT, OUTPUT);
  digitalWrite(PIN_TOF_XSHUT, LOW);
  delay(XSHUT_RESET_MS);
  digitalWrite(PIN_TOF_XSHUT, HIGH);
  delay(XSHUT_SETTLE_MS);

  if (!addressResponds(ADDR_VL53L0X)) {
    Serial.printf("VL53L0X: no ACK at 0x%02X even after driving XSHUT high.\n", ADDR_VL53L0X);
    Serial.println("          Check VCC, GND, the 2.8V on VREF, and the pull-ups on");
    Serial.println("          SDA and SCL — this breakout carries none of its own.");
    tofInitFailure = "no-ack@0x29";
    return false;
  }

  tofModelByte = readByte(ADDR_VL53L0X, REG_MODEL_ID);
  tofRevisionByte = readByte(ADDR_VL53L0X, REG_REVISION_ID);
  reportSensorIdentity();

  tof.setAddress(ADDR_VL53L0X);
  tof.setTimeout(TOF_IO_TIMEOUT_MS);
  if (!tof.init(TOF_IO_2V8)) {
    Serial.printf("VL53L0X: answered at 0x%02X but failed to initialise.\n", ADDR_VL53L0X);
    Serial.println("          The driver returns false from three places: a model byte");
    Serial.println("          check, the SPAD map read, and the reference calibration.");
    Serial.println("          If the model byte above was 0xEE, the failure is in one of");
    Serial.println("          the latter two: XSHUT settling (XSHUT_SETTLE_MS) or VREF.");
    Serial.println("          Check VREF is 2.8 V with a meter, and TOF_IO_2V8.");
    if (tofModelByte != MODEL_ID_VL53L0X) {
      tofInitFailure = "bad-model-id";
    } else {
      tofInitFailure = "spad-map-or-calibration";
    }
    return false;
  }
  if (!tof.setMeasurementTimingBudget(TOF_TIMING_BUDGET_US)) {
    Serial.printf("VL53L0X: %lu us budget refused; using the driver default\n",
                  static_cast<unsigned long>(TOF_TIMING_BUDGET_US));
  }

  tof.startContinuous(TOF_PERIOD_MS);
  return true;
}

/**
 * Classify a raw VL53L0X reading.
 *
 * @param rawMm Value returned by readRangeContinuousMillimeters(), in millimetres.
 * @param timedOut Whether the driver's own timeout flag was raised for this read.
 * @return Valid only for a genuine measurement; the driver's sentinel codes and its
 *         timeout map to their own status, so a failed reading is never read as a
 *         distance.
 */
static ToFStatus classifyRange(uint16_t rawMm, bool timedOut) {
  if (timedOut || rawMm == TOF_TIMEOUT) {
    return ToFStatus::Timeout;
  }
  if (rawMm == TOF_SIGNAL_FAIL) {
    return ToFStatus::SignalFail;
  }
  if (rawMm >= TOF_OUT_OF_RANGE) {
    return ToFStatus::OutOfRange;
  }
  return ToFStatus::Valid;
}

/**
 * Take one range reading.
 *
 * @param[out] out Decoded sample; untouched on failure.
 */
static void readRange(ToFSample &out) {
  out.fresh = false;
  out.raw_mm = 0;
  out.dist_mm = 0;
  out.status = ToFStatus::Unavailable;
  if (!tofReady) {
    return;
  }

  out.raw_mm = tof.readRangeContinuousMillimeters();
  const bool timedOut = tof.timeoutOccurred();  // Reads and clears the driver flag.
  out.status = classifyRange(out.raw_mm, timedOut);
  out.dist_mm = (out.status == ToFStatus::Valid) ? out.raw_mm : 0;
  out.fresh = true;
}

/**
 * Sample the laser button.
 *
 * @return true while the button is held down, i.e. while GPIO 18 is pulled to GND.
 */
static bool buttonPressed() {
  return digitalRead(PIN_BUTTON) == LOW;
}

/**
 * Print the CSV header naming every column and its unit.
 */
static void emitCsvHeader() {
  Serial.println("t_ms,raw,dist_mm,status");
}

/**
 * Format one sample as a CSV row into a caller-supplied buffer.
 *
 * The same formatted row is used for both sinks, so the USB record and the copy the
 * compute unit receives can never disagree about what was measured.
 *
 * @param state Decoded range sample.
 * @param[out] buf Destination buffer; always NUL-terminated.
 * @param capacity Size of @p buf in bytes, including room for the terminator.
 * @return Number of characters written, excluding the terminator.
 */
static size_t formatCsvRow(const ToFSample &state, char *buf, size_t capacity) {
  return snprintf(buf, capacity, "%lu,%u,%u,%u", static_cast<unsigned long>(millis()),
                  static_cast<unsigned>(state.raw_mm),
                  static_cast<unsigned>(state.dist_mm),
                  static_cast<unsigned>(state.status));
}

/**
 * Scan and print every network this module can currently see.
 *
 * Printed after a failed join, because the most likely cause is a 5 GHz SSID: the
 * ESP32-WROOM-32 radio is 2.4 GHz only, so such a network does not appear in a scan
 * and does not fail for any reason a status code alone would explain.
 */
static void scanAndPrintNetworks() {
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_OFF);
  delay(300);
  WiFi.mode(WIFI_STA);
  delay(300);

  const int found = WiFi.scanNetworks(false, true);
  if (found < 0) {
    Serial.println("wifi: scan failed to start");
    return;
  }
  Serial.printf("wifi: %d network(s) in range\n", found);
  for (int i = 0; i < found; ++i) {
    Serial.printf("  [%d] %s  ch=%d  rssi=%d dBm  %s\n", i, WiFi.SSID(i).c_str(),
                  WiFi.channel(i), WiFi.RSSI(i),
                  (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "encrypted");
  }
  WiFi.scanDelete();
}

/**
 * Bring up Wi-Fi and join the configured network.
 *
 * Returns without raising when no credentials file is present, because the sketch is
 * still useful over USB serial alone.
 *
 * @return true when the station is associated and datagrams may be sent.
 */
static bool wifiConnect() {
#ifdef HAVE_WIFI_CREDENTIALS
  Serial.printf("wifi: joining \"%s\"\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const uint32_t startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf("wifi: FAILED to join after %lu ms (status %d)\n",
                  static_cast<unsigned long>(WIFI_CONNECT_TIMEOUT_MS), WiFi.status());
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("wifi: SSID was found but the join was refused -- check the password.");
    } else {
      Serial.println("wifi: did not join. This module is 2.4 GHz ONLY -- a 5 GHz SSID is");
      Serial.println("wifi: invisible to it and will never appear in the scan below.");
    }
    Serial.println("wifi: networks this module can see:");
    scanAndPrintNetworks();
    return false;
  }

  Serial.print("wifi: joined, local address ");
  Serial.println(WiFi.localIP());
  Serial.printf("wifi: sending to %s", LINK_UNICAST_HOST[0] ? "unicast " : "broadcast ");
  Serial.print(linkDestination());
  Serial.printf(":%u\n", static_cast<unsigned>(LINK_PORT));
  return true;
#else
  Serial.println("wifi: no wifi_secrets.h found - USB serial only, no link to the cu");
  return false;
#endif
}

/**
 * Broadcast one already-formatted CSV line to the compute unit.
 *
 * Fire and forget: a datagram that does not arrive is not retried, because on this
 * link a late sample is worse than a missing one.
 *
 * @param line NUL-terminated CSV row, without its trailing newline.
 */
static void linkSend(const char *line) {
  if (!linkUp) {
    return;
  }
  WiFiUDP udp;
  if (!udp.begin(LINK_PORT)) {
    if (!linkFaultReported) {
      Serial.println("link: udp.begin failed - not sending");
      linkFaultReported = true;
    }
    return;
  }

  if (udp.beginPacket(linkDestination(), LINK_PORT)) {
    udp.write(reinterpret_cast<const uint8_t *>(line), strlen(line));
    udp.endPacket();
  }
  udp.stop();
}

/**
 * Print the periodic sensor diagnostic: acknowledgement, XSHUT, and last reading.
 *
 * @param last The most recent decoded sample.
 */
static void reportSensorState(const ToFSample &last) {
  bool sdaIdle = false;
  bool sclIdle = false;
  readIdleBusLevels(sdaIdle, sclIdle);

  // Re-read the identity registers on every probe rather than trusting the boot-time
  // values. The distinction matters and cannot be made from a cached read: 0xEE at boot
  // means the part is healthy, while a byte that is wrong at boot but correct now means
  // it simply was not ready yet and the XSHUT settle interval is too short. Those two
  // call for opposite fixes, so the probe has to sample live.
  const uint8_t liveModel = readByte(ADDR_VL53L0X, REG_MODEL_ID);
  const uint8_t liveRevision = readByte(ADDR_VL53L0X, REG_REVISION_ID);

  // Dump 32 bytes of the identification region, 0xC0 through 0xDF. A single model byte
  // read back as something unexpected cannot be told apart from a data-phase read that
  // is returning noise, because both simply fail to equal 0xEE. Searching the whole
  // region for the expected byte settles which of the two it is, and also catches the
  // part answering correctly at an offset other than the one this driver reads.
  Serial.print("idreg ");
  for (uint8_t i = 0; i < 4; ++i) {
    char group[20];
    size_t groupLen = 0;
    const uint8_t base = static_cast<uint8_t>(REG_MODEL_ID + i * 8);
    for (uint8_t j = 0; j < 8; ++j) {
      const uint8_t value = readByte(ADDR_VL53L0X, static_cast<uint8_t>(base + j));
      groupLen += snprintf(group + groupLen, sizeof(group) - groupLen, "%02X",
                           static_cast<unsigned>(value));
    }
    Serial.printf("%02X:%s ", base, group);
  }
  Serial.println();

  // Read one address repeatedly before moving on. Earlier captures of 0xC0 returned
  // 0x00, then 0x01, then 0x08, then 0x09 across runs, which cannot all be the true
  // contents of a fixed identification register. Either the reads are unreliable or
  // something is driving the bus between transactions. Sampling the same address four
  // times in a row, and doing it for three addresses at once, distinguishes the two:
  // a register that returns one stable value is being read correctly, and one that
  // varies is not a register at all.
  Serial.print("idstable ");
  const uint8_t probeAddrs[] = {0xC0, 0xC1, 0xDA};
  for (uint8_t a = 0; a < 3; ++a) {
    Serial.printf("%02X=", probeAddrs[a]);
    for (uint8_t k = 0; k < 4; ++k) {
      Serial.printf("%02X", static_cast<unsigned>(readByte(ADDR_VL53L0X, probeAddrs[a])));
    }
    Serial.printf(" ");
  }
  Serial.println();

  // Differential read: this sketch's hand-rolled read against the vendor driver's own
  // read, on the same register, with both raw I2C status codes alongside. Two
  // implementations disagreeing, or agreeing while the wire status reports a failed
  // transfer, localises the fault to the code or to the hardware. Hand-rolled and driver
  // agreeing on a wrong answer means the bytes genuinely came off the bus.
  const uint8_t mineAtC0 = readByte(ADDR_VL53L0X, REG_MODEL_ID);
  const uint8_t mineStatus = gLastWireStatus;
  tof.setAddress(ADDR_VL53L0X);
  const uint8_t driverAtC0 = tof.readReg(REG_MODEL_ID);
  const uint8_t driverStatus = tof.last_status;

  // The same read again, but holding the bus with a repeated START instead of releasing
  // it with a STOP between the register pointer and the data. ST specifies register
  // access on this part as write-then-repeated-START-then-read, and both the vendor
  // driver and the routine above send a STOP instead. A device that NAKs the data phase
  // after acknowledging its address is behaving exactly as one that wants the bus held.
  Wire.beginTransmission(ADDR_VL53L0X);
  Wire.write(REG_MODEL_ID);
  const uint8_t rsStatus = Wire.endTransmission(false);  // false: repeated START, no STOP
  uint8_t rsAtC0 = 0xFF;
  if (rsStatus == 0 && Wire.requestFrom(ADDR_VL53L0X, static_cast<uint8_t>(1)) == 1) {
    rsAtC0 = static_cast<uint8_t>(Wire.read());
  }
  Serial.printf("cmp C0: mine=%02X ws=%u  driver=%02X ws=%u  rstart=%02X ws=%u\n",
                mineAtC0, mineStatus, driverAtC0, driverStatus, rsAtC0, rsStatus);

  // Sample each line sixteen times while the bus is idle and report the bit pattern.
  // This is the test for a missing pull-up, and it exists because the single-sample idle
  // reading proved nothing: a floating line with only parasitic capacitance will sit high
  // for the microseconds after the pin is released and still have no defined level. A
  // line held up by a resistor reads high every time; a line with nothing on it wanders.
  pinMode(PIN_I2C_SDA, INPUT);
  pinMode(PIN_I2C_SCL, INPUT);
  uint16_t sdaBits = 0;
  uint16_t sclBits = 0;
  for (uint16_t i = 0; i < 16; ++i) {
    if (digitalRead(PIN_I2C_SDA) == HIGH) {
      sdaBits |= static_cast<uint16_t>(1u << i);
    }
    if (digitalRead(PIN_I2C_SCL) == HIGH) {
      sclBits |= static_cast<uint16_t>(1u << i);
    }
    delayMicroseconds(300);
  }
  Wire.end();
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(I2C_CLOCK_HZ);
  Serial.printf("idlebits SDA=%04X SCL=%04X (%s)\n", sdaBits, sclBits,
                (sdaBits == 0xFFFF && sclBits == 0xFFFF) ? "stable high" : "NOT stable");

  // Sweep the whole 7-bit address space and report everything that answers. The
  // question is whether 0x29 is the only responder, which is what a correctly attached
  // VL53L0X looks like, or whether the module's data pin has landed on a neighbouring
  // net and a different address is answering for it.
  Serial.print("scan:");
  for (uint8_t a = 0x08; a < 0x78; ++a) {
    if (addressResponds(a)) {
      Serial.printf(" %02X", a);
    }
  }
  Serial.println();

  Serial.printf("probe t=%lu dev(0x%02X)=%s ctl(0x%02X)=%s XSHUT=%s model=0x%02X "
                "rev=0x%02X idle SDA=%s SCL=%s init=%s link=%s "
                "raw=%u dist=%u status=%u\n",
                static_cast<unsigned long>(millis()), ADDR_VL53L0X,
                addressResponds(ADDR_VL53L0X) ? "ACK" : "NAK", ADDR_CONTROL_PROBE,
                addressResponds(ADDR_CONTROL_PROBE) ? "ACK" : "NAK",
                digitalRead(PIN_TOF_XSHUT) == HIGH ? "high" : "LOW", liveModel,
                liveRevision, sdaIdle ? "HIGH" : "LOW",
                sclIdle ? "HIGH" : "LOW",
                tofReady ? "OK" : (tofInitFailure ? tofInitFailure : "?"),
                linkUp ? "up" : "DOWN", static_cast<unsigned>(last.raw_mm),
                static_cast<unsigned>(last.dist_mm), static_cast<unsigned>(last.status));

  // Toggle XSHUT and see whether the module notices. This is the one wire whose
  // continuity cannot be verified from the ESP32 side: the probe reads the pin it drives,
  // so a broken or missing XSHUT wire still reports "high" while the module sits with
  // XSHUT floating. A correctly wired module drops off the bus when XSHUT is asserted and
  // reappears when released; one that answers identically both ways is not listening to
  // that pin, and a sensor that never received a shutdown is exactly what a floating
  // XSHUT produces.
  digitalWrite(PIN_TOF_XSHUT, LOW);
  delay(60);
  const bool ackWhileShut = addressResponds(ADDR_VL53L0X);
  digitalWrite(PIN_TOF_XSHUT, HIGH);
  delay(XSHUT_SETTLE_MS);
  const bool ackAfterRelease = addressResponds(ADDR_VL53L0X);
  Serial.printf("xshuttest: shutdown ACK=%s  released ACK=%s  -> %s\n",
                ackWhileShut ? "yes" : "no", ackAfterRelease ? "yes" : "no",
                (ackWhileShut != ackAfterRelease) ? "pin reaches the module"
                                                 : "module IGNORES XSHUT - CHECK WIRE");
}

/**
 * Configure the bus and the sensor, then start the CSV stream.
 *
 * @note Called once by the Arduino core before loop(). If the sensor does not answer,
 *       the sketch stays in a repeating error report rather than streaming invented
 *       zeros, because a column of zeros looks like a working sensor.
 */
void setup() {
  Serial.begin(115200);

  Serial.println();
  Serial.println("Auris pointer - VL53L0X ToF test (IMU not connected)");
#ifdef ESP_ARDUINO_VERSION_STR
  Serial.printf("arduino-esp32 core %s\n", ESP_ARDUINO_VERSION_STR);
#endif

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(I2C_CLOCK_HZ);

  Serial.println("i2c scan:");
  for (uint8_t addr = 1; addr < 0x7F; ++addr) {
    if (addressResponds(addr)) {
      Serial.printf("  device at 0x%02X\n", addr);
    }
  }

  tofReady = tofInit();

  // Discard the first few ranges; the earliest is not worth reporting.
  for (uint8_t i = 0; i < TOF_WARMUP_READINGS && tofReady; ++i) {
    ToFSample discard{};
    readRange(discard);
  }

  linkUp = wifiConnect();

  Serial.printf("tof %s\n", tofReady ? "ready" : "FAILED");
  Serial.println("hold the button to stream ranges to the compute unit over wifi");
  emitCsvHeader();
  Serial.flush();
}

/**
 * Take one range, emit the resulting CSV row, and transmit it while held.
 *
 * @note The line rate is the ranging rate, not LOOP_PERIOD_MS: readRange() blocks
 *       until the sensor has a measurement, so the loop self-paces to TOF_PERIOD_MS.
 */
void loop() {
  const uint32_t startMs = millis();

  ToFSample sample{};
  readRange(sample);

  char row[64];
  const size_t len = formatCsvRow(sample, row, sizeof(row));
  Serial.write(reinterpret_cast<const uint8_t *>(row), len);
  Serial.println();

  const bool held = buttonPressed();
  if (held != wasButtonHeld) {
    Serial.println();
    Serial.println(held ? "button DOWN - streaming to the compute unit"
                        : "button UP - stopped streaming to the compute unit");
    wasButtonHeld = held;
  }
  if (held) {
    linkSend(row);
  }

  if (startMs - lastProbeMs >= 5000) {
    lastProbeMs = startMs;
    reportSensorState(sample);
  }

  // readRange() blocks until the sensor has a measurement, so when the sensor is up
  // the loop self-paces to the ranging period with no help from here. When it is down
  // readRange() returns immediately and the loop would free-run at the rate the CPU can
  // format rows -- thousands per second, which overruns the 115200-baud TX buffer and
  // garbles the boot banner and the samples alike. Pace it explicitly in that case.
  if (!tofReady) {
    delay(TOF_PERIOD_MS);
  }
}