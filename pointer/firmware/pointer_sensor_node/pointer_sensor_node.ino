/*
 * Auris pointer sensor node — raw IMU and ToF readout over USB serial.
 *
 * Purpose: emit one CSV line per sample carrying the pointer's MPU6050 (accelerometer,
 *          gyroscope, die temperature), its VL53L0X range, and the laser button state.
 *          This is the data-source milestone only — no fusion, no Wi-Fi, no audio.
 *
 * Inputs:  MPU6050 on I2C address 0x68 (AD0 tied to GND), VL53L0X on address 0x29,
 *          momentary button pulling GPIO 18 to GND. Pin map per pointer/README.md.
 *
 * Outputs: CSV on the USB serial port at 115200 baud; see emitCsvHeader() for the schema.
 *
 * Build:   arduino-cli compile --profile pointer-sensor-node
 * Flash:   arduino-cli upload  --profile pointer-sensor-node
 */

#include <Wire.h>
#include <VL53L0X.h>

// ---------------------------------------------------------------------------
// Pin map — mirrors the wiring table in pointer/README.md.
// ---------------------------------------------------------------------------

/** ESP32 I2C SDA line, shared by both sensors. */
static constexpr uint8_t PIN_I2C_SDA = 21;
/** ESP32 I2C SCL line, shared by both sensors. */
static constexpr uint8_t PIN_I2C_SCL = 22;
/** Laser button input; configured INPUT_PULLUP so a press reads LOW. */
static constexpr uint8_t PIN_BUTTON = 18;

/** I2C address of the MPU6050 with AD0 tied low. */
static constexpr uint8_t ADDR_MPU6050 = 0x68;
/** Default I2C address of the VL53L0X. */
static constexpr uint8_t ADDR_VL53L0X = 0x29;

/** Loop period in milliseconds, giving roughly 50 output lines per second. */
static constexpr uint32_t LOOP_PERIOD_MS = 20;
/** I2C bus speed in Hz; both parts are rated well above this. */
static constexpr uint32_t I2C_CLOCK_HZ = 400000;

/**
 * Whether to re-probe the two sensor addresses periodically.
 *
 * The boot-time scan runs once, so a sensor that is seated or reseated after power-up
 * never shows up again in the log. A periodic probe of just the two known addresses
 * keeps that state visible without the cost of rescanning all 127, and costs one
 * address phase per sensor because an absent device NAKs immediately rather than
 * timing out. Set false for a clean CSV-only stream.
 */
static constexpr bool DIAG_PERIODIC_PROBE = true;

/** Interval between repeated sensor presence probes, in milliseconds. */
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

// ---------------------------------------------------------------------------
// VL53L0X constants.
// ---------------------------------------------------------------------------

/** Sensor return code meaning the echo was too weak to measure. */
static constexpr uint16_t TOF_SIGNAL_FAIL = 8190;
/** Sensor return code meaning no target was in range. */
static constexpr uint16_t TOF_OUT_OF_RANGE = 8191;
/** Sensor return code the driver substitutes when its I2C transaction times out. */
static constexpr uint16_t TOF_TIMEOUT = 65535;

/**
 * Driver I2C transaction timeout in milliseconds.
 *
 * The driver defaults this to zero, which disables the timeout entirely and leaves
 * readRangeContinuousMillimeters() spinning forever on a sensor that stops
 * reporting. It must be set to something non-zero.
 */
static constexpr uint16_t TOF_IO_TIMEOUT_MS = 200;
/** Per-measurement ranging budget in microseconds; 33 ms is the accurate mode. */
static constexpr uint32_t TOF_TIMING_BUDGET_US = 33000;
/** Inter-measurement period in milliseconds, giving about 20 fresh readings/s. */
static constexpr uint32_t TOF_PERIOD_MS = 50;

/**
 * Whether init() switches the sensor's I/O pads into 2.8 V mode.
 *
 * Most VL53L0X breakouts feed VREF from an on-board 2.8 V regulator and are meant to
 * be initialised this way, which is also the driver's own default. It only has to be
 * false for a module that ties VREF straight to VCC and so cannot supply the higher
 * pad voltage.
 */
static constexpr bool TOF_IO_2V8 = true;

/** Why a ToF reading is or is not usable, as written into the tof_status column. */
enum class ToFStatus : uint8_t {
  Valid = 0,        ///< dist_mm holds a real measurement.
  SignalFail = 1,   ///< Sensor returned 8190; target too dark or too far.
  OutOfRange = 2,   ///< Sensor returned 8191; no target within its range.
  Timeout = 3,      ///< No fresh measurement arrived within TOF_IO_TIMEOUT_MS.
  Unavailable = 4,  ///< Sensor did not come up at boot, so no reading was attempted.
};

