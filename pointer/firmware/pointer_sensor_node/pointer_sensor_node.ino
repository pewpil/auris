/*
 * Auris pointer sensor node — raw IMU readout over USB serial.
 *
 * Purpose: emit one CSV line per sample carrying the pointer's MPU6050 (accelerometer,
 *          gyroscope, die temperature) and the laser button state. This is the
 *          data-source milestone only — no fusion, no Wi-Fi, no audio.
 *
 * Inputs:  MPU6050 on I2C address 0x68 (AD0 tied to GND), and a momentary button
 *          pulling GPIO 18 to GND. Pin map per pointer/README.md.
 *
 * Outputs: CSV on the USB serial port at 115200 baud; see emitCsvHeader() for the schema.
 *
 * Note:    The VL53L0X time-of-flight sensor is not wired on this build, so there is
 *          no range column. See pointer/README.md — §1.5 of AGENTS.md still specifies a
 *          ToF on the pointer, so this is a gap in the prototype, not a design change.
 *
 * Build:   arduino-cli compile --profile pointer-sensor-node
 * Flash:   arduino-cli upload  --profile pointer-sensor-node --port /dev/ttyUSB0
 */

#include <Wire.h>

// ---------------------------------------------------------------------------
// Pin map — mirrors the wiring table in pointer/README.md.
// ---------------------------------------------------------------------------

/** ESP32 I2C SDA line. */
static constexpr uint8_t PIN_I2C_SDA = 21;
/** ESP32 I2C SCL line. */
static constexpr uint8_t PIN_I2C_SCL = 22;
/** Laser button input; configured INPUT_PULLUP so a press reads LOW. */
static constexpr uint8_t PIN_BUTTON = 18;

/** I2C address of the MPU6050 with AD0 tied low. */
static constexpr uint8_t ADDR_MPU6050 = 0x68;

/** Loop period in milliseconds, giving roughly 50 output lines per second. */
static constexpr uint32_t LOOP_PERIOD_MS = 20;
/** I2C bus speed in Hz; the MPU6050 is rated well above this. */
static constexpr uint32_t I2C_CLOCK_HZ = 400000;

/**
 * Whether to interleave a diagnostic line with the CSV stream.
 *
 * The bus scan runs once at boot, so a fault that appears afterwards — a connector
 * working loose, a sensor reseated — would never be reported again. A periodic probe
 * of the one known address costs a single I2C address phase, which an absent device
 * NAKs immediately rather than timing out, so it cannot slow the sample loop. Set
 * false for a stream of nothing but CSV.
 */
static constexpr bool DIAG_PERIODIC_PROBE = true;

/** Interval between repeated sensor probes, in milliseconds. */
static constexpr uint32_t PROBE_PERIOD_MS = 5000;

// ---------------------------------------------------------------------------
// MPU6050 register map (MPU-6000A register map, RM-MPU-6000A-1).
// ---------------------------------------------------------------------------

static constexpr uint8_t REG_SMPLRT_DIV = 0x19;
static constexpr uint8_t REG_CONFIG = 0x1A;
static constexpr uint8_t REG_GYRO_CONFIG = 0x1B;
static constexpr uint8_t REG_ACCEL_CONFIG = 0x1C;
static constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;
static constexpr uint8_t REG_WHO_AM_I = 0x75;

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

/** One decoded MPU6050 sample, in engineering units, in the sensor's own body frame. */
struct ImuSample {
  float accel_g[3];  ///< X, Y, Z specific force in g, gravity included.
  float gyro_dps[3]; ///< X, Y, Z angular rate in deg/s.
  float temp_c;      ///< Die temperature in degrees Celsius.
};

/** Timestamp of the last periodic probe; drives the interleaved diagnostic line. */
static uint32_t lastProbeMs = 0;

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
 * @note No exceptions; failure is reported as the return value.
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
 * Scan the whole I2C bus and print every address that acknowledges.
 *
 * Runs once at boot so a misconfigured part is named outright rather than surfacing
 * later as an unexplained column of zeros.
 *
 * @return Number of devices found; 0 means the bus itself is suspect.
 */
static uint8_t probeI2cBus() {
  uint8_t found = 0;
  for (uint8_t addr = 1; addr < 0x7F; ++addr) {
    if (addressResponds(addr)) {
      Serial.printf("  i2c device at 0x%02X\n", addr);
      ++found;
    }
  }
  return found;
}

