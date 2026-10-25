#include "link.h"

#include <cstdio>
#include <cstring>

#include "config.h"

void Link::begin(const char *ssid, const char *password) {
  if (ssid == nullptr || ssid[0] == '\0') {
    enabled_ = false;
    return;
  }
  std::strncpy(ssid_, ssid, sizeof(ssid_) - 1);
  std::strncpy(password_, password == nullptr ? "" : password,
               sizeof(password_) - 1);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(cfg::WIFI_HOSTNAME);
  // Low latency while the pointer is in use matters more than the last few
  // milliamps; the device is a handheld aid, not a battery logger.
  WiFi.setSleep(false);
  WiFi.begin(ssid_, password_);
  lastAttemptMs_ = millis();
  enabled_ = true;
}

void Link::poll(uint32_t nowMs) {
  if (!enabled_) {
    return;
  }

  const bool connected = WiFi.status() == WL_CONNECTED;
  if (connected && !up_) {
    up_ = true;
    if (!udpStarted_) {
      udpStarted_ = udp_.begin(cfg::LINK_PORT) != 0;
    }
    if (everConnected_) {
      ++reconnects_;
    }
    everConnected_ = true;
    Serial.print("#link up ip=");
    Serial.print(WiFi.localIP());
    Serial.print(" dest=");
    Serial.print(destination());
    Serial.printf(":%u\n", static_cast<unsigned>(cfg::LINK_PORT));
  } else if (!connected && up_) {
    up_ = false;
    if (udpStarted_) {
      udp_.stop();
      udpStarted_ = false;
    }
    Serial.println("#link down");
  }

  if (!connected && nowMs - lastAttemptMs_ >= cfg::WIFI_RETRY_MS) {
    lastAttemptMs_ = nowMs;
    WiFi.reconnect();
  }
}

bool Link::send(const char *line) {
  if (!enabled_ || !up_) {
    return false;
  }
  if (!udpStarted_) {
    udpStarted_ = udp_.begin(cfg::LINK_PORT) != 0;
    if (!udpStarted_) {
      ++failed_;
      return false;
    }
  }

  const IPAddress dest = destination();
  if (udp_.beginPacket(dest, cfg::LINK_PORT) == 0) {
    ++failed_;
    return false;
  }

  const size_t length = std::strlen(line);
  const size_t written =
      udp_.write(reinterpret_cast<const uint8_t *>(line), length);
  const int ended = udp_.endPacket();
  if (written != length || ended == 0) {
    ++failed_;
    return false;
  }

  ++sent_;
  return true;
}

void Link::setUnicast(const String &destination) {
  if (destination.isEmpty()) {
    unicast_[0] = '\0';
    return;
  }
  std::strncpy(unicast_, destination.c_str(), sizeof(unicast_) - 1);
  unicast_[sizeof(unicast_) - 1] = '\0';
}

IPAddress Link::destination() const {
  if (unicast_[0] != '\0') {
    unsigned a = 0;
    unsigned b = 0;
    unsigned c = 0;
    unsigned d = 0;
    if (std::sscanf(unicast_, "%u.%u.%u.%u", &a, &b, &c, &d) == 4 &&
        a <= 255 && b <= 255 && c <= 255 && d <= 255) {
      return IPAddress(static_cast<uint8_t>(a), static_cast<uint8_t>(b),
                       static_cast<uint8_t>(c), static_cast<uint8_t>(d));
    }
  }

  const IPAddress ip = WiFi.localIP();
  const IPAddress mask = WiFi.subnetMask();
  return IPAddress(static_cast<uint8_t>(ip[0] | ~mask[0]),
                   static_cast<uint8_t>(ip[1] | ~mask[1]),
                   static_cast<uint8_t>(ip[2] | ~mask[2]),
                   static_cast<uint8_t>(ip[3] | ~mask[3]));
}

int32_t Link::rssi() const {
  return up_ ? WiFi.RSSI() : 0;
}
