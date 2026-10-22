/*
 * Auris pointer — VL53L1X bring-up test (CJMCU-531 module).
 *
 * ToF-ONLY TEST WIRING  (IMU, button, and the link to the cu all excluded)
 *
 *   The two rows are numbered differently: r1 is the top row numbered 1-19, r2 is
 *   the bottom row numbered 60-42 across the same columns.  A column therefore has
 *   two unrelated numbers - r1 #9 and r2 #50 sit side by side.  The GPIO number is
 *   the authority; the position only tells you where to look.
 *
 *   ESP32 r1 #19  3V3    --  CJMCU-531  VCC     (module regulates its own supply;
 *                                             fed from 3V3 it ACKed steadily)
 *   ESP32 r2 #42  GND    --  CJMCU-531  GND     (r1 #2 / r1 #6 / r2 #48 equivalent)
 *   ESP32 r2 #47  GPIO21 --  CJMCU-531  SDA
 *   ESP32 r2 #44  GPIO22 --  CJMCU-531  SCL
 *   ESP32 r1 #9   GPIO27 --  CJMCU-531  XSHUT   (driven HIGH for the whole run)
 *
 *   100 nF ceramic cap -- across CJMCU-531 VCC and GND (the canonical decoupling
 *                                             passive; non-polarized, either way
 *                                             round)
 *
 *   CJMCU-531  GPIO1 (INT) --  not connected
 *   MPU6050  --  not connected for this test
 *   button   --  not connected; the sketch never touches GPIO 18
 *
 *   Fit both 4.7 k pull-ups (SDA and SCL each to 3V3): this module supplies none
 *   of its own, and the GY-521 that usually carries them is absent here.
 *
 * Purpose: prove the ToF RANGES. The module on the bench is a CJMCU-531, which is
 *          a VL53L1X breakout -- two-byte register indexing, model id 0xEACC read
 *          at 16-bit index 0x010F -- that several vendors list mislabeled as a
 *          "VL53L0X". The Pololu VL53L1X driver is used, because the VL53L0X
 *          driver cannot speak this part's register protocol.
 *
 *          XSHUT bootstrap: LOW for 10 ms into hardware standby, released HIGH,
 *          and >= 100 ms of settle before any I2C traffic
 *          (pointer/AGENTS.md treats anything shorter as unverified). XSHUT stays
 *          actively driven HIGH for the whole run.
 *
 * Inputs:  VL53L1X on I2C address 0x29, with XSHUT on GPIO 27.
 * Outputs: Bring-up diagnostics (bus scan, identity, config, 5-second probes) and
 *          a tabular range stream at 115200 baud. See emitTableHeader() for the
 *          column list; the 5-second probe reprints identity and the columns so
 *          a monitor attaching after boot is oriented.
 *
 * Build:   arduino-cli compile --profile tof-test-l1x
 * Flash:   arduino-cli upload  --profile tof-test-l1x --port /dev/ttyUSB0
 */

#include <Wire.h>
#include <VL53L1X.h>

// ---------------------------------------------------------------------------
// Pin map — matches the wiring comment above.
// ---------------------------------------------------------------------------

/** ESP32 I2C SDA line, at r2 #47. */
static constexpr uint8_t PIN_I2C_SDA = 21;
/** ESP32 I2C SCL line, at r2 #44. */
static constexpr uint8_t PIN_I2C_SCL = 22;
/** ToF enable line (XSHUT), at r1 #9. Actively driven, never floated. */
static constexpr uint8_t PIN_XSHUT = 27;

/**
 * I2C address of the VL53L1X.
 * The part has no address pins — 0x29 is its only identity, the same default as
 * the VL53L0X, which is partly why the two parts get confused at the vendor.
 */
static constexpr uint8_t ADDR_VL53L1X = 0x29;

/**
 * I2C bus speed in Hz, at the datasheet's 400 kHz ceiling.
 * The L1X family supports it, and the same silicon ACKed stably at 400 kHz
 * through every earlier attempt.
 */
static constexpr uint32_t I2C_CLOCK_HZ = 400000;

/** Time XSHUT is held low during the hardware reset, in milliseconds. */
static constexpr uint32_t XSHUT_LOW_MS = 10;

/**
 * Settling time after XSHUT is released, in milliseconds, before any I2C traffic.
 *
 * pointer/AGENTS.md's rule: treat anything under about 100 ms as unverified. This
 * constant is that rule expressed in code.
 */
static constexpr uint32_t XSHUT_SETTLE_MS = 100;

/**
 * Output pacing in milliseconds: rows are emitted no faster than
 * once per LOOP_PERIOD_MS, and the actual cadence is the sensor's own
 * inter-measurement period (RANGE_PERIOD_MS below).
 */
