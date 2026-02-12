#include "WifiScan.h"
#include "Globals.h"
#include "WifiManager.h"

#include <WiFi.h>
#include <ArduinoJson.h>
#include <vector>

// esp-idf includes
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_err.h"

namespace WifiScan {

// Internal state
static volatile Status sStatus = IDLE;
static bool sHandlerRegistered = false;

// Cached AP results (filled by deferred reader)
static std::vector<APEntry> sCachedAps;
static volatile bool sCacheReady = false;
static volatile esp_err_t sLastErr = ESP_FAIL;

// Scheduling for deferred read (set by event handler)
static volatile uint32_t sCacheAttemptAt = 0; // 0 = none scheduled

// Attempt limiter + fallback gate
static volatile uint8_t sDeferredAttempts = 0;
static constexpr uint8_t MAX_DEFERRED_ATTEMPTS = 3; // allow a few tries before fallback

// [ADDED] Track a scan session and whether we expect IDF SCAN_DONE events
static volatile bool sScanSessionActive = false;  // true between startAsync and first finished list
static volatile bool sExpectIdfScanEvent = false; // true only for esp_wifi_scan_start

// Helper: convert esp-idf auth mode to string
static const char* encToStr(wifi_auth_mode_t m) {
  return (m == WIFI_AUTH_OPEN) ? "OPEN" : "SECURED";
}

// End the scan session cleanly: stop all retries and mark not expecting events
static void endSession() { // [ADDED]
  sScanSessionActive = false;
  sExpectIdfScanEvent = false;
  sCacheAttemptAt = 0;
  sDeferredAttempts = 0;
  // We intentionally leave sCacheReady as-is (UI can still read results)
  // Put status back to IDLE so modules don't think scanning continues
  sStatus = IDLE;
}

// Forward: function that reads & caches AP records once via esp-idf; returns true on success.
// Note: runs in normal task context (not event callback). Logs summary/errors only.
static bool readAndCacheApRecordsOnce() {
  uint16_t num = 0;
  esp_err_t rc = esp_wifi_scan_get_ap_num(&num);
  // [UPDATED] gate debug print behind debugMode
  if (debugMode) Serial.printf("[WiFiScan] esp_wifi_scan_get_ap_num rc=%d num=%u\n", (int)rc, (unsigned)num);

  if (rc != ESP_OK) {
    Globals::webDebug(String("[WiFiScan] scan_get_ap_num failed rc=") + String((int)rc) + " (" + esp_err_to_name(rc) + ")", true);
    sLastErr = rc;
    sCacheReady = true; // mark cache ready (empty) to avoid repeated attempts
    sCachedAps.clear();
    endSession();       // [ADDED] stop trying after terminal error
    return false;
  }

  if (num == 0) {
    // Treat zero APs reported by esp_wifi_scan_get_ap_num as a transient condition.
    sCachedAps.clear();
    sCacheReady = false;      // keep not-ready so poll() can retry or fallback
    sLastErr = ESP_OK;
    Globals::webDebug("[WiFiScan] idf reports 0 APs; will retry", false); // [UPDATED]
    return false;             // request retry via poll()'s deferred attempts
  }

  // Cap to protect heap
  uint16_t cap = (num > 32) ? 32 : num; // conservative cap

  // One attempt to allocate and fetch records (keep poll light)
  wifi_ap_record_t* recs = (wifi_ap_record_t*)malloc(sizeof(wifi_ap_record_t) * cap);
  if (!recs) {
    Globals::webDebug(String("[WiFiScan] malloc failed for cap=") + String(cap), true);
    sLastErr = ESP_ERR_NO_MEM;
    sCacheReady = true;
    sCachedAps.clear();
    endSession();       // [ADDED] stop trying on OOM
    return false;
  }

  uint16_t outCount = cap;
  rc = esp_wifi_scan_get_ap_records(&outCount, recs);
  // [UPDATED] gate debug print behind debugMode
  if (debugMode) Serial.printf("[WiFiScan] esp_wifi_scan_get_ap_records rc=%d outCount=%u\n", (int)rc, (unsigned)outCount);

  if (rc == ESP_OK && outCount > 0) {
    sCachedAps.clear();
    sCachedAps.reserve(outCount);
    for (uint16_t i = 0; i < outCount; ++i) {
      APEntry e;
      e.ssid = String(reinterpret_cast<const char*>(recs[i].ssid));
      e.rssi = (int)recs[i].rssi;
      e.chan = (int)recs[i].primary;
      char bssid[18];
      snprintf(bssid, sizeof(bssid), "%02X:%02X:%02X:%02X:%02X:%02X",
               (uint8_t)recs[i].bssid[0], (uint8_t)recs[i].bssid[1], (uint8_t)recs[i].bssid[2],
               (uint8_t)recs[i].bssid[3], (uint8_t)recs[i].bssid[4], (uint8_t)recs[i].bssid[5]);
      e.bssid = String(bssid);
      e.enc = String(encToStr(recs[i].authmode));
      sCachedAps.push_back(e);
    }
    free(recs);
    sCacheReady = true;
    sLastErr = ESP_OK;
    Globals::webDebug(String("[WiFiScan] cached ") + String(sCachedAps.size()) + " APs", false);
    endSession();       // [ADDED] success: stop trying
    return true;
  } else {
    Globals::webDebug(String("[WiFiScan] idf get_ap_records rc=") + String((int)rc) +
                      " (" + esp_err_to_name(rc) + "), outCount=" + String(outCount), false);
    free(recs);
    sCachedAps.clear();
    sCacheReady = false;   // keep as not-ready so poll() can retry or fallback
    sLastErr = rc;         // may be ESP_OK; sDeferredAttempts decides fallback
    return false;
  }
}

// Arduino synchronous fallback used only after IDF read repeatedly yields 0
static void buildCacheFromArduinoSync() {
  // Disable IDF scan event expectation to ignore SCAN_DONE from Arduino scan
  sExpectIdfScanEvent = false; // [ADDED]

  // Ensure STA present (AP+STA to keep AP serving UI)
  if (!(WiFi.getMode() & WIFI_STA)) {
    WiFi.mode((WifiManager::wifiState == WIFI_AP_ACTIVE) ? WIFI_AP_STA : WIFI_STA);
  }

  Globals::webDebug("[WiFiScan] fallback: running Arduino sync scan", true);
  int st = WiFi.scanNetworks(false /* sync */, true /* show_hidden */); // blocking
  sCachedAps.clear();
  if (st > 0) {
    sCachedAps.reserve((size_t)st);
    for (int i = 0; i < st; ++i) {
      APEntry e;
      e.ssid = WiFi.SSID(i);
      e.rssi = WiFi.RSSI(i);
      e.chan = WiFi.channel(i);
      e.bssid = WiFi.BSSIDstr(i);
      int enc = WiFi.encryptionType(i);
      e.enc = (enc == WIFI_AUTH_OPEN ? "OPEN" : "SECURED");
      sCachedAps.push_back(e);
    }
    sCacheReady = true;
    sLastErr = ESP_OK;
    Globals::webDebug(String("[WiFiScan] fallback cached ") + String(sCachedAps.size()) + " APs", true);
  } else if (st == 0) {
    sCacheReady = true;
    sLastErr = ESP_OK;
    Globals::webDebug("[WiFiScan] fallback found 0 APs", true);
  } else {
    sCacheReady = true;
    sLastErr = ESP_FAIL;
    Globals::webDebug("[WiFiScan] fallback scan failed", true);
  }
  WiFi.scanDelete(); // free Arduino scan results

  endSession(); // [ADDED] stop trying after fallback result
}

// SCAN_DONE event handler: schedule deferred read and mark finished.
// Keep this handler minimal & non-blocking.
static void onScanDone(void* arg, esp_event_base_t ev_base, int32_t ev_id, void* event_data) {
  (void)arg; (void)ev_base; (void)ev_id; (void)event_data;

  // Ignore SCAN_DONE not belonging to our IDF session (e.g., from Arduino fallback)
  if (!sScanSessionActive || !sExpectIdfScanEvent) { // [ADDED]
    if (debugMode) Serial.printf("[WiFiScan] SCAN_DONE ignored: sScanSessionActive=%d sExpectIdfScanEvent=%d\n",
                                 (int)sScanSessionActive, (int)sExpectIdfScanEvent); // [UPDATED] gated
    Globals::webDebug("[WiFiScan] SCAN_DONE ignored", false);
    return;
  }

  // schedule a deferred read shortly from main loop context
  sStatus = DONE;              // mark scan phase done; records to be read
  sCacheReady = false;
  sLastErr = ESP_FAIL;
  sDeferredAttempts = 0;
  sCacheAttemptAt = millis() + 200; // slightly longer to avoid premature reads
  if (debugMode) Serial.println("[WiFiScan] SCAN_DONE: scheduled cache read (sCacheAttemptAt set)"); // [UPDATED] gated
  Globals::webDebug("[WiFiScan] SCAN_DONE: scheduled cache read", true);
}

static void ensureHandler() {
  if (sHandlerRegistered) return;
  esp_err_t err = esp_event_handler_instance_register(
      WIFI_EVENT, WIFI_EVENT_SCAN_DONE,
      onScanDone,
      nullptr, nullptr
  );
  if (err == ESP_OK) {
    sHandlerRegistered = true;
    Globals::webDebug("[WiFiScan] SCAN_DONE handler registered", true);
  } else {
    if (debugMode) Serial.printf("[WiFiScan] handler register failed err=%d (%s)\n", (int)err, esp_err_to_name(err)); // [UPDATED] gated
    Globals::webDebug(String("[WiFiScan] handler register failed: ") + String((int)err), true);
  }
}

void begin() {
  ensureHandler();
  sCachedAps.clear();
  sCacheReady = false;
  sLastErr = ESP_FAIL;
  sCacheAttemptAt = 0;
  sDeferredAttempts = 0;
  sScanSessionActive = false;   // [ADDED]
  sExpectIdfScanEvent = false;  // [ADDED]
  sStatus = IDLE;
}

bool startAsync(bool showHidden, bool passive, uint8_t channel) {
  ensureHandler();

  if (!(WiFi.getMode() & WIFI_STA)) {
    WiFi.mode((WifiManager::wifiState == WIFI_AP_ACTIVE) ? WIFI_AP_STA : WIFI_STA);
  }

  // reset cache/request
  sCacheReady = false;
  sCachedAps.clear();
  sLastErr = ESP_FAIL;
  sCacheAttemptAt = 0;
  sDeferredAttempts = 0;

  // [ADDED] begin new session and expect IDF SCAN_DONE
  sScanSessionActive = true;
  sExpectIdfScanEvent = true;

  if (debugMode) Serial.println("[WiFiScan] startAsync called - about to start esp_wifi_scan_start()"); // [UPDATED] gated

  wifi_scan_config_t cfg = {};
  cfg.ssid = nullptr;
  cfg.bssid = nullptr;
  cfg.channel = channel;
  cfg.show_hidden = showHidden;
  cfg.scan_type = passive ? WIFI_SCAN_TYPE_PASSIVE : WIFI_SCAN_TYPE_ACTIVE;
  if (cfg.scan_type == WIFI_SCAN_TYPE_ACTIVE) {
    cfg.scan_time.active.min = 120;  // slightly longer dwell
    cfg.scan_time.active.max = 300;
  } else {
    cfg.scan_time.passive = 200;
  }

  esp_err_t r = esp_wifi_scan_start(&cfg, false);
  if (debugMode) Serial.printf("[WiFiScan] esp_wifi_scan_start rc=%d (%s)\n", (int)r, esp_err_to_name(r)); // [UPDATED] gated
  if (r == ESP_OK) {
    sStatus = SCANNING;
    Globals::webDebug("[WiFiScan] esp_wifi_scan_start OK (async)", true);
    return true;
  } else {
    sStatus = FAILED;
    // session failed; end it to avoid any retries
    endSession(); // [ADDED]
    Globals::webDebug(String("[WiFiScan] esp_wifi_scan_start failed: ") + String((int)r) + " (" + esp_err_to_name(r) + ")", true);
    return false;
  }
}

bool isScanning() {
  // Only true while a session is active and we've said SCANNING
  return sScanSessionActive && (sStatus == SCANNING); // [UPDATED]
}

Status getStatus() {
  if (sScanSessionActive) return sStatus;
  if (sCacheReady) return DONE;
  return IDLE;
}

// poll(): called from main loop. performs deferred cache reads when scheduled.
void poll() {
  // If no active session, nothing to do
  if (!sScanSessionActive) return; // [ADDED]

  // If a cache read was scheduled and time has arrived, attempt once.
  uint32_t at = sCacheAttemptAt;
  if (at != 0 && (int32_t)(millis() - at) >= 0) {
    // clear schedule to avoid redundant attempts
    sCacheAttemptAt = 0;

    // Try IDF record read first
    bool ok = readAndCacheApRecordsOnce();
    if (!ok) {
      sDeferredAttempts++;
      // If IDF record read failed (including ESP_OK + outCount==0), retry or fall back
      if (sDeferredAttempts < MAX_DEFERRED_ATTEMPTS) {
        sCacheAttemptAt = millis() + 300;     // slightly longer before retry
        Globals::webDebug("[WiFiScan] read not ready; retry scheduled", false);
      } else {
        Globals::webDebug("[WiFiScan] switching to Arduino sync fallback", true);
        buildCacheFromArduinoSync();          // blocking fallback (one-shot); endSession() inside
      }
    }
  }
}

void buildStatusJSON(DynamicJsonDocument& d) {
  // If in a session and cache not ready yet, report scanning
  if (sScanSessionActive && !sCacheReady) { // [UPDATED]
    d["status"] = "scanning";
    return;
  }
  if (!sScanSessionActive && !sCacheReady) {
    // No active session, no cache
    d["status"] = "idle";
    d["count"] = 0;
    d.createNestedArray("aps");
    return;
  }

  // Cache is ready: report once and stop trying (session already ended)
  d["status"] = "done";
  d["count"] = (uint32_t)sCachedAps.size();
  JsonArray arr = d.createNestedArray("aps");
  for (const auto& e : sCachedAps) {
    JsonObject o = arr.createNestedObject();
    o["ssid"] = e.ssid;
    o["rssi"] = e.rssi;
    o["chan"] = e.chan;
    o["bssid"] = e.bssid;
    o["enc"] = e.enc;
  }
  if (sLastErr != ESP_OK) d["error"] = String("last_err=") + String((int)sLastErr) + " (" + String(esp_err_to_name(sLastErr)) + ")";
}

void clearCache() {
  sCachedAps.clear();
  sCacheReady = false;
  sLastErr = ESP_FAIL;
  sCacheAttemptAt = 0;
  sDeferredAttempts = 0;
  // Do NOT alter session flags here; this is a manual cache clear.
}

} // namespace WifiScan