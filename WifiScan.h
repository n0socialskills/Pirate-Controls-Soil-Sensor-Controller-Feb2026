#ifndef WIFISCAN_H
#define WIFISCAN_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

// Centralized WiFi scan helper (esp-idf based).
// - Registers WIFI_EVENT_SCAN_DONE handler.
// - SCAN_DONE schedules a cached-read; actual record fetch happens in poll().
// - Minimal data per AP cached in APEntry for safe JSON serialization.

namespace WifiScan {

  enum Status {
    IDLE = 0,
    SCANNING,
    DONE,
    FAILED
  };

  struct APEntry {
    String ssid;
    int rssi;
    int chan;
    String bssid;
    String enc;
  };

  // Initialize scan subsystem (register SCAN_DONE handler once)
  void begin();

  // Start async scan (non-blocking). Returns true if started.
  bool startAsync(bool showHidden = true, bool passive = false, uint8_t channel = 0);

  // Query helpers
  bool isScanning();
  Status getStatus();

  // Called from main loop: performs any pending post-scan reads / caching.
  void poll();

  // Fill DynamicJsonDocument with status/results (reads cached results if available)
  // d["status"] = "scanning" | "done" | "idle" | "failed"
  // when done: d["count"], d["aps"] = [...]
  void buildStatusJSON(DynamicJsonDocument& d);

  // Clear cached results (optional)
  void clearCache();

} // namespace WifiScan

#endif // WIFISCAN_H