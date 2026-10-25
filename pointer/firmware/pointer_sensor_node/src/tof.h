#pragma once

#include <Arduino.h>
#include <VL53L1X.h>

#include <cstdint>

/**
 * Non-blocking wrapper around the Pololu VL53L1X driver.
 *
 * Responsibility: own the sensor's XSHUT reset, identity gate, long-mode
 * configuration, continuous ranging, and recovery. The sketch polls `poll()`
 * and never blocks on a measurement; `dataReady()` is checked first and a hard
 * reset is issued after `TOF_STALE_MS` without a result. The wrapper keeps the
 * last measurement and its timestamp so callers can report a stale range with
 * an age rather than hiding it.
 *
 * Invariants: after a successful `begin()`, `healthy()` is true until a
 * measurement has been missing for longer than the stale window. `rangeMm()`
 * is only meaningful together with `hasRange()` and `rangeStatus()`.
 */
class Tof {
 public:
  /**
   * Reset the sensor through XSHUT, confirm its 0xEACC model identity, and
   * start continuous long-mode ranging.
   *
   * Blocks for roughly 110 ms (the XSHUT low time plus settling) and does not
   * call `Wire.begin()`; the sketch owns the bus.
   *
   * @return true when the identity check, driver init, mode, budget and
   *     continuous start all succeeded.
   */
  bool begin();

  /**
   * Poll for a ready measurement, or recover the sensor if it has gone quiet.
   *
   * Call at least every `TOF_POLL_PERIOD_MS`. Recovery re-runs `begin()`,
   * including the XSHUT reset, and increments the reinit counter.
   *
   * @param nowMs Current `millis()` value.
   */
  void poll(uint32_t nowMs);

  /**
   * @param nowMs Current `millis()` value.
   * @return true when a measurement has arrived within the stale window.
   */
  bool healthy(uint32_t nowMs) const;

  /** @return true once at least one measurement has been read. */
  bool hasRange() const { return hasRange_; }

  /** @return The last range in millimetres; meaningless unless `hasRange()`. */
  uint16_t rangeMm() const { return rangeMm_; }

  /** @return The last driver range status, 0-13, or 255 when none yet. */
  uint8_t rangeStatus() const { return status_; }

  /**
   * @param nowMs Current `millis()` value.
   * @return Age of the last measurement in milliseconds, or -1 when none has
   *     arrived since boot.
   */
  int32_t rangeAgeMs(uint32_t nowMs) const;

  /** @return Recovery resets attempted since boot. */
  uint32_t reinits() const { return reinits_; }

  /** @return Recovery resets that failed to re-establish ranging. */
  uint32_t recoverFailures() const { return recoverFailures_; }

  /**
   * Read the VL53L1X identity at 16-bit index 0x010F.
   *
   * The part uses two-byte register indexing; one-byte reads (the VL53L0X
   * protocol) return pointer-driven garbage. The four bytes are stored for the
   * boot log.
   *
   * @param[out] out Receives model id bytes [0..1] then the two trailing bytes.
   * @return true when the transfer completed.
   */
  bool probeIdentity(uint8_t out[4]);

  /** @return The four identity bytes from the last successful probe. */
  const uint8_t *identity() const { return identity_; }

 private:
  VL53L1X sensor_{};
  uint8_t identity_[4] = {0, 0, 0, 0};
  bool initialized_ = false;
  bool hasRange_ = false;
  uint16_t rangeMm_ = 0;
  uint8_t status_ = 255;
  uint32_t lastRangeMs_ = 0;
  uint32_t reinits_ = 0;
  uint32_t recoverFailures_ = 0;
};
