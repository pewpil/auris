/*
 * Auris pointer — sensor-node bring-up test (ToF + IMU + button together).
 *
 * FULL NODE TEST WIRING  (laptop USB for power and the local record; the cu
 *                         link is Wi-Fi and active only while the button is held)
 *
 *   The two rows are numbered differently: r1 is the top row numbered 1-19, r2 is
 *   the bottom row numbered 60-42 across the same columns.  A column therefore has
 *   two unrelated numbers - r1 #9 and r2 #50 sit side by side.  The GPIO number is
 *   the authority; the position only tells you where to look.
 *
 *   ESP32 r1 #19  3V3    --  CJMCU-531  VCC     (module regulates its own supply)
 *   ESP32 r2 #42  GND    --  CJMCU-531  GND     (r1 #2 / r1 #6 / r2 #48 equivalent)
 *   ESP32 r2 #47  GPIO21 --  CJMCU-531  SDA  }  the I2C bus, now shared by BOTH
 *   ESP32 r2 #44  GPIO22 --  CJMCU-531  SCL  }  modules
 *   ESP32 r1 #9   GPIO27 --  CJMCU-531  XSHUT   (driven HIGH for the whole run)
 *   CJMCU-531  GPIO1 (INT) --  not connected
 *   100 nF ceramic cap -- across CJMCU-531 VCC and GND (non-polarized)
 *
 *   GY-521 (MPU6050) VCC --  the + rail
 *   GY-521 (MPU6050) GND --  the - rail
 *   GY-521 (MPU6050) SDA --  the GPIO21 net (same one the CJMCU-531 hangs off)
 *   GY-521 (MPU6050) SCL --  the GPIO22 net
 *   GY-521 (MPU6050) AD0 --  the - rail  (ties the IMU to address 0x68; floating
 *                                         puts it at 0x69, which nothing probes)
 *   GY-521  XDA, XCL, INT --  not connected
 *
 *   Tactile button leg A --  ESP32 r2 #50  GPIO18
 *   Tactile button leg B --  the - rail (pick legs on OPPOSITE sides of the
 *                                        switch contact; the two legs on one
 *                                        side are internally joined, so wiring
 *                                        across one side shorts the input)
 *
 *   Both 4.7 k pull-ups (SDA and SCL each to the + rail) stay fitted: the GY-521
 *   carries its own onboard pull-ups in parallel, giving about 2.3 k net - the
 *   parent AGENTS.md's stated, allowed case.  Optional: the 100 uF electrolytic
 *   kept from the earlier phase can bridge the GY-521's VCC and GND; the run is
 *   also healthy without it.
 *
 * Purpose: prototype the pointer's sensor node as a whole on one breadboard. The
 *          two halves were proved on their own (imu_test/, tof_test_l1x/); this
 *          test proves them SHARING one bus, alongside the button, and drains one
 *          combined tabular stream. Reads both sensors directly over Wire - the
 *          MPU6050 with the parent contract's minimal register sequence (a
 *          WHO_AM_I probe, four configuration writes, one 14-byte burst per
 *          sample) - the one library exception being the VL53L1X's own driver,
 *          whose init sequence is long and must be correct. While the button is
 *          held, each combined row is also broadcast to the compute unit over
 *          UDP on port 5005, mirroring imu_test/; released, the broadcast stops
 *          and the USB record carries on.
 *
 * Inputs:  VL53L1X on I2C address 0x29 (XSHUT on GPIO 27), MPU6050 at 0x68,
 *          button on GPIO 18 with the internal pull-up.
 * Outputs: Bring-up diagnostics (bus scan, both identities, config, 5-second
 *          probes with stats and link state) and a tabular combined stream at
 *          115200 baud. emitTableHeader() lists the columns; the 5-second probe
 *          reprints identity, link state, and the column header so a
 *          late-attaching monitor orients. While the button is held, each row is
 *          also sent to the compute unit as one UDP datagram on LINK_PORT.
 *
 * Secrets: Wi-Fi credentials live in wifi_secrets.h, untracked by git. With no
 *          such file the sketch builds and runs USB-only, saying so at boot.
 *
 * Build:   arduino-cli compile --profile node-test
 * Flash:   arduino-cli upload  --profile node-test --port /dev/ttyUSB0
 */

#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>   // core 3.x spells it this way; it typedefs WiFiUDP
#include <VL53L1X.h>

// Credentials live in an untracked header so the password never reaches git. The
// sketch still builds without it, falling back to USB serial only.
#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define HAVE_WIFI_CREDENTIALS 1
#endif

// ---------------------------------------------------------------------------
// Pin map — matches the wiring comment above.
// ---------------------------------------------------------------------------