static constexpr uint32_t LOOP_PERIOD_MS = 20;

/**
 * Inter-measurement period handed to startContinuous(), in milliseconds: 50 ms,
 * matching the 50 ms range budget, for a nominal 20 Hz sample rate.
 */
static constexpr uint32_t RANGE_PERIOD_MS = 50;

/**
 * Measurement budget in microseconds.
 *
 * Long distance mode requires at least 33 ms (the driver's README carries the
 * constraint); 50 ms leaves margin and pairs with the period above.
 */
static constexpr uint32_t RANGE_BUDGET_US = 50000;

/**
 * Interval between repeated sensor probes, in milliseconds.
 *
 * Boot diagnostics repeat so a monitor attaching after boot is served: a banner
 * printed once inside setup() is missed by the normal case, which is attaching
 * afterwards.
 */
static constexpr uint32_t PROBE_PERIOD_MS = 5000;

/**
 * Duration a range read may block, in milliseconds, before it is abandoned.
 * A healthy sensor at a 50 ms period produces long before this; a timeout means
 * the part stopped producing measurements.
 */
static constexpr uint16_t READ_TIMEOUT_MS = 500;

/** The driver object. Address defaults to 0x29, matching ADDR_VL53L1X. */
static VL53L1X sensor{};

/** Timestamp of the last periodic sensor probe. */
static uint32_t lastProbeMs = 0;

/** Ranging reads that completed and were emitted, since boot. */
static uint32_t g_sampleCount = 0;
/** Reads that timed out waiting for a measurement, since boot. */
static uint32_t g_timeoutCount = 0;
/** Smallest valid range seen, in millimetres; INT32_MAX until one arrives. */
static int32_t g_minRangeMm = INT32_MAX;
/** Largest valid range seen, in millimetres; -1 until one arrives. */
static int32_t g_maxRangeMm = -1;
/** The last sample's range status, printed by name in the periodic probe. */
static VL53L1X::RangeStatus g_lastRangeStatus = VL53L1X::None;

/**
 * Force a hardware reset of the VL53L1X through XSHUT and release it cleanly.
 *
 * XSHUT low puts the part in hardware standby, where its I2C interface is off —
 * so traffic must not resume until after XSHUT_SETTLE_MS past release. XSHUT
 * stays driven HIGH for the whole run afterwards: the parent connection list
 * demands an actively driven enable, and floating it is what made every device
 * on the bus unreadable in the previous prototype.
 */
static void vl53HardReset() {
  pinMode(PIN_XSHUT, OUTPUT);
  digitalWrite(PIN_XSHUT, LOW);
  delay(XSHUT_LOW_MS);
  digitalWrite(PIN_XSHUT, HIGH);
  delay(XSHUT_SETTLE_MS);
}

/**
 * Scan the bus and print every address that acknowledges.
 *
 * Address 0 is never probed (general call) and 0x78-0x7F are skipped as reserved;
 * everything else is a real candidate, so a miswired part is named outright.
 *
 * @return Number of acknowledging addresses found, 0-119.
 */
static uint8_t scanI2c() {
  uint8_t found = 0;
  Serial.print("i2c scan:");
  for (uint8_t addr = 1; addr < 0x78; ++addr) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf(" 0x%02X", addr);
      ++found;
    }
  }
  if (found == 0) {
    Serial.print(" <nothing ACKs>");
  }
  Serial.println();
  return found;
}

/**
 * Read the VL53L1X's identity registers with the part's own two-byte indexing
 * and print what comes back.
 *
 * The VL53L1X indexes registers with two bytes and advances an internal pointer
 * in every read, which is why one-byte-index reads (the VL53L0X protocol) print
 * pointer-driven garbage. The identity lives at 16-bit index 0x010F: model id
 * 0xEACC, then the next two bytes |10 FF| as read earlier. Only the model bytes
 * gate — the trailing pair is printed but not gated, since its meaning was never
 * pinned down.
 *
 * @return true when the two model bytes read 0xEA 0xCC, i.e. a VL53L1X.
 */
static bool printIdentityReport() {
  Wire.beginTransmission(ADDR_VL53L1X);
  Wire.write(static_cast<uint8_t>(0x01));  // index 0x010F, high byte
  Wire.write(static_cast<uint8_t>(0x0F));  // index 0x010F, low byte
  const bool indexed = (Wire.endTransmission(false) == 0);
  uint8_t identity[4] = {0, 0, 0, 0};
  bool complete = false;
  if (indexed && Wire.requestFrom(ADDR_VL53L1X, static_cast<uint8_t>(4)) == 4) {
    for (uint8_t i = 0; i < 4; ++i) {
      identity[i] = static_cast<uint8_t>(Wire.read());
    }
    complete = true;
  }
  Serial.print("identity at 16-bit index 0x010F:");
  if (complete) {
    for (uint8_t i = 0; i < 4; ++i) {
      Serial.printf(" %02X", identity[i]);
    }
    const bool isL1x = (identity[0] == 0xEA && identity[1] == 0xCC);
    Serial.printf("%s\n", isL1x ? "  (model 0xEACC -- VL53L1X confirmed)"
                                : "  -- model bytes are NOT 0xEACC");
    return isL1x;
  }
  Serial.printf(" <failed: index %s>\n",
                indexed ? "read was short" : "write was NAKed");
  return false;
}

