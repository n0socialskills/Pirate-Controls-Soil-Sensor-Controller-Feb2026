#include "WebUI.h"
#include "Globals.h"
#include "Config.h"
#include "THCSensor.h"
#include "History.h"
#include "MQTTManager.h"
#include "WifiManager.h"
#include "OLEDDisplay.h"
#include "Utils.h"
#include "WifiScan.h"
#include "ActiveSensor.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFi.h>

static const size_t WS_MAX_MESSAGE_SIZE = 4096;

struct HistoryHeaderLocal {
  uint32_t magic;
  uint16_t version;
  uint16_t reserved;
  uint32_t head;
  uint32_t count;
  uint32_t maxRecords;
  uint32_t interval_s;
};

struct HistoryRecordLocal {
  uint32_t epoch;
  float    vwc;
  float    pwec;
  float    temp_c;
};

static String fmtUtcOffsetStr(long sec) {
  long a = (sec >= 0) ? sec : -sec;
  char sign = (sec >= 0) ? '+' : '-';
  int hh = a / 3600;
  int mm = (a % 3600) / 60;
  char buf[16];
  snprintf(buf, sizeof(buf), "UTC%c%02d:%02d", sign, hh, mm);
  return String(buf);
}

// History UI toggle removed; graph always enabled

static String resolveMacOrFallback() {
  String mac = Globals::macStr;
  if (mac.length() == 0 || mac == "00:00:00:00:00:00" || mac.startsWith("00:00:00")) {
    String wm = WiFi.macAddress();
    if (wm.length() > 0) {
      wm.toUpperCase();
      return wm;
    }
    return String("00:00:00:00:00:00");
  }
  mac.toUpperCase();
  return mac;
}

void WebUI::sendStatusWS() {
  DynamicJsonDocument d(920);
  d["type"] = "status";
  d["wifi_connected"] = (WifiManager::wifiState == WIFI_STA_CONNECTED && WiFi.status() == WL_CONNECTED);
  d["wifi_mode"] = (WifiManager::wifiState == WIFI_AP_ACTIVE) ? "ap" : "sta";
  d["ap_active"] = (WifiManager::wifiState == WIFI_AP_ACTIVE);

  if (WifiManager::wifiState == WIFI_STA_CONNECTED && WiFi.status() == WL_CONNECTED) {
    d["ssid"] = WifiManager::staSSID;
    d["ip"] = WiFi.localIP().toString();
    d["rssi"] = WiFi.RSSI();
  } else if (WifiManager::wifiState == WIFI_AP_ACTIVE) {
    d["ssid"] = AP_SSID_DEFAULT;
    d["ip"] = "192.168.4.1";
    d["rssi"] = 0;
  } else {
    d["ssid"] = WifiManager::staSSID;
    d["ip"] = "N/A";
    d["rssi"] = 0;
  }

  d["next_sta_retry_s"] = (WifiManager::wifiState == WIFI_AP_ACTIVE && WifiManager::nextStaRetryAt > millis())
                          ? (uint32_t)((WifiManager::nextStaRetryAt - millis()) / 1000) : 0;

  d["mqtt_connected"] = Globals::mqtt.connected();
  d["mqtt_enabled"] = mqttEnabled;

  // ----- Effective user topic with sensor-type segment and /sN suffix -----
  String effTopic = mqttTopicUser;
  if (effTopic.length()) {
    int s1 = effTopic.indexOf('/');
    if (s1 >= 0) {
      int s2 = effTopic.indexOf('/', s1 + 1);
      String seg0 = effTopic.substring(0, s1);                    // e.g. "sensor"
      String segRest = (s2 >= 0) ? effTopic.substring(s2) : "";   // rest after second segment
      String sensorSeg = (sensorType == SENSOR_MEC20) ? "mec20" : "thcs"; // [ADDED]
      effTopic = seg0 + "/" + sensorSeg + segRest;                // e.g. "sensor/mec20"
    }
    if (sensorNumber >= 1 && sensorNumber <= 255 && effTopic.length()) {
      effTopic += "/s" + String(sensorNumber);
    }
  }
  d["mqtt_topic"] = effTopic; // [UPDATED]

  d["sensor_error"] = ActiveSensor::hasError();

  uint32_t up = millis() / 1000;
  d["uptime_sec"] = up;
  d["uptime"] = up;
  d["uptime_human"] = Utils::uptimeHuman(up);
  d["since_reset_sec"] = up;

  d["debug_mode"] = debugMode;
  d["heap_free"] = ESP.getFreeHeap();
  d["heap_min"] = ESP.getMinFreeHeap();
#ifdef BOARD_HAS_PSRAM
  d["psram_free"] = ESP.getFreePsram();
#else
  d["psram_free"] = 0;
#endif

  d["useFahrenheit"] = useFahrenheit;
  d["structured_enabled"] = allowStructured;
  d["ha_enabled"] = haEnabled;
  d["publish_mode"] = (publishMode == PUBLISH_ALWAYS ? "always" : "delta");
  JsonObject thr = d.createNestedObject("thresholds");
  thr["vwc"] = deltaVWC;
  thr["temp"] = deltaTempC;
  thr["ec"] = deltaECuS;
  thr["pwec"] = deltaPWEC;

  d["timezone_offset"] = gmtOffset_sec;
  d["dst_offset"] = daylightOffset_sec;

  d["tz_name"] = tzName;
  d["webserial_enabled"] = webSerialEnabled;
  d["sensor_interval_ms"] = sensorIntervalMs;
  d["tz_offset_str"] = fmtUtcOffsetStr((long)gmtOffset_sec + (long)daylightOffset_sec);
  d["history_enabled"] = true;                        // always enabled
  struct tm tmnow;
  bool tsOk = getLocalTime(&tmnow, 1);
  d["time_synced"] = tsOk ? true : false;
  if (tsOk) {
    char buf[24];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmnow);
    d["time_str"] = buf;
  } else d["time_str"] = "";

  d["chipid"] = Globals::chipIdStr;
  d["sensor_number"] = sensorNumber;

  {
    String mac = resolveMacOrFallback();
    Globals::macStr = mac;
    d["mac"] = mac;
  }
  {
    String mac = d["mac"] | String("");
    String macNoColons = mac;
    macNoColons.replace(":", "");
    macNoColons.toUpperCase();
    String haBase;
    if (macNoColons.length() >= 6) {
      haBase = macNoColons.substring(macNoColons.length() - 6);
    } else if (macNoColons.length() > 0) {
      haBase = macNoColons;
    } else {
      haBase = Globals::chipIdStr;
    }
    String prefix = (sensorType == SENSOR_MEC20) ? "mec20/" : "thcs/"; // [ADDED]
    d["ha_topic"] = prefix + haBase + "/sensor";                      // [UPDATED]
  }

  d["sensor_type"] = (sensorType == SENSOR_MEC20 ? "mec20" : "thcs");

  String out;
  serializeJson(d, out);
  Globals::ws.textAll(out);
}