/** ESP32 I2C SDA line, at r2 #47; now shared by both sensor modules. */
static constexpr uint8_t PIN_I2C_SDA = 21;
/** ESP32 I2C SCL line, at r2 #44; now shared by both sensor modules. */
static constexpr uint8_t PIN_I2C_SCL = 22;
/** ToF enable line (XSHUT), at r1 #9. Actively driven, never floated. */
static constexpr uint8_t PIN_XSHUT = 27;
/**
 * Laser-trigger button input, at r2 #50, in the exact position the finished
 * firmware will use. Configured INPUT_PULLUP: idle reads HIGH (emitted 0),
 * pressed closes to ground and reads LOW (emitted 1).
 */
static constexpr uint8_t PIN_BUTTON = 18;

/**
 * I2C address of the VL53L1X. Fixed by silicon — the part has no address pins.
 */
static constexpr uint8_t ADDR_VL53L1X = 0x29;
/**
 * I2C address of the MPU6050 with AD0 tied to ground. Floating AD0 would put it
 * at 0x69, which nothing here probes — hence AD0 must be wired to the - rail.
 */
static constexpr uint8_t ADDR_MPU6050 = 0x68;

/** I2C bus speed in Hz, at the datasheet's 400 kHz ceiling for both parts. */
static constexpr uint32_t I2C_CLOCK_HZ = 400000;

// ---------------------------------------------------------------------------
// Pointer -> compute unit link.
//
// Rows leave the pointer over Wi-Fi UDP while the laser button is held, mirroring
// imu_test/. Datagrams are broadcast to the subnet's directed broadcast address
// and the compute unit binds 0.0.0.0, so neither side needs the other's address;
// the compute unit has moved address repeatedly, which is why the repository
// records its MAC instead. The USB stream keeps running either way, so a broken
// link costs the compute unit rows, never the local record.
// ---------------------------------------------------------------------------

/** UDP port the compute unit listens on. Must match --port on the cu side. */
static constexpr uint16_t LINK_PORT = 5005;

/** Milliseconds allowed for the Wi-Fi association to complete at boot. */
static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;

/**
 * Optional unicast override for the datagram destination.
 *
 * Empty (the default) means broadcast to the subnet's directed broadcast address.
 * Set it to an address such as "192.168.1.16" if the access point filters
 * client-to-client broadcast (AP isolation). A unicast destination is a moving
 * target under DHCP, so broadcast is the better default and this is the escape
 * hatch.
 */
static const char *LINK_UNICAST_HOST = "";

/** Whether the link came up and datagrams may be sent. */
static bool linkUp = false;

/** Set on the first send failure, so repeated failures do not spam the serial log. */
static bool linkFaultReported = false;

/** Rows handed to the link since boot, whether or not the datagram arrived. */
static uint32_t g_linkSentCount = 0;
/** Send attempts that failed at the UDP layer, since boot. */
static uint32_t g_linkFailCount = 0;

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
 * Scan and print every network this module can currently see.
 *
 * Printed after a failed join, because the most likely cause is that the
 * configured SSID names a 5 GHz band: the ESP32-WROOM-32 radio is 2.4 GHz only,
 * so such a network does not appear in a scan and never produces a useful status
 * code on its own.
 */
static void scanAndPrintNetworks() {
  // The radio will not start a scan from a failed-join state, so cycle it off
  // and back before scanning. Disconnect without erasing the stored config.
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
    Serial.printf("  [%d] %s  ch=%d  rssi=%d dBm  %s\n", i,
                  WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i),
                  (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "encrypted");
  }
  WiFi.scanDelete();
  if (found == 0) {
    Serial.println("  none -- check the module is seated and the antenna is clear");
  }
}

/**
 * Bring up Wi-Fi and join the configured network.
 *
 * Returns without raising when no credentials file is present: the sensor test
 * is still useful over USB alone, and a missing secret must not stop bring-up.
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

  const IPAddress destination = linkDestination();
  Serial.printf("wifi: sending to %s", LINK_UNICAST_HOST[0] ? "unicast " : "broadcast ");
  Serial.print(destination);
  Serial.printf(":%u\n", static_cast<unsigned>(LINK_PORT));
  return true;
#else
  Serial.println("wifi: no wifi_secrets.h found - USB serial only, no link to the cu");
  return false;
#endif
}

/**
 * Broadcast one already-formatted row to the compute unit.
 *
 * Fire and forget: a datagram that does not arrive is not retried, because on
 * this link a late sample is worse than a missing one. The row is printed
 * locally either way, so a broken link never costs the local record.
 *
 * @param line NUL-terminated row, without its trailing newline.
 */
