#include "Globals.h"
#include "WifiManager.h"
#include "Config.h"
#include <time.h>

Preferences Globals::prefs;
U8G2_SH1106_128X64_NONAME_F_HW_I2C Globals::u8g2(
    U8G2_R0,
    U8X8_PIN_NONE,
    I2C_SCL_PIN,
    I2C_SDA_PIN
);
HardwareSerial Globals::RS485(2);

AsyncWebServer Globals::server(80);
AsyncWebSocket Globals::ws("/ws");
WiFiClient Globals::espClient;
PubSubClient Globals::mqtt(Globals::espClient);

String Globals::chipIdStr = "";
String Globals::macStr = "";
String Globals::baseTopic = "";
String Globals::clientId = "";

bool Globals::webSerialStarted = false;
bool Globals::fsMounted = false;                  // [ADDED]

uint32_t Globals::nextHistoryAt = 0;
uint32_t Globals::restartAt = 0;

uint32_t Globals::alignToInterval(uint32_t nowMs, uint32_t intervalMs) {
  return nowMs - (nowMs % intervalMs) + intervalMs;
}

String Globals::getChipId() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char id[7];
  snprintf(id, sizeof(id), "%02X%02X%02X", mac[3], mac[4], mac[5]);
  return String(id);
}

String Globals::getMacStr() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

bool Globals::ensureFS() {                           // [ADDED]
  if (fsMounted) return true;
  if (LittleFS.begin(false)) {
    fsMounted = true;
    return true;
  }
  if (LittleFS.begin(true)) { // last-resort format
    fsMounted = true;
    return true;
  }
  return false;
}

void Globals::ensureWebSerial() {
  if (!webSerialStarted) {
    WebSerial.begin(&Globals::server);
    webSerialStarted = true;
    WebSerial.println("[WebSerial] Started");
  }
}

void Globals::stopWebSerial() {
  if (webSerialStarted) {
    webSerialStarted = false;
  }
}

void Globals::webDebug(const String& s, bool force) {
  if (force || debugMode) {
    Serial.println(s);
    if (webSerialStarted) WebSerial.println(s);
  }
}

void Globals::begin() {
  // Filesystem
  if (ensureFS()) {                                  // [UPDATED]
    Serial.println("[FS] LittleFS mounted successfully.");
  } else {
    Serial.println("[FS] CRITICAL: LittleFS mount failed.");
  }

  chipIdStr = getChipId();
  macStr = getMacStr();
  Serial.println("[ID] ChipID: " + chipIdStr + " MAC: " + macStr);

  clientId = "thcs_" + chipIdStr;
  baseTopic = "thcs/" + chipIdStr;
}

extern void factoryReset();

void Globals::handleScheduled() {
  using namespace WifiManager;

  if (doFactoryReset) {
    doFactoryReset = false;
    factoryReset();
  }
  if (restartAt && millis() >= restartAt) {
    restartAt = 0;
    ESP.restart();
  }
}