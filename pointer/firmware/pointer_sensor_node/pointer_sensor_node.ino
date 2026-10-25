/*
 * Auris pointer — deployable sensor-node firmware.
 *
 * This is the firmware that runs on the pointer itself, untethered from any
 * desktop: the ESP32 is powered from a USB power bank, a laptop USB port, or a
 * LiPo feeding a 5 V regulator, and it talks to the compute unit over the same
 * Wi-Fi UDP link imu_test established. USB is optional in every sense — the
 * firmware never waits for a serial host, never blocks on a send, and the exact
 * same binary is used on the bench and in the field.
 *
 * Responsibilities:
 *   - bring up the MPU6050 (raw registers, no IMU library) and the VL53L1X
 *     (Pololu driver, the parent contract's one library exception);
 *   - fuse the IMU into a body-to-world quaternion at 200 Hz;
 *   - poll the ToF without blocking, with a hard-reset recovery path;
 *   - while the button is held, send a versioned CSV packet at 100 Hz to the
 *     compute unit over UDP, and stop on release;
 *   - emit the same packet row over USB at 25 Hz for bench work, plus a small
 *     `@` command console for calibration and link provisioning;
 *   - persist calibration, the boot counter, and the unicast override in NVS;
 *   - recover from sensor resets and Wi-Fi drops without rebooting.
 *
 * The "laser" is the VL53L1X's invisible IR ranging beam; there is no separate
 * laser output pin. The button gates the pointer's output (and therefore the
 * rendered sound), while the ToF ranges continuously so the first pressed
 * sample is fresh.
 *
 * Wiring, calibration, power options, packet schema and verification status are
 * documented in AGENTS.md beside this file.
 *
 * Build:   arduino-cli compile --profile pointer-sensor-node
 * Flash:   arduino-cli upload  --profile pointer-sensor-node --port /dev/ttyUSB0
 * Install: arduino-cli lib install VL53L1X   (once, or via sketch.yaml)
 */

#include <Arduino.h>
#include <Wire.h>
#include <esp_system.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <strings.h>

#include "src/config.h"
#include "src/fusion.h"
#include "src/link.h"
#include "src/mpu6050.h"
#include "src/packet.h"
#include "src/store.h"
#include "src/tof.h"

// Credentials live in an untracked header so the password never reaches git. The
// sketch still builds without it, falling back to USB serial only.
#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define HAVE_WIFI_CREDENTIALS 1
#endif

/** Degrees-to-radians factor for the fused gyro rate. */
static constexpr float kDegToRad = 0.017453292519943295f;

// ---------------------------------------------------------------------------
// Persistent and runtime state.
// ---------------------------------------------------------------------------

/** MPU6050 driver instance. */
static Mpu6050 g_mpu;
/** VL53L1X wrapper instance. */
static Tof g_tof;
/** Attitude filter instance. */
static MahonyFilter g_fusion;
/** Wi-Fi/UDP link instance. */
static Link g_link;
/** NVS-backed settings instance. */
static Store g_store;
/** Calibration currently in force. */
static Calibration g_cal;

/** Increments once per boot and survives resets; carried in every packet. */
static uint32_t g_bootId = 0;
/** Reset cause captured at boot; carried in every packet. */
static uint8_t g_resetReason = 0;
/** Monotonic microseconds since boot, extended across the 32-bit wrap. */
static uint64_t g_timeUs = 0;
/** Last raw `micros()` value used by the extension. */
static uint32_t g_lastMicrosUs = 0;

/** Whether the last IMU burst was good and non-zero. */
static bool g_imuOk = false;
/** Whether the ToF has a measurement inside its stale window. */
static bool g_tofOk = false;
/** Whether the fusion has been seeded from a real sample. */
static bool g_fusionStarted = false;

/** Latest calibrated accelerometer reading in g, sensor axes. */
static Vec3 g_accelG{0.0f, 0.0f, 0.0f};
/** Latest calibrated gyro reading in deg/s, sensor axes. */
static Vec3 g_gyroDps{0.0f, 0.0f, 0.0f};
/** Latest die temperature in degC. */
static float g_tempC = 0.0f;