static void linkSend(const char *line) {
  if (!linkUp) {
    return;
  }
  WiFiUDP udp;
  if (!udp.begin(LINK_PORT)) {
    ++g_linkFailCount;
    if (!linkFaultReported) {
      Serial.println("link: udp.begin failed - not sending");
      linkFaultReported = true;
    }
    return;
  }

  const IPAddress destination = linkDestination();

  if (udp.beginPacket(destination, LINK_PORT)) {
    udp.write(reinterpret_cast<const uint8_t *>(line), strlen(line));
    udp.endPacket();
    ++g_linkSentCount;
  } else {
    ++g_linkFailCount;
    if (!linkFaultReported) {
      Serial.println("link: beginPacket failed - not sending");
      linkFaultReported = true;
    }
  }
  udp.stop();
}

/**
 * Sample the laser button.
 *
 * @return true while the button is held down, i.e. while GPIO 18 is pulled to GND.
 */
static bool buttonPressed() {
  return digitalRead(PIN_BUTTON) == LOW;
}

// ---------------------------------------------------------------------------
// MPU6050 register map and conversion constants — driven from raw registers per
// the parent contract: no IMU library.
// ---------------------------------------------------------------------------

/** WHO_AM_I register (0x75); equals 0x68 on a genuine MPU6050. */
static constexpr uint8_t MPU_REG_WHO_AM_I = 0x75;
/** Power management register (0x6B). */
static constexpr uint8_t MPU_REG_PWR_MGMT_1 = 0x6B;
/** Digital low-pass filter configuration (0x1A). */
static constexpr uint8_t MPU_REG_CONFIG = 0x1A;
/** Gyroscope full-scale selection (0x1B). */
static constexpr uint8_t MPU_REG_GYRO_CONFIG = 0x1B;
/** Accelerometer full-scale selection (0x1C). */
static constexpr uint8_t MPU_REG_ACCEL_CONFIG = 0x1C;
/** Start of the 14-byte measurement burst (0x3B), read straight through. */
static constexpr uint8_t MPU_REG_BURST = 0x3B;

/** Expected WHO_AM_I value of a genuine MPU6050. */
static constexpr uint8_t MPU_WHO_AM_I_EXPECTED = 0x68;
/** Value written to PWR_MGMT_1: wake from sleep, clock from the gyro PLL. */
static constexpr uint8_t MPU_PWR_WAKE_PLLCLOCK = 0x01;
/** CONFIG value: DLPF at ~44 Hz, keeping gyro noise bounded without lag. */
static constexpr uint8_t MPU_CONFIG_DLPF_44HZ = 0x03;
/** GYRO_CONFIG value: full-scale range ±250 deg/s, the standard bring-up gain. */
static constexpr uint8_t MPU_GYRO_CONFIG_250DPS = 0x00;
/** ACCEL_CONFIG value: full-scale range ±2 g, the standard bring-up gain. */
static constexpr uint8_t MPU_ACCEL_CONFIG_2G = 0x00;

/** LSB per unit of acceleration at ±2 g full scale: 16384 LSB/g. */
static constexpr float MPU_ACCEL_LSB_PER_G = 16384.0f;
/** LSB per unit of angular rate at ±250 deg/s full scale: 131 LSB/(deg/s). */
static constexpr float MPU_GYRO_LSB_PER_DPS = 131.0f;
/** LSB per degC of the MPU6050's die-temperature register. */
static constexpr float MPU_TEMP_LSB_PER_C = 340.0f;
/** The MPU6050's temperature register's offset in degC. */
static constexpr float MPU_TEMP_OFFSET_C = 36.53f;

// ---------------------------------------------------------------------------
// Timing — the same XSHUT rule, range configuration, pacing, and probe cadence
// that tof_test_l1x validated on this bench.
// ---------------------------------------------------------------------------

/** Time XSHUT is held low during the hardware reset, in milliseconds. */
static constexpr uint32_t XSHUT_LOW_MS = 10;
/**
 * Settling time after XSHUT is released, in milliseconds, before any I2C
 * traffic — the parent AGENTS.md's ~100 ms minimum, expressed in code.
 */
static constexpr uint32_t XSHUT_SETTLE_MS = 100;
/** Output pacing in milliseconds; rows are emitted no faster than this apart. */
static constexpr uint32_t LOOP_PERIOD_MS = 20;
/**
 * Inter-measurement period handed to startContinuous(), in milliseconds:
 * 50 ms, matching the 50 ms range budget, for a nominal 20 Hz sample rate.
 */
