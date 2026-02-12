#include "OLEDDisplay.h"
#include "Globals.h"
#include "Config.h"
#include "THCSensor.h"
#include "ActiveSensor.h"
#include "WifiManager.h"
#include "Utils.h"
#include <time.h>

static uint32_t lastOled = 0;
const uint32_t OLED_INTERVAL_MS = 1000;
static uint32_t lastPageSwitch = 0;
const uint32_t PAGE_INTERVAL_MS = 8000;
static bool showPage1 = true;
static bool showApNotice = false;
static uint32_t apNoticeStart = 0;

static bool buttonPressed = false;
static uint32_t buttonPressStart = 0;
const uint32_t FACTORY_RESET_HOLD_MS = 10000;

const uint32_t SHORT_PRESS_MS = 400;
const uint32_t DOUBLE_TAP_WINDOW_MS = 600;
static int tapCount = 0;
static uint32_t firstTapTime = 0;
static bool apHoldForStart = false;
static uint32_t lastTapExpiry = 0;

static const char* sensorTypeLabel() {                // [ADDED]
  return (sensorType == SENSOR_MEC20) ? "MEC20" : "THCS";
}

static void oledDrawHeartbeat() {
  Globals::u8g2.setDrawColor(ActiveSensor::heartbeat() ? 1 : 0);
  Globals::u8g2.drawBox(124, 0, 4, 4);
  Globals::u8g2.setDrawColor(1);
}

static String ipOrApText() {
  if (WifiManager::wifiState == WIFI_STA_CONNECTED && WiFi.isConnected()) return WiFi.localIP().toString();
  if (WifiManager::wifiState == WIFI_AP_ACTIVE) return String("AP 192.168.4.1");
  return String("WiFi...");
}

static void drawPage1() {
  char buf[32];
  Globals::u8g2.clearBuffer();
  Globals::u8g2.setFont(u8g2_font_ncenB08_tr);

  // Sensor type badge top-right
  Globals::u8g2.drawStr(90, 12, sensorTypeLabel());   // [ADDED]

  snprintf(buf, sizeof(buf), "VWC: %.1f %%", ActiveSensor::getVWC());
  Globals::u8g2.drawStr(0, 12, buf);
  
  if (useFahrenheit) {
    float f = ActiveSensor::getTempC() * 9.0f/5.0f + 32.0f;
    snprintf(buf, sizeof(buf), "Temp: %.1f F", f);
  } else {
    snprintf(buf, sizeof(buf), "Temp: %.1f C", ActiveSensor::getTempC());
  }
  Globals::u8g2.drawStr(0, 26, buf);

  if (ActiveSensor::hasError()) {
    Globals::u8g2.drawStr(0, 40, "SENSOR ERROR!");
  } else {
    snprintf(buf, sizeof(buf), "pwEC: %.2f dS/m", ActiveSensor::getPwec());
    Globals::u8g2.drawStr(0, 40, buf);
  }

  String iptxt = "IP: " + ipOrApText();
  Globals::u8g2.drawStr(0, 54, iptxt.c_str());

  oledDrawHeartbeat();
  Globals::u8g2.sendBuffer();
}