/** Timestamp of the last IMU sample, in extended microseconds. */
static uint64_t g_lastImuUs = 0;
/** Timestamp of the last fusion update, in extended microseconds. */
static uint64_t g_lastFusionUs = 0;
/** Timestamp of the last USB debug row, in extended microseconds. */
static uint64_t g_lastSerialUs = 0;
/** Timestamp of the last UDP packet, in extended microseconds. */
static uint64_t g_lastPacketUs = 0;
/** Last ToF poll time in milliseconds. */
static uint32_t g_lastTofPollMs = 0;
/** Last time the serial header was repeated, in milliseconds. */
static uint32_t g_lastHeaderMs = 0;
/** Last MPU configuration rewrite, in milliseconds. */
static uint32_t g_lastReconfigMs = 0;

/** Datagram sequence counter, incremented for every UDP packet. */
static uint32_t g_seq = 0;
/** LOW-going button edges, since boot. */
static uint32_t g_pressEdges = 0;
/** HIGH-going button edges, since boot. */
static uint32_t g_releaseEdges = 0;
/** Consecutive all-zero IMU bursts, used to trigger a reconfiguration. */
static uint32_t g_imuZeroBursts = 0;
/** Consecutive IMU failures, used by the LED and health flag. */
static uint32_t g_imuConsecutiveErrors = 0;
/** MPU configuration rewrites since boot. */
static uint32_t g_imuReconfigs = 0;

/** Raw button level from the last read. */
static bool g_buttonRaw = false;
/** Debounced button level. */
static bool g_buttonStable = false;
/** Time of the last raw button change, in milliseconds. */
static uint32_t g_buttonChangedMs = 0;
/** Set on release so one final button=0 packet leaves the device. */
static bool g_releasePending = false;

/** USB command line accumulator. */
static char g_cmdBuf[96] = {0};
/** Number of valid bytes in `g_cmdBuf`. */
static size_t g_cmdLen = 0;

// ---------------------------------------------------------------------------
// Small helpers.
// ---------------------------------------------------------------------------

/** @return A short human-readable name for an `esp_reset_reason()` value. */
static const char *resetReasonName(uint8_t reason) {
  switch (reason) {
    case 1: return "power-on";
    case 2: return "external";
    case 3: return "software";
    case 4: return "panic";
    case 5: return "int-wdt";
    case 6: return "task-wdt";
    case 7: return "wdt";
    case 8: return "deep-sleep";
    case 9: return "brownout";
    case 10: return "sdio";
    default: return "other";
  }
}

/** Drive the onboard LED. */
static void setLed(bool on) {
  digitalWrite(cfg::PIN_LED, on ? HIGH : LOW);
}

/**
 * Advance the 64-bit microsecond clock from the wrapping `micros()` counter.
 *
 * `micros()` wraps every ~71.6 minutes. Extending it here keeps `t_us` strictly
 * increasing for study-length sessions and keeps packet timestamps comparable
 * across the wrap.
 */
static void updateTimeUs() {
  const uint32_t now = micros();
  g_timeUs += static_cast<uint32_t>(now - g_lastMicrosUs);
  g_lastMicrosUs = now;
}

/**
 * Print the USB column header.
 *
 * The row schema is shared with the UDP datagram, so the header is the same
 * string; it is prefixed with `#` to keep it distinguishable from data rows.
 */
static void printPacketHeader() {
  Serial.print("#");
  Serial.println(packetHeader());
}

// ---------------------------------------------------------------------------
// Packet construction.
// ---------------------------------------------------------------------------

/**
 * Build one packet from the current runtime state.
 *
 * `seq` is the last datagram sequence, so a USB row never invents sequence
 * numbers that no datagram carried. Range fields carry -1/255/-1 when no
 * measurement exists; the flags byte states whether the values are trustworthy.
 *
 * @param nowUs Extended microseconds for the packet timestamp.
 * @param button 1 while the pointer is being held, 0 otherwise.
 * @return The assembled packet.
 */
static PointerPacket buildPacket(uint64_t nowUs, uint8_t button) {
  PointerPacket p;
  p.seq = g_seq;
  p.tUs = nowUs;
  p.bootId = g_bootId;
  p.button = button;
  p.q = g_fusion.quaternion();
  p.euler = g_fusion.eulerDeg();
  p.gyroDps = g_gyroDps;
  p.accelG = g_accelG;
  p.tempC = g_tempC;

  const uint32_t nowMs = millis();
  if (g_tof.hasRange()) {
    p.rangeMm = static_cast<int16_t>(g_tof.rangeMm());
    p.rangeStatus = g_tof.rangeStatus();
    int32_t age = g_tof.rangeAgeMs(nowMs);
    if (age > 32767) {
      age = 32767;
    }
    p.rangeAgeMs = static_cast<int16_t>(age);
  }

  p.flags = 0;
  if (g_cal.valid) {
    p.flags |= kFlagCalibrated;
  }
  if (g_imuOk) {
    p.flags |= kFlagImuOk;
  }
  if (g_tofOk) {
    p.flags |= kFlagTofOk;
  }
  if (g_link.up()) {
    p.flags |= kFlagLinkUp;
  }
  p.resetReason = g_resetReason;
  return p;
}