static constexpr uint32_t RANGE_PERIOD_MS = 50;
/** Measurement budget in microseconds; long mode's minimum is 33 ms. */
static constexpr uint32_t RANGE_BUDGET_US = 50000;
/** Duration a range read may block, in milliseconds, before it is abandoned. */
static constexpr uint16_t READ_TIMEOUT_MS = 500;
/** Interval between the periodic diagnostic probes, in milliseconds. */
static constexpr uint32_t PROBE_PERIOD_MS = 5000;

/** The Pololu VL53L1X driver object, at its default address 0x29. */
static VL53L1X sensor{};

/** Timestamp of the last periodic sensor probe. */
static uint32_t lastProbeMs = 0;

/** Combined-stream rows emitted since boot. */
static uint32_t g_rowCount = 0;
/** ToF reads that completed, since boot. */
static uint32_t g_tofOkCount = 0;
/** ToF reads that timed out waiting for a measurement, since boot. */
static uint32_t g_tofTimeoutCount = 0;
/** IMU 14-byte bursts that arrived whole, since boot. */
static uint32_t g_imuOkCount = 0;
/** IMU bursts that arrived short (fewer than 14 bytes), since boot. */
static uint32_t g_imuShortCount = 0;
/** IMU transfers whose address was not acknowledged, since boot. */
static uint32_t g_imuNackCount = 0;
/** Button presses counted, as LOW-going edges, since boot. */
static uint32_t g_pressCount = 0;
/** The previous loop's raw button level, for counting press edges. */
static bool g_buttonWasPressed = false;
/** Smallest valid range seen, in millimetres; INT32_MAX until one arrives. */
static int32_t g_minRangeMm = INT32_MAX;
/** Largest valid range seen, in millimetres; -1 until one arrives. */
static int32_t g_maxRangeMm = -1;
/** The last sample's ToF range status, printed by name in the periodic probe. */
static VL53L1X::RangeStatus g_lastRangeStatus = VL53L1X::None;

/**
 * Force a hardware reset of the VL53L1X through XSHUT and release it cleanly.
 *
 * XSHUT low puts the part in hardware standby, where its I2C interface is off —
 * so traffic must not resume until XSHUT_SETTLE_MS past release. XSHUT stays
 * driven HIGH for the whole run afterwards: the parent connection list demands
 * an actively driven enable, and floating it is what made every device on the
 * bus unreadable in the earlier prototype.
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
 * Address 0 is never probed (general call) and 0x78-0x7F are skipped as
 * reserved; everything else is a real candidate, so a miswired part is named
 * outright. 0x69 on the list specifically means the MPU6050's AD0 is floating.
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
 * The VL53L1X indexes registers with two bytes and advances an internal
 * pointer in every read, which is why one-byte-index reads (the VL53L0X
 * protocol) print pointer-driven garbage. The identity lives at 16-bit index
 * 0x010F: model id 0xEACC, then the next two bytes |10 FF| as read on this
 * bench. Only the model bytes gate — the trailing pair is printed but not
 * gated, since its meaning was never pinned down.
 *
 * @return true when the two model bytes read 0xEA 0xCC, i.e. a VL53L1X.
 */