static void drawPage2() {
  char buf[64];
  Globals::u8g2.clearBuffer();
  Globals::u8g2.setFont(u8g2_font_ncenB08_tr);

  // Sensor type badge
  Globals::u8g2.drawStr(90, 12, sensorTypeLabel());   // [ADDED]

  int ecInt = Utils::safeRoundToInt(ActiveSensor::getECuS(), 0, 1000000);
  int n = snprintf(buf, sizeof(buf), "EC: %d uS/cm", ecInt);
  if (n >= (int)sizeof(buf)) {
    Globals::webDebug("[OLED] EC string truncated");
  }
  Globals::u8g2.drawStr(0, 12, buf);
  
  n = snprintf(buf, sizeof(buf), "Raw EC: %.0f", ActiveSensor::getRawEC());
  if (n >= (int)sizeof(buf)) {
    Globals::webDebug("[OLED] Raw EC string truncated");
  }
  Globals::u8g2.drawStr(0, 26, buf);
  
  n = snprintf(buf, sizeof(buf), "Cal: %.2f/%.1f/%.3f", EC_SLOPE, EC_INTERCEPT, EC_TEMP_COEFF);
  if (n >= (int)sizeof(buf)) {
    Globals::webDebug("[OLED] Calibration string truncated");
  }
  Globals::u8g2.drawStr(0, 40, buf);

  String link = "WiFi/";
  link += (WifiManager::wifiState == WIFI_STA_CONNECTED ? "OK" : (WifiManager::wifiState == WIFI_AP_ACTIVE ? "AP" : "--"));
  link += " / MQTT/";
  link += (Globals::mqtt.connected() ? "OK" : "--");
  if (debugMode) link += " DBG";
  Globals::u8g2.drawStr(0, 54, link.c_str());

  oledDrawHeartbeat();
  Globals::u8g2.sendBuffer();
}

static void drawApNotice() {
  Globals::u8g2.clearBuffer();
  Globals::u8g2.setFont(u8g2_font_ncenB08_tr);
  Globals::u8g2.drawStr(0, 12, "AP MODE ACTIVE");
  Globals::u8g2.drawStr(0, 26, (String("SSID: ") + AP_SSID_DEFAULT).c_str());      // [UPDATED]
  Globals::u8g2.drawStr(0, 40, (String("Pass: ") + AP_PASSWORD_DEFAULT).c_str());   // [UPDATED]
  IPAddress apIp = WiFi.softAPIP();
  String ipLine = String("Go to: ") + (apIp.toString().length() ? apIp.toString() : "192.168.4.1"); // [ADDED]
  Globals::u8g2.drawStr(0, 54, ipLine.c_str());
  Globals::u8g2.sendBuffer();
}

static void drawFactoryResetHold() {
  Globals::u8g2.clearBuffer();
  Globals::u8g2.setFont(u8g2_font_ncenB08_tr);

  if (apHoldForStart) {
    Globals::u8g2.drawStr(0, 18, "Hold Button");
    Globals::u8g2.drawStr(0, 34, "to START AP");
    Globals::u8g2.setFont(u8g2_font_ncenB12_tr);
    uint32_t held = millis() - buttonPressStart;
    int rem = (int)((FACTORY_RESET_HOLD_MS - held) / 1000);
    if (rem < 0) rem = 0;
    char b[8]; snprintf(b, sizeof(b), "%ds", rem);
    Globals::u8g2.drawStr(104, 60, b);
    Globals::u8g2.sendBuffer();
    return;
  }

  Globals::u8g2.drawStr(0, 20, "Hold Button");
  Globals::u8g2.drawStr(0, 36, "for Factory Reset");
  Globals::u8g2.setFont(u8g2_font_ncenB12_tr);
  uint32_t held = millis() - buttonPressStart;
  int rem = (int)((FACTORY_RESET_HOLD_MS - held) / 1000);
  if (rem < 0) rem = 0;
  char b[8]; snprintf(b, sizeof(b), "%ds", rem);
  Globals::u8g2.drawStr(104, 60, b);
  Globals::u8g2.sendBuffer();
}