/**
 * Format and send one UDP packet at the current time.
 *
 * The sequence counter advances even when the send fails, so the receiver can
 * distinguish a lost datagram from a silent pointer by the gap.
 *
 * @param nowUs Extended microseconds.
 * @param button 1 for a held-button packet, 0 for the release notification.
 */
static void sendPacket(uint64_t nowUs, uint8_t button) {
  PointerPacket p = buildPacket(nowUs, button);
  p.seq = ++g_seq;
  char line[256];
  if (formatPacket(line, sizeof(line), p) > 0) {
    g_link.send(line);
  }
}

// ---------------------------------------------------------------------------
// Sensor servicing.
// ---------------------------------------------------------------------------

/**
 * Sample the IMU at 200 Hz, apply calibration, and advance the fusion.
 *
 * A good burst resets the consecutive-failure counter; an all-zero burst is the
 * signature of a part that has reset into sleep, so it is counted and, after
 * `MPU_ZERO_BURSTS_BEFORE_RECONFIG`, the configuration registers are rewritten.
 * The fusion keeps its last orientation through a fault; the packet flags say
 * the value is not fresh.
 */
static void serviceImu() {
  if (g_timeUs - g_lastImuUs < cfg::IMU_PERIOD_US) {
    return;
  }
  g_lastImuUs = g_timeUs;

  ImuReading reading{};
  if (!g_mpu.read(reading)) {
    ++g_imuConsecutiveErrors;
    g_imuOk = false;
    return;
  }
  if (reading.allRawZero()) {
    ++g_imuZeroBursts;
    ++g_imuConsecutiveErrors;
    g_imuOk = false;
    if (g_imuZeroBursts >= cfg::MPU_ZERO_BURSTS_BEFORE_RECONFIG &&
        millis() - g_lastReconfigMs >= cfg::MPU_RECONFIG_INTERVAL_MS) {
      g_lastReconfigMs = millis();
      g_imuZeroBursts = 0;
      ++g_imuReconfigs;
      if (g_mpu.configure()) {
        Serial.println("#imu reconfigured after zero bursts");
      }
    }
    return;
  }

  g_imuZeroBursts = 0;
  g_imuConsecutiveErrors = 0;
  g_imuOk = true;

  g_accelG = Vec3{reading.axG - g_cal.accelOffsetG.x,
                  reading.ayG - g_cal.accelOffsetG.y,
                  reading.azG - g_cal.accelOffsetG.z};
  g_gyroDps = Vec3{reading.gxDps - g_cal.gyroBiasDps.x,
                   reading.gyDps - g_cal.gyroBiasDps.y,
                   reading.gzDps - g_cal.gyroBiasDps.z};
  g_tempC = reading.tempC;

  const float dt = static_cast<float>(g_timeUs - g_lastFusionUs) / 1000000.0f;
  g_lastFusionUs = g_timeUs;
  if (!g_fusionStarted) {
    g_fusion.begin(g_accelG);
    g_fusionStarted = true;
  }
  g_fusion.update(Vec3{g_gyroDps.x * kDegToRad, g_gyroDps.y * kDegToRad,
                       g_gyroDps.z * kDegToRad},
                  g_accelG, dt);
}

/** Poll the ToF at `TOF_POLL_PERIOD_MS` and refresh the health flag. */
static void serviceTof() {
  const uint32_t nowMs = millis();
  if (nowMs - g_lastTofPollMs >= cfg::TOF_POLL_PERIOD_MS) {
    g_lastTofPollMs = nowMs;
    g_tof.poll(nowMs);
  }
  g_tofOk = g_tof.healthy(nowMs);
}

/**
 * Debounce the button and emit press/release edges.
 *
 * A press forces the next packet immediately by winding `g_lastPacketUs` back
 * one packet period; a release requests one final button=0 datagram so the
 * compute unit can stop cleanly even before its own timeout fires.
 */
