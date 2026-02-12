#include "WifiManager.h"
#include "Globals.h"
#include "Config.h"
#include "OLEDDisplay.h"
#include <time.h>

using namespace WifiManager;

WifiState WifiManager::wifiState = WIFI_STA_TRYING;
bool WifiManager::apActive = false;
String WifiManager::staSSID = "";
String WifiManager::staPASS = "";
uint32_t WifiManager::wifiAttemptStart = 0;
uint32_t WifiManager::nextStaRetryAt = 0;
uint32_t WifiManager::apDisableAt = 0;
bool WifiManager::doFactoryReset = false;

void WifiManager::begin() {
  if (!staSSID.isEmpty()) startSTA();
  else startAP();
}

void WifiManager::startSTA() {
  if (!apActive) {
    WiFi.mode(WIFI_AP_STA);
    // Check if AP initialization succeeded before marking apActive = true
    bool apOk = WiFi.softAP(AP_SSID_DEFAULT, AP_PASSWORD_DEFAULT);
    if (apOk) {
      apActive = true;
      OLEDDisplay::setApNotice(true);
      Globals::webDebug(String("[WiFi] AP+STA: AP up (") + AP_SSID_DEFAULT + "), trying STA: " + staSSID, true);
    } else {
      // Log error if AP failed to start
      apActive = false;
      Globals::webDebug("[WiFi] AP failed to start; attempting STA-only mode.", true);
    }
  } else {
    WiFi.mode(WIFI_AP_STA);
    Globals::webDebug("[WiFi] AP already up, trying STA: " + staSSID, true);
  }
  WiFi.begin(staSSID.c_str(), staPASS.c_str());
  wifiAttemptStart = millis();
  wifiState = WIFI_STA_TRYING;
}

void WifiManager::stopSTA() {
  WiFi.disconnect(true);
}

void WifiManager::startAP() {
  WiFi.mode(WIFI_AP);
  // Check AP initialization result and handle failure
  bool ok = WiFi.softAP(AP_SSID_DEFAULT, AP_PASSWORD_DEFAULT);
  if (!ok) {
    Globals::webDebug("[WiFi] AP failed to start. System may be unstable.", true);
  }
  apActive = ok;
  wifiState = WIFI_AP_ACTIVE;
  nextStaRetryAt = millis() + WIFI_AP_RETRY_PERIOD_MS;
  OLEDDisplay::setApNotice(ok);  // Only show AP notice if AP actually started
  Globals::webDebug(String("[WiFi] AP ") + (ok ? "started" : "failed") +
                    String(", SSID: ") + AP_SSID_DEFAULT + " Pass: " + AP_PASSWORD_DEFAULT, true);
}

void WifiManager::stopAP() {
  if (WiFi.getMode() & WIFI_AP) {
    WiFi.softAPdisconnect(true);
    Globals::webDebug("[WiFi] AP stopped");
  }
  apActive = false;
}

void WifiManager::wifiRetryNow() {
  if (wifiState == WIFI_AP_ACTIVE) {
    nextStaRetryAt = millis() + 50;
  } else {
    stopSTA();
    nextStaRetryAt = millis() + 100;
  }
}

void WifiManager::loop() {
  uint32_t now = millis();
  if (wifiState == WIFI_STA_TRYING) {
    if (WiFi.status() == WL_CONNECTED) {
      wifiState = WIFI_STA_CONNECTED;
      apDisableAt = now + AP_DISABLE_DELAY_MS;
      Globals::webDebug("[WiFi] Connected (STA). IP: " + WiFi.localIP().toString() + " (AP will stop soon)", true);
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    } else {
      if (now - wifiAttemptStart > WIFI_CONNECT_WINDOW_MS) {
        nextStaRetryAt = now + WIFI_AP_RETRY_PERIOD_MS;
        wifiState = WIFI_AP_ACTIVE;
      }
    }
  } else if (wifiState == WIFI_AP_ACTIVE) {
    if (now >= nextStaRetryAt) {
      startSTA();
    }
  } else if (wifiState == WIFI_STA_CONNECTED) {
    if (WiFi.status() != WL_CONNECTED) {
      Globals::webDebug("[WiFi] Lost connection, trying STA again", true);
      startSTA();
    } else if (apActive && apDisableAt > 0 && now >= apDisableAt) {
      stopAP();
      apDisableAt = 0;
    }
  }
}