static void checkFactoryResetButton() {
  bool cur = (digitalRead(FACTORY_RESET_PIN) == LOW);
  if (tapCount == 1 && (int32_t)(millis() - firstTapTime) > (int32_t)DOUBLE_TAP_WINDOW_MS) {
    tapCount = 0;
    firstTapTime = 0;
    lastTapExpiry = 0;
  }

  if (cur && !buttonPressed) {
    buttonPressed = true;
    buttonPressStart = millis();
    if (tapCount == 1 && (int32_t)(millis() - firstTapTime) <= (int32_t)DOUBLE_TAP_WINDOW_MS) {
      apHoldForStart = true;
      Globals::u8g2.clearBuffer();
      Globals::u8g2.setFont(u8g2_font_ncenB08_tr);
      Globals::u8g2.drawStr(0, 18, "Double-tap detected");
      Globals::u8g2.drawStr(0, 36, "Hold to START AP");
      Globals::u8g2.sendBuffer();
      Globals::webDebug("[OLED] Double-tap detected: awaiting hold for AP start", true);
    } else {
      apHoldForStart = false;
    }
  } else if (cur && buttonPressed) {
    if (apHoldForStart) {
      if (millis() - buttonPressStart >= FACTORY_RESET_HOLD_MS) {
        Globals::u8g2.clearBuffer();
        Globals::u8g2.setFont(u8g2_font_ncenB10_tr);
        Globals::u8g2.drawStr(10, 35, "Starting AP...");
        Globals::u8g2.sendBuffer();
        Globals::webDebug("[OLED] AP start triggered (double-tap+hold)", true);
        WifiManager::startAP();
        apHoldForStart = false;
        buttonPressed = false;
        tapCount = 0;
        firstTapTime = 0;
        lastTapExpiry = 0;
      }
    } else {
      if (millis() - buttonPressStart >= FACTORY_RESET_HOLD_MS) {
        Globals::u8g2.clearBuffer();
        Globals::u8g2.setFont(u8g2_font_ncenB10_tr);
        Globals::u8g2.drawStr(10, 35, "Factory Reset!");
        Globals::u8g2.sendBuffer();
        WifiManager::doFactoryReset = true;
        tapCount = 0;
        firstTapTime = 0;
        lastTapExpiry = 0;
      }
    }
  } else if (!cur && buttonPressed) {
    uint32_t held = millis() - buttonPressStart;
    if (held < SHORT_PRESS_MS) {
      if (tapCount == 0) {
        tapCount = 1;
        firstTapTime = millis();
        lastTapExpiry = firstTapTime + DOUBLE_TAP_WINDOW_MS;
      } else if (tapCount == 1) {
        tapCount = 0;
        firstTapTime = 0;
        lastTapExpiry = 0;
        apHoldForStart = false;
      } else {
        tapCount = 0;
        firstTapTime = 0;
        lastTapExpiry = 0;
        apHoldForStart = false;
      }
    } else {
      apHoldForStart = false;
      tapCount = 0;
      firstTapTime = 0;
      lastTapExpiry = 0;
    }
    buttonPressed = false;
  }
}

void OLEDDisplay::begin() {
  Globals::u8g2.begin();
  Globals::u8g2.clearBuffer();
  Globals::u8g2.setFont(u8g2_font_ncenB08_tr);
  Globals::u8g2.drawStr(0, 12, "Pirate Controls");
  char line[32];
  snprintf(line, sizeof(line), "%s %s",
           (sensorType == SENSOR_MEC20 ? "MEC20 Sensor" : "THCS Sensor"),
           FW_VERSION);
  Globals::u8g2.drawStr(0, 28, line);
  Globals::u8g2.drawStr(0, 44, "Booting...");
  Globals::u8g2.drawStr(0, 60, sensorTypeLabel());      // [ADDED] sensor badge on boot
  Globals::u8g2.sendBuffer();
}

void OLEDDisplay::setApNotice(bool on) {
  showPage1 = true;
  showApNotice = on;
  if (on) apNoticeStart = millis();
}

void OLEDDisplay::setShowFactoryHold(bool on, uint32_t startMs) {
  if (on) { buttonPressed = true; buttonPressStart = startMs; }
  else { buttonPressed = false; }
}

void OLEDDisplay::loop() {
  checkFactoryResetButton();
  uint32_t now = millis();
  if (buttonPressed) { drawFactoryResetHold(); return; }
  if (showApNotice && (now - apNoticeStart) < 10000) { drawApNotice(); return; }
  if (now - lastOled < OLED_INTERVAL_MS) return;
  if (now - lastPageSwitch >= PAGE_INTERVAL_MS) {
    showPage1 = !showPage1;
    lastPageSwitch = now;
  }
  if (showPage1) drawPage1(); else drawPage2();
  lastOled = now;
}