static void serviceButton() {
  const bool raw = digitalRead(cfg::PIN_BUTTON) == LOW;
  const uint32_t nowMs = millis();
  if (raw != g_buttonRaw) {
    g_buttonRaw = raw;
    g_buttonChangedMs = nowMs;
  }
  if (nowMs - g_buttonChangedMs < cfg::BUTTON_DEBOUNCE_MS ||
      g_buttonStable == g_buttonRaw) {
    return;
  }

  g_buttonStable = g_buttonRaw;
  if (g_buttonStable) {
    ++g_pressEdges;
    g_lastPacketUs = g_timeUs - cfg::PACKET_PERIOD_US;
  } else {
    ++g_releaseEdges;
    g_releasePending = true;
  }
}

/** Send held-button packets at 100 Hz and the one-shot release packet. */
static void servicePacket() {
  const bool canSend = g_link.up();
  if (g_buttonStable && canSend &&
      g_timeUs - g_lastPacketUs >= cfg::PACKET_PERIOD_US) {
    g_lastPacketUs = g_timeUs;
    sendPacket(g_timeUs, 1);
  }
  if (g_releasePending) {
    if (canSend) {
      sendPacket(g_timeUs, 0);
    }
    g_releasePending = false;
  }
}

// ---------------------------------------------------------------------------
// USB serial and console.
// ---------------------------------------------------------------------------

/**
 * Emit the USB debug row at 25 Hz and repeat the header every five seconds.
 *
 * This is the desktop-interoperability path: the same schema as the UDP
 * datagram, readable with `arduino-cli monitor`, and harmless when no host is
 * connected because every write is non-blocking at this rate.
 */
static void serviceSerial() {
  if (g_timeUs - g_lastSerialUs < cfg::SERIAL_PERIOD_US) {
    return;
  }
  g_lastSerialUs = g_timeUs;

  const uint32_t nowMs = millis();
  if (nowMs - g_lastHeaderMs >= cfg::HEADER_PERIOD_MS) {
    g_lastHeaderMs = nowMs;
    printPacketHeader();
  }

  PointerPacket p = buildPacket(g_timeUs, g_buttonStable ? 1 : 0);
  char line[256];
  if (formatPacket(line, sizeof(line), p) > 0) {
    Serial.println(line);
  }
}

/**
 * Print the persistent state, counters and link status over USB.
 *
 * Intended for `@STATUS` during provisioning; every line starts with `#` so a
 * parser can keep it out of the sample stream.
 */
static void printStatus() {
  Serial.printf("#status boot=%lu reset=%u(%s) cal=%d imu=%d tof=%d link=%d\n",
                static_cast<unsigned long>(g_bootId),
                static_cast<unsigned>(g_resetReason),
                resetReasonName(g_resetReason), g_cal.valid ? 1 : 0,
                g_imuOk ? 1 : 0, g_tofOk ? 1 : 0, g_link.up() ? 1 : 0);
  Serial.printf("#cal ax=%.4f ay=%.4f az=%.4f gx=%.3f gy=%.3f gz=%.3f\n",
                g_cal.accelOffsetG.x, g_cal.accelOffsetG.y,
                g_cal.accelOffsetG.z, g_cal.gyroBiasDps.x,
                g_cal.gyroBiasDps.y, g_cal.gyroBiasDps.z);
  Serial.printf("#counters imu_err=%lu imu_zero=%lu imu_reconfig=%lu "
                "tof_reinit=%lu tof_recover_fail=%lu link_sent=%lu "
                "link_fail=%lu link_reconnects=%lu presses=%lu releases=%lu\n",
                static_cast<unsigned long>(g_mpu.readErrors()),
                static_cast<unsigned long>(g_imuZeroBursts),
                static_cast<unsigned long>(g_imuReconfigs),
                static_cast<unsigned long>(g_tof.reinits()),
                static_cast<unsigned long>(g_tof.recoverFailures()),
                static_cast<unsigned long>(g_link.sent()),
                static_cast<unsigned long>(g_link.failed()),
                static_cast<unsigned long>(g_link.reconnects()),
                static_cast<unsigned long>(g_pressEdges),
                static_cast<unsigned long>(g_releaseEdges));
  if (g_link.enabled()) {
    Serial.print("#link ip=");
    Serial.print(g_link.localIp());
    Serial.print(" dest=");
    Serial.print(g_link.destination());
    Serial.print(" override=");
    Serial.print(g_link.unicast().isEmpty() ? "broadcast" : g_link.unicast());
    Serial.printf(" rssi=%ld\n", static_cast<long>(g_link.rssi()));
  } else {
    Serial.println("#link disabled (no wifi_secrets.h)");
  }
}

