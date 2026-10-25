#pragma once

#include <cstdint>

/**
 * A three-component vector.
 *
 * The unit is stated at every use site: g for accelerometer readings, deg/s or
 * rad/s for gyroscope readings, and a unit vector where noted.
 */
struct Vec3 {
  float x;
  float y;
  float z;
};

/**
 * A unit quaternion (w, x, y, z) describing the pointer body's orientation
 * relative to the world.
 *
 * The convention is **body-to-world**: applying the quaternion to a vector
 * expressed in the body frame yields that vector in the world frame. Equivalently
 * `quatRotateWorldToBody(q, worldGravity)` returns the accelerometer's expected
 * reading (the reaction to gravity, +1 g along world +Z) in the body frame.
 */
struct Quat {
  float w;
  float x;
  float y;
  float z;
};

/**
 * Intrinsic ZYX Euler angles in degrees: roll about the body X axis, pitch about
 * the body Y axis, yaw about the body Z axis, each relative to the world frame.
 */
struct EulerDeg {
  float roll;
  float pitch;
  float yaw;
};

/**
 * Build a body-to-world quaternion from intrinsic ZYX Euler angles.
 *
 * @param e Roll, pitch and yaw in degrees.
 * @return The unit quaternion representing the same rotation.
 */
Quat quatFromEulerDeg(const EulerDeg &e);

/**
 * Convert a body-to-world quaternion to intrinsic ZYX Euler angles.
 *
 * @param q Unit quaternion in the body-to-world convention.
 * @return Roll, pitch and yaw in degrees; pitch is in [-90, 90].
 */
EulerDeg eulerFromQuatDeg(const Quat &q);

/**
 * Normalise a quaternion, with a safe fallback when the input is degenerate.
 *
 * @param q Any quaternion.
 * @return The unit quaternion in the same direction, or identity when the norm
 *     is too small to normalise.
 */
Quat quatNormalized(const Quat &q);

/**
 * Rotate a world-frame vector into the body frame.
 *
 * @param q Body-to-world quaternion.
 * @param v Vector in world coordinates.
 * @return The same vector expressed in body coordinates.
 */
Vec3 quatRotateWorldToBody(const Quat &q, const Vec3 &v);

/**
 * Compute the accelerometer zero-g offset from a flat, Z-up mean reading.
 *
 * The device must be flat with the sensor's +Z axis pointing up, so a perfect
 * part would average (0, 0, +1) g. The offset is the measured mean minus that
 * reference, in g.
 *
 * @param meanG Mean raw accelerometer reading in g while flat, Z up.
 * @return Per-axis offset in g to subtract from subsequent readings.
 */
Vec3 accelOffsetFromFlatMean(const Vec3 &meanG);

/**
 * Mahony complementary attitude filter using gyroscope and accelerometer only.
 *
 * Responsibility: integrate body angular rate (bias-corrected) and pull roll and
 * pitch toward the accelerometer's gravity direction. Yaw is unobservable
 * without a magnetometer and therefore integrates gyro error; that is a property
 * of the hardware and is recorded in the firmware documentation.
 *
 * State: the quaternion and the initialisation flag. The filter is deterministic,
 * allocation-free, and safe to call from the single Arduino loop task. Callers
 * may rely on `quaternion()` always being unit length after `begin()`.
 */
class MahonyFilter {
 public:
  /**
   * Seed the filter from one accelerometer reading.
   *
   * Roll and pitch come from gravity; yaw starts at zero because nothing
   * observes it. A reading whose magnitude is far from 1 g (free fall or a
   * shock) seeds the identity instead of a confidently wrong tilt.
   *
   * @param accelG Accelerometer reading in g, sensor axes.
   */
  void begin(const Vec3 &accelG);

  /**
   * Advance the filter by one sample.
   *
   * @param gyroRadPerSec Bias-corrected body angular rate in rad/s.
   * @param accelG Accelerometer reading in g, sensor axes; used only when its
   *     magnitude is within a plausible gravity window.
   * @param dtSeconds Time since the previous update in seconds; clamped to
   *     [1 ms, 50 ms] so a scheduling hiccup cannot fling the quaternion.
   */
  void update(const Vec3 &gyroRadPerSec, const Vec3 &accelG, float dtSeconds);

  /** @return The current body-to-world quaternion. */
  const Quat &quaternion() const;

  /**
   * @return The accelerometer reading this orientation predicts, in the body
   *     frame, as a unit direction (gravity reaction, nominally +1 g on Z when
   *     level).
   */
  Vec3 gravityEstimate() const;

  /** @return Roll, pitch and yaw in degrees. */
  EulerDeg eulerDeg() const;

 private:
  Quat q_{1.0f, 0.0f, 0.0f, 0.0f};
  bool initialized_ = false;
};
