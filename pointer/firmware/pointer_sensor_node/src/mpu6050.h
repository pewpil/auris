#pragma once

#include <Arduino.h>
#include <Wire.h>

#include <cstdint>

/**
 * One decoded MPU6050 burst.
 *
 * Raw counts are kept alongside the converted values so the caller can detect
 * the all-zero register dump a sleeping or reset MPU6050 returns, which would
 * otherwise look like a perfectly plausible stationary reading.
 */
struct ImuReading {
  int16_t raw[7];  ///< ax, ay, az, temp, gx, gy, gz in register order.
  float axG;       ///< Acceleration along X in g.
  float ayG;       ///< Acceleration along Y in g.
  float azG;       ///< Acceleration along Z in g.
  float gxDps;     ///< Angular rate about X in deg/s.
  float gyDps;     ///< Angular rate about Y in deg/s.
  float gzDps;     ///< Angular rate about Z in deg/s.
  float tempC;     ///< Die temperature in degC.

  /**
   * @return true when every raw field is zero, the signature of an MPU6050 that
   *     has reset into sleep rather than one that is genuinely still.
   */
  bool allRawZero() const;
};

/**
 * Minimal register-level driver for the MPU6050.
 *
 * Responsibility: bring the part out of sleep, apply the four configuration
 * registers the parent contract names, and read the 14-byte measurement burst.
 * It performs no filtering, calibration, or orientation maths; those live in
 * MahonyFilter and the sketch. The driver is bound to the Arduino `Wire`
 * instance and the address in config.h. Every read failure is counted; callers
 * may rely on `read()` never emitting a fabricated sample.
 */
class Mpu6050 {
 public:
  /**
   * Probe WHO_AM_I and record the byte.
   *
   * The caller must have called `Wire.begin()` first. An ACK is enough to
   * proceed: known clone/quiet revisions answer values other than 0x68, and the
   * observed byte is kept for the record and the boot log.
   *
   * @return true when the register address was acknowledged.
   */
  bool begin();

  /**
   * Wake the part and write its four configuration registers, then verify them.
   *
   * PWR_MGMT_1 selects the gyro PLL clock and clears sleep; CONFIG sets the
   * DLPF; GYRO_CONFIG and ACCEL_CONFIG set ±500 deg/s and ±4 g. After writing,
   * every register is read back and compared, and one burst is checked for the
   * all-zero signature so a still-sleeping part is reported as a failure rather
   * than as data.
   *
   * @return true when all writes, readbacks, and the probe burst are good.
   */
  bool configure();

  /**
   * Read and decode one 14-byte burst starting at ACCEL_XOUT_H (0x3B).
   *
   * @param[out] out Receives the decoded sample; untouched on failure.
   * @return true when all 14 bytes arrived; false on a NAK or a short transfer,
   *     in which case the driver's error counter is incremented.
   */
  bool read(ImuReading &out);

  /** @return The WHO_AM_I byte observed at the last `begin()`, 0 if unread. */
  uint8_t whoAmI() const { return whoAmI_; }

  /** @return Cumulative failed bursts since boot. */
  uint32_t readErrors() const { return readErrors_; }

  /** @return Cumulative failed configuration attempts since boot. */
  uint32_t configErrors() const { return configErrors_; }

 private:
  /**
   * Write one register.
   *
   * @param reg Register address.
   * @param value Byte to write.
   * @return true when the transfer was acknowledged.
   */
  bool writeRegister(uint8_t reg, uint8_t value);

  /**
   * Read one register.
   *
   * @param reg Register address.
   * @param[out] value Receives the byte.
   * @return true when the transfer completed.
   */
  bool readRegister(uint8_t reg, uint8_t &value);

  uint8_t whoAmI_ = 0;
  uint32_t readErrors_ = 0;
  uint32_t configErrors_ = 0;
};