/**
 * Run one bring-up attempt for the ToF and report what happened.
 *
 * The order matters: hardware reset first so every attempt starts from the same
 * XSHUT-cycled state, then the bus scan, then the identity read with the part's
 * own protocol, then the driver's init and the range configuration.
 *
 * @return true when the driver initialised and streaming may start; false means
 *     the caller may retry, because every failure diagnosed here is fixed by
 *     refitting a wire, not by reflashing.
 */
static bool attemptBringUp() {
  vl53HardReset();
  const uint8_t found = scanI2c();
  if (found == 0) {
    Serial.println("bring-up failed: nothing ACKs on the bus");
    Serial.println("  check XSHUT to GPIO27, SDA/SCL to GPIO21/22, 3V3, common GND,");
    Serial.println("  both 4.7k pull-ups");
    return false;
  }
  Serial.printf("        expecting the VL53L1X at 0x%02X; 0x68 on that list means the GY-521 is still wired\n",
                ADDR_VL53L1X);

  if (!printIdentityReport()) {
    Serial.println("bring-up failed: 0x010F did not read 0xEACC (a VL53L1X)");
    Serial.println("  if it reads garbage consistently, quote the bytes to the vendor:");
    Serial.println("  several listings sell this exact board mislabelled as a VL53L0X");
    return false;
  }

  if (!sensor.init()) {
    Serial.println("bring-up failed: driver init() was rejected");
    Serial.println("  init() re-checks the model id first, so this usually means the");
    Serial.println("  part stopped ACKing mid-sequence -- refit the I2C jumpers");
    return false;
  }

  const bool modeSet = sensor.setDistanceMode(VL53L1X::Long);
  const bool budgetSet = sensor.setMeasurementTimingBudget(RANGE_BUDGET_US);
  sensor.setTimeout(READ_TIMEOUT_MS);
  Serial.printf("range config: %s mode, %u us budget (%s), %u ms read timeout\n",
                modeSet ? "long" : "<mode rejected>",
                static_cast<unsigned>(RANGE_BUDGET_US),
                budgetSet ? "accepted" : "rejected",
                static_cast<unsigned>(READ_TIMEOUT_MS));
  return true;
}

/**
 * Print the periodic diagnostic: identity, active configuration, and running stats.
 *
 * The stats are the test's verdict in one line: ok/timeout counts, the valid
 * range extremes, and the last measurement's status by name.
 */
static void reportSensorState() {
  printIdentityReport();
  Serial.printf("config: mode=%s budget=%u us timeout=%u ms\n",
                sensor.getDistanceMode() == VL53L1X::Long ? "long"
                : sensor.getDistanceMode() == VL53L1X::Medium ? "medium"
                : sensor.getDistanceMode() == VL53L1X::Short ? "short" : "?",
                static_cast<unsigned>(sensor.getMeasurementTimingBudget()),
                static_cast<unsigned>(sensor.getTimeout()));
  if (g_minRangeMm <= g_maxRangeMm) {
    Serial.printf("stats: ok=%lu timeout=%lu range_mm min=%ld max=%ld last_status=%s\n",
                  static_cast<unsigned long>(g_sampleCount),
                  static_cast<unsigned long>(g_timeoutCount),
                  static_cast<long>(g_minRangeMm), static_cast<long>(g_maxRangeMm),
                  sensor.rangeStatusToString(g_lastRangeStatus));
  } else {
    Serial.printf("stats: ok=%lu timeout=%lu (no valid range yet) last_status=%s\n",
                  static_cast<unsigned long>(g_sampleCount),
                  static_cast<unsigned long>(g_timeoutCount),
                  sensor.rangeStatusToString(g_lastRangeStatus));
  }
  emitTableHeader();
}

/**
 * Print the column header of the tabular range stream.
 *
 * Every field is padded to a fixed width and closed with a tab, so the monitor
 * lines the stream up as a table and a field that overflows its width still
 * lands on the next tab stop. Columns: t_ms, milliseconds since boot; range_mm,
 * distance along the beam in millimetres; status, the measurement's verdict as
 * the RangeStatus enum value (0 is RangeValid, printed by name in the legend
 * setup() emits once); sig_MCPS and amb_MCPS, peak signal and ambient count
 * rates in million counts per second; read, the outcome of the read — ok or
 * timeout. The five-second probe reprints this header so a monitor attaching
 * after boot misses nothing.
 */
