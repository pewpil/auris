#include "link.h"

#include <WiFi.h>
#include <esp_wifi.h>

#include <cstdio>
#include <cstring>

#include "config.h"

namespace {

/** The one Link instance, used by the C-style ESP-NOW send callback. */
Link *g_active = nullptr;

}  // namespace

void Link::begin() {
  channel_ = cfg::ESPNOW_CHANNEL;
  g_active = this;

  // ESP-NOW requires the Wi-Fi driver to be running, but the link must not
  // involve a network. Station mode with no SSID and an explicit disconnect is
  // what starts the driver without associating.
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setSleep(false);
  if (esp_wifi_set_channel(channel_, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
    Serial.println("#link channel set failed");
  }
  if (initRadio()) {
    Serial.printf("#link espnow peer=%s channel=%u\n",
                  peerMac().c_str(), static_cast<unsigned>(channel_));
  } else {
    Serial.println("#link espnow not ready; retrying");
  }
}

bool Link::initRadio() {
  if (esp_now_init() != ESP_OK) {
    ready_ = false;
    return false;
  }
  esp_now_register_send_cb(Link::onSent);
  ready_ = true;
  return addPeer();
}

bool Link::addPeer() {
  if (esp_now_is_peer_exist(cfg::ESPNOW_PEER_MAC)) {
    peerAdded_ = true;
    return true;
  }
  esp_now_peer_info_t peer{};
  std::memcpy(peer.peer_addr, cfg::ESPNOW_PEER_MAC, 6);
  peer.channel = channel_;
  peer.encrypt = false;
  const esp_err_t result = esp_now_add_peer(&peer);
  peerAdded_ = result == ESP_OK;
  if (!peerAdded_) {
    Serial.printf("#link peer add failed (%d)\n", static_cast<int>(result));
  }
  return peerAdded_;
}

void Link::poll(uint32_t nowMs) {
  if (nowMs - lastRetryMs_ < cfg::LINK_RETRY_MS) {
    return;
  }
  if (!ready_) {
    lastRetryMs_ = nowMs;
    ++retries_;
    initRadio();
    return;
  }
  if (!peerAdded_) {
    lastRetryMs_ = nowMs;
    ++retries_;
    addPeer();
  }
}

bool Link::send(const char *line) {
  if (!up() || line == nullptr) {
    ++failed_;
    return false;
  }
  const size_t length = std::strlen(line);
  if (length == 0 || length > ESP_NOW_MAX_DATA_LEN) {
    ++failed_;
    return false;
  }
  if (esp_now_send(cfg::ESPNOW_PEER_MAC,
                   reinterpret_cast<const uint8_t *>(line), length) != ESP_OK) {
    ++failed_;
    return false;
  }
  return true;
}

void Link::noteSend(esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    ++sent_;
  } else {
    ++failed_;
  }
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void Link::onSent(const esp_now_send_info_t *info, esp_now_send_status_t status) {
  (void)info;
#else
void Link::onSent(const uint8_t *mac, esp_now_send_status_t status) {
  (void)mac;
#endif
  if (g_active != nullptr) {
    g_active->noteSend(status);
  }
}

String Link::peerMac() const {
  char text[18];
  std::snprintf(text, sizeof(text), "%02X:%02X:%02X:%02X:%02X:%02X",
                cfg::ESPNOW_PEER_MAC[0], cfg::ESPNOW_PEER_MAC[1],
                cfg::ESPNOW_PEER_MAC[2], cfg::ESPNOW_PEER_MAC[3],
                cfg::ESPNOW_PEER_MAC[4], cfg::ESPNOW_PEER_MAC[5]);
  return String(text);
}
