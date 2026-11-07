#pragma once

#include <Arduino.h>
#include <esp_now.h>

#include <cstdint>

/**
 * ESP-NOW sender for the compute unit link.
 *
 * Responsibility: start the Wi-Fi driver without joining any network, initialise
 * ESP-NOW, register the bridge as a peer, and send one already-formatted line per
 * packet. Sends are fire-and-forget: a failure is counted and the line is
 * dropped, because on this link a late sample is worse than a missing one. The
 * class owns no calibration or sensor state.
 *
 * The link involves exactly two devices: this pointer and the compute unit's
 * bridge. No access point, router, DHCP server or IP address is in the path.
 *
 * Invariants: `send()` returns false whenever the radio or peer is not ready;
 * the sketch keeps sampling and the USB record running regardless. `sent()`
 * counts packets the radio acknowledged, not packets merely queued.
 */
class Link {
 public:
  /**
   * Start the radio and register the bridge peer.
   *
   * Puts the Wi-Fi driver into station mode with no SSID configured, pins the
   * ESP-NOW channel, initialises ESP-NOW, and adds the bridge's MAC as an
   * unencrypted peer. No association is attempted or possible.
   */
  void begin();

  /**
   * Retry the radio or the peer registration if either is not ready.
   *
   * Call every few milliseconds. The retry interval is `cfg::LINK_RETRY_MS`.
   *
   * @param nowMs Current `millis()` value.
   */
  void poll(uint32_t nowMs);

  /** @return true when ESP-NOW is initialised and the bridge peer is registered. */
  bool up() const { return ready_ && peerAdded_; }

  /**
   * Send one line as a single ESP-NOW payload.
   *
   * @param line NUL-terminated payload without a trailing newline.
   * @return true when the radio accepted the payload for transmission. Delivery
   *     is reported asynchronously through `sent()`/`failed()`.
   */
  bool send(const char *line);

  /** @return Payloads the radio reported as delivered since boot. */
  uint32_t sent() const { return sent_; }

  /** @return Payloads that were rejected or reported as undelivered. */
  uint32_t failed() const { return failed_; }

  /** @return Radio or peer retry attempts since boot. */
  uint32_t retries() const { return retries_; }

  /** @return Always true; the link has no credentials to be missing. */
  bool enabled() const { return true; }

  /** @return The bridge MAC as colon-separated hex. */
  String peerMac() const;

  /** @return The pinned ESP-NOW channel. */
  uint8_t channel() const { return channel_; }

 private:
  /** Initialise ESP-NOW and register the send callback. @return true on success. */
  bool initRadio();

  /** Add the bridge peer if it is not already registered. @return true when present. */
  bool addPeer();

  /**
   * Record one asynchronous send result.
   *
   * @param status ESP-NOW's delivery verdict.
   */
  void noteSend(esp_now_send_status_t status);

  /**
   * ESP-NOW send callback.
   *
   * The signature follows arduino-esp32: core 3.x passes an
   * ``esp_now_send_info_t`` pointer, core 2.x passes the destination MAC. Only
   * the status is used.
   *
   * @param info Destination metadata in core 3.x, or the MAC in 2.x; unused.
   * @param status Delivery verdict.
   */
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  static void onSent(const esp_now_send_info_t *info, esp_now_send_status_t status);
#else
  static void onSent(const uint8_t *mac, esp_now_send_status_t status);
#endif

  bool ready_ = false;
  bool peerAdded_ = false;
  uint8_t channel_ = 1;
  uint32_t lastRetryMs_ = 0;
  uint32_t sent_ = 0;
  uint32_t failed_ = 0;
  uint32_t retries_ = 0;
};