static void emitTableHeader() {
  Serial.printf("%-7s\t%-8s\t%-6s\t%-8s\t%-8s\t%s\n",
                "t_ms", "range_mm", "status", "sig_MCPS", "amb_MCPS", "read");
}

/**
 * Print the range_status legend the header above refers to.
 *
 * Values live in the driver's RangeStatus enum; nothing here re-invents them.
 */
static void emitStatusLegend() {
  Serial.println("range_status legend: 0=RangeValid 1=SigmaFail 2=SignalFail"
                 " 3=RangeValidMinRangeClipped 4=OutOfBoundsFail");
  Serial.println("                     5=HardwareFail 6=RangeValidNoWrapCheckFail"
                 " 7=WrapTargetFail 9=XtalkSignalFail");
  Serial.println("                     10=SynchronizationInt 13=MinRangeFail 255=None");
}

/**
 * Configure the bus, force the ToF up through XSHUT, and start the tabular stream.
 *
 * @note Called once by the Arduino core before loop(). Bring-up is retried every
 *       second because every failure this sketch diagnoses is a wiring failure,
 *       and refitting the wire must be picked up without reflashing. Should the
 *       part stay absent, the retry itself is the repeating error report.
 */
void setup() {
  Serial.begin(115200);

  Serial.println();
  Serial.println("Auris pointer - VL53L1X test (CJMCU-531; no button, no link to the cu)");
#ifdef ESP_ARDUINO_VERSION_STR
  Serial.printf("arduino-esp32 core %s\n", ESP_ARDUINO_VERSION_STR);
#endif

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(I2C_CLOCK_HZ);

  while (!attemptBringUp()) {
    Serial.println("retrying in 1 s -- refit the wiring and it will be picked up");
    delay(1000);
  }

  sensor.startContinuous(RANGE_PERIOD_MS);

  Serial.println("streaming continuous range in long mode (to 4 m); a wall in front"
                 " should read at its true distance with range_status 0");
  emitStatusLegend();
  emitTableHeader();
  Serial.flush();
}

/**
 * Read one continuous-mode measurement and emit its table row.
 *
 * The read blocks on the part's data-ready flag, and its 500 ms timeout guards
 * against a sensor that has stopped producing measurements: a timeout is emitted
 * as a timeout row, never as a plausible zero. Nonzero range_status values are
 * emitted as-is with nan for the range and the count rates, because the part's
 * verdict is data, not silence. The loop is paced to LOOP_PERIOD_MS so the
 * serial link is never overrun.
 */
void loop() {
  const uint32_t nowMs = millis();

  const uint16_t rangeMm = sensor.read();
  if (rangeMm == 0 && sensor.timeoutOccurred()) {
    ++g_timeoutCount;
    g_lastRangeStatus = VL53L1X::None;
    Serial.printf("%-7lu\t%-8s\t%-6s\t%-8s\t%-8s\ttimeout\n",
                  static_cast<unsigned long>(millis()), "nan", "nan", "nan", "nan");
  } else {
    g_lastRangeStatus = sensor.ranging_data.range_status;
    ++g_sampleCount;
    if (g_lastRangeStatus == VL53L1X::RangeValid && rangeMm > 0) {
      // Both comparisons run in signed arithmetic: g_minRangeMm starts at
      // INT32_MAX (any range is smaller) and g_maxRangeMm at -1 (any range is
      // larger). Casting g_maxRangeMm to uint16_t would turn that seed into
      // 65535 and the first valid range would never update the max.
      const int32_t signedRange = static_cast<int32_t>(rangeMm);
      if (signedRange < g_minRangeMm) {
        g_minRangeMm = signedRange;
      }
      if (signedRange > g_maxRangeMm) {
        g_maxRangeMm = signedRange;
      }
    }
    Serial.printf("%-7lu\t%-8u\t%-6u\t%-8.2f\t%-8.2f\tok\n",
                  static_cast<unsigned long>(millis()),
                  static_cast<unsigned>(rangeMm),
                  static_cast<unsigned>(g_lastRangeStatus),
                  sensor.ranging_data.peak_signal_count_rate_MCPS,
                  sensor.ranging_data.ambient_count_rate_MCPS);
  }

  if (nowMs - lastProbeMs >= PROBE_PERIOD_MS) {
    lastProbeMs = nowMs;
    reportSensorState();
  }

  const uint32_t elapsedMs = millis() - nowMs;
  if (elapsedMs < LOOP_PERIOD_MS) {
    delay(LOOP_PERIOD_MS - elapsedMs);
  }
}