void WebUI::sendSensorWS() {
  DynamicJsonDocument d(256);
  d["type"] = "sensor";
  d["soil_hum"] = serialized(String(ActiveSensor::getVWC(), 1));
  d["soil_temp"] = serialized(String(ActiveSensor::getTempC(), 1));
  d["soil_pw_ec"] = serialized(String(ActiveSensor::getPwec(), 2));
  float ecF = ActiveSensor::getECuS();
  int ec = Utils::safeRoundToInt(ecF, 0, 1000000);
  d["soil_ec"] = serialized(String(ec));
  d["raw_ec"] = serialized(String(ActiveSensor::getRawEC(), 0));
  d["sensor_error"] = ActiveSensor::hasError();
  String out;
  serializeJson(d, out);
  Globals::ws.textAll(out);
}

void WebUI::sendConfigWS() {
  DynamicJsonDocument c(512);
  c["type"] = "config_update";
  c["ssid"] = WifiManager::staSSID;
  c["mqttServer"] = mqttServer;
  c["mqttPort"] = mqttPort;
  c["mqttUsername"] = mqttUsername;
  c["mqttTopic"] = mqttTopicUser;
  c["ecSlope"] = EC_SLOPE;
  c["ecIntercept"] = EC_INTERCEPT;
  c["ecTempCoeff"] = EC_TEMP_COEFF;
  c["useFahrenheit"] = useFahrenheit;
  c["allowStructured"] = allowStructured;
  c["haEnabled"] = haEnabled;
  c["mqttEnabled"] = mqttEnabled;
  c["publishMode"] = (publishMode == PUBLISH_ALWAYS ? "always" : "delta");
  c["deltaVWC"] = deltaVWC;
  c["deltaTemp"] = deltaTempC;
  c["deltaEC"] = deltaECuS;
  c["deltaPWEC"] = deltaPWEC;
  c["tzOffset"] = gmtOffset_sec;
  c["dstOffset"] = daylightOffset_sec;
  c["sensorIntervalMs"] = sensorIntervalMs;
  c["webSerialEnabled"] = webSerialEnabled;
  c["debugMode"] = debugMode;
  c["sensorNumber"] = sensorNumber;
  c["historyEnabled"] = true;                         // always enabled
  c["tzName"] = tzName;
  c["tzOffsetStr"] = fmtUtcOffsetStr((long)gmtOffset_sec + (long)daylightOffset_sec);
  c["sensorType"] = (sensorType == SENSOR_MEC20 ? "mec20" : "thcs");

  String out2;
  serializeJson(c, out2);
  Globals::ws.textAll(out2);
}

void WebUI::sendMetaWS() {
  DynamicJsonDocument d(384);
  d["type"] = "meta";
  d["fw"] = FW_NAME;
  d["ver"] = FW_VERSION;
  d["build"] = (uint32_t)BUILD_UNIX_TIME;
  d["chipid"] = Globals::chipIdStr;
  {
    String mac = resolveMacOrFallback();
    Globals::macStr = mac;
    d["mac"] = mac;
  }
  String out;
  serializeJson(d, out);
  Globals::ws.textAll(out);
}

