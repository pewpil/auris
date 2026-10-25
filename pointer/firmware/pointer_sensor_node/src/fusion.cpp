#include "fusion.h"

#include <algorithm>
#include <cmath>

namespace {

/** Degrees-to-radians factor. */
constexpr float kDegToRad = 0.017453292519943295f;
/** Radians-to-degrees factor. */
constexpr float kRadToDeg = 57.29577951308232f;
/** Mahony proportional gain, dimensionless. The gyro bias is calibrated, so no
 *  integral term is used; a proportional-only correction cannot wind up. */
constexpr float kMahonyKp = 0.8f;
/** Weight of gravity: samples outside this window (free fall, a hard tap) skip
 *  the accelerometer correction for that update. */
constexpr float kMinAccelG = 0.2f;
constexpr float kMaxAccelG = 5.0f;

/** Clamp a value into [lo, hi]. */
float clampf(float v, float lo, float hi) {
  return std::min(std::max(v, lo), hi);
}

}  // namespace

Quat quatFromEulerDeg(const EulerDeg &e) {
  const float hr = 0.5f * e.roll * kDegToRad;
  const float hp = 0.5f * e.pitch * kDegToRad;
  const float hy = 0.5f * e.yaw * kDegToRad;
  const float cr = std::cos(hr);
  const float sr = std::sin(hr);
  const float cp = std::cos(hp);
  const float sp = std::sin(hp);
  const float cy = std::cos(hy);
  const float sy = std::sin(hy);
  return Quat{
      cr * cp * cy + sr * sp * sy,
      sr * cp * cy - cr * sp * sy,
      cr * sp * cy + sr * cp * sy,
      cr * cp * sy - sr * sp * cy,
  };
}

EulerDeg eulerFromQuatDeg(const Quat &q) {
  const Quat n = quatNormalized(q);
  const float sinPitch = clampf(2.0f * (n.w * n.y - n.z * n.x), -1.0f, 1.0f);
  return EulerDeg{
      std::atan2(2.0f * (n.w * n.x + n.y * n.z),
                 1.0f - 2.0f * (n.x * n.x + n.y * n.y)) * kRadToDeg,
      std::asin(sinPitch) * kRadToDeg,
      std::atan2(2.0f * (n.w * n.z + n.x * n.y),
                 1.0f - 2.0f * (n.y * n.y + n.z * n.z)) * kRadToDeg,
  };
}

Quat quatNormalized(const Quat &q) {
  const float n = std::sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
  if (!std::isfinite(n) || n < 1.0e-6f) {
    return Quat{1.0f, 0.0f, 0.0f, 0.0f};
  }
  return Quat{q.w / n, q.x / n, q.y / n, q.z / n};
}

Vec3 quatRotateWorldToBody(const Quat &q, const Vec3 &v) {
  const Quat n = quatNormalized(q);
  const float w = n.w;
  const float x = n.x;
  const float y = n.y;
  const float z = n.z;

  // Body-to-world rotation matrix R. A world vector is taken into the body
  // frame with R^T, whose rows are R's columns.
  const float r00 = 1.0f - 2.0f * (y * y + z * z);
  const float r01 = 2.0f * (x * y - w * z);
  const float r02 = 2.0f * (x * z + w * y);
  const float r10 = 2.0f * (x * y + w * z);
  const float r11 = 1.0f - 2.0f * (x * x + z * z);
  const float r12 = 2.0f * (y * z - w * x);
  const float r20 = 2.0f * (x * z - w * y);
  const float r21 = 2.0f * (y * z + w * x);
  const float r22 = 1.0f - 2.0f * (x * x + y * y);

  return Vec3{
      r00 * v.x + r10 * v.y + r20 * v.z,
      r01 * v.x + r11 * v.y + r21 * v.z,
      r02 * v.x + r12 * v.y + r22 * v.z,
  };
}

Vec3 accelOffsetFromFlatMean(const Vec3 &meanG) {
  return Vec3{meanG.x, meanG.y, meanG.z - 1.0f};
}

void MahonyFilter::begin(const Vec3 &accelG) {
  const float magnitude = std::sqrt(accelG.x * accelG.x + accelG.y * accelG.y +
                                    accelG.z * accelG.z);
  if (!std::isfinite(magnitude) || magnitude < kMinAccelG ||
      magnitude > kMaxAccelG) {
    q_ = Quat{1.0f, 0.0f, 0.0f, 0.0f};
    initialized_ = true;
    return;
  }

  const float roll = std::atan2(accelG.y, accelG.z) * kRadToDeg;
  const float pitch =
      std::atan2(-accelG.x, std::sqrt(accelG.y * accelG.y + accelG.z * accelG.z)) *
      kRadToDeg;
  q_ = quatFromEulerDeg(EulerDeg{roll, pitch, 0.0f});
  initialized_ = true;
}

void MahonyFilter::update(const Vec3 &gyroRadPerSec, const Vec3 &accelG,
                          float dtSeconds) {
  if (!initialized_) {
    begin(accelG);
  }

  const float dt = clampf(dtSeconds, 0.001f, 0.05f);
  const float magnitude = std::sqrt(accelG.x * accelG.x + accelG.y * accelG.y +
                                    accelG.z * accelG.z);
  float ex = 0.0f;
  float ey = 0.0f;
  float ez = 0.0f;
  if (std::isfinite(magnitude) && magnitude >= kMinAccelG &&
      magnitude <= kMaxAccelG) {
    const Vec3 ax = Vec3{accelG.x / magnitude, accelG.y / magnitude,
                         accelG.z / magnitude};
    const Vec3 v = gravityEstimate();
    ex = ax.y * v.z - ax.z * v.y;
    ey = ax.z * v.x - ax.x * v.z;
    ez = ax.x * v.y - ax.y * v.x;
  }

  const float gx = gyroRadPerSec.x + kMahonyKp * ex;
  const float gy = gyroRadPerSec.y + kMahonyKp * ey;
  const float gz = gyroRadPerSec.z + kMahonyKp * ez;

  const float w = q_.w;
  const float x = q_.x;
  const float y = q_.y;
  const float z = q_.z;
  const float h = 0.5f * dt;
  q_.w += h * (-x * gx - y * gy - z * gz);
  q_.x += h * (w * gx + y * gz - z * gy);
  q_.y += h * (w * gy - x * gz + z * gx);
  q_.z += h * (w * gz + x * gy - y * gx);
  q_ = quatNormalized(q_);
}

const Quat &MahonyFilter::quaternion() const {
  return q_;
}

Vec3 MahonyFilter::gravityEstimate() const {
  return quatRotateWorldToBody(q_, Vec3{0.0f, 0.0f, 1.0f});
}

EulerDeg MahonyFilter::eulerDeg() const {
  return eulerFromQuatDeg(q_);
}