/**
 * Identify and configure the MPU6050.
 *
 * Reads WHO_AM_I, then selects the +/-2 g / +/-250 deg/s ranges and a ~44 Hz low-pass
 * filter at a 1 kHz internal sample rate. Any WHO_AM_I in the MPU6050/6500/9250
 * family is accepted, since GY-521 breakouts ship with several parts fitted.
 *
 * @return true when the identity byte was recognised and every configuration write
 *         was acknowledged. False means the scale factors applied when decoding are
 *         not the ones the part was told to use, so readings must not be trusted.
 */
static bool imuInit() {
  uint8_t who = 0;
  if (!readRegister8(ADDR_MPU6050, REG_WHO_AM_I, who)) {
    Serial.printf("MPU6050: no ACK at 0x%02X — check SDA/SCL, GND and the 3V3 rail\n",
                  ADDR_MPU6050);
    return false;
  }

  const bool identified = (who == 0x68 || who == 0x70 || who == 0x71 || who == 0x73);
  Serial.printf("MPU6050: WHO_AM_I = 0x%02X%s\n", who,
                identified ? "" : "  (unrecognised — expecting 0x68, 0x70, 0x71 or 0x73)");

  return identified
         && writeRegister8(ADDR_MPU6050, REG_SMPLRT_DIV, 0x00)
         && writeRegister8(ADDR_MPU6050, REG_CONFIG, DLPF_CONFIG)
         && writeRegister8(ADDR_MPU6050, REG_GYRO_CONFIG, GYRO_FS_250_DPS)
         && writeRegister8(ADDR_MPU6050, REG_ACCEL_CONFIG, ACCEL_FS_2G);
}

/**
 * Fetch one undecoded 14-byte measurement burst from the MPU6050.
 *
 * The burst starts at the accelerometer's high byte, so accelerometer, temperature
 * and gyroscope all come from a single address phase and therefore describe the same
 * instant. Kept separate from decoding so the raw bytes can be printed for diagnosis.
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
 * Decode a measurement burst into engineering units.
 *
 * @param[in] buf IMU_BURST_BYTES as returned by readImuRaw().
 * @param[out] out Sample in the sensor body frame, in g, deg/s and degC.
 */
static void decodeImuSample(const uint8_t *buf, ImuSample &out) {
  for (uint8_t axis = 0; axis < 3; ++axis) {
    const uint16_t raw = (static_cast<uint16_t>(buf[axis * 2]) << 8) | buf[axis * 2 + 1];
    out.accel_g[axis] = s16FromU16(raw) / LSB_PER_G;
  }

  const uint16_t rawTemp = (static_cast<uint16_t>(buf[6]) << 8) | buf[7];
  out.temp_c = (s16FromU16(rawTemp) / 340.0f) + 36.49f;

  for (uint8_t axis = 0; axis < 3; ++axis) {
    const uint8_t i = static_cast<uint8_t>(8 + axis * 2);
    const uint16_t raw = (static_cast<uint16_t>(buf[i]) << 8) | buf[i + 1];
    out.gyro_dps[axis] = s16FromU16(raw) / LSB_PER_DPS;
  }
}

/**
 * Print the raw burst as space-separated hex bytes.
 *
 * This is the line that settles whether the part is genuinely reporting. A device
 * that acknowledges but returns all zeros is not a healthy sensor: the accelerometer
 * must show roughly 16384 counts on whichever axis gravity points along, and the
 * temperature register is never exactly zero. All zeros mean the bus is being read
 * but nothing real is behind it.
 *
 * @param buf IMU_BURST_BYTES as returned by readImuRaw().
 */
