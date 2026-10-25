#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include <cstdint>

/**
 * Wi-Fi station and UDP sender for the compute unit link.
 *
 * Responsibility: join the configured network without ever blocking the sensor
 * loop, keep the datagram socket alive across reconnects, resolve the
 * destination (configured unicast or the subnet's directed broadcast), and send
 * one already-formatted line per packet. Sends are fire-and-forget: a failure is
 * counted and the line is dropped, because on this link a late sample is worse
 * than a missing one. The class owns no calibration or sensor state.
 *
 * Invariants: `send()` returns false whenever the station is down; the sketch
 * keeps sampling and the USB record running regardless.
 */
class Link {
 public:
  /**
   * Start a non-blocking station join.
   *
   * @param ssid Network name; null or empty disables the link and leaves the
   *     firmware in USB-only mode.
   * @param password Network password, copied into the object.
   */
  void begin(const char *ssid, const char *password);

  /**
   * Advance the connection state machine.
   *
   * Call every few milliseconds. On a loss it tears the socket down; while
   * disconnected it retries the join every `WIFI_RETRY_MS`. All transitions are
   * announced once on the serial log, prefixed with `#`.
   *
   * @param nowMs Current `millis()` value.
   */
  void poll(uint32_t nowMs);

  /** @return true while the station is associated and datagrams may be sent. */
  bool up() const { return up_; }

  /**
   * Send one line as a single UDP datagram.
   *
   * @param line NUL-terminated payload without a trailing newline.
   * @return true when the datagram was handed to the network stack.
   */
  bool send(const char *line);

  /**
   * Set or clear the unicast destination override.
   *
   * @param destination Dotted-quad address, or empty for directed broadcast.
   */
  void setUnicast(const String &destination);

  /** @return The current override, empty when broadcast is used. */
  String unicast() const { return String(unicast_); }

  /** @return The address datagrams are currently sent to. */
  IPAddress destination() const;

  /** @return The station address, or 0.0.0.0 before association. */
  IPAddress localIp() const { return WiFi.localIP(); }

  /** @return Datagrams accepted by the stack since boot. */
  uint32_t sent() const { return sent_; }

  /** @return Datagrams or socket starts that failed since boot. */
  uint32_t failed() const { return failed_; }

  /** @return Re-associations after the first successful join. */
  uint32_t reconnects() const { return reconnects_; }

  /** @return RSSI in dBm while associated, 0 otherwise. */
  int32_t rssi() const;

  /** @return true when Wi-Fi credentials were supplied. */
  bool enabled() const { return enabled_; }

 private:
  bool enabled_ = false;
  bool up_ = false;
  bool udpStarted_ = false;
  bool everConnected_ = false;
  char ssid_[33] = {0};
  char password_[65] = {0};
  char unicast_[16] = {0};
  uint32_t lastAttemptMs_ = 0;
  uint32_t sent_ = 0;
  uint32_t failed_ = 0;
  uint32_t reconnects_ = 0;
  WiFiUDP udp_{};
};
