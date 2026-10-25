#include "tof.h"

#include <Wire.h>

#include "config.h"

bool Tof::begin() {
  pinMode(cfg::PIN_TOF_XSHUT, OUTPUT);
  digitalWrite(cfg::PIN_TOF_XSHUT, LOW);
  delay(cfg::TOF_XSHUT_LOW_MS);
  digitalWrite(cfg::PIN_TOF_XSHUT, HIGH);
  delay(cfg::TOF_XSHUT_SETTLE_MS);

  if (!probeIdentity(identity_)) {
    initialized_ = false;
    return false;
  }
  if (identity_[0] != 0xEA || identity_[1] != 0xCC) {
    initialized_ = false;
    return false;
  }
  if (!sensor_.init()) {
    initialized_ = false;
    return false;
  }
  if (!sensor_.setDistanceMode(VL53L1X::Long)) {
    initialized_ = false;
    return false;
  }
  if (!sensor_.setMeasurementTimingBudget(cfg::TOF_BUDGET_US)) {
    initialized_ = false;
    return false;
  }
  sensor_.setTimeout(cfg::TOF_TIMEOUT_MS);
  sensor_.startContinuous(cfg::TOF_PERIOD_MS);

  initialized_ = true;
  hasRange_ = false;
  status_ = 255;
  lastRangeMs_ = millis();
  return true;
}

void Tof::poll(uint32_t nowMs) {
  if (!initialized_) {
    return;
  }

  if (sensor_.dataReady()) {
    rangeMm_ = sensor_.read(false);
    status_ = static_cast<uint8_t>(sensor_.ranging_data.range_status);
    hasRange_ = true;
    lastRangeMs_ = nowMs;
  }

  if (nowMs - lastRangeMs_ > cfg::TOF_STALE_MS) {
    ++reinits_;
    if (!begin()) {
      ++recoverFailures_;
      // Back off one stale window before the next attempt; begin() already
      // restarted the clock on success.
      lastRangeMs_ = nowMs;
    }
  }
}

bool Tof::healthy(uint32_t nowMs) const {
  return initialized_ && (nowMs - lastRangeMs_ <= cfg::TOF_STALE_MS);
}

int32_t Tof::rangeAgeMs(uint32_t nowMs) const {
  if (!hasRange_) {
    return -1;
  }
  return static_cast<int32_t>(nowMs - lastRangeMs_);
}

bool Tof::probeIdentity(uint8_t out[4]) {
  Wire.beginTransmission(cfg::ADDR_VL53L1X);
  Wire.write(static_cast<uint8_t>(0x01));
  Wire.write(static_cast<uint8_t>(0x0F));
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(cfg::ADDR_VL53L1X, static_cast<uint8_t>(4)) != 4) {
    return false;
  }
  for (uint8_t i = 0; i < 4; ++i) {
    out[i] = static_cast<uint8_t>(Wire.read());
  }
  return true;
}