static void reportRawBurst(const uint8_t *buf) {
  Serial.print("      raw:");
  for (uint8_t i = 0; i < IMU_BURST_BYTES; ++i) {
    Serial.printf(" %02X", buf[i]);
  }
  Serial.println();
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
 * Column meanings: t_ms milliseconds since boot, ax_g/ay_g/az_g accelerometer in g,
 * gx_dps/gy_dps/gz_dps gyroscope in deg/s, temp_c die temperature in degC,
 * btn 1 while the button is held.
 */
static void emitCsvHeader() {
  Serial.println("t_ms,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,temp_c,btn");
}

/**
 * Print one sample as a single CSV line.
 *
 * A failed IMU read prints nan in every IMU column, so a dropped transfer shows up as
 * missing data rather than as a plausible-looking zero.
 *
 * @param tMs Milliseconds since boot, stamped at the top of the loop.
 * @param imu Decoded sample, or nullptr to print nan columns.
 * @param pressed Whether the laser button is held.
 */
static void emitSample(uint32_t tMs, const ImuSample *imu, bool pressed) {
  const unsigned button = pressed ? 1u : 0u;

  if (imu != nullptr) {
    Serial.printf("%lu,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%.2f,%u\n", static_cast<unsigned long>(tMs),
                  imu->accel_g[0], imu->accel_g[1], imu->accel_g[2], imu->gyro_dps[0],
                  imu->gyro_dps[1], imu->gyro_dps[2], imu->temp_c, button);
  } else {
    Serial.printf("%lu,nan,nan,nan,nan,nan,nan,nan,%u\n", static_cast<unsigned long>(tMs), button);
  }
}

/**
 * Print the periodic sensor diagnostic: acknowledgement, identity byte, raw bytes.
 *
 * @param sampleOk Whether the burst read in the same loop iteration succeeded.
 */
static void reportSensorState(bool sampleOk) {
  uint8_t who = 0;
  const bool identityReadable = readRegister8(ADDR_MPU6050, REG_WHO_AM_I, who);

  Serial.printf("probe t=%lu MPU6050(0x%02X)=%s WHO_AM_I=%s sample=%s\n",
                static_cast<unsigned long>(millis()), ADDR_MPU6050,
                addressResponds(ADDR_MPU6050) ? "ACK" : "NAK",
                identityReadable ? "readable" : "unreadable", sampleOk ? "ok" : "FAILED");
  if (identityReadable) {
    Serial.printf("      WHO_AM_I=0x%02X\n", who);
  }

  uint8_t buf[IMU_BURST_BYTES];
  if (readImuRaw(buf)) {
    reportRawBurst(buf);
  } else {
    Serial.println("      raw: <burst read failed>");
  }
}

/**
 * Configure the bus and the IMU, then start the CSV stream.
 *
 * @note Called once by the Arduino core before loop(). Runs to completion even when
 *       the sensor is missing, so the failure is reported and streaming still begins.
 */
void setup() {
  Serial.begin(115200);

  Serial.println();
  Serial.println("Auris pointer sensor node — ESP32 IMU");
#ifdef ESP_ARDUINO_VERSION_STR
  Serial.printf("arduino-esp32 core %s\n", ESP_ARDUINO_VERSION_STR);
#endif

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(I2C_CLOCK_HZ);

  Serial.println("i2c scan:");
  const uint8_t found = probeI2cBus();
  if (found == 0) {
    Serial.println("  nothing acknowledged — the bus is not wired or not powered");
  }

  const bool imuReady = imuInit();
  Serial.printf("imu %s\n", imuReady ? "ready" : "FAILED");
  Serial.println();
  emitCsvHeader();
  Serial.flush();
}

/**
 * Read the IMU once and emit the resulting CSV line.
 *
 * The loop is paced to LOOP_PERIOD_MS rather than run free, so the line rate stays
 * steady whether or not a read blocks.
 *
 * While DIAG_PERIODIC_PROBE is set, a diagnostic line carrying the sensor's
 * acknowledgement, identity byte, and raw burst is interleaved every PROBE_PERIOD_MS,
 * so a sensor that stops answering is visible without having to catch the boot log.
 */
void loop() {
  const uint32_t startMs = millis();

  uint8_t buf[IMU_BURST_BYTES];
  const bool sampleOk = readImuRaw(buf);

  ImuSample imu{};
  if (sampleOk) {
    decodeImuSample(buf, imu);
  }

  emitSample(startMs, sampleOk ? &imu : nullptr, buttonPressed());

  if (DIAG_PERIODIC_PROBE && (startMs - lastProbeMs) >= PROBE_PERIOD_MS) {
    lastProbeMs = startMs;
    reportSensorState(sampleOk);
  }

  const uint32_t elapsedMs = millis() - startMs;
  if (elapsedMs < LOOP_PERIOD_MS) {
    delay(LOOP_PERIOD_MS - elapsedMs);
  }
}