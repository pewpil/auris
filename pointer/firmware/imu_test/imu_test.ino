/*
 * Auris pointer — IMU bring-up test.
 *
 * IMU-ONLY TEST WIRING  (VL53L0X deliberately NOT connected)
 *
 *   The two rows are numbered differently: r1 is the top row numbered 1-19, r2 is
 *   the bottom row numbered 60-42 across the same columns.  A column therefore has
 *   two unrelated numbers - r1 #9 and r2 #50 sit side by side.
 *
 *   ESP32 r1 #19  3V3    --  MPU6050/GY-521  VCC
 *   ESP32 r2 #42  GND    --  MPU6050/GY-521  GND
 *   ESP32 r2 #47  GPIO21 --  MPU6050/GY-521  SDA
 *   ESP32 r2 #44  GPIO22 --  MPU6050/GY-521  SCL
 *   ESP32 r2 #42  GND    --  MPU6050/GY-521  AD0      (selects I2C address 0x68)
 *
 *   ESP32 r2 #50  GPIO18 --  button  leg A
 *   ESP32 r2 #42  GND    --  button  leg B
 *
 *   MPU6050  XDA, XCL, INT --  not connected
 *   VL53L0X  --  not connected at all for this test
 *
 *   Ground: r2 #42, or r1 #2 / r1 #6 / r2 #48 -- any of them.
 *   No external pull-ups needed: the GY-521 carries its own 4.7k on SDA and SCL.
 *
 * Purpose: stream the IMU's orientation and raw motion data as CSV on the USB serial
 *          port, continuously, until the board is reset; and, WHILE THE BUTTON IS
 *          HELD, broadcast the same rows to the compute unit over UDP.
 *
 *          The button is not a data column. It gates the wireless link: held, samples
 *          are transmitted; released, transmission stops. The local USB stream runs
 *          either way, because it is the operator's own record and must not depend on
 *          the radio. There is no pin-level connection for the radio -- the ESP32's
 *          antenna is on the module -- so Wi-Fi adds no wiring, but it does add two
 *          requirements: a 2.4 GHz network (this radio cannot see 5 GHz at all) and
 *          credentials in wifi_secrets.h.
 *
 *          Orientation is a complementary filter: gravity from the accelerometer pins
 *          down roll and pitch absolutely, and the gyroscope carries them between
 *          updates. Yaw has no absolute reference -- there is no magnetometer here --
 *          so it is gyro-integrated and will drift. That drift is inherent to the
 *          sensor set, not a defect in this filter, and the gyro bias is calibrated
 *          out at startup to keep it slow.
 *
 * Inputs:  MPU6050 on I2C address 0x68; momentary button pulling GPIO 18 to GND;
 *          Wi-Fi credentials from the untracked wifi_secrets.h.
 * Outputs: CSV on the USB serial port at 115200 baud; the same rows broadcast to the
 *           compute unit over UDP port 5005 while the button is held. See
 *           emitCsvHeader() for the column schema.
 *
 * Build:   arduino-cli compile --profile imu-test
 * Flash:   arduino-cli upload  --profile imu-test --port /dev/ttyUSB0
 */

#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>   // core 3.x spells it this way; it typedefs WiFiUDP

// Credentials live in an untracked header so the password never reaches git. The
// sketch still builds without it, falling back to USB serial only.
#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define HAVE_WIFI_CREDENTIALS 1
#endif

// ---------------------------------------------------------------------------
// Types.
//
// The struct is declared here, immediately after the includes and ahead of every
// function, and not down beside the other constants. That is deliberate: the Arduino
// preprocessor hoists function prototypes to the top of the file, so a struct defined
// after the first function gets referenced by a prototype that appears before the
// struct exists, and the build fails with "ImuState was not declared in this scope".
// Putting the type first is what keeps that from happening.
// ---------------------------------------------------------------------------