/**
 * Run the flat, Z-up accelerometer and gyro calibration and store it in NVS.
 *
 * The pointer must be placed flat with the sensor's +Z axis up and left still
 * for the two-second window. Spread thresholds reject a run taken while the
 * device was being handled, because a bad offset would corrupt every fused
 * orientation until the next CAL.
 *
 * Blocks for roughly two seconds; the Arduino task watchdog is not fed during
 * that window but the default five-second limit is not approached.
 */
static void runCalibration() {
  Serial.println("#cal place the pointer flat (sensor Z up) and still;"
                 " sampling 2 s");

  double sumA[3] = {0.0, 0.0, 0.0};
  double sumG[3] = {0.0, 0.0, 0.0};
  float aMin[3] = {1.0e9f, 1.0e9f, 1.0e9f};
  float aMax[3] = {-1.0e9f, -1.0e9f, -1.0e9f};
  float gMin[3] = {1.0e9f, 1.0e9f, 1.0e9f};
  float gMax[3] = {-1.0e9f, -1.0e9f, -1.0e9f};

  for (uint32_t i = 0; i < cfg::CAL_SAMPLES; ++i) {
    ImuReading r{};
    if (!g_mpu.read(r) || r.allRawZero()) {
      Serial.printf("#cal aborted: IMU read failed at sample %lu\n",
                    static_cast<unsigned long>(i));
      return;
    }
    const float a[3] = {r.axG, r.ayG, r.azG};
    const float g[3] = {r.gxDps, r.gyDps, r.gzDps};
    for (uint8_t axis = 0; axis < 3; ++axis) {
      sumA[axis] += a[axis];
      sumG[axis] += g[axis];
      if (a[axis] < aMin[axis]) aMin[axis] = a[axis];
      if (a[axis] > aMax[axis]) aMax[axis] = a[axis];
      if (g[axis] < gMin[axis]) gMin[axis] = g[axis];
      if (g[axis] > gMax[axis]) gMax[axis] = g[axis];
    }
    delay(cfg::CAL_PERIOD_MS);
  }

  float aSpread = 0.0f;
  float gSpread = 0.0f;
  for (uint8_t axis = 0; axis < 3; ++axis) {
    aSpread = std::max(aSpread, aMax[axis] - aMin[axis]);
    gSpread = std::max(gSpread, gMax[axis] - gMin[axis]);
  }
  if (aSpread > cfg::CAL_ACCEL_MAX_SPREAD_G ||
      gSpread > cfg::CAL_GYRO_MAX_SPREAD_DPS) {
    Serial.printf("#cal unstable: accel spread %.3f g, gyro spread %.2f dps;"
                  " not saved\n",
                  aSpread, gSpread);
    return;
  }

  const Vec3 meanA{static_cast<float>(sumA[0] / cfg::CAL_SAMPLES),
                   static_cast<float>(sumA[1] / cfg::CAL_SAMPLES),
                   static_cast<float>(sumA[2] / cfg::CAL_SAMPLES)};
  const Vec3 meanG{static_cast<float>(sumG[0] / cfg::CAL_SAMPLES),
                   static_cast<float>(sumG[1] / cfg::CAL_SAMPLES),
                   static_cast<float>(sumG[2] / cfg::CAL_SAMPLES)};

  Calibration next{};
  next.valid = true;
  next.accelOffsetG = accelOffsetFromFlatMean(meanA);
  next.gyroBiasDps = meanG;
  if (!g_store.saveCalibration(next)) {
    Serial.println("#cal warning: NVS write failed; calibration is RAM-only");
  }
  g_cal = next;

  // The filtered orientation must start from the corrected mean, not from the
  // pre-calibration sample that seeded it.
  g_fusion.begin(Vec3{meanA.x - g_cal.accelOffsetG.x,
                      meanA.y - g_cal.accelOffsetG.y,
                      meanA.z - g_cal.accelOffsetG.z});
  g_fusionStarted = true;
  g_lastFusionUs = g_timeUs;

  Serial.printf("#cal saved accel=(%.4f, %.4f, %.4f) g gyro=(%.3f, %.3f,"
                " %.3f) dps\n",
                g_cal.accelOffsetG.x, g_cal.accelOffsetG.y,
                g_cal.accelOffsetG.z, g_cal.gyroBiasDps.x,
                g_cal.gyroBiasDps.y, g_cal.gyroBiasDps.z);
}