/** One decoded MPU6050 sample, in engineering units, in the sensor's own body frame. */
struct ImuSample {
  float accel_g[3];  ///< X, Y, Z specific force in g, gravity included.
  float gyro_dps[3]; ///< X, Y, Z angular rate in deg/s.
  float temp_c;      ///< Die temperature in degrees Celsius.
};

/** Pololu driver instance owning the VL53L0X; its state lives across the loop. */
static VL53L0X tof;
/** Whether the VL53L0X came up at boot; gates every range read in loop(). */
static bool tofReady = false;
/** Timestamp of the last periodic sensor presence probe; drives the diagnostic line. */
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
 * Scan the I2C bus and print every address that acknowledges.
 *
 * Runs once at boot so an absent sensor is named rather than guessed at: a VL53L0X
 * held in shutdown by a low XSHUT line never drives an ACK, and otherwise presents
 * only as an endless run of plausible-looking zeros.
 *
 * @return Number of devices found; 0 means the bus itself is suspect.
 */
static uint8_t probeI2cBus() {
  uint8_t found = 0;
  for (uint8_t addr = 1; addr < 0x7F; ++addr) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
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
 *         was acknowledged. False means the ranges in the decoded output are not
 *         trustworthy, so the caller should not present the readings as calibrated.
 */
static bool imuInit() {
  uint8_t who = 0;
  if (!readRegister8(ADDR_MPU6050, REG_WHO_AM_I, who)) {
    Serial.printf("MPU6050: no ACK at 0x%02X — check SDA/SCL, GND and the 3V3 rail\n", ADDR_MPU6050);
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
 * Read and decode one IMU sample in a single 14-byte burst.
 *
 * The burst starts at the accelerometer's high byte, so accelerometer, temperature
 * and gyroscope all come from one address phase and land in the same instant.
 *
 * @param[out] out Decoded sample in the sensor body frame, in g, deg/s and degC.
 * @return true on a complete transfer; false leaves @p out untouched, and the
 *         caller prints nan rather than a stale or invented value.
 */
static bool readImuSample(ImuSample &out) {
  uint8_t buf[IMU_BURST_BYTES];

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

  return true;
}

/**
 * Report whether a single I2C address acknowledges.
 *
 * An address phase against an absent device is NAKed immediately, so this costs about
 * as much for a missing sensor as for a present one and never blocks the loop.
 *
 * @param addr 7-bit address to probe.
 * @return true when something acknowledged.
 */
static bool addressResponds(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

/**
 * Print a compact presence report for the two sensor addresses.
 *
 * @param label Short prefix identifying when the report was taken.
 */
static void reportSensorPresence(const char *label) {
  Serial.printf("%s MPU6050(0x%02X)=%s VL53L0X(0x%02X)=%s\n", label, ADDR_MPU6050,
                addressResponds(ADDR_MPU6050) ? "ACK" : "NAK", ADDR_VL53L0X,
                addressResponds(ADDR_VL53L0X) ? "ACK" : "NAK");
}

/**
 * Bring up the VL53L0X and leave it ranging continuously at a fixed period.
 *
 * The address is checked on the bus before the driver is initialised, because a part
 * held in shutdown by a low XSHUT line answers nothing and the driver's own
 * diagnostics stay silent about it.
 *
 * @return true when the sensor acknowledged, initialised, and continuous ranging was
 *         started; false leaves @c tofReady false and every range column unavailable.
 * @note A refused timing budget is reported but not fatal — the driver's own default
 *       still applies, so the sensor keeps streaming at whatever rate it chose.
 */
static bool tofInit() {
  Wire.beginTransmission(ADDR_VL53L0X);
  if (Wire.endTransmission() != 0) {
    Serial.printf("VL53L0X: no ACK at 0x%02X — check power, and whether this "
                  "breakout needs XSHUT held high\n",
                  ADDR_VL53L0X);
    return false;
  }

  tof.setAddress(ADDR_VL53L0X);
  tof.setTimeout(TOF_IO_TIMEOUT_MS);
  if (!tof.init(TOF_IO_2V8)) {
    Serial.printf("VL53L0X: answered at 0x%02X but failed to initialise — check the "
                  "I/O voltage mode for this breakout\n",
                  ADDR_VL53L0X);
    return false;
  }
  if (!tof.setMeasurementTimingBudget(TOF_TIMING_BUDGET_US)) {
    Serial.printf("VL53L0X: timing budget of %lu us refused; using the driver default\n",
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
 * dist_mm range in millimetres, tof_status a ToFStatus value, btn 1 when held.
 */
static void emitCsvHeader() {
  Serial.println("t_ms,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,temp_c,dist_mm,tof_status,btn");
}

/**
 * Print one sample as a single CSV line.
 *
 * A failed IMU read prints nan in every IMU column, so a dropped transfer is visible
 * as missing data rather than as a plausible-looking zero.
 *
 * @param tMs Milliseconds since boot, stamped at the top of the loop.
 * @param imu Decoded IMU sample, or nullptr to print nan columns.
 * @param rangeMm Range in millimetres; 0 whenever @p status is not Valid.
 * @param status Why the range is or is not usable.
 * @param pressed Whether the laser button is held.
 */
static void emitSample(uint32_t tMs, const ImuSample *imu, uint16_t rangeMm, ToFStatus status,
                       bool pressed) {
  const unsigned button = pressed ? 1u : 0u;
  const unsigned rawStatus = static_cast<unsigned>(status);

  if (imu != nullptr) {
    Serial.printf("%lu,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%.2f,%u,%u,%u\n",
                  static_cast<unsigned long>(tMs), imu->accel_g[0], imu->accel_g[1],
                  imu->accel_g[2], imu->gyro_dps[0], imu->gyro_dps[1], imu->gyro_dps[2],
                  imu->temp_c, static_cast<unsigned>(rangeMm), rawStatus, button);
  } else {
    Serial.printf("%lu,nan,nan,nan,nan,nan,nan,nan,%u,%u,%u\n", static_cast<unsigned long>(tMs),
                  static_cast<unsigned>(rangeMm), rawStatus, button);
  }
}

/**
 * Configure the bus and both sensors, then start the CSV stream.
 *
 * @note Called once by the Arduino core before loop(); runs to completion even when
 *       a sensor is missing, so the failure is reported and streaming still begins.
 */
void setup() {
  Serial.begin(115200);

  Serial.println();
  Serial.println("Auris pointer sensor node — ESP32 ToF + IMU");
#ifdef ESP_ARDUINO_VERSION_STR
  Serial.printf("arduino-esp32 core %s\n", ESP_ARDUINO_VERSION_STR);
#endif
  Serial.println("i2c scan:");

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(I2C_CLOCK_HZ);

  const uint8_t found = probeI2cBus();
  if (found == 0) {
    Serial.println("  nothing acknowledged — the bus is not wired or not powered");
  }

  const bool imuReady = imuInit();
  tofReady = tofInit();
  reportSensorPresence("probe");

  Serial.printf("imu %s, tof %s\n", imuReady ? "ready" : "FAILED", tofReady ? "ready" : "FAILED");
  Serial.println();
  emitCsvHeader();
  Serial.flush();
}

/**
 * Read both sensors once and emit the resulting CSV line.
 *
 * The loop is paced to LOOP_PERIOD_MS rather than run free, so the line rate stays
 * steady whether or not a read blocks. The ToF refreshes on its own TOF_PERIOD_MS
 * cadence, which is slower than the loop, so consecutive lines legitimately repeat
 * a range; a driver timeout skips a cycle rather than corrupting a line.
 *
 * While DIAG_PERIODIC_PROBE is set, a diagnostic line naming each sensor's
 * acknowledgement and the IMU's identity byte is interleaved every PROBE_PERIOD_MS,
 * so a missing or miswired sensor is visible without needing to catch the boot log.
 */
void loop() {
  const uint32_t startMs = millis();

  ImuSample imu{};
  const bool imuOk = readImuSample(imu);

  uint16_t rangeMm = 0;
  ToFStatus status = ToFStatus::Unavailable;
  if (tofReady) {
    const uint16_t rawMm = tof.readRangeContinuousMillimeters();
    const bool timedOut = tof.timeoutOccurred();  // Reads and clears the driver flag.
    status = classifyRange(rawMm, timedOut);
    rangeMm = (status == ToFStatus::Valid) ? rawMm : 0;
  }

  emitSample(startMs, imuOk ? &imu : nullptr, rangeMm, status, buttonPressed());

  if (DIAG_PERIODIC_PROBE && (startMs - lastProbeMs) >= PROBE_PERIOD_MS) {
    lastProbeMs = startMs;
    uint8_t who = 0;
    const bool readable = readRegister8(ADDR_MPU6050, REG_WHO_AM_I, who);
    Serial.printf("probe t=%lu imu_ack=%s who_am_i=%s imu_sample=%s\n",
                  static_cast<unsigned long>(startMs), addressResponds(ADDR_MPU6050) ? "yes" : "no",
                  readable ? "readable" : "unreadable", imuOk ? "ok" : "FAILED");
    if (readable) {
      Serial.printf("      MPU6050 WHO_AM_I=0x%02X\n", who);
    }
    reportSensorPresence("      ");
  }

  const uint32_t elapsedMs = millis() - startMs;
  if (elapsedMs < LOOP_PERIOD_MS) {
    delay(LOOP_PERIOD_MS - elapsedMs);
  }
}