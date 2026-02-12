#ifndef GLOBALS_H
#define GLOBALS_H

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <WebSerial.h>
#include <PubSubClient.h>
#include <U8g2lib.h>
#include <Preferences.h>
#include <LittleFS.h>
#include "Config.h"

namespace Globals {

  extern Preferences prefs;
  extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;
  extern HardwareSerial RS485;

  extern AsyncWebServer server;
  extern AsyncWebSocket ws;
  extern WiFiClient espClient;
  extern PubSubClient mqtt;

  extern String chipIdStr;
  extern String macStr;
  extern String baseTopic;
  extern String clientId;

  extern bool webSerialStarted;
  extern bool fsMounted;            // [ADDED] track LittleFS mount state

  extern uint32_t nextHistoryAt;
  extern uint32_t restartAt;

  void begin();
  void handleScheduled();

  String getChipId();
  String getMacStr();

  void ensureWebSerial();
  void stopWebSerial();
  void webDebug(const String& s, bool force=false);

  uint32_t alignToInterval(uint32_t nowMs, uint32_t intervalMs);

  bool ensureFS();                  // [ADDED] safe mount helper
}

#endif