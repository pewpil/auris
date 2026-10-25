// Host-side tests for the pure firmware core.
//
// These exercise the Mahony filter, the quaternion helpers, the calibration
// offset maths, and the packet serialiser without an ESP32. Build and run with:
//
//   g++ -std=c++17 -Wall -Wextra -O2 test/test_fusion.cpp src/fusion.cpp
//       src/packet.cpp -o /tmp/pointer_host_test && /tmp/pointer_host_test
//
// The simulated cases are the [SIM] evidence for the fusion and the packet
// schema; they are not a substitute for the in-use telemetry the thesis needs.

#include "../src/fusion.h"
#include "../src/packet.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace {

/** Degrees-to-radians factor, matching the firmware. */
constexpr float kDegToRad = 0.017453292519943295f;

int g_failures = 0;

void check(bool condition, const char *what) {
  if (!condition) {
    std::printf("FAIL: %s\n", what);
    ++g_failures;
  }
}

void checkNear(float actual, float expected, float tolerance,
               const char *what) {
  if (std::fabs(actual - expected) > tolerance) {
    std::printf("FAIL: %s (got %.6f, expected %.6f +/- %.6f)\n", what, actual,
                expected, tolerance);
    ++g_failures;
  }
}

}  // namespace

int main() {
  // 1. Euler <-> quaternion round trip.
  {
    const EulerDeg in{30.0f, -20.0f, 45.0f};
    const EulerDeg out = eulerFromQuatDeg(quatFromEulerDeg(in));
    checkNear(out.roll, in.roll, 1.0e-3f, "euler roll round trip");
    checkNear(out.pitch, in.pitch, 1.0e-3f, "euler pitch round trip");
    checkNear(out.yaw, in.yaw, 1.0e-3f, "euler yaw round trip");
  }

  // 2. The identity accelerometer seeds the identity quaternion.
  {
    MahonyFilter f;
    f.begin(Vec3{0.0f, 0.0f, 1.0f});
    checkNear(f.quaternion().w, 1.0f, 1.0e-5f, "identity seed w");
    checkNear(f.quaternion().x, 0.0f, 1.0e-5f, "identity seed x");
  }

  // 3. A static 30-degree roll is recovered from an identity start.
  {
    MahonyFilter f;
    f.begin(Vec3{0.0f, 0.0f, 1.0f});
    for (int i = 0; i < 2000; ++i) {
      f.update(Vec3{0.0f, 0.0f, 0.0f}, Vec3{0.0f, 0.5f, 0.8660254f}, 0.005f);
    }
    const EulerDeg e = f.eulerDeg();
    checkNear(e.roll, 30.0f, 0.5f, "static roll convergence");
    checkNear(e.pitch, 0.0f, 0.5f, "static pitch stays level");
  }

  // 4. Gyro-only yaw integration: 90 deg/s for 1 s is 90 degrees.
  {
    MahonyFilter f;
    f.begin(Vec3{0.0f, 0.0f, 1.0f});
    for (int i = 0; i < 200; ++i) {
      f.update(Vec3{0.0f, 0.0f, 90.0f * kDegToRad}, Vec3{0.0f, 0.0f, 1.0f},
               0.005f);
    }
    checkNear(f.eulerDeg().yaw, 90.0f, 2.0f, "gyro yaw integration");
  }

  // 5. The quaternion stays unit length through a mixed update.
  {
    MahonyFilter f;
    f.begin(Vec3{0.0f, 0.0f, 1.0f});
    for (int i = 0; i < 1000; ++i) {
      f.update(Vec3{0.3f, -0.2f, 0.5f}, Vec3{0.1f, -0.2f, 0.97f}, 0.005f);
    }
    const Quat q = f.quaternion();
    const float norm =
        std::sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    checkNear(norm, 1.0f, 1.0e-4f, "quaternion remains unit length");
  }

  // 6. The gravity estimate matches the accelerometer that seeded it.
  {
    MahonyFilter f;
    f.begin(Vec3{0.0f, 0.5f, 0.8660254f});
    const Vec3 v = f.gravityEstimate();
    checkNear(v.x, 0.0f, 1.0e-4f, "gravity estimate x");
    checkNear(v.y, 0.5f, 1.0e-4f, "gravity estimate y");
    checkNear(v.z, 0.8660254f, 1.0e-4f, "gravity estimate z");
  }

  // 7. A 90-degree pitch maps world +Z to body -X.
  {
    const Vec3 v =
        quatRotateWorldToBody(quatFromEulerDeg(EulerDeg{0.0f, 90.0f, 0.0f}),
                              Vec3{0.0f, 0.0f, 1.0f});
    checkNear(v.x, -1.0f, 1.0e-4f, "pitch 90 world-to-body x");
    checkNear(v.y, 0.0f, 1.0e-4f, "pitch 90 world-to-body y");
    checkNear(v.z, 0.0f, 1.0e-4f, "pitch 90 world-to-body z");
  }

  // 8. Flat calibration removes the measured offset.
  {
    const Vec3 offset =
        accelOffsetFromFlatMean(Vec3{0.1f, -0.2f, 1.35f});
    checkNear(offset.x, 0.1f, 1.0e-6f, "accel offset x");
    checkNear(offset.y, -0.2f, 1.0e-6f, "accel offset y");
    checkNear(offset.z, 0.35f, 1.0e-6f, "accel offset z");
  }

  // 9. Packet serialisation: prefix, field count, and the no-range sentinel.
  {
    PointerPacket p;
    p.seq = 7;
    p.tUs = 1234567890123ULL;
    p.bootId = 42;
    p.button = 1;
    p.rangeMm = 1234;
    p.rangeStatus = 0;
    p.rangeAgeMs = 42;
    p.flags = kFlagCalibrated | kFlagImuOk | kFlagLinkUp;
    p.resetReason = 9;

    char line[256];
    const int written = formatPacket(line, sizeof(line), p);
    check(written > 0, "packet formats");
    const char *expectedPrefix = "P1,7,1234567890123,42,1,";
    check(std::strncmp(line, expectedPrefix, std::strlen(expectedPrefix)) == 0,
          "packet prefix and header fields");
    int commas = 0;
    for (const char *c = line; *c != '\0'; ++c) {
      if (*c == ',') {
        ++commas;
      }
    }
    check(commas == 23, "packet has 24 comma-separated fields");
    check(std::strstr(line, ",1234,0,42,0x0B,9") != nullptr,
          "packet carries range, status, age, flags and reset");

    p.rangeMm = -1;
    p.rangeStatus = 255;
    p.rangeAgeMs = -1;
    formatPacket(line, sizeof(line), p);
    check(std::strstr(line, ",-1,255,-1,0x0B,9") != nullptr,
          "packet carries the no-range sentinel");
    check(std::strstr(packetHeader(), "range_mm") != nullptr,
          "header names the range field");
  }

  if (g_failures == 0) {
    std::printf("all host tests passed\n");
    return 0;
  }
  std::printf("%d host test(s) failed\n", g_failures);
  return 1;
}