static void wsOnEvent(AsyncWebSocket *wsServer, AsyncWebSocketClient *client, AwsEventType evType,
                      void *arg, uint8_t *data, size_t len) {
  switch (evType) {
    case WS_EVT_CONNECT:
      Globals::webDebug("[WS] Client connected: #" + String(client->id()));
      WebUI::sendStatusWS();
      WebUI::sendConfigWS();
      WebUI::sendMetaWS();
      break;
    case WS_EVT_DISCONNECT:
      Globals::webDebug("[WS] Client disconnected: #" + String(client->id()));
      break;
    case WS_EVT_DATA: {
      AwsFrameInfo* info = (AwsFrameInfo*)arg;
      if (!(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)) return;
      if (len > WS_MAX_MESSAGE_SIZE) {
        DynamicJsonDocument errResp(128);
        errResp["type"] = "error";
        errResp["message"] = "Payload too large";
        String errOut; serializeJson(errResp, errOut);
        client->text(errOut);
        return;
      }
      StaticJsonDocument<1024> j;
      DeserializationError de = deserializeJson(j, data, len);
      if (de) { return; }

      const char* msgType = j["type"] | "";
      if (!*msgType) return;

      if (strcmp(msgType, "get_status") == 0) {
        WebUI::sendStatusWS();
        WebUI::sendSensorWS();
      } else if (strcmp(msgType, "get_config") == 0) {
        WebUI::sendConfigWS();
      } else if (strcmp(msgType, "request_meta") == 0) {
        WebUI::sendMetaWS();
      } else if (strcmp(msgType, "set_sensor_type") == 0) {
        const char* st = j["sensor_type"] | "";
        SensorType newType = SENSOR_THCS;
        if (strcmp(st, "mec20") == 0) newType = SENSOR_MEC20;
        sensorType = newType;
        Globals::prefs.begin(NS_SENSOR, false);
        Globals::prefs.putInt("sensor_type", (int)sensorType);
        Globals::prefs.end();
        ActiveSensor::begin();
        DynamicJsonDocument r(160);
        r["type"] = "sensor_type_update";
        r["success"] = true;
        r["sensorType"] = (sensorType == SENSOR_MEC20 ? "mec20" : "thcs");
        r["message"] = "Sensor type updated.";
        String out; serializeJson(r, out);
        Globals::ws.textAll(out);
        WebUI::sendStatusWS();
        WebUI::sendConfigWS();
      } else if (strcmp(msgType, "get_chart_data") == 0) {
        uint32_t hours = j["hours"] | 24;
        if (hours == 0) hours = 24;
        if (hours > 36) hours = 36;

        String metaJson = History::getMetaJSON();
        DynamicJsonDocument metaDoc(256);
        if (deserializeJson(metaDoc, metaJson)) {
          DynamicJsonDocument resp(192);
          resp["type"] = "history_end";
          resp["idxTotal"] = 0;
          resp["hours"] = hours;
          resp["message"] = "History meta error";
          String o; serializeJson(resp, o);
          client->text(o);
          break;
        }
        uint32_t interval_s = metaDoc["interval_s"] | 0;
        uint32_t maxRecords = metaDoc["max_records"] | 0;
        uint32_t count      = metaDoc["count"] | 0;
        if (interval_s == 0 || maxRecords == 0 || count == 0) {
          DynamicJsonDocument resp(160);
          resp["type"] = "history_end";
          resp["idxTotal"] = 0;
          resp["hours"] = hours;
          resp["message"] = "No history available";
          String o; serializeJson(resp, o);
          client->text(o);
          break;
        }
        uint32_t retention_h = (maxRecords * interval_s) / 3600UL;
        uint32_t clampHours  = hours;
        if (clampHours == 0 || clampHours > retention_h) clampHours = retention_h;
        if (clampHours > 36) clampHours = 36;
        struct tm tmnow;
        if (!getLocalTime(&tmnow, 1)) {
          DynamicJsonDocument resp(160);
          resp["type"] = "history_end";
          resp["idxTotal"] = 0;
          resp["hours"] = clampHours;
          resp["message"] = "Time not synced";
          String o; serializeJson(resp, o);
          client->text(o);
          break;
        }
        time_t nowT = mktime(&tmnow);
        uint32_t nowEpoch = (uint32_t)nowT;
        uint32_t fromEpoch = nowEpoch - clampHours * 3600UL;
        File f = LittleFS.open(HISTORY_FILE, "r");
        if (!f) {
          DynamicJsonDocument resp(160);
          resp["type"] = "history_end";
          resp["idxTotal"] = 0;
          resp["hours"] = clampHours;
          resp["message"] = "No history file";
          String o; serializeJson(resp, o);
          client->text(o);
          break;
        }
        HistoryHeaderLocal hdr;
        if (f.readBytes((char*)&hdr, sizeof(hdr)) != (int)sizeof(hdr) ||
            hdr.magic != 0x53434854UL || hdr.version != 0x0001) {
          f.close();
          DynamicJsonDocument resp(160);
          resp["type"] = "history_end";
          resp["idxTotal"] = 0;
          resp["hours"] = clampHours;
          resp["message"] = "Bad header";
          String o; serializeJson(resp, o);
          client->text(o);
          break;
        }
        uint32_t validCount = hdr.count;
        uint32_t fileMaxRecs = hdr.maxRecords;
        uint32_t oldestIndex = (hdr.head + fileMaxRecs - validCount) % fileMaxRecs;
        HistoryRecordLocal rec;
        uint32_t startOffset = 0;
        bool foundStart = false;
        for (uint32_t i2 = 0; i2 < validCount; ++i2) {
          uint32_t idx = (oldestIndex + i2) % fileMaxRecs;
          uint32_t pos = sizeof(hdr) + idx * sizeof(rec);
          if (!f.seek(pos, SeekSet)) break;
          if (f.readBytes((char*)&rec, sizeof(rec)) != (int)sizeof(rec)) break;
          if (rec.epoch >= fromEpoch) {
            startOffset = i2; foundStart = true; break;
          }
        }
        if (!foundStart) {
          f.close();
          DynamicJsonDocument resp(160);
          resp["type"] = "history_end";
          resp["idxTotal"] = 0;
          resp["hours"] = clampHours;
          resp["message"] = "No history in window";
          String o; serializeJson(resp, o);
          client->text(o);
          break;
        }
        const uint32_t CHUNK_POINTS = 128;
        uint32_t totalEmitted = 0;
        for (uint32_t i2 = startOffset; i2 < validCount;) {
          String chunkArr; chunkArr.reserve(2048);
          chunkArr += "[";
          bool firstPoint = true;
          uint32_t batchCount = 0;
          uint32_t localIdxStart = totalEmitted;
          while (i2 < validCount && batchCount < CHUNK_POINTS) {
            uint32_t idx = (oldestIndex + i2) % fileMaxRecs;
            uint32_t pos = sizeof(hdr) + idx * sizeof(rec);
            if (!f.seek(pos, SeekSet)) break;
            if (f.readBytes((char*)&rec, sizeof(rec)) != (int)sizeof(rec)) break;
            if (rec.epoch < fromEpoch) { ++i2; continue; }
            if (!firstPoint) chunkArr += ",";
            firstPoint = false;
            chunkArr += "{\"t\":" + String(rec.epoch) +
                        ",\"vwc\":" + String(rec.vwc,3) +
                        ",\"pwec\":" + String(rec.pwec,3) +
                        ",\"temp\":" + String(rec.temp_c,2) + "}";
            ++batchCount; ++totalEmitted; ++i2;
          }
          chunkArr += "]";
          if (batchCount == 0 || chunkArr == "[]") break;
          DynamicJsonDocument doc(256);
          doc["type"] = "history_chunk";
          doc["idxStart"] = localIdxStart;
          doc["idxEnd"] = localIdxStart + batchCount - 1;
          doc["hours"] = clampHours;
          doc["data"] = serialized(chunkArr);
          String outStr; serializeJson(doc, outStr);
          client->text(outStr);
          delay(0);
        }
        f.close();
        DynamicJsonDocument endDoc(160);
        endDoc["type"] = "history_end";
        endDoc["idxTotal"] = totalEmitted;
        endDoc["hours"] = clampHours;
        String endOut; serializeJson(endDoc, endOut);
        client->text(endOut);
      } else if (strcmp(msgType, "get_history_meta") == 0) {
        String meta = History::getMetaJSON();
        client->text(meta);
      } else if (strcmp(msgType, "debug_add_point") == 0) {
        bool ok = History::addPoint(ActiveSensor::getVWC(), ActiveSensor::getPwec(), ActiveSensor::getTempC());
        DynamicJsonDocument r(128);
        r["type"] = "debug_add_point";
        r["success"] = ok;
        r["message"] = ok ? "History point added" : "Failed to add point";
        String out; serializeJson(r, out);
        client->text(out);
      } else if (strcmp(msgType, "update_wifi") == 0) {
        const char* ssid = j["ssid"] | "";
        const char* pass = j["password"] | "";
        if (strlen(ssid) == 0) {
          DynamicJsonDocument r(128);
          r["type"] = "wifi_update";
          r["success"] = false;
          r["message"] = "SSID required.";
          String out; serializeJson(r, out);
          Globals::ws.textAll(out);
        } else {
          Globals::prefs.begin(NS_WIFI, false);
          Globals::prefs.putString("ssid", ssid);
          Globals::prefs.putString("password", pass);
          Globals::prefs.end();
          WifiManager::staSSID = ssid;
          WifiManager::staPASS = pass;
          DynamicJsonDocument r(128);
          r["type"] = "wifi_update";
          r["success"] = true;
          r["message"] = "WiFi saved. Rebooting...";
          String out; serializeJson(r, out);
          Globals::ws.textAll(out);
          Globals::restartAt = millis() + 500;
        }
      } else if (strcmp(msgType, "wifi_retry_now") == 0) {
        WifiManager::wifiRetryNow();
        DynamicJsonDocument r(128);
        r["type"] = "wifi_update";
        r["success"] = true;
        r["message"] = "Retrying STA now.";
        String out; serializeJson(r, out);
        Globals::ws.textAll(out);
      } else if (strcmp(msgType, "update_mqtt") == 0) {
        mqttServer = String(j["server"] | mqttServer.c_str());
        mqttPort = j["port"] | mqttPort;
        mqttUsername = String(j["user"] | mqttUsername.c_str());
        mqttPassword = String(j["pass"] | mqttPassword.c_str());
        Globals::prefs.begin(NS_MQTT, false);
        Globals::prefs.putString("mqttServer", mqttServer);
        Globals::prefs.putInt("mqttPort", mqttPort);
        Globals::prefs.putString("mqttUsername", mqttUsername);
        Globals::prefs.putString("mqttPassword", mqttPassword);
        Globals::prefs.end();
        Globals::mqtt.disconnect();
        DynamicJsonDocument r(128);
        r["type"] = "mqtt_update";
        r["success"] = true;
        r["message"] = "MQTT saved. Reconnecting...";
        String out; serializeJson(r, out);
        Globals::ws.textAll(out);
        WebUI::sendStatusWS();
      } else if (strcmp(msgType, "update_mqtt_topic") == 0) {
        mqttTopicUser = String(j["topic"] | mqttTopicUser.c_str());
        Globals::prefs.begin(NS_MQTT, false);
        Globals::prefs.putString("topic", mqttTopicUser);
        Globals::prefs.end();
        DynamicJsonDocument r(128);
        r["type"] = "mqtt_topic_update";
        r["success"] = true;
        r["message"] = "MQTT topic updated.";
        String out; serializeJson(r, out);
        Globals::ws.textAll(out);
        WebUI::sendStatusWS();
      } else if (strcmp(msgType, "toggle_mqtt") == 0) {
        bool en = j["enabled"] | false;
        mqttEnabled = en;
        Globals::prefs.begin(NS_MQTT, false);
        Globals::prefs.putBool("mqtt_enabled", mqttEnabled);
        Globals::prefs.end();
        if (!mqttEnabled && Globals::mqtt.connected()) {
          String willTopic = Globals::baseTopic + "/status";
          Globals::mqtt.publish(willTopic.c_str(), "offline", true);
          Globals::mqtt.disconnect();
          Globals::webDebug("[MQTT] Disabled by user", true);
        }
        DynamicJsonDocument r(160);
        r["type"] = "mqtt_state";
        r["enabled"] = mqttEnabled;
        r["message"] = mqttEnabled ? "MQTT enabled" : "MQTT disabled";
        String out; serializeJson(r, out);
        Globals::ws.textAll(out);
        WebUI::sendStatusWS();
      } else if (strcmp(msgType, "update_publish_mode") == 0) {                     // [ADDED]
        const char* modeStr = j["mode"] | "";                                       // [ADDED]
        PublishMode newMode = (strcmp(modeStr, "delta") == 0) ? PUBLISH_DELTA       // [ADDED]
                            : PUBLISH_ALWAYS;                                       // [ADDED]
        float newVwc  = j["vwc_delta"]  | deltaVWC;                                 // [ADDED]
        float newTemp = j["temp_delta"] | deltaTempC;                               // [ADDED]
        float newEc   = j["ec_delta"]   | deltaECuS;                                // [ADDED]
        float newPwec = j["pwec_delta"] | deltaPWEC;                                // [ADDED]
        if (newVwc  < 0) newVwc = 0;                                                // [ADDED]
        if (newTemp < 0) newTemp = 0;                                               // [ADDED]
        if (newEc   < 0) newEc = 0;                                                 // [ADDED]
        if (newPwec < 0) newPwec = 0;                                               // [ADDED]
        publishMode = newMode;                                                      // [ADDED]
        deltaVWC    = newVwc;                                                       // [ADDED]
        deltaTempC  = newTemp;                                                      // [ADDED]
        deltaECuS   = newEc;                                                        // [ADDED]
        deltaPWEC   = newPwec;                                                      // [ADDED]
        Globals::prefs.begin(NS_MQTT, false);                                       // [ADDED]
        Globals::prefs.putString("pub_mode", (publishMode == PUBLISH_DELTA) ? "delta" : "always"); // [ADDED]
        Globals::prefs.putFloat("d_vwc",  deltaVWC);                                // [ADDED]
        Globals::prefs.putFloat("d_temp", deltaTempC);                              // [ADDED]
        Globals::prefs.putFloat("d_ec",   deltaECuS);                               // [ADDED]
        Globals::prefs.putFloat("d_pwec", deltaPWEC);                               // [ADDED]
        Globals::prefs.end();                                                       // [ADDED]
        MQTTManager::resetPublishCache();                                           // [ADDED]
        DynamicJsonDocument r(192);                                                 // [ADDED]
        r["type"] = "publish_mode_update";                                          // [ADDED]
        r["success"] = true;                                                        // [ADDED]
        r["message"] = "Publish mode updated.";                                     // [ADDED]
        r["publishMode"] = (publishMode == PUBLISH_DELTA ? "delta" : "always");     // [ADDED]
        JsonObject thr = r.createNestedObject("thresholds");                        // [ADDED]
        thr["vwc"] = deltaVWC;                                                      // [ADDED]
        thr["temp"] = deltaTempC;                                                   // [ADDED]
        thr["ec"] = deltaECuS;                                                      // [ADDED]
        thr["pwec"] = deltaPWEC;                                                    // [ADDED]
        String out; serializeJson(r, out);                                          // [ADDED]
        Globals::ws.textAll(out);                                                   // [ADDED]
        WebUI::sendStatusWS();                                                      // [ADDED]
        WebUI::sendConfigWS();                                                      // [ADDED]
      } else if (strcmp(msgType, "set_sensor_number") == 0) {
        int id = j["id"] | 0;
        if (id < 1 || id > 255) {
          DynamicJsonDocument r(128);
          r["type"] = "sensor_number_update";
          r["success"] = false;
          r["message"] = "Sensor # must be 1..255";
          String out; serializeJson(r, out);
          Globals::ws.textAll(out);
        } else {
          sensorNumber = (uint8_t)id;
          Globals::prefs.begin(NS_SENSOR, false);
          Globals::prefs.putUChar("sensor_number", sensorNumber);
          Globals::prefs.end();
          DynamicJsonDocument r(128);
          r["type"] = "sensor_number_update";
          r["success"] = true;
          r["id"] = sensorNumber;
          r["message"] = "Sensor # saved.";
          String out; serializeJson(r, out);
          Globals::ws.textAll(out);
          WebUI::sendStatusWS();
        }
      } else if (strcmp(msgType, "set_mqtt_features") == 0) {
        allowStructured = j["allow_structured"] | allowStructured;
        Globals::prefs.begin(NS_MQTT, false);
        Globals::prefs.putBool("allow_structured", allowStructured);
        Globals::prefs.end();
        DynamicJsonDocument r(128);
        r["type"] = "mqtt_features_update";
        r["success"] = true;
        r["message"] = "Structured topics updated.";
        String out; serializeJson(r, out);
        Globals::ws.textAll(out);
        WebUI::sendStatusWS();
      } else if (strcmp(msgType, "update_sensor_interval") == 0) {
        uint32_t ms = j["ms"] | 0;
        DynamicJsonDocument r(160);
        r["type"] = "sensor_interval_update";
        if (ms < 1000 || ms > 60000) {
          r["success"] = false;
          r["message"] = "Interval must be 1000..60000 ms";
          r["sensorIntervalMs"] = sensorIntervalMs;
          String out; serializeJson(r, out);
          Globals::ws.textAll(out);
        } else {
          sensorIntervalMs = ms;
          Globals::prefs.begin(NS_SENSOR, false);
          Globals::prefs.putUInt("sensor_int_ms", sensorIntervalMs);
          Globals::prefs.end();
          r["success"] = true;
          r["message"] = "Sensor poll interval updated.";
          r["sensorIntervalMs"] = sensorIntervalMs;
          String out; serializeJson(r, out);
          Globals::ws.textAll(out);
          WebUI::sendStatusWS();
          WebUI::sendConfigWS();
        }
      } else if (strcmp(msgType, "enable_ha") == 0) {
        haEnabled = j["enabled"] | haEnabled;
        Globals::prefs.begin(NS_MQTT, false);
        Globals::prefs.putBool("ha_enabled", haEnabled);
        Globals::prefs.end();
        DynamicJsonDocument r(128);
        r["type"] = "ha_state";
        r["enabled"] = haEnabled;
        r["message"] = "HA discovery state updated.";
        String out; serializeJson(r, out);
        Globals::ws.textAll(out);
        if (haEnabled && Globals::mqtt.connected()) {
          String mac = Globals::macStr; mac.replace(":", ""); mac.toUpperCase();
          String haBaseId = (mac.length() >= 6) ? mac.substring(mac.length() - 6) : mac;
          String prefix = (sensorType == SENSOR_MEC20) ? "mec20/" : "thcs/"; // [ADDED]
          String haStateBase = prefix + haBaseId + "/sensor";               // [UPDATED]
          MQTTManager::announceHA(haBaseId, haStateBase);
        }
      } else if (strcmp(msgType, "ha_announce_now") == 0) {
        DynamicJsonDocument r(160);
        bool ok = false;
        if (Globals::mqtt.connected() && haEnabled) {
          String mac = Globals::macStr; mac.replace(":", ""); mac.toUpperCase();
          String haBaseId = (mac.length() >= 6) ? mac.substring(mac.length() - 6) : mac;
          String prefix = (sensorType == SENSOR_MEC20) ? "mec20/" : "thcs/"; // [ADDED]
          String haStateBase = prefix + haBaseId + "/sensor";               // [UPDATED]
          ok = MQTTManager::announceHA(haBaseId, haStateBase);
        }
        r["type"] = "ha_state";
        r["enabled"] = haEnabled;
        r["message"] = ok ? "HA discovery published." : "HA discovery not published.";
        String out; serializeJson(r, out);
        Globals::ws.textAll(out);
      } else if (strcmp(msgType, "update_calibration") == 0) {
        if (j.containsKey("slope") && j.containsKey("intercept") && j.containsKey("tempcoeff")) {
          EC_SLOPE = j["slope"].as<float>();
          EC_INTERCEPT = j["intercept"].as<float>();
          EC_TEMP_COEFF = j["tempcoeff"].as<float>();
          Globals::prefs.begin(NS_SENSOR, false);
          Globals::prefs.putFloat("EC_SLOPE", EC_SLOPE);
          Globals::prefs.putFloat("EC_INTERCEPT", EC_INTERCEPT);
          Globals::prefs.putFloat("EC_TEMP_COEFF", EC_TEMP_COEFF);
          Globals::prefs.end();
          DynamicJsonDocument r(128);
          r["type"] = "calibration_update";
          r["success"] = true;
          r["message"] = "Calibration saved.";
          String out; serializeJson(r, out);
          Globals::ws.textAll(out);
        } else {
          DynamicJsonDocument r(128);
          r["type"] = "calibration_update";
          r["success"] = false;
          r["message"] = "Missing fields.";
          String out; serializeJson(r, out);
          Globals::ws.textAll(out);
        }
      } else if (strcmp(msgType, "temp_unit") == 0) {
        bool f = j["fahrenheit"] | false;
        useFahrenheit = f;
        Globals::prefs.begin(NS_SENSOR, false);
        Globals::prefs.putBool("USE_FAHRENHEIT", useFahrenheit);
        Globals::prefs.end();
        DynamicJsonDocument r(128);
        r["type"] = "temp_unit_changed";
        r["success"] = true;
        r["fahrenheit"] = useFahrenheit;
        r["message"] = "Temperature unit updated.";
        String out; serializeJson(r, out);
        Globals::ws.textAll(out);
      } else if (strcmp(msgType, "update_time") == 0) {
        long off = j["gmt_offset_sec"] | gmtOffset_sec;
        int dst = j["dst_offset_sec"] | daylightOffset_sec;
        gmtOffset_sec = off;
        daylightOffset_sec = dst;
        Globals::prefs.begin(NS_SYSTEM, false);
        Globals::prefs.putInt("tz_offset", gmtOffset_sec);
        Globals::prefs.putInt("dst_offset", daylightOffset_sec);
        if (j.containsKey("tz_name")) {
          const char* tzTmp = j["tz_name"] | "";
          tzName = String(tzTmp);
          Globals::prefs.putString("tz_name", tzName);
        }
        Globals::prefs.end();
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        DynamicJsonDocument r(192);
        r["type"] = "time_update";
        r["success"] = true;
        r["timezone_offset"] = gmtOffset_sec;
        r["dst_offset"] = daylightOffset_sec;
        r["tz_name"] = tzName;
        r["tz_offset_str"] = fmtUtcOffsetStr((long)gmtOffset_sec + (long)daylightOffset_sec);
        r["message"] = "Time settings applied.";
        String out; serializeJson(r, out);
        Globals::ws.textAll(out);
        WebUI::sendStatusWS();
      } else if (strcmp(msgType, "clear_history") == 0) {
        History::clear();
        DynamicJsonDocument r(128);
        r["type"] = "history_cleared";
        r["success"] = true;
        r["message"] = "History cleared.";
        String out; serializeJson(r, out);
        Globals::ws.textAll(out);
      } else if (strcmp(msgType, "reboot_device") == 0) {
        DynamicJsonDocument r(128);
        r["type"] = "reboot_initiated";
        r["message"] = "Rebooting...";
        String out; serializeJson(r, out);
        Globals::ws.textAll(out);
        Globals::restartAt = millis() + 300;
      } else if (strcmp(msgType, "factory_reset") == 0) {
        DynamicJsonDocument r(128);
        r["type"] = "reset_initiated";
        r["message"] = "Factory reset...";
        String out; serializeJson(r, out);
        Globals::ws.textAll(out);
        WifiManager::doFactoryReset = true;
      } else if (strcmp(msgType, "toggle_debug_mode") == 0) {
        bool en = j["enabled"] | false;
        debugMode = en;
        Globals::prefs.begin(NS_SYSTEM, false);
        Globals::prefs.putBool("debug", debugMode);
        Globals::prefs.end();
        DynamicJsonDocument r(128);
        r["type"] = "debug_mode_state";
        r["enabled"] = debugMode;
        r["message"] = debugMode ? "Debug Mode enabled" : "Debug Mode disabled";
        String out; serializeJson(r, out);
        Globals::ws.textAll(out);
        WebUI::sendStatusWS();
      } else if (strcmp(msgType, "toggle_webserial") == 0) {
        bool en = j["enabled"] | false;
        webSerialEnabled = en;
        Globals::prefs.begin(NS_SYSTEM, false);
        Globals::prefs.putBool("webserial", webSerialEnabled);
        Globals::prefs.end();
        if (webSerialEnabled) Globals::ensureWebSerial();
        else Globals::stopWebSerial();
        DynamicJsonDocument r(160);
        r["type"] = "webserial_state";
        r["enabled"] = webSerialEnabled;
        r["message"] = webSerialEnabled ? "WebSerial enabled" : "WebSerial disabled";
        String out; serializeJson(r, out);
        Globals::ws.textAll(out);
        WebUI::sendStatusWS();
      } else if (strcmp(msgType, "ping") == 0) {
        DynamicJsonDocument r(96);
        r["type"] = "pong";
        if (j.containsKey("t")) r["t"] = j["t"];
        String out; serializeJson(r, out);
        Globals::ws.textAll(out);
      }
    } break;
    case WS_EVT_PONG: break;
    case WS_EVT_ERROR: break;
  }
}

static void setupRoutes() {
  Globals::server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    extern const char index_html[] PROGMEM;
    extern const size_t index_html_len;
    AsyncWebServerResponse* resp =
        req->beginResponse_P(200, "text/html",
                             reinterpret_cast<const uint8_t*>(index_html),
                             index_html_len);
    req->send(resp);
  });

  Globals::server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* req) {
    DynamicJsonDocument d(512);
    d["fw"] = FW_NAME;
    d["ver"] = FW_VERSION;
    d["wifi_connected"] = (WifiManager::wifiState == WIFI_STA_CONNECTED && WiFi.status() == WL_CONNECTED);
    d["ip"] = (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "N/A");
    d["mqtt_connected"] = Globals::mqtt.connected();
    d["mqtt_enabled"] = mqttEnabled;
    uint32_t up = (uint32_t)(millis() / 1000);
    d["uptime_s"] = up;
    d["uptime_human"] = Utils::uptimeHuman(up);
    d["history_enabled"] = true;                       // always enabled
    d["sensor_type"] = (sensorType == SENSOR_MEC20 ? "mec20" : "thcs");
    String out; serializeJson(d, out);
    req->send(200, "application/json", out);
  });

  Globals::server.on("/download/history", HTTP_GET, [](AsyncWebServerRequest* req) {
    uint32_t hours = 168;
    if (req->hasParam("hours")) {
      String h = req->getParam("hours")->value();
      uint32_t hv = (uint32_t)h.toInt();
      if (hv > 0) hours = hv;
    }
    if (hours > 168) hours = 168;
    AsyncResponseStream *resp = req->beginResponseStream("text/csv; charset=utf-8");
    const char* fn = (sensorType == SENSOR_MEC20 ? "history_mec20.csv" : "history_thcs.csv");
    resp->addHeader("Content-Disposition", String("attachment; filename=\"") + fn + "\"");
    History::streamCSV(hours, *resp);
    req->send(resp);
  });

  Globals::server.on("/api/history/meta", HTTP_GET, [](AsyncWebServerRequest* req) {
    String meta = History::getMetaJSON();
    req->send(200, "application/json", meta);
  });

  Globals::server.on("/api/wifi_scan_start", HTTP_POST, [](AsyncWebServerRequest* req) {
    WifiScan::begin();
    bool started = WifiScan::startAsync(true, false, 0);
    int syncCount = -1;
    String message;
    if (started) {
      message = "Async scan started";
    } else {
      Globals::webDebug("[WiFi] Async scan failed; attempting synchronous scan", true);
      if (!(WiFi.getMode() & WIFI_STA)) {
        WiFi.mode((WifiManager::wifiState == WIFI_AP_ACTIVE) ? WIFI_AP_STA : WIFI_STA);
      }
      int res = WiFi.scanNetworks(false, true);
      if (res >= 0) {
        started = true;
        syncCount = res;
        message = "Synchronous scan completed";
      } else {
        message = "Failed to start scan";
      }
    }
    DynamicJsonDocument d(256);
    d["started"] = started;
    if (syncCount >= 0) d["count"] = syncCount;
    d["message"] = message;
    String out; serializeJson(d, out);
    req->send(200, "application/json", out);
  });

  Globals::server.on("/api/wifi_scan_status", HTTP_GET, [](AsyncWebServerRequest* req) {
    DynamicJsonDocument d(4096);
    if (WifiScan::getStatus() != WifiScan::IDLE) {
      WifiScan::buildStatusJSON(d);
    } else {
      int st = WiFi.scanComplete();
      if (st == -1) {
        d["status"] = "scanning";
      } else if (st >= 0) {
        d["status"] = "done";
        d["count"] = st;
        JsonArray arr = d.createNestedArray("aps");
        for (int i = 0; i < st; ++i) {
          JsonObject o = arr.createNestedObject();
          o["ssid"] = WiFi.SSID(i);
          o["rssi"] = WiFi.RSSI(i);
          o["chan"] = WiFi.channel(i);
          o["bssid"] = WiFi.BSSIDstr(i);
          int enc = WiFi.encryptionType(i);
          o["enc"] = (enc == WIFI_AUTH_OPEN ? "OPEN" : "SECURED");
        }
        WiFi.scanDelete();
      } else {
        d["status"] = "idle";
      }
    }
    String out; serializeJson(d, out);
    req->send(200, "application/json", out);
  });

  Globals::ws.onEvent(wsOnEvent);
  Globals::server.addHandler(&Globals::ws);

  if (webSerialEnabled) Globals::ensureWebSerial();
  Globals::server.begin();
  Globals::webDebug("[HTTP] Server started", true);
  Serial.println("[HTTP] Server started");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[HTTP] Local IP: "); Serial.println(WiFi.localIP().toString());
  } else {
    Serial.println("[HTTP] WiFi not connected at server start");
  }
}

void WebUI::begin() {
  setupRoutes();
  String resolvedMac = resolveMacOrFallback();
  Serial.print("[WebUI] Resolved MAC: ");
  Serial.println(resolvedMac);
}

void WebUI::loop() {
  static uint32_t lastWSsensor = 0, lastWSstatus = 0;
  uint32_t now = millis();
  if (now - lastWSsensor >= 2000) {
    WebUI::sendSensorWS();
    lastWSsensor = now;
  }
  if (now - lastWSstatus >= 5000) {
    WebUI::sendStatusWS();
    lastWSstatus = now;
  }
}