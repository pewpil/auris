#include "packet.h"

#include <cstdio>

int formatPacket(char *out, size_t outSize, const PointerPacket &p) {
  return std::snprintf(
      out, outSize,
      "P1,%lu,%llu,%lu,%u,%.4f,%.4f,%.4f,%.4f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,"
      "%.2f,%.2f,%.2f,%.2f,%d,%u,%d,0x%02X,%u",
      static_cast<unsigned long>(p.seq),
      static_cast<unsigned long long>(p.tUs),
      static_cast<unsigned long>(p.bootId),
      static_cast<unsigned>(p.button),
      p.q.w, p.q.x, p.q.y, p.q.z,
      p.euler.roll, p.euler.pitch, p.euler.yaw,
      p.gyroDps.x, p.gyroDps.y, p.gyroDps.z,
      p.accelG.x, p.accelG.y, p.accelG.z,
      p.tempC,
      static_cast<int>(p.rangeMm),
      static_cast<unsigned>(p.rangeStatus),
      static_cast<int>(p.rangeAgeMs),
      static_cast<unsigned>(p.flags),
      static_cast<unsigned>(p.resetReason));
}

const char *packetHeader() {
  return "P1 seq,t_us,boot,btn,qw,qx,qy,qz,roll_deg,pitch_deg,yaw_deg,"
         "gx_dps,gy_dps,gz_dps,ax_g,ay_g,az_g,temp_C,range_mm,range_status,"
         "range_age_ms,flags,reset";
}