/** One decoded IMU measurement: raw sensor data plus the derived orientation. */
struct ImuState {
  float accel_g[3];   ///< X, Y, Z specific force in g, bias-corrected.
  float gyro_dps[3];  ///< X, Y, Z angular rate in deg/s, bias-corrected.
  float temp_c;       ///< Die temperature in degrees Celsius.
  float roll_deg;     ///< Rotation about the X axis, degrees, +ve right-side down.
  float pitch_deg;    ///< Rotation about the Y axis, degrees, +ve nose up.
  float yaw_deg;      ///< Rotation about the Z axis, degrees; gyro-integrated, drifts.
  bool fresh;         ///< False when the last transfer failed and the angles are held.
};


// ---------------------------------------------------------------------------
// Pin map — matches the wiring comment above.
// ---------------------------------------------------------------------------

/** ESP32 I2C SDA line, at r2 #47. */
static constexpr uint8_t PIN_I2C_SDA = 21;
/** ESP32 I2C SCL line, at r2 #44. */
static constexpr uint8_t PIN_I2C_SCL = 22;
/** Button input, at r2 #50; configured INPUT_PULLUP so a press reads LOW. */
static constexpr uint8_t PIN_BUTTON = 18;

/** I2C address of the MPU6050 with AD0 tied to GND. */
static constexpr uint8_t ADDR_MPU6050 = 0x68;

/** I2C bus speed in Hz; the MPU6050 is rated well above this. */
static constexpr uint32_t I2C_CLOCK_HZ = 400000;

/**
 * Loop period in milliseconds, giving roughly 50 output lines per second.
 *
 * This is not optional. A full CSV line is around 85 bytes, so 50 lines per second is
 * about 4.3 kB/s against the 11.5 kB/s a 115200-baud link carries. Left unpaced, the
 * loop reads and prints as fast as the I2C bus allows, overruns the UART's transmit
 * buffer, and emits visibly truncated lines.
 */
static constexpr uint32_t LOOP_PERIOD_MS = 20;

/**
 * Interval between repeated sensor probes, in milliseconds.
 *
 * The identity byte and raw burst are reported periodically rather than only at boot,
 * because a boot banner emitted once inside setup() is missed by any reader that
 * attaches afterwards -- and attaching afterwards is the normal case.
 */
static constexpr uint32_t PROBE_PERIOD_MS = 5000;

// ---------------------------------------------------------------------------
// Pointer -> compute unit link.
//
// The pointer is wirelessly connected to the compute unit, so samples leave over
// UDP rather than the USB cable. Datagrams are BROADCAST to the subnet's directed
// broadcast address and the compute unit binds 0.0.0.0, so neither side needs to know
// the other's IP address. That matters in practice: this project's compute unit has
// moved address three times, which is why the repository AGENTS.md records its MAC as
// the stable identity instead.
// ---------------------------------------------------------------------------

/** UDP port the compute unit listens on. Must match --port on the cu side. */
static constexpr uint16_t LINK_PORT = 5005;

/** Milliseconds allowed for the Wi-Fi association to complete at boot. */
static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;

/**
 * Optional unicast override for the datagram destination.
 *
 * Empty (the default) means broadcast to the subnet's directed broadcast address.
 * Set it to an address such as "192.168.1.16" if the access point turns out to
 * filter client-to-client broadcast, which some do when AP isolation is on. A
 * unicast destination is a moving target under DHCP, so broadcast is the better
 * default and this is the escape hatch, not the other way round.
 */
static const char *LINK_UNICAST_HOST = "";

/** Whether the link came up and datagrams may be sent. */
static bool linkUp = false;

/** Set on the first send failure, so repeated failures do not spam the serial log. */
static bool linkFaultReported = false;

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

// ---------------------------------------------------------------------------
// MPU6050 register map (MPU-6000A register map, RM-MPU-6000A-1).
// ---------------------------------------------------------------------------

static constexpr uint8_t REG_SMPLRT_DIV = 0x19;
static constexpr uint8_t REG_CONFIG = 0x1A;
static constexpr uint8_t REG_GYRO_CONFIG = 0x1B;
static constexpr uint8_t REG_ACCEL_CONFIG = 0x1C;
static constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;
static constexpr uint8_t REG_WHO_AM_I = 0x75;
static constexpr uint8_t REG_PWR_MGMT_1 = 0x6B;