static bool printToFIdentity() {
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
  Serial.print("ToF identity at 16-bit index 0x010F:");
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
 * Probe the MPU6050's WHO_AM_I register and print the byte whatever it is.
 *
 * A genuine MPU6050 answers 0x68. Well-known quiet revisions (and the odd
 * clone) answer other values, so a mismatch is reported loudly but does not
 * stop bring-up on its own — the observed value stays in the record for either
 * quote-back to the vendor or a later firmware decision.
 *
 * @return true when the register address itself was acknowledged; false means
 *     the IMU did not answer at 0x68 at all, and nothing IMU-shaped follows.
 */
static bool printMpuWhoAmI() {
  Wire.beginTransmission(ADDR_MPU6050);
  Wire.write(MPU_REG_WHO_AM_I);
  const bool indexed = (Wire.endTransmission(false) == 0);
  if (!indexed) {
    Serial.println("MPU6050 WHO_AM_I: <NAKed at 0x68>");
    return false;
  }
  if (Wire.requestFrom(ADDR_MPU6050, static_cast<uint8_t>(1)) != 1) {
    Serial.println("MPU6050 WHO_AM_I: <read was short>");
    return false;
  }
  const uint8_t whoAmI = static_cast<uint8_t>(Wire.read());
  Serial.printf("MPU6050 WHO_AM_I: 0x%02X%s\n", whoAmI,
                whoAmI == MPU_WHO_AM_I_EXPECTED
                    ? "  (MPU6050 confirmed)"
                    : "  -- NOT 0x68; possibly a resumed clone/quiet revision");
  return true;
}

/**
 * Write the MPU6050's four configuration registers for the bring-up stream.
 *
 * Sequence per the parent contract: wake with the gyro PLL as the clock
 * source, the DLPF at ~44 Hz, and both full-scale ranges at the standard
 * gains. The sample-rate divider is left at its reset default of 0, which
 * delivers bursts at the internal rate and lets the loop pace itself.
 *
 * @return true when every write was acknowledged; false means the transfer of
 *     the failing register was not, and the diagnostics name the register.
 */
static bool mpuConfigure() {
  const struct {
    uint8_t reg;
    uint8_t value;
    const char *name;
  } writes[] = {
      {MPU_REG_PWR_MGMT_1, MPU_PWR_WAKE_PLLCLOCK, "PWR_MGMT_1 = wake, gyro PLL"},
      {MPU_REG_CONFIG, MPU_CONFIG_DLPF_44HZ, "CONFIG = DLPF 44 Hz"},
      {MPU_REG_GYRO_CONFIG, MPU_GYRO_CONFIG_250DPS,
       "GYRO_CONFIG = +/-250 deg/s"},
      {MPU_REG_ACCEL_CONFIG, MPU_ACCEL_CONFIG_2G, "ACCEL_CONFIG = +/-2 g"},
  };
  for (const auto &w : writes) {
    Wire.beginTransmission(ADDR_MPU6050);
    Wire.write(w.reg);
    Wire.write(w.value);
    if (Wire.endTransmission() != 0) {
      Serial.printf("MPU6050 config failed at %s\n", w.name);
      return false;
    }
  }
  Serial.println("MPU6050 config: wake/PLL, DLPF 44 Hz, gyro +/-250 deg/s,"
                 " accel +/-2 g (all acknowledged)");
  return true;
}

/**
 * Run one bring-up attempt for the whole sensor node and report what happened.
 *
 * The order matters: the ToF is hardware-reset through XSHUT first so every
 * attempt starts from the same state, then the bus scan names everything that
 * answers, then each device is brought up and identified, and finally the
 * button's input is configured. The scan output is the diagnostic for "which
 * device is missing": only 0x29 on the list means the IMU wiring is at fault,
 * only 0x68 means the ToF side is, and 0x69 instead of 0x68 means the MPU6050's
 * AD0 is floating.
 *
 * @return true when both sensors are confirmed and streaming may start; false
 *     means the caller may retry, because every failure diagnosed here is
 *     fixed by refitting a wire, not by reflashing.
 */
static bool attemptBringUp() {
  vl53HardReset();
  const uint8_t found = scanI2c();
  if (found == 0) {
    Serial.println("bring-up failed: nothing ACKs on the bus");
    Serial.println("  check XSHUT to GPIO27, SDA/SCL to GPIO21/22, 3V3, common");
    Serial.println("  GND, both 4.7k pull-ups");
    return false;
  }
  Serial.printf("        expecting the VL53L1X at 0x%02X and the MPU6050 at"
                " 0x%02X; an extra 0x%02X would mean the AD0 is floating\n",
                ADDR_VL53L1X, ADDR_MPU6050, ADDR_MPU6050 + 1);

  if (!printToFIdentity()) {
    Serial.println("bring-up failed: the ToF's 0x010F did not read 0xEACC");
    return false;
  }
  if (!sensor.init()) {
    Serial.println("bring-up failed: ToF driver init() was rejected");
    Serial.println("  init() re-checks the model id first, so this usually");
    Serial.println("  means the part stopped ACKing mid-sequence -- refit the");
    Serial.println("  I2C jumpers");
    return false;
  }
  const bool modeSet = sensor.setDistanceMode(VL53L1X::Long);
  const bool budgetSet = sensor.setMeasurementTimingBudget(RANGE_BUDGET_US);
  sensor.setTimeout(READ_TIMEOUT_MS);
  Serial.printf("tof config: %s mode, %u us budget (%s), %u ms read timeout\n",
                modeSet ? "long" : "<mode rejected>",
                static_cast<unsigned>(RANGE_BUDGET_US),
                budgetSet ? "accepted" : "rejected",
                static_cast<unsigned>(READ_TIMEOUT_MS));

  if (!printMpuWhoAmI()) {
    Serial.println("bring-up failed: nothing answered WHO_AM_I at 0x68 -- at");
    Serial.println("  0x69 instead of 0x68 means the AD0 is floating; a scan");
    Serial.println("  with neither address means the IMU's VCC/GND/SDA/SCL");
    return false;
  }
  if (!mpuConfigure()) {
    Serial.println("bring-up failed: an MPU6050 config write was rejected;");
    Serial.println("  refit the IMU jumpers");
    return false;
  }

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  g_buttonWasPressed = false;
  Serial.printf("button: GPIO%u INPUT_PULLUP, idle reads %d (0 = pressed)\n",
                PIN_BUTTON, digitalRead(PIN_BUTTON) == LOW ? 0 : 1);
  return true;
}

/**
 * Print the periodic diagnostic: both identities, active configuration, and
 * the running stats of every stream, then the table header.
 *
 * The stats are the test's verdict in one line pair: ToF ok/timeout counts and
 * valid-range extremes, IMU ok/short/nack counts, and the button presses
 * counted. A link line follows, because a monitor attaching after boot cannot
 * see the one-shot Wi-Fi join messages. Sending the header last means a monitor
 * attaching after boot orients on the first thing it sees.
 */
static void reportSensorState() {
  printToFIdentity();
  printMpuWhoAmI();
  Serial.printf("tof config: mode %s, budget %u us, timeout %u ms\n",
                sensor.getDistanceMode() == VL53L1X::Long ? "long"
                : sensor.getDistanceMode() == VL53L1X::Medium ? "medium"
                : sensor.getDistanceMode() == VL53L1X::Short ? "short" : "?",
                static_cast<unsigned>(sensor.getMeasurementTimingBudget()),
                static_cast<unsigned>(sensor.getTimeout()));
  if (g_minRangeMm <= g_maxRangeMm) {
    Serial.printf("stats: rows=%lu tof ok=%lu timeout=%lu range_mm min=%ld"
                  " max=%ld tof_last=%s | imu ok=%lu short=%lu nack=%lu |"
                  " presses=%lu\n",
                  static_cast<unsigned long>(g_rowCount),
                  static_cast<unsigned long>(g_tofOkCount),
                  static_cast<unsigned long>(g_tofTimeoutCount),
                  static_cast<long>(g_minRangeMm),
                  static_cast<long>(g_maxRangeMm),
                  sensor.rangeStatusToString(g_lastRangeStatus),
                  static_cast<unsigned long>(g_imuOkCount),
                  static_cast<unsigned long>(g_imuShortCount),
                  static_cast<unsigned long>(g_imuNackCount),
                  static_cast<unsigned long>(g_pressCount));
  } else {
    Serial.printf("stats: rows=%lu tof ok=%lu timeout=%lu (no valid range yet)"
                  " | imu ok=%lu short=%lu nack=%lu | presses=%lu\n",
                  static_cast<unsigned long>(g_rowCount),
                  static_cast<unsigned long>(g_tofOkCount),
                  static_cast<unsigned long>(g_tofTimeoutCount),
                  static_cast<unsigned long>(g_imuOkCount),
                  static_cast<unsigned long>(g_imuShortCount),
                  static_cast<unsigned long>(g_imuNackCount),
                  static_cast<unsigned long>(g_pressCount));
  }
  if (linkUp) {
    Serial.print("link: up, sending to ");
    Serial.print(linkDestination());
    Serial.printf(":%u, sent=%lu failed=%lu\n",
                  static_cast<unsigned>(LINK_PORT),
                  static_cast<unsigned long>(g_linkSentCount),
                  static_cast<unsigned long>(g_linkFailCount));
  } else {
    Serial.printf("link: down (no wifi_secrets.h or join failed),"
                  " sent=%lu failed=%lu\n",
                  static_cast<unsigned long>(g_linkSentCount),
                  static_cast<unsigned long>(g_linkFailCount));
  }
  emitTableHeader();
}

/**
 * Print the column header of the tabular combined stream.
 *
 * Every field is padded to a fixed width and closed with a tab, so the monitor
 * lines the stream up as a table and a field that overflows its width still
 * lands on the next tab stop. Columns: t_ms, milliseconds since boot; btn, 1
 * while the button is pressed and 0 otherwise (raw level, no debounce);
 * ax_g/ay_g/az_g, acceleration in g at ±2 g full scale; gx_dps/gy_dps/gz_dps,
 * angular rate in deg/s at ±250 deg/s; temp_C, the MPU6050's die temperature in
 * degC; imu, the burst outcome ok/short/nack; range_mm, ToF distance along the
 * beam in millimetres; status, the ToF measurement's verdict as the RangeStatus
 * enum (the legend prints at boot); sig_MCPS and amb_MCPS, the ToF's peak
 * signal and ambient rates in million counts per second; tof, the ToF read's
 * outcome ok/timeout.
 */
static void emitTableHeader() {
  Serial.printf("%-7s\t%-3s\t%-7s\t%-7s\t%-7s\t%-8s\t%-8s\t%-8s\t%-6s\t%-6s"
                "\t%-8s\t%-6s\t%-8s\t%-8s\t%s\n",
                "t_ms", "btn", "ax_g", "ay_g", "az_g", "gx_dps", "gy_dps",
                "gz_dps", "temp_C", "imu", "range_mm", "status", "sig_MCPS",
                "amb_MCPS", "tof");
}

/**
 * Print the ToF range_status legend the `status` column refers to.
 *
 * Values live in the driver's RangeStatus enum; nothing here re-invents them.
 */
static void emitStatusLegend() {
  Serial.println("status legend: 0=RangeValid 1=SigmaFail 2=SignalFail"
                 " 3=RangeValidMinRangeClipped 4=OutOfBoundsFail");
  Serial.println("               5=HardwareFail 6=RangeValidNoWrapCheckFail"
                 " 7=WrapTargetFail 9=XtalkSignalFail");
  Serial.println("               10=SynchronizationInt 13=MinRangeFail 255=None");
}

/**
 * Configure the bus, bring both sensors up through their own probes, configure
 * the button, and start the combined tabular stream.
 *
 * @note Called once by the Arduino core before loop(). Bring-up is retried
 *       every second because every failure this sketch diagnoses is a wiring
 *       failure, and refitting the wire must be picked up without reflashing.
 *       Should a device stay absent, the retry itself is the repeating error
 *       report, and the scan output names which of the two is missing.
 */
void setup() {
  Serial.begin(115200);

  Serial.println();
  Serial.println("Auris pointer - sensor-node test (ToF + IMU + button; laptop"
                 " USB power; Wi-Fi link to the cu while the button is held)");
#ifdef ESP_ARDUINO_VERSION_STR
  Serial.printf("arduino-esp32 core %s\n", ESP_ARDUINO_VERSION_STR);
#endif

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(I2C_CLOCK_HZ);

  while (!attemptBringUp()) {
    Serial.println("retrying in 1 s -- refit the wiring and it will be picked up");
    delay(1000);
  }

  linkUp = wifiConnect();

  sensor.startContinuous(RANGE_PERIOD_MS);

  Serial.println("streaming ToF long mode (to 4 m) alongside IMU and button:"
                 " wave a hand across the lens for range, tap the button for"
                 " btn=1, tilt the board for the gyro columns");
  Serial.println("hold the button to broadcast the same rows to the compute unit");
  emitStatusLegend();
  emitTableHeader();
  Serial.flush();
}

/**
 * Read one IMU burst, one ToF measurement, and the button level, and emit one
 * combined table row.
 *
 * Each sensor's fallout is confined to its own columns, per the parent
 * contract's no-plausible-zero rule: a ToF read that times out emits nan for
 * the range columns while the IMU columns still carry that row's true burst,
 * an IMU transfer that is short or NAKed emits nan for the IMU columns while
 * the range columns still carry the row's fresh measurement, and neither ever
 * decrescendos into a plausible-looking zero. While the button is held, the
 * same row is also broadcast to the compute unit; the local print happens first,
 * so the USB record does not depend on the link. The loop is paced to
 * LOOP_PERIOD_MS so the serial link is never overrun; with the ToF's blocking
 * read doing the waiting, the effective row cadence is the ~94 ms two-period
 * stride the tof_test_l1x bench captures measured.
 */
void loop() {
  const uint32_t nowMs = millis();

  // --- IMU: one 14-byte big-endian burst from 0x3B ---------------------------
  int16_t rawAx = 0, rawAy = 0, rawAz = 0, rawTemp = 0;
  int16_t rawGx = 0, rawGy = 0, rawGz = 0;
  const char *imuStatus = "nack";
  float accelXg = NAN, accelYg = NAN, accelZg = NAN;
  float gyroXdps = NAN, gyroYdps = NAN, gyroZdps = NAN, tempC = NAN;

  Wire.beginTransmission(ADDR_MPU6050);
  Wire.write(MPU_REG_BURST);
  if (Wire.endTransmission(false) == 0) {
    const uint8_t got = Wire.requestFrom(ADDR_MPU6050, static_cast<uint8_t>(14));
    if (got == 14) {
      uint8_t burst[14];
      for (uint8_t i = 0; i < 14; ++i) {
        burst[i] = static_cast<uint8_t>(Wire.read());
      }
      rawAx = static_cast<int16_t>((burst[0] << 8) | burst[1]);
      rawAy = static_cast<int16_t>((burst[2] << 8) | burst[3]);
      rawAz = static_cast<int16_t>((burst[4] << 8) | burst[5]);
      rawTemp = static_cast<int16_t>((burst[6] << 8) | burst[7]);
      rawGx = static_cast<int16_t>((burst[8] << 8) | burst[9]);
      rawGy = static_cast<int16_t>((burst[10] << 8) | burst[11]);
      rawGz = static_cast<int16_t>((burst[12] << 8) | burst[13]);
      accelXg = static_cast<float>(rawAx) / MPU_ACCEL_LSB_PER_G;
      accelYg = static_cast<float>(rawAy) / MPU_ACCEL_LSB_PER_G;
      accelZg = static_cast<float>(rawAz) / MPU_ACCEL_LSB_PER_G;
      gyroXdps = static_cast<float>(rawGx) / MPU_GYRO_LSB_PER_DPS;
      gyroYdps = static_cast<float>(rawGy) / MPU_GYRO_LSB_PER_DPS;
      gyroZdps = static_cast<float>(rawGz) / MPU_GYRO_LSB_PER_DPS;
      tempC = static_cast<float>(rawTemp) / MPU_TEMP_LSB_PER_C
              + MPU_TEMP_OFFSET_C;
      imuStatus = "ok";
      ++g_imuOkCount;
    } else if (got > 0) {
      // Drain whatever arrived short so the bus is left clean for the next
      // attempt; the row's IMU columns go out as nan.
      for (uint8_t i = 0; i < got; ++i) {
        Wire.read();
      }
      imuStatus = "short";
      ++g_imuShortCount;
    } else {
      ++g_imuNackCount;
    }
  } else {
    ++g_imuNackCount;
  }

  // --- Button: raw level, one column, and the gate on the UDP link ------------
  const bool pressed = buttonPressed();
  if (pressed != g_buttonWasPressed) {
    Serial.println();
    Serial.println(pressed ? "button DOWN - streaming to the compute unit"
                           : "button UP - stopped streaming to the compute unit");
  }
  if (!g_buttonWasPressed && pressed) {
    ++g_pressCount;
  }
  g_buttonWasPressed = pressed;

  // --- ToF: one continuous-mode measurement, timeout-guarded ------------------
  const uint16_t readRangeMm = sensor.read();
  const bool tofTimeout = (readRangeMm == 0 && sensor.timeoutOccurred());

  // --- One row ----------------------------------------------------------------
  char rangeBuf[10] = "nan";
  char statusBuf[6] = "nan";
  char sigBuf[10] = "nan";
  char ambBuf[10] = "nan";
  char tofBuf[8] = "timeout";
  uint16_t rangeMm = 0;
  if (!tofTimeout) {
    rangeMm = readRangeMm;
    const VL53L1X::RangeStatus rangeStatus = sensor.ranging_data.range_status;
    g_lastRangeStatus = rangeStatus;
    snprintf(rangeBuf, sizeof(rangeBuf), "%u",
             static_cast<unsigned>(rangeMm));
    snprintf(statusBuf, sizeof(statusBuf), "%u",
             static_cast<unsigned>(rangeStatus));
    if (rangeStatus == VL53L1X::RangeValid && rangeMm > 0) {
      const int32_t signedRange = static_cast<int32_t>(rangeMm);
      if (signedRange < g_minRangeMm) {
        g_minRangeMm = signedRange;
      }
      if (signedRange > g_maxRangeMm) {
        g_maxRangeMm = signedRange;
      }
    }
    snprintf(sigBuf, sizeof(sigBuf), "%.2f",
             sensor.ranging_data.peak_signal_count_rate_MCPS);
    snprintf(ambBuf, sizeof(ambBuf), "%.2f",
             sensor.ranging_data.ambient_count_rate_MCPS);
    snprintf(tofBuf, sizeof(tofBuf), "ok");
    ++g_tofOkCount;
  } else {
    g_lastRangeStatus = VL53L1X::None;
    ++g_tofTimeoutCount;
  }

  char row[192];
  snprintf(row, sizeof(row),
           "%-7lu\t%-3d\t%-7.3f\t%-7.3f\t%-7.3f\t%-8.2f\t%-8.2f\t%-8.2f"
           "\t%-6.2f\t%-6s\t%-8s\t%-6s\t%-8s\t%-8s\t%s",
           static_cast<unsigned long>(millis()), pressed ? 1 : 0,
           accelXg, accelYg, accelZg, gyroXdps, gyroYdps, gyroZdps,
           tempC, imuStatus, rangeBuf, statusBuf, sigBuf, ambBuf, tofBuf);
  Serial.println(row);
  if (pressed) {
    linkSend(row);
  }
  ++g_rowCount;

  if (nowMs - lastProbeMs >= PROBE_PERIOD_MS) {
    lastProbeMs = nowMs;
    reportSensorState();
  }

  const uint32_t elapsedMs = millis() - nowMs;
  if (elapsedMs < LOOP_PERIOD_MS) {
    delay(LOOP_PERIOD_MS - elapsedMs);
  }
}
