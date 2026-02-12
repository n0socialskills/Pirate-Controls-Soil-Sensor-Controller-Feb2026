
#include "Config.h"
#include "Globals.h"
#include "THCSensor.h"
#include "OLEDDisplay.h"
#include "History.h"
#include "WifiManager.h"
#include "MQTTManager.h"
#include "WebUI.h"
#include "Utils.h"         
#include "html.h"
#include "WifiScan.h"
#include "ActiveSensor.h"  
#include "MEC20Sensor.h"   

void setup() {
  pinMode(RS485_DE_PIN, OUTPUT);
  pinMode(RS485_RE_PIN, OUTPUT);
  pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
  digitalWrite(RS485_DE_PIN, LOW);
  digitalWrite(RS485_RE_PIN, LOW);

  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.printf("--- %s v%s Booting ---\n", FW_NAME, FW_VERSION);

  Globals::begin();

  if (Globals::chipIdStr.isEmpty()) {
    Serial.println("[Setup] ERROR: Failed to initialize chip ID; system may be unstable");
  } else {
    Serial.println("[Setup] Globals initialized: ChipID=" + Globals::chipIdStr);
  }

  loadPreferences();

  Serial.printf("[Setup] Config loaded: SensorIntervalMs=%u, PublishMode=%s\n",
                sensorIntervalMs, (publishMode == PUBLISH_ALWAYS ? "ALWAYS" : "DELTA"));

  Serial.println("[Setup] Initializing subsystems...");
  ActiveSensor::begin(); // [UPDATED] initialize selected sensor (THCS/MEC20)
  OLEDDisplay::begin();
  History::begin();
  WifiManager::begin();
  WebUI::begin();
  MQTTManager::begin();
  WifiScan::begin();

  Globals::nextHistoryAt = Globals::alignToInterval(millis(), HISTORY_INTERVAL_MS);
  Serial.printf("[Setup] History interval aligned to %u ms\n", Globals::nextHistoryAt);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.printf("[Setup] NTP configured: offset=%lds, DST=%ds\n",
                gmtOffset_sec, daylightOffset_sec);

  Globals::webDebug("[Setup] Completed - device ready", true);
  Serial.println("[Setup] Boot complete - entering main loop\n");
}

void loop() {
  WifiManager::loop();
  WifiScan::poll();
  MQTTManager::loop();
  // THCSensor::loop();            // [REMOVED] duplicate polling; ActiveSensor handles driver polling
  ActiveSensor::loop();           // [UPDATED] poll active sensor (THCS or MEC20)
  MQTTManager::publishIfNeeded();
  WebUI::loop();
  History::loop();
  OLEDDisplay::loop();
  Globals::handleScheduled();
  Globals::ws.cleanupClients();
}
