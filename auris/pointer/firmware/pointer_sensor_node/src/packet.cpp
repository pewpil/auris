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

int formatTableRow(char *out, size_t outSize, const PointerPacket &p) {
  char flags[8];
  std::snprintf(flags, sizeof(flags), "0x%02X",
                static_cast<unsigned>(p.flags));
  return std::snprintf(
      out, outSize,
      "%-8llu %-3u %-7.1f %-7.1f %-7.1f %-6.2f %-6.2f %-6.2f %-6d %-3u %-4d"
      " %-5s",
      static_cast<unsigned long long>(p.tUs / 1000ULL),
      static_cast<unsigned>(p.button),
      p.euler.roll, p.euler.pitch, p.euler.yaw,
      p.accelG.x, p.accelG.y, p.accelG.z,
      static_cast<int>(p.rangeMm),
      static_cast<unsigned>(p.rangeStatus),
      static_cast<int>(p.rangeAgeMs),
      flags);
}

int formatTableHeader(char *out, size_t outSize) {
  return std::snprintf(
      out, outSize,
      "%-8s %-3s %-7s %-7s %-7s %-6s %-6s %-6s %-6s %-3s %-4s %-5s",
      "t_ms", "btn", "roll", "pitch", "yaw", "ax", "ay", "az", "range",
      "st", "age", "flags");
}
