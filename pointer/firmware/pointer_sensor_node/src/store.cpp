#include "store.h"

#include "config.h"

bool Store::begin() {
  if (!prefs_.begin(cfg::NVS_NAMESPACE, false)) {
    return false;
  }
  cal_.valid = prefs_.getBool("cal", false);
  cal_.accelOffsetG.x = prefs_.getFloat("ax", 0.0f);
  cal_.accelOffsetG.y = prefs_.getFloat("ay", 0.0f);
  cal_.accelOffsetG.z = prefs_.getFloat("az", 0.0f);
  cal_.gyroBiasDps.x = prefs_.getFloat("gx", 0.0f);
  cal_.gyroBiasDps.y = prefs_.getFloat("gy", 0.0f);
  cal_.gyroBiasDps.z = prefs_.getFloat("gz", 0.0f);
  return true;
}

bool Store::saveCalibration(const Calibration &calibration) {
  cal_ = calibration;
  cal_.valid = true;
  const bool ok = prefs_.putBool("cal", true) > 0 &&
                  prefs_.putFloat("ax", cal_.accelOffsetG.x) > 0 &&
                  prefs_.putFloat("ay", cal_.accelOffsetG.y) > 0 &&
                  prefs_.putFloat("az", cal_.accelOffsetG.z) > 0 &&
                  prefs_.putFloat("gx", cal_.gyroBiasDps.x) > 0 &&
                  prefs_.putFloat("gy", cal_.gyroBiasDps.y) > 0 &&
                  prefs_.putFloat("gz", cal_.gyroBiasDps.z) > 0;
  cal_.valid = ok;
  return ok;
}

void Store::clearCalibration() {
  prefs_.remove("cal");
  prefs_.remove("ax");
  prefs_.remove("ay");
  prefs_.remove("az");
  prefs_.remove("gx");
  prefs_.remove("gy");
  prefs_.remove("gz");
  cal_ = Calibration{};
}

uint32_t Store::nextBootId() {
  const uint32_t id = prefs_.getUInt("boot", 0U) + 1U;
  prefs_.putUInt("boot", id);
  return id;
}

String Store::unicastDestination() {
  return prefs_.getString("dest", "");
}

bool Store::setUnicastDestination(const String &destination) {
  if (destination.isEmpty()) {
    return prefs_.remove("dest");
  }
  return prefs_.putString("dest", destination) > 0;
}

void Store::clearAll() {
  prefs_.clear();
  cal_ = Calibration{};
}