/**
 * PWR_MGMT_1 value that wakes the sensor: CLKSEL = 1, the X-axis gyro PLL.
 *
 * The MPU6050 comes out of reset in SLEEP. While asleep its identity registers still
 * answer -- WHO_AM_I reads back perfectly -- but the data registers never update and
 * read back as all zeroes. Writing this is the only thing that makes it report.
 */
static constexpr uint8_t PWR_WAKE_PLL_XGYRO = 0x01;

/** Settling time after waking the sensor, in milliseconds, before trusting data. */
static constexpr uint32_t WAKE_SETTLE_MS = 100;

/** Digital low-pass filter setting 3: ~44 Hz accelerometer and ~42 Hz gyro corner. */
static constexpr uint8_t DLPF_CONFIG = 3;
/** GYRO_CONFIG full scale 0: +/- 250 deg/s. */
static constexpr uint8_t GYRO_FS_250_DPS = 0x00;
/** ACCEL_CONFIG full scale 0: +/- 2 g. */
static constexpr uint8_t ACCEL_FS_2G = 0x00;
/** Accelerometer counts per g at the +/- 2 g setting. */
static constexpr float LSB_PER_G = 16384.0f;
/** Gyroscope counts per deg/s at the +/- 250 deg/s setting. */
static constexpr float LSB_PER_DPS = 131.0f;
/** Bytes in one burst from REG_ACCEL_XOUT_H: 3 accelerometer, temperature, 3 gyroscope. */
static constexpr uint8_t IMU_BURST_BYTES = 14;

/** Radius of pi/180, for converting the filter's angle arithmetic to degrees. */
static constexpr float DEG_PER_RAD = 57.2957795f;

/**
 * Complementary filter time constant in seconds.
 *
 * Gravity from the accelerometer corrects the gyro-integrated angle slowly, over this
 * horizon, while the gyro carries the fast motion. Longer trusts the accelerometer and
 * responds sluggishly; shorter trusts the gyro and accumulates its drift faster.
 */
static constexpr float FILTER_TIME_CONSTANT_S = 0.5f;

/** Duration of the startup bias measurement, during which the board must be still. */
static constexpr uint32_t CALIBRATION_MS = 2000;

/** Per-axis zero offsets measured at startup, subtracted from every reading. */
static float g_accelBiasG[3] = {0.0f, 0.0f, 0.0f};
/** Per-axis zero offsets measured at startup, subtracted from every reading. */
static float g_gyroBiasDps[3] = {0.0f, 0.0f, 0.0f};

/** Orientation and motion state, seeded in setup() and advanced once per loop(). */
static ImuState setLine{};
/** Timestamp of the previous loop iteration; the filter's dt is derived from it. */
static uint32_t lastUpdateMs = 0;
/** Timestamp of the last periodic sensor probe. */
static uint32_t lastProbeMs = 0;
/** Last observed button state, so press and release are reported once each. */
static bool wasButtonHeld = false;

/**
 * Reinterpret an unsigned 16-bit I2C payload as the signed value it encodes.
 *
 * Sensor registers carry two's-complement data in 16 bits. Converting uint16_t to
 * int16_t is only guaranteed to wrap for values above 32767 from C++20 onward, so the
 * subtraction is written out rather than left to the cast.
 *
 * @param raw The two bytes as received, in sensor register order (high byte first).
 * @return The same 16 bits read as a two's-complement signed integer, -32768..32767.
 */
static int16_t s16FromU16(uint16_t raw) {
  const int32_t signedRaw = static_cast<int32_t>(raw);
  return static_cast<int16_t>(signedRaw > 32767 ? signedRaw - 65536 : signedRaw);
}

/**
 * Read one byte from a register on an I2C device.
 *
 * @param addr 7-bit I2C address of the target device.
 * @param reg Register to read from.
 * @param[out] value Receives the register contents; untouched on failure.
 * @return true when the address phase and the single data byte both succeeded.
 */
static bool readRegister8(uint8_t addr, uint8_t reg, uint8_t &value) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(addr, static_cast<uint8_t>(1)) != 1) {
    return false;
  }
  value = static_cast<uint8_t>(Wire.read());
  return true;
}

