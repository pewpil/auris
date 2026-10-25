#pragma once

#include <cstdint>

/**
 * Compile-time configuration for the deployable Auris pointer firmware.
 *
 * Every value here is a build constant: pins, bus speed, sample and packet
 * rates, full-scale gains, recovery thresholds, and the NVS namespace. Runtime
 * values that a technician can change — calibration offsets, the unicast
 * destination override, the boot counter — live in NVS and are handled by
 * Store (src/store.h), not here.
 */
namespace cfg {

// ---------------------------------------------------------------------------
// Pins. The wiring is the parent pointer/AGENTS.md list; the button is GPIO 18
// and the ToF enable must be actively driven, never floated.
// ---------------------------------------------------------------------------

/** I2C SDA, shared by the MPU6050 and the VL53L1X. */
constexpr uint8_t PIN_I2C_SDA = 21;
/** I2C SCL, shared by the MPU6050 and the VL53L1X. */
constexpr uint8_t PIN_I2C_SCL = 22;
/** ToF enable (XSHUT), driven low for a hardware reset and high for the run. */
constexpr uint8_t PIN_TOF_XSHUT = 27;
/** Laser-trigger button, INPUT_PULLUP: idle high, pressed low. */
constexpr uint8_t PIN_BUTTON = 18;
/** Onboard LED on the NodeMCU-32S; used for provisioning and fault status. */
constexpr uint8_t PIN_LED = 2;

/** MPU6050 address with AD0 tied to ground. Floating AD0 would move it to 0x69. */
constexpr uint8_t ADDR_MPU6050 = 0x68;
/** VL53L1X address, fixed by the part. */
constexpr uint8_t ADDR_VL53L1X = 0x29;
/** I2C bus speed, the datasheet ceiling for both parts. */
constexpr uint32_t I2C_CLOCK_HZ = 400000;

// ---------------------------------------------------------------------------
// Timing. The IMU is sampled at 200 Hz and fused every sample; packets leave
// at 100 Hz while the button is held; the USB debug stream is decimated to
// 25 Hz so a 115200-baud monitor is never overrun; the ToF is polled at 50 Hz
// although it produces a measurement only every 50 ms.
// ---------------------------------------------------------------------------

/** IMU sampling period in microseconds (200 Hz). */
constexpr uint32_t IMU_PERIOD_US = 5000;
/** Packet period while the button is held, in microseconds (100 Hz). */
constexpr uint32_t PACKET_PERIOD_US = 10000;
/** USB debug period, in microseconds (25 Hz). */
constexpr uint32_t SERIAL_PERIOD_US = 40000;
/** How often the serial column header is repeated, in milliseconds. */
constexpr uint32_t HEADER_PERIOD_MS = 5000;
/** How often the ToF is polled for a ready measurement, in milliseconds. */
constexpr uint32_t TOF_POLL_PERIOD_MS = 20;
/** Button debounce interval, in milliseconds. */
constexpr uint32_t BUTTON_DEBOUNCE_MS = 15;

// ---------------------------------------------------------------------------
// MPU6050. Full-scale gains are chosen for a handheld pointer rather than the
// bench: ±4 g keeps handling shocks from clipping, and ±500 deg/s covers fast
// wrist rotation. The DLPF sits at ~94 Hz so the filter lag stays small at a
// 200 Hz sample rate.
// ---------------------------------------------------------------------------

/** PWR_MGMT_1 value: wake from sleep, clock from the gyro PLL. */
constexpr uint8_t MPU_PWR_WAKE_PLLCLOCK = 0x01;
/** CONFIG value: DLPF ~94 Hz accel / ~98 Hz gyro. */
constexpr uint8_t MPU_CONFIG_DLPF_94HZ = 0x02;
/** GYRO_CONFIG value: ±500 deg/s (AFS_SEL = 1). */
constexpr uint8_t MPU_GYRO_CONFIG_500DPS = 0x08;
/** ACCEL_CONFIG value: ±4 g (AFS_SEL = 1). */
constexpr uint8_t MPU_ACCEL_CONFIG_4G = 0x08;

/** MPU6050 registers used by the driver. */
constexpr uint8_t MPU_REG_WHO_AM_I = 0x75;
constexpr uint8_t MPU_REG_PWR_MGMT_1 = 0x6B;
constexpr uint8_t MPU_REG_CONFIG = 0x1A;
constexpr uint8_t MPU_REG_GYRO_CONFIG = 0x1B;
constexpr uint8_t MPU_REG_ACCEL_CONFIG = 0x1C;
constexpr uint8_t MPU_REG_BURST = 0x3B;
/** The one acceptable WHO_AM_I answer for the MPU6050 (clones may differ; the
 *  byte is recorded and a mismatch is reported, not fatal). */
constexpr uint8_t MPU_WHO_AM_I_EXPECTED = 0x68;

/** LSB per g at ±4 g full scale. */
constexpr float MPU_ACCEL_LSB_PER_G = 8192.0f;
/** LSB per deg/s at ±500 deg/s full scale. */
constexpr float MPU_GYRO_LSB_PER_DPS = 65.5f;
/** LSB per degC of the die-temperature register. */
constexpr float MPU_TEMP_LSB_PER_C = 340.0f;
/** Offset of the die-temperature register in degC. */
constexpr float MPU_TEMP_OFFSET_C = 36.53f;
/** Consecutive all-zero bursts that trigger a configuration rewrite. */
constexpr uint32_t MPU_ZERO_BURSTS_BEFORE_RECONFIG = 20;
/** Minimum interval between configuration rewrites, in milliseconds. */
constexpr uint32_t MPU_RECONFIG_INTERVAL_MS = 1000;

// ---------------------------------------------------------------------------
// VL53L1X. Long mode, 50 ms budget and period, with a 500 ms read timeout used
// only by the driver's own blocking helpers. The firmware polls dataReady()
// instead, so a missing measurement never stalls the 200 Hz IMU loop.
// ---------------------------------------------------------------------------

/** Measurement budget in microseconds (long mode minimum is 33 ms). */
constexpr uint32_t TOF_BUDGET_US = 50000;
/** Inter-measurement period in milliseconds. */
constexpr uint32_t TOF_PERIOD_MS = 50;
/** Driver timeout, in milliseconds; only relevant to blocking driver calls. */
constexpr uint16_t TOF_TIMEOUT_MS = 500;
/** Age without a fresh measurement that counts as stale, in milliseconds. */
constexpr uint32_t TOF_STALE_MS = 600;
/** XSHUT low time during a hardware reset, in milliseconds. */
constexpr uint32_t TOF_XSHUT_LOW_MS = 10;
/** Settling time after XSHUT release, in milliseconds (the parent's ≥100 ms). */
constexpr uint32_t TOF_XSHUT_SETTLE_MS = 100;

// ---------------------------------------------------------------------------
// Calibration. `@CAL` samples the IMU for two seconds with the pointer flat,
// Z up and still, then stores the offsets in NVS. The spread thresholds reject
// a calibration taken while the device was being handled.
// ---------------------------------------------------------------------------

/** Number of samples in a calibration window (2 s at 200 Hz). */
constexpr uint32_t CAL_SAMPLES = 400;
/** Sample period during calibration, in milliseconds. */
constexpr uint32_t CAL_PERIOD_MS = 5;
/** Maximum peak-to-peak gyro spread accepted as "still", in deg/s. */
constexpr float CAL_GYRO_MAX_SPREAD_DPS = 8.0f;
/** Maximum peak-to-peak accel spread accepted as "flat and still", in g. */
constexpr float CAL_ACCEL_MAX_SPREAD_G = 0.35f;

// ---------------------------------------------------------------------------
// Link. The same Wi-Fi UDP path as imu_test: a station join, datagrams to the
// subnet's directed broadcast unless a unicast override is stored, one
// datagram per packet while the button is held. The link never blocks the
// sensor loop; a failed send is counted and dropped.
// ---------------------------------------------------------------------------

/** UDP port the compute unit listens on. */
constexpr uint16_t LINK_PORT = 5005;
/** Interval between reconnect attempts while the station is down, in ms. */
constexpr uint32_t WIFI_RETRY_MS = 5000;
/** Hostname advertised on the network. */
constexpr const char *WIFI_HOSTNAME = "auris-pointer";

// ---------------------------------------------------------------------------
// Storage.
// ---------------------------------------------------------------------------

/** NVS namespace holding calibration, boot counter, and link override. */
constexpr const char *NVS_NAMESPACE = "auris";

}  // namespace cfg
