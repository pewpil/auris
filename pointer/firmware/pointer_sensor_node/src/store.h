#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include <cstdint>

#include "fusion.h"

/**
 * Per-device calibration in the sensor's own units.
 *
 * `accelOffsetG` is subtracted from every accelerometer reading, and
 * `gyroBiasDps` from every gyroscope reading, before fusion. The offsets are
 * measured once by `@CAL` with the pointer flat, Z up and still; this GY-521's
 * raw accelerometer has a zero-g offset far outside the MPU6050 datasheet, so a
 * valid calibration is what makes the fused orientation usable.
 */
struct Calibration {
  bool valid = false;                 ///< True once a CAL result has been stored.
  Vec3 accelOffsetG{0.0f, 0.0f, 0.0f};  ///< Per-axis accelerometer offset in g.
  Vec3 gyroBiasDps{0.0f, 0.0f, 0.0f};   ///< Per-axis gyro bias in deg/s.
};

/**
 * NVS-backed settings for the pointer.
 *
 * Responsibility: load and save the calibration, the boot counter, and the
 * optional unicast destination override. The store owns the Preferences handle
 * for the lifetime of the sketch; `begin()` must be called once before any
 * other method. Corrupt or absent values fall back to safe defaults rather than
 * throwing.
 */
class Store {
 public:
  /**
   * Open the NVS namespace and load the stored calibration.
   *
   * @return true when the namespace opened; false leaves the store with
   *     defaults and is reported at boot rather than treated as fatal.
   */
  bool begin();

  /** @return The calibration loaded from NVS, valid or not. */
  const Calibration &calibration() const { return cal_; }

  /**
   * Persist a calibration and make it current.
   *
   * @param calibration Offsets to store.
   * @return true when every write succeeded.
   */
  bool saveCalibration(const Calibration &calibration);

  /** Remove the stored calibration and mark the current one invalid. */
  void clearCalibration();

  /**
   * Increment and persist the boot counter.
   *
   * @return The new boot id, starting at 1 on a factory-fresh namespace.
   */
  uint32_t nextBootId();

  /** @return The stored unicast destination, or an empty String for broadcast. */
  String unicastDestination();

  /**
   * Persist or clear the unicast destination override.
   *
   * @param destination Dotted-quad address, or empty to use broadcast.
   * @return true when the write succeeded.
   */
  bool setUnicastDestination(const String &destination);

  /** Erase every key in the namespace, including calibration and boot count. */
  void clearAll();

 private:
  Preferences prefs_{};
  Calibration cal_{};
};