/**
 * Write one byte to a register on an I2C device.
 *
 * @param addr 7-bit I2C address of the target device.
 * @param reg Register to write.
 * @param value Byte to store in that register.
 * @return true when the device acknowledged the stop condition.
 */
static bool writeRegister8(uint8_t addr, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission(true) == 0;
}

/**
 * Identify and configure the MPU6050.
 *
 * Reads WHO_AM_I, then selects the +/-2 g / +/-250 deg/s ranges and a ~44 Hz low-pass
 * filter at a 1 kHz internal sample rate. Any WHO_AM_I in the MPU6050/6500/9250
 * family is accepted, since GY-521 breakouts ship with several parts fitted.
 *
 * @return true when the identity byte was recognised and every configuration write
 *         was acknowledged. False means the scale factors used when decoding are not
 *         the ones the part was told to use, so readings must not be trusted.
 */
static bool imuInit() {
  uint8_t who = 0;
  if (!readRegister8(ADDR_MPU6050, REG_WHO_AM_I, who)) {
    Serial.printf("MPU6050: no ACK at 0x%02X - check SDA/SCL, GND and the 3V3 rail\n",
                  ADDR_MPU6050);
    return false;
  }

  const bool identified = (who == 0x68 || who == 0x70 || who == 0x71 || who == 0x73);
  Serial.printf("MPU6050: WHO_AM_I = 0x%02X%s\n", who,
                identified ? "" : "  (unrecognised - expecting 0x68, 0x70, 0x71 or 0x73)");

  if (!identified) {
    return false;
  }

  // Wake it first. Everything below is inert while the part is still asleep.
  const bool woke = writeRegister8(ADDR_MPU6050, REG_PWR_MGMT_1, PWR_WAKE_PLL_XGYRO);
  if (!woke) {
    Serial.println("MPU6050: could not write PWR_MGMT_1 - the part is not accepting writes");
    return false;
  }
  delay(WAKE_SETTLE_MS);

  return writeRegister8(ADDR_MPU6050, REG_SMPLRT_DIV, 0x00)
         && writeRegister8(ADDR_MPU6050, REG_CONFIG, DLPF_CONFIG)
         && writeRegister8(ADDR_MPU6050, REG_GYRO_CONFIG, GYRO_FS_250_DPS)
         && writeRegister8(ADDR_MPU6050, REG_ACCEL_CONFIG, ACCEL_FS_2G);
}

/**
 * Check that the sensor's data registers are actually moving.
 *
 * WHO_AM_I answers even from a sleeping part, so a correct identity byte proves only
 * that the chip is on the bus. This samples the burst repeatedly and reports whether
 * anything other than zero ever appears -- which is the difference between a wired-up
 * sensor and one that is acknowledged but inert.
 *
 * @param samples Number of bursts to read before giving up.
 * @param[out] nonZeroCount Receives how many of them contained a non-zero byte.
 * @return true when at least one burst contained data.
 */
static bool dataRegistersAreLive(uint16_t samples, uint16_t &nonZeroCount) {
  nonZeroCount = 0;
  for (uint16_t n = 0; n < samples; ++n) {
    uint8_t buf[IMU_BURST_BYTES];
    if (!readImuRaw(buf)) {
      continue;
    }
    bool anyNonZero = false;
    for (uint8_t i = 0; i < IMU_BURST_BYTES; ++i) {
      if (buf[i] != 0x00) {
        anyNonZero = true;
        break;
      }
    }
    if (anyNonZero) {
      ++nonZeroCount;
    }
    delay(10);
  }
  return nonZeroCount > 0;
}

/**
 * Fetch one undecoded 14-byte measurement burst from the MPU6050.
 *
 * The burst starts at the accelerometer's high byte, so accelerometer, temperature
 * and gyroscope all come from a single address phase and therefore describe the same
 * instant.
 *
 * @param[out] buf Receives IMU_BURST_BYTES in sensor register order (high byte first).
 * @return true only when every requested byte arrived; false leaves @p buf untouched.
 */