/**
 * Execute one `@` command from the USB console.
 *
 * Commands are case-insensitive and intentionally small: CAL, STATUS, DEST,
 * DEFAULTS, REBOOT, HELP. Everything writes back a `#` line.
 *
 * @param line Command text after the leading `@`, NUL-terminated.
 */
static void handleCommand(char *line) {
  while (*line == ' ' || *line == '\t') {
    ++line;
  }

  if (strcasecmp(line, "CAL") == 0) {
    runCalibration();
  } else if (strcasecmp(line, "STATUS") == 0) {
    printStatus();
  } else if (strncasecmp(line, "DEST", 4) == 0 &&
             (line[4] == ' ' || line[4] == '\0')) {
    char *arg = line + 4;
    while (*arg == ' ' || *arg == '\t') {
      ++arg;
    }
    if (*arg == '\0' || strcasecmp(arg, "broadcast") == 0) {
      g_store.setUnicastDestination("");
      g_link.setUnicast("");
      Serial.println("#dest broadcast");
      return;
    }
    unsigned a = 0;
    unsigned b = 0;
    unsigned c = 0;
    unsigned d = 0;
    if (std::sscanf(arg, "%u.%u.%u.%u", &a, &b, &c, &d) != 4 || a > 255 ||
        b > 255 || c > 255 || d > 255) {
      Serial.println("#dest invalid; expected dotted-quad or broadcast");
      return;
    }
    char normalized[16];
    std::snprintf(normalized, sizeof(normalized), "%u.%u.%u.%u", a, b, c, d);
    g_store.setUnicastDestination(String(normalized));
    g_link.setUnicast(String(normalized));
    Serial.printf("#dest %s\n", normalized);
  } else if (strcasecmp(line, "DEFAULTS") == 0) {
    g_store.clearAll();
    g_cal = Calibration{};
    g_link.setUnicast("");
    Serial.println("#defaults cleared (calibration and destination)");
  } else if (strcasecmp(line, "REBOOT") == 0) {
    Serial.println("#rebooting");
    Serial.flush();
    delay(100);
    ESP.restart();
  } else if (strcasecmp(line, "HELP") == 0 || *line == '\0') {
    Serial.println("#commands: @CAL @STATUS @DEST <ip|broadcast> @DEFAULTS"
                   " @REBOOT");
  } else {
    Serial.printf("#unknown command: %s\n", line);
  }
}

/** Drain and dispatch complete `@` command lines from USB serial. */
static void serviceConsole() {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\r' || c == '\n') {
      if (g_cmdLen > 0) {
        g_cmdBuf[g_cmdLen] = '\0';
        if (g_cmdBuf[0] == '@') {
          handleCommand(g_cmdBuf + 1);
        }
        g_cmdLen = 0;
      }
    } else if (g_cmdLen < sizeof(g_cmdBuf) - 1) {
      g_cmdBuf[g_cmdLen++] = c;
    }
  }
}

// ---------------------------------------------------------------------------
// LED and boot.
// ---------------------------------------------------------------------------

/**
 * Show a coarse device state on the onboard LED.
 *
 * Patterns: fast blink for a sensor fault, slow blink while the Wi-Fi link is
 * down (or disabled), solid while the button is held, off when everything is
 * nominal. The LED is for the researcher during provisioning; it is not the
 * user-facing interface.
 *
 * @param nowMs Current `millis()` value.
 */
static void updateLed(uint32_t nowMs) {
  const bool sensorFault =
      g_imuConsecutiveErrors >= 10 || !g_tof.healthy(nowMs);
  if (sensorFault) {
    setLed(((nowMs / 100) % 2) == 0);
    return;
  }
  if (g_link.enabled() && !g_link.up()) {
    setLed(((nowMs / 500) % 2) == 0);
    return;
  }
  setLed(g_buttonStable);
}

/**
 * Bring both sensors up, retrying until each answers.
 *
 * This is the only intentionally blocking loop in the firmware, and it runs
 * before the device is usable. The Wi-Fi state machine is polled during the
 * waits so the link can associate in parallel. Every failure is reported on USB
 * so a technician can diagnose a dead module without a logic analyser.
 */
static void bringUpSensors() {
  bool imuReady = false;
  bool tofReady = false;
  uint32_t attempt = 0;

  while (!imuReady || !tofReady) {
    ++attempt;
    g_link.poll(millis());
    if (!imuReady) {
      imuReady = g_mpu.begin();
      if (imuReady) {
        imuReady = g_mpu.configure();
      }
      if (imuReady) {
        Serial.printf("#imu who=0x%02X ok (attempt %lu)\n", g_mpu.whoAmI(),
                      static_cast<unsigned long>(attempt));
        if (g_mpu.whoAmI() != cfg::MPU_WHO_AM_I_EXPECTED) {
          Serial.println("#imu warning: WHO_AM_I is not 0x68; streaming anyway");
        }
      } else {
        Serial.printf("#imu bring-up attempt %lu failed\n",
                      static_cast<unsigned long>(attempt));
      }
    }
    if (!tofReady) {
      tofReady = g_tof.begin();
      if (tofReady) {
        const uint8_t *id = g_tof.identity();
        Serial.printf("#tof identity=%02X %02X %02X %02X, long mode,"
                      " %lu us budget, %lu ms period\n",
                      id[0], id[1], id[2], id[3],
                      static_cast<unsigned long>(cfg::TOF_BUDGET_US),
                      static_cast<unsigned long>(cfg::TOF_PERIOD_MS));
      } else {
        Serial.printf("#tof bring-up attempt %lu failed (check XSHUT,"
                      " VCC, SDA/SCL)\n",
                      static_cast<unsigned long>(attempt));
      }
    }
    if (!imuReady || !tofReady) {
      setLed(true);
      delay(250);
      setLed(false);
      delay(250);
    }
  }
  setLed(false);
}

// ---------------------------------------------------------------------------
// Arduino entry points.
// ---------------------------------------------------------------------------

/**
 * Initialise the device: bus, settings, link, sensors, fusion seed, and the
 * initial USB banner. Never waits for a serial host.
 */
void setup() {
  Serial.begin(115200);
  pinMode(cfg::PIN_LED, OUTPUT);
  setLed(false);

  Wire.begin(cfg::PIN_I2C_SDA, cfg::PIN_I2C_SCL);
  Wire.setClock(cfg::I2C_CLOCK_HZ);

  const bool nvsOk = g_store.begin();
  g_cal = g_store.calibration();
  g_bootId = g_store.nextBootId();
  g_resetReason = static_cast<uint8_t>(esp_reset_reason());

  Serial.println();
  Serial.println("# Auris pointer sensor node");
  Serial.printf("#build %s %s\n", __DATE__, __TIME__);
  Serial.printf("#boot=%lu reset=%u(%s) nvs=%d cal=%d\n",
                static_cast<unsigned long>(g_bootId),
                static_cast<unsigned>(g_resetReason),
                resetReasonName(g_resetReason), nvsOk ? 1 : 0,
                g_cal.valid ? 1 : 0);
  if (!g_cal.valid) {
    Serial.println("#cal missing: run @CAL with the pointer flat, Z up, still");
  }

  g_link.setUnicast(g_store.unicastDestination());
#ifdef HAVE_WIFI_CREDENTIALS
  g_link.begin(WIFI_SSID, WIFI_PASSWORD);
#else
  g_link.begin(nullptr, nullptr);
  Serial.println("#wifi disabled: no wifi_secrets.h; USB serial only");
#endif

  bringUpSensors();

  g_tofOk = g_tof.healthy(millis());
  g_lastMicrosUs = micros();
  g_lastTofPollMs = millis();
  g_lastHeaderMs = millis();
  printPacketHeader();
  printStatus();
  Serial.println("#ready; hold the button to stream to the compute unit");
  Serial.flush();
}

/**
 * Main superloop.
 *
 * The order is deliberate: extend time, service the link's state machine, then
 * the sensors, the button, the packet gate, the console, the USB row, and the
 * LED. No call in this loop is allowed to wait for a network peer or a serial
 * host.
 */
void loop() {
  updateTimeUs();
  g_link.poll(millis());
  serviceImu();
  serviceTof();
  serviceButton();
  servicePacket();
  serviceConsole();
  serviceSerial();
  updateLed(millis());
}
