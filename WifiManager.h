#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>

// [UPDATED] Enum documenting WiFi state machine states
enum WifiState { 
  WIFI_STA_TRYING,      // Attempting to connect to saved STA credentials
  WIFI_STA_CONNECTED,   // Successfully connected to STA; AP will auto-disable after grace period
  WIFI_AP_ACTIVE        // AP mode active; will retry STA periodically
};

namespace WifiManager {
  // [UPDATED] Initialize WiFi based on stored credentials (STA if available, else AP)
  void begin();
  
  // [UPDATED] Non-blocking WiFi state machine loop
  // Monitors STA connection, manages AP enable/disable, handles retry scheduling
  void loop();
  
  // [UPDATED] Start STA mode with stored credentials (creates AP+STA if AP not already active)
  // Returns immediately; actual connection happens in loop()
  // [ADDED] Now validates AP initialization and logs failures
  void startSTA();
  
  // [UPDATED] Stop STA mode connection (does not stop AP)
  void stopSTA();
  
  // [UPDATED] Start AP mode (SSID: THCS_SETUP, Pass: Controls)
  // [ADDED] Now validates AP initialization result and handles failures gracefully
  void startAP();
  
  // [UPDATED] Stop AP mode (does not affect STA)
  void stopAP();
  
  // [UPDATED] Trigger immediate WiFi retry (used by Web UI)
  void wifiRetryNow();

  // State variables (read-only from outside this namespace)
  extern WifiState wifiState;    // Current WiFi state machine state
  extern bool apActive;          // [UPDATED] Now accurately reflects actual AP status (not assumed)
  extern String staSSID;         // Stored STA SSID
  extern String staPASS;         // Stored STA password
  extern uint32_t wifiAttemptStart;   // Timestamp of last STA attempt (for timeout calculation)
  extern uint32_t nextStaRetryAt;     // Scheduled time for next STA retry attempt
  extern uint32_t apDisableAt;        // Scheduled time to disable AP after STA connects

  // [UPDATED] Scheduled action flag for factory reset (set by System.cpp or UI)
  extern bool doFactoryReset;
}

#endif // WIFIMANAGER_H