static bool readImuRaw(uint8_t *buf) {
  Wire.beginTransmission(ADDR_MPU6050);
  Wire.write(REG_ACCEL_XOUT_H);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(ADDR_MPU6050, static_cast<uint8_t>(IMU_BURST_BYTES)) != IMU_BURST_BYTES) {
    return false;
  }
  for (uint8_t i = 0; i < IMU_BURST_BYTES; ++i) {
    buf[i] = static_cast<uint8_t>(Wire.read());
  }
  return true;
}

/**
 * Decode a measurement burst into engineering units, before bias correction.
 *
 * @param[in] buf IMU_BURST_BYTES as returned by readImuRaw().
 * @param[out] accelG Receives accelerometer axes in g.
 * @param[out] gyroDps Receives gyroscope axes in deg/s.
 * @return Die temperature in degrees Celsius.
 */
static float decodeBurst(const uint8_t *buf, float *accelG, float *gyroDps) {
  for (uint8_t axis = 0; axis < 3; ++axis) {
    const uint16_t raw = (static_cast<uint16_t>(buf[axis * 2]) << 8) | buf[axis * 2 + 1];
    accelG[axis] = s16FromU16(raw) / LSB_PER_G;
  }

  const uint16_t rawTemp = (static_cast<uint16_t>(buf[6]) << 8) | buf[7];
  const float tempC = (s16FromU16(rawTemp) / 340.0f) + 36.49f;

  for (uint8_t axis = 0; axis < 3; ++axis) {
    const uint8_t i = static_cast<uint8_t>(8 + axis * 2);
    const uint16_t raw = (static_cast<uint16_t>(buf[i]) << 8) | buf[i + 1];
    gyroDps[axis] = s16FromU16(raw) / LSB_PER_DPS;
  }
  return tempC;
}

/**
 * Average accelerometer and gyroscope readings to measure the sensor's zero offsets.
 *
 * Must run with the board motionless and held flat, Z axis up. The accelerometer
 * offset matters most: a part reading 1.38 g where it should read 1.00 g would
 * otherwise bias every angle derived from gravity. The gyroscope offset is what stops
 * yaw drifting at several degrees per second while the board sits still.
 *
 * Gravity is deliberately NOT folded into the accelerometer offsets -- see below.
 *
 * @param[out] accelBiasG Receives the mean accelerometer offset in g, per axis.
 * @param[out] gyroBiasDps Receives the mean gyroscope offset in deg/s, per axis.
 * @return false if any burst failed during the averaging window, so the caller can
 *         report an unsettled measurement instead of a confidently wrong one.
 */
static bool measureBias(float *accelBiasG, float *gyroBiasDps) {
  float sumAccel[3] = {0.0f, 0.0f, 0.0f};
  float sumGyro[3] = {0.0f, 0.0f, 0.0f};
  uint32_t samples = 0;

  const uint32_t startMs = millis();
  while (millis() - startMs < CALIBRATION_MS) {
    uint8_t buf[IMU_BURST_BYTES];
    if (!readImuRaw(buf)) {
      continue;
    }
    float accel[3];
    float gyro[3];
    decodeBurst(buf, accel, gyro);
    for (uint8_t axis = 0; axis < 3; ++axis) {
      sumAccel[axis] += accel[axis];
      sumGyro[axis] += gyro[axis];
    }
    ++samples;
    delay(2);
  }

  if (samples < 50) {
    return false;
  }

  for (uint8_t axis = 0; axis < 3; ++axis) {
    gyroBiasDps[axis] = sumGyro[axis] / static_cast<float>(samples);
  }

  // The gyroscope reads zero when still, so its average is the bias outright. The
  // accelerometer does not: at rest it reads 1 g of gravity along the up axis, and
  // averaging that in and subtracting it would delete gravity itself -- leaving no
  // reference for the filter and angles that jump at random. Only the *excess* over
  // gravity is bias, so the Z average has one g removed from it. This is why the board
  // must be held flat, Z up, for the duration.
  accelBiasG[0] = sumAccel[0] / static_cast<float>(samples);
  accelBiasG[1] = sumAccel[1] / static_cast<float>(samples);
  accelBiasG[2] = (sumAccel[2] / static_cast<float>(samples)) - 1.0f;
  Serial.printf("calibrated over %lu samples: accel bias %.3f %.3f %.3f g, "
                "gyro bias %.2f %.2f %.2f deg/s\n",
                static_cast<unsigned long>(samples), accelBiasG[0], accelBiasG[1],
                accelBiasG[2], gyroBiasDps[0], gyroBiasDps[1], gyroBiasDps[2]);
  return true;
}

/**
 * Derive roll and pitch from gravity alone, using the accelerometer as a spirit level.
 *
 * Only valid while the board is not accelerating; during motion the accelerometer
 * measures specific force, not gravity, and the complementary filter below is what
 * keeps that transient from corrupting the angles.
 *
 * @param[in] accelG Bias-corrected accelerometer axes in g.
 * @param[out] rollDeg Receives the gravity-derived roll in degrees.
 * @param[out] pitchDeg Receives the gravity-derived pitch in degrees.
 */
static void anglesFromGravity(const float *accelG, float *rollDeg, float *pitchDeg) {
  *rollDeg = atan2f(accelG[1], accelG[2]) * DEG_PER_RAD;
  const float planar = sqrtf(accelG[1] * accelG[1] + accelG[2] * accelG[2]);
  *pitchDeg = atan2f(-accelG[0], planar) * DEG_PER_RAD;
}

/**
 * Advance the orientation estimate by one sample.
 *
 * Gyro integration is blended with the gravity reference over FILTER_TIME_CONSTANT_S.
 * Yaw integrates alone, because no absolute heading reference exists on this sensor
 * set, so it accumulates error over time and is reported as such.
 *
 * @param[io,out] state Filter state; angles are updated in place.
 * @param dtSeconds Elapsed time since the previous call, in seconds.
 */
static void updateOrientation(ImuState *state, float dtSeconds) {
  const float alpha = FILTER_TIME_CONSTANT_S / (FILTER_TIME_CONSTANT_S + dtSeconds);

  const float rollFromGyro = state->roll_deg + state->gyro_dps[0] * dtSeconds;
  const float pitchFromGyro = state->pitch_deg + state->gyro_dps[1] * dtSeconds;

  float rollFromGravity = 0.0f;
  float pitchFromGravity = 0.0f;
  anglesFromGravity(state->accel_g, &rollFromGravity, &pitchFromGravity);

  state->roll_deg = alpha * rollFromGyro + (1.0f - alpha) * rollFromGravity;
  state->pitch_deg = alpha * pitchFromGyro + (1.0f - alpha) * pitchFromGravity;
  state->yaw_deg += state->gyro_dps[2] * dtSeconds;
}

/**
 * Print the periodic sensor diagnostic: acknowledgement, identity byte, raw bytes.
 *
 * The raw line is what settles whether the part is genuinely reporting. A device that
 * acknowledges but returns all zeros is not a healthy sensor: the accelerometer must
 * show roughly 16384 counts on whichever axis gravity points along, and the
 * temperature register is never exactly zero.
 */
static void reportSensorState() {
  uint8_t who = 0;
  const bool identityReadable = readRegister8(ADDR_MPU6050, REG_WHO_AM_I, who);

  Serial.printf("probe t=%lu MPU6050(0x%02X)=%s WHO_AM_I=%s\n",
                static_cast<unsigned long>(millis()), ADDR_MPU6050,
                identityReadable ? "ACK" : "NAK", identityReadable ? "readable" : "unreadable");
  if (identityReadable) {
    Serial.printf("      WHO_AM_I=0x%02X\n", who);
  }

  uint8_t buf[IMU_BURST_BYTES];
  if (readImuRaw(buf)) {
    bool live = false;
    for (uint8_t i = 0; i < IMU_BURST_BYTES; ++i) {
      if (buf[i] != 0x00) {
        live = true;
        break;
      }
    }
    Serial.printf("      data=%s\n", live ? "live" : "ALL ZERO - sensor is inert");
    Serial.print("      raw:");
    for (uint8_t i = 0; i < IMU_BURST_BYTES; ++i) {
      Serial.printf(" %02X", buf[i]);
    }
    Serial.println();
  } else {
    Serial.println("      raw: <burst read failed>");
  }
}

