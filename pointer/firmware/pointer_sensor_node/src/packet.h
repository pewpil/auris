#pragma once

#include <cstddef>
#include <cstdint>

#include "fusion.h"

/**
 * One pointer sample as it appears on both the USB debug stream and the Wi-Fi
 * datagram.
 *
 * Fields are in SI-derived units named in the header: quaternion dimensionless,
 * angles in degrees, angular rate in deg/s, acceleration in g, temperature in
 * degC, range in millimetres. `rangeMm < 0`, `rangeStatus == 255` and
 * `rangeAgeMs < 0` mean "no measurement yet"; they are never a plausible zero.
 */
struct PointerPacket {
  uint32_t seq = 0;           ///< Datagram sequence number, per boot.
  uint64_t tUs = 0;           ///< Microseconds since boot, monotonic.
  uint32_t bootId = 0;        ///< Increments once per boot and survives resets.
  uint8_t button = 0;         ///< 1 while the button is held, 0 otherwise.
  Quat q{1.0f, 0.0f, 0.0f, 0.0f};  ///< Body-to-world orientation.
  EulerDeg euler{0.0f, 0.0f, 0.0f};  ///< ZYX roll/pitch/yaw in degrees.
  Vec3 gyroDps{0.0f, 0.0f, 0.0f};    ///< Calibrated body rate in deg/s.
  Vec3 accelG{0.0f, 0.0f, 0.0f};     ///< Calibrated acceleration in g.
  float tempC = 0.0f;         ///< MPU6050 die temperature in degC.
  int16_t rangeMm = -1;       ///< ToF range in millimetres, -1 if none yet.
  uint8_t rangeStatus = 255;  ///< Driver RangeStatus, 255 when none yet.
  int16_t rangeAgeMs = -1;    ///< Age of the last range in ms, -1 if none.
  uint8_t flags = 0;          ///< Bit field, see the flag constants below.
  uint8_t resetReason = 0;    ///< esp_reset_reason() captured at boot.
};

/** Bit 0: calibration loaded and valid. */
constexpr uint8_t kFlagCalibrated = 0x01;
/** Bit 1: the most recent IMU burst was good and non-zero. */
constexpr uint8_t kFlagImuOk = 0x02;
/** Bit 2: the ToF has a measurement inside its stale window. */
constexpr uint8_t kFlagTofOk = 0x04;
/** Bit 3: the Wi-Fi station is associated. */
constexpr uint8_t kFlagLinkUp = 0x08;

/**
 * Format one packet as a single line without a trailing newline.
 *
 * The line begins with `P1,` so a receiver can reject other text, and every
 * field is comma separated in the order named by `packetHeader()`. The buffer
 * must hold at least 256 bytes; a truncated line is still NUL-terminated.
 *
 * @param out Destination buffer.
 * @param outSize Capacity of `out` in bytes.
 * @param p Packet to serialise.
 * @return Number of characters written, excluding the terminator.
 */
int formatPacket(char *out, size_t outSize, const PointerPacket &p);

/**
 * @return The comma-separated field names matching `formatPacket()`.
 */
const char *packetHeader();
