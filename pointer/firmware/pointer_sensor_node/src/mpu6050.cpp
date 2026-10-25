#include "mpu6050.h"

#include "config.h"

bool ImuReading::allRawZero() const {
  for (uint8_t i = 0; i < 7; ++i) {
    if (raw[i] != 0) {
      return false;
    }
  }
  return true;
}

bool Mpu6050::begin() {
  uint8_t value = 0;
  if (!readRegister(cfg::MPU_REG_WHO_AM_I, value)) {
    return false;
  }
  whoAmI_ = value;
  return true;
}

bool Mpu6050::configure() {
  if (!writeRegister(cfg::MPU_REG_PWR_MGMT_1, cfg::MPU_PWR_WAKE_PLLCLOCK)) {
    ++configErrors_;
    return false;
  }
  // The oscillator needs a moment before the first sample is valid.
  delay(10);
  if (!writeRegister(cfg::MPU_REG_CONFIG, cfg::MPU_CONFIG_DLPF_94HZ) ||
      !writeRegister(cfg::MPU_REG_GYRO_CONFIG, cfg::MPU_GYRO_CONFIG_500DPS) ||
      !writeRegister(cfg::MPU_REG_ACCEL_CONFIG, cfg::MPU_ACCEL_CONFIG_4G)) {
    ++configErrors_;
    return false;
  }

  const struct {
    uint8_t reg;
    uint8_t expected;
  } checks[] = {
      {cfg::MPU_REG_PWR_MGMT_1, cfg::MPU_PWR_WAKE_PLLCLOCK},
      {cfg::MPU_REG_CONFIG, cfg::MPU_CONFIG_DLPF_94HZ},
      {cfg::MPU_REG_GYRO_CONFIG, cfg::MPU_GYRO_CONFIG_500DPS},
      {cfg::MPU_REG_ACCEL_CONFIG, cfg::MPU_ACCEL_CONFIG_4G},
  };
  for (const auto &check : checks) {
    uint8_t observed = 0;
    if (!readRegister(check.reg, observed) || observed != check.expected) {
      ++configErrors_;
      return false;
    }
  }

  ImuReading probe{};
  if (!read(probe) || probe.allRawZero()) {
    ++configErrors_;
    return false;
  }
  return true;
}

bool Mpu6050::read(ImuReading &out) {
  Wire.beginTransmission(cfg::ADDR_MPU6050);
  Wire.write(cfg::MPU_REG_BURST);
  if (Wire.endTransmission(false) != 0) {
    ++readErrors_;
    return false;
  }

  const uint8_t received =
      Wire.requestFrom(cfg::ADDR_MPU6050, static_cast<uint8_t>(14));
  if (received != 14) {
    while (Wire.available() > 0) {
      Wire.read();
    }
    ++readErrors_;
    return false;
  }

  uint8_t burst[14];
  for (uint8_t i = 0; i < 14; ++i) {
    burst[i] = static_cast<uint8_t>(Wire.read());
  }

  ImuReading decoded{};
  for (uint8_t i = 0; i < 7; ++i) {
    decoded.raw[i] = static_cast<int16_t>((burst[2 * i] << 8) | burst[2 * i + 1]);
  }
  decoded.axG = static_cast<float>(decoded.raw[0]) / cfg::MPU_ACCEL_LSB_PER_G;
  decoded.ayG = static_cast<float>(decoded.raw[1]) / cfg::MPU_ACCEL_LSB_PER_G;
  decoded.azG = static_cast<float>(decoded.raw[2]) / cfg::MPU_ACCEL_LSB_PER_G;
  decoded.tempC = static_cast<float>(decoded.raw[3]) / cfg::MPU_TEMP_LSB_PER_C +
                  cfg::MPU_TEMP_OFFSET_C;
  decoded.gxDps = static_cast<float>(decoded.raw[4]) / cfg::MPU_GYRO_LSB_PER_DPS;
  decoded.gyDps = static_cast<float>(decoded.raw[5]) / cfg::MPU_GYRO_LSB_PER_DPS;
  decoded.gzDps = static_cast<float>(decoded.raw[6]) / cfg::MPU_GYRO_LSB_PER_DPS;
  out = decoded;
  return true;
}

bool Mpu6050::writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(cfg::ADDR_MPU6050);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool Mpu6050::readRegister(uint8_t reg, uint8_t &value) {
  Wire.beginTransmission(cfg::ADDR_MPU6050);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(cfg::ADDR_MPU6050, static_cast<uint8_t>(1)) != 1) {
    return false;
  }
  value = static_cast<uint8_t>(Wire.read());
  return true;
}