/**
 * Scan and print every network this module can currently see.
 *
 * Printed after a failed join, because the single most likely cause is that the
 * configured SSID names a 5 GHz band: the ESP32-WROOM-32 radio is 2.4 GHz only, so
 * such a network does not appear in a scan and does not fail for any reason a status
 * code alone would explain.
 */
static void scanAndPrintNetworks() {
  // The radio will not start a scan while it is still in a failed-join state, so
  // cycle it through off and back before scanning. Disconnect without erasing the
  // stored configuration, so nothing else has to be re-entered.
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
 * Returns without raising when no credentials file is present, because the sketch is
 * still useful over USB serial alone and a missing secret should not stop the bring-up
 * test from running.
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
    // Scan on ANY failure. The status code after a failed join is usually the
    // generic WL_DISCONNECTED rather than anything specific, so branching on it tells
    // you nothing; the scan is what actually identifies the cause.
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

  // Directed broadcast for this subnet, which more access points pass through than
  // 255.255.255.255. Computed from the address the DHCP server actually handed out,
  // so it needs no configuration here.
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
 * Broadcast one already-formatted CSV line to the compute unit.
 *
 * Fire and forget: a datagram that does not arrive is not retried, because on this
 * link a late sample is worse than a missing one. The sample is printed locally either
 * way, so a broken link costs the compute unit a line, never the local record.
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

  const IPAddress destination = linkDestination();

  if (udp.beginPacket(destination, LINK_PORT)) {
    udp.write(reinterpret_cast<const uint8_t *>(line), strlen(line));
    udp.endPacket();
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

/**
 * Print the CSV header naming every column and its unit.
 *
 * Column meanings: t_ms milliseconds since boot, roll/pitch/yaw orientation in
 * degrees, ax_g/ay_g/az_g accelerometer in g, gx/gy/gz gyroscope in deg/s, temp_c die
 * temperature in degC.
 */
static void emitCsvHeader() {
  Serial.println("t_ms,roll_d,pitch_d,yaw_d,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,temp_c");
}

/**
 * Format one sample as a CSV row into a caller-supplied buffer.
 *
 * The same formatted row is used for both sinks -- the USB serial port and the link to
 * the compute unit -- so the two can never disagree about what was measured. Kept
 * separate from printing because the link needs the text and not a write to Serial.
 *
 * @param state Orientation and motion state; a stale sample is flagged in the row.
 * @param[out] buf Destination buffer; always NUL-terminated.
 * @param capacity Size of @p buf in bytes, including room for the terminator.
 * @return Number of characters written, excluding the terminator.
 */
static size_t formatCsvRow(const ImuState &state, char *buf, size_t capacity) {
  if (!state.fresh) {
    return snprintf(buf, capacity, "%lu,stale,stale,stale,nan,nan,nan,nan,nan,nan,nan",
                    static_cast<unsigned long>(millis()));
  }
  return snprintf(buf, capacity, "%lu,%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%.2f",
                  static_cast<unsigned long>(millis()), state.roll_deg, state.pitch_deg,
                  state.yaw_deg, state.accel_g[0], state.accel_g[1], state.accel_g[2],
                  state.gyro_dps[0], state.gyro_dps[1], state.gyro_dps[2], state.temp_c);
}

/**
 * Configure the bus and the IMU, then start the CSV stream.
 *
 * @note Called once by the Arduino core before loop(). If the IMU does not answer,
 *       the sketch stays in a repeating error report rather than streaming invented
 *       zeros, because a column of zeros looks like a working sensor.
 */
void setup() {
  Serial.begin(115200);

  Serial.println();
  Serial.println("Auris pointer - IMU test (ToF not connected)");
#ifdef ESP_ARDUINO_VERSION_STR
  Serial.printf("arduino-esp32 core %s\n", ESP_ARDUINO_VERSION_STR);
#endif

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(I2C_CLOCK_HZ);

  while (!imuInit()) {
    Serial.println("halting - fix the wiring, then reset the board");
    delay(1000);
  }

  // A correct WHO_AM_I does not prove the sensor is reporting -- that register answers
  // even from a sleeping part. Prove the data path before streaming anything.
  uint16_t liveCount = 0;
  if (!dataRegistersAreLive(20, liveCount)) {
    Serial.println("WARNING: sensor identifies at 0x68 but every data byte reads zero.");
    Serial.println("         The part is acknowledged but inert - check power at VCC,");
    Serial.println("         and that PWR_MGMT_1 was accepted.");
  } else {
    Serial.printf("data path live (%u of 20 bursts contained data)\n", liveCount);
  }

  Serial.println();
  Serial.println();
  Serial.println("calibrating - hold the pointer FLAT and STILL, Z axis up, for 2 s");
  Serial.printf("  (calibration needs the board level so gravity is not read as bias)\n");
  Serial.flush();
  delay(200);
  while (!measureBias(g_accelBiasG, g_gyroBiasDps)) {
    Serial.println("calibration failed - too few samples, retrying");
    delay(500);
  }

  // Seed the filter from gravity so the first printed angle is already meaningful.
  uint8_t seedBuf[IMU_BURST_BYTES];
  ImuState state{};
  if (readImuRaw(seedBuf)) {
    float accel[3];
    float gyro[3];
    state.temp_c = decodeBurst(seedBuf, accel, gyro);
    for (uint8_t axis = 0; axis < 3; ++axis) {
      state.accel_g[axis] = accel[axis] - g_accelBiasG[axis];
      state.gyro_dps[axis] = gyro[axis] - g_gyroBiasDps[axis];
    }
    anglesFromGravity(state.accel_g, &state.roll_deg, &state.pitch_deg);
  }

  linkUp = wifiConnect();

  Serial.println();
  Serial.println("yaw is gyro-integrated with no absolute reference; it will drift.");
  Serial.println("hold the button to stream samples to the compute unit over wifi");
  emitCsvHeader();
  Serial.flush();

  // Hand the seeded state to loop() and start the integration clock.
  setLine = state;
  lastUpdateMs = millis();
}

/**
 * Read the IMU, advance the filter, and emit the resulting CSV row.
 *
 * A failed transfer holds the previous orientation rather than advancing it, so a
 * dropped sample cannot inject a fictitious rotation into the integral. The loop is
 * paced to LOOP_PERIOD_MS so the serial link is never overrun.
 *
 * The row is formatted once and then used for both sinks. The USB serial port receives
 * it unconditionally, because it is the local record; the compute unit receives it only
 * while the button is held, which is what the button now means. Edge transitions are
 * reported on the serial port so the operator can see the gate opening and closing
 * without having to watch the cu's shell to know it is running.
 */
void loop() {
  const uint32_t nowMs = millis();
  const float dtSeconds = (nowMs - lastUpdateMs) / 1000.0f;
  lastUpdateMs = nowMs;

  uint8_t buf[IMU_BURST_BYTES];
  if (readImuRaw(buf)) {
    float accel[3];
    float gyro[3];
    setLine.temp_c = decodeBurst(buf, accel, gyro);
    for (uint8_t axis = 0; axis < 3; ++axis) {
      setLine.accel_g[axis] = accel[axis] - g_accelBiasG[axis];
      setLine.gyro_dps[axis] = gyro[axis] - g_gyroBiasDps[axis];
    }
    updateOrientation(&setLine, dtSeconds);
    setLine.fresh = true;
  } else {
    setLine.fresh = false;
  }

  char row[128];
  const size_t len = formatCsvRow(setLine, row, sizeof(row));
  Serial.write(reinterpret_cast<const uint8_t *>(row), len);
  Serial.println();

  const bool held = buttonPressed();
  if (held != wasButtonHeld) {
    Serial.println();
    if (held) {
      Serial.println("button DOWN - streaming to the compute unit");
    } else {
      Serial.println("button UP - stopped streaming to the compute unit");
    }
    wasButtonHeld = held;
  }
  if (held) {
    linkSend(row);
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