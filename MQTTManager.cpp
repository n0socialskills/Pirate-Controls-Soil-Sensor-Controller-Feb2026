#include "MQTTManager.h"
#include "Globals.h"
#include "Config.h"
#include "WifiManager.h"
#include "WifiScan.h"
#include "THCSensor.h"
#include "ActiveSensor.h"
#include "Utils.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <cmath>

namespace MQTTManager {
  bool mqttWasConnected = false;
}

static uint32_t nextMqttAttemptAt = 0;
const uint32_t MQTT_RETRY_MS = 5000;
const uint32_t MQTT_AUTH_FAIL_BACKOFF_MS = 30000;
static uint32_t lastMqttPub = 0;
static uint32_t lastScanDeferLog = 0;

// Track last published values for delta mode
static float sPrevVwc   = NAN;
static float sPrevTempC = NAN;
static float sPrevPwec  = NAN;
static float sPrevECuS  = NAN;

// Have we seen at least one successful (non-error) sensor reading?
static bool sSeenValidReading = false;

static String formatUptimeDDHHMMSS(uint32_t secs) {
  uint32_t days = secs / 86400;
  secs %= 86400;
  uint32_t hours = secs / 3600;
  secs %= 3600;
  uint32_t mins = secs / 60;
  uint32_t s = secs % 60;
  char buf[32];
  snprintf(buf, sizeof(buf), "%02u:%02u:%02u:%02u",
           (unsigned)days, (unsigned)hours, (unsigned)mins, (unsigned)s);
  return String(buf);
}

static String getPermanentMacFull() {
  static String cachedFull;
  if (cachedFull.length()) return cachedFull;
  uint64_t mac64 = ESP.getEfuseMac();
  uint8_t macBytes[6];
  macBytes[0] = (uint8_t)((mac64 >> 40) & 0xFF);
  macBytes[1] = (uint8_t)((mac64 >> 32) & 0xFF);
  macBytes[2] = (uint8_t)((mac64 >> 24) & 0xFF);
  macBytes[3] = (uint8_t)((mac64 >> 16) & 0xFF);
  macBytes[4] = (uint8_t)((mac64 >> 8) & 0xFF);
  macBytes[5] = (uint8_t)(mac64 & 0xFF);
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           macBytes[0], macBytes[1], macBytes[2],
           macBytes[3], macBytes[4], macBytes[5]);
  cachedFull = String(buf);
  Globals::macStr = cachedFull;
  return cachedFull;
}

static String getPermanentMac6() {
  String full = getPermanentMacFull();
  String noColons = full; noColons.replace(":", ""); noColons.toUpperCase();
  if (noColons.length() >= 6) return noColons.substring(noColons.length() - 6);
  return noColons;
}

static bool publishHASensorConfig(const String& uniqueId,
                                  const String& name,
                                  const String& stateTopic,
                                  const char* unit,
                                  const char* deviceClass,
                                  const char* valueTemplate) {
  if (!Globals::mqtt.connected()) return false;
  String discTopic = String("homeassistant/sensor/") + uniqueId + "/config";
  StaticJsonDocument<768> doc;
  doc["name"] = name;
  doc["unique_id"] = uniqueId;
  doc["state_topic"] = stateTopic;
  if (unit && unit[0]) doc["unit_of_measurement"] = unit;
  if (deviceClass && deviceClass[0]) doc["device_class"] = deviceClass;
  doc["availability_topic"] = Globals::baseTopic + "/status";
  doc["payload_available"] = "online";
  doc["payload_not_available"] = "offline";
  if (valueTemplate && valueTemplate[0]) doc["value_template"] = valueTemplate;

  JsonObject dev = doc.createNestedObject("device");
  JsonArray ids = dev.createNestedArray("identifiers");
  ids.add(String("thcs_") + getPermanentMac6());
  dev["manufacturer"] = "Pirate Controls";
  // Reflect active sensor type in model/name
  if (sensorType == SENSOR_MEC20) {
    dev["model"] = "MEC20 Soil Sensor";
    dev["name"]  = "MEC20 Sensor " + getPermanentMac6();
  } else {
    dev["model"] = "THCS V1 ESP32";
    dev["name"]  = "THCS Sensor " + getPermanentMac6();
  }
  dev["sw_version"] = String(FW_NAME) + " " + String(FW_VERSION);
  String payload; serializeJson(doc, payload);
  return Globals::mqtt.publish(discTopic.c_str(), payload.c_str(), true);
}

namespace MQTTManager {

bool announceHA(const String&, const String& stateBase) {
  if (!Globals::mqtt.connected()) {
    Globals::webDebug("[MQTT] announceHA skipped: MQTT not connected", true);
    return false;
  }
  if (!haEnabled) {
    Globals::webDebug("[MQTT] announceHA skipped: HA disabled", true);
    return false;
  }
  String mac6 = getPermanentMac6();
  String haStateBase = stateBase; // already <type>/<MAC6>/sensor
  String uidPrefix = String("thcs_") + mac6;
  const char* vwcTpl    = "{{ value_json.vwc | float }}";
  const char* tempTpl   = "{{ value_json.temp | float }}";
  const char* tempFTpl  = "{{ value_json.temp_f | default(((value_json.temp | float) * 9/5) + 32) | float }}";
  const char* ecTpl     = "{{ value_json.ec | float }}";
  const char* pwecTpl   = "{{ value_json.pwec | float }}";
  const char* uptimeTpl = "{{ value_json.uptime_str | default('00:00:00:00') }}";
  bool any = false;
  any |= publishHASensorConfig(uidPrefix + "_vwc",   "Soil VWC",      haStateBase, "%",     nullptr, vwcTpl);
  any |= publishHASensorConfig(uidPrefix + "_temp",  "Soil Temp (C)", haStateBase, "°C",    "temperature", tempTpl);
  any |= publishHASensorConfig(uidPrefix + "_temp_f","Soil Temp (F)", haStateBase, "°F",    nullptr, tempFTpl);
  any |= publishHASensorConfig(uidPrefix + "_ec",    "Bulk EC",       haStateBase, "µS/cm", nullptr, ecTpl);
  any |= publishHASensorConfig(uidPrefix + "_pwec",  "pwEC",          haStateBase, "dS/m",  nullptr, pwecTpl);
  any |= publishHASensorConfig(uidPrefix + "_uptime","Uptime",        haStateBase, "",      nullptr, uptimeTpl);
  Globals::webDebug("[MQTT] announceHA completed for " + mac6, true);
  return any;
}

} // namespace MQTTManager

// ===== Publish helper =====
static void publishStateForHA(float vwc, float tempC, float pwec, int ec_uS,
                              int raw_ec, bool sensorError) {
  DynamicJsonDocument d(384);
  d["vwc"] = vwc;
  d["temp"] = tempC;
  float pwecRounded = roundf(pwec * 100.0f) / 100.0f;
  d["pwec"] = pwecRounded;
  d["ec"] = ec_uS;
  float tempF = (tempC * 9.0f / 5.0f) + 32.0f;
  float tempFRounded = roundf(tempF * 10.0f) / 10.0f;
  d["temp_f"] = tempFRounded;
  d["raw_ec"] = raw_ec;
  d["error"] = sensorError;
  d["sensor_type"] = (sensorType == SENSOR_MEC20 ? "mec20" : "thcs");

  uint32_t up_s = (uint32_t)(millis() / 1000);
  d["uptime_str"] = formatUptimeDDHHMMSS(up_s);

  String payload;
  serializeJson(d, payload);
  if (debugMode) {
    Serial.println("[MQTT] publishStateForHA payload: " + payload);
  }

  // ----- User-configured base topic (always allowed when MQTT is enabled) -----
  if (mqttEnabled && Globals::mqtt.connected() && mqttTopicUser.length()) {
    String userBase = mqttTopicUser;
    int s1 = userBase.indexOf('/');
    if (s1 >= 0) {
      int s2 = userBase.indexOf('/', s1 + 1);
      String seg0 = userBase.substring(0, s1);
      String segRest = (s2 >= 0) ? userBase.substring(s2) : "";
      String mac6 = getPermanentMac6();
      String sensorSeg = (sensorType == SENSOR_MEC20) ? "mec20" : "thcs";
      userBase = seg0 + "/" + mac6 + "/" + sensorSeg + segRest;   // UPDATED: insert MAC6 in path
    }
    String eff = userBase;
    if (sensorNumber >= 1 && sensorNumber <= 255) eff += "/s" + String(sensorNumber);
    Globals::mqtt.publish(eff.c_str(), payload.c_str(), false);
  }

  // ----- HA state base: <type>/<MAC6>/sensor (only if HA discovery enabled) -----
  if (haEnabled && Globals::mqtt.connected()) {                         // [UPDATED] gate HA publishes behind haEnabled
    String mac6 = getPermanentMac6();
    String prefix = (sensorType == SENSOR_MEC20) ? "mec20/" : "thcs/";
    String haState = prefix + mac6 + "/sensor";
    Globals::mqtt.publish(haState.c_str(), payload.c_str(), false);
  }

  // ----- Structured subtopics -----
  if (allowStructured && Globals::mqtt.connected()) {
    // User structured topics (always allowed when structured is enabled)
    String userStructured = mqttTopicUser;
    int s1u = userStructured.indexOf('/');
    if (s1u >= 0) {
      int s2u = userStructured.indexOf('/', s1u + 1);
      String seg0u = userStructured.substring(0, s1u);
      String segRestu = (s2u >= 0) ? userStructured.substring(s2u) : "";
      String mac6 = getPermanentMac6();
      String sensorSegu = (sensorType == SENSOR_MEC20) ? "mec20" : "thcs";
      userStructured = seg0u + "/" + mac6 + "/" + sensorSegu + segRestu; // UPDATED
    }
    if (sensorNumber >= 1 && sensorNumber <= 255 && userStructured.length()) userStructured += "/s" + String(sensorNumber);

    if (userStructured.length()) {
      Globals::mqtt.publish((userStructured + "/vwc").c_str(),  String(vwc,1).c_str(), false);
      Globals::mqtt.publish((userStructured + "/temp").c_str(), String(tempC,1).c_str(), false);
      Globals::mqtt.publish((userStructured + "/temp_f").c_str(), String(tempFRounded,1).c_str(), false);
      Globals::mqtt.publish((userStructured + "/ec").c_str(),   String(ec_uS,0).c_str(), false);
      Globals::mqtt.publish((userStructured + "/pwec").c_str(), String(pwecRounded,2).c_str(), false);
      Globals::mqtt.publish((userStructured + "/uptime_str").c_str(), formatUptimeDDHHMMSS(up_s).c_str(), false);
    }

    // HA structured topics only when HA is enabled
    if (haEnabled) {                                                   // [UPDATED] gate HA structured behind haEnabled
      String mac6 = getPermanentMac6();
      String prefix = (sensorType == SENSOR_MEC20) ? "mec20/" : "thcs/";
      String haStructured = prefix + mac6 + "/sensor";
      Globals::mqtt.publish((haStructured + "/vwc").c_str(),  String(vwc,1).c_str(), false);
      Globals::mqtt.publish((haStructured + "/temp").c_str(), String(tempC,1).c_str(), false);
      Globals::mqtt.publish((haStructured + "/temp_f").c_str(), String(tempFRounded,1).c_str(), false);
      Globals::mqtt.publish((haStructured + "/ec").c_str(),   String(ec_uS,0).c_str(), false);
      Globals::mqtt.publish((haStructured + "/pwec").c_str(), String(pwecRounded,2).c_str(), false);
      Globals::mqtt.publish((haStructured + "/uptime_str").c_str(), formatUptimeDDHHMMSS(up_s).c_str(), false);
    }
  }
}

namespace MQTTManager {

void resetPublishCache() {
  sPrevVwc = NAN;
  sPrevTempC = NAN;
  sPrevPwec = NAN;
  sPrevECuS = NAN;
  sSeenValidReading = false;
}

void begin() {
  Globals::mqtt.setKeepAlive(15);
  Globals::mqtt.setSocketTimeout(3);
  Globals::mqtt.setBufferSize(1024);
  if (mqttServer.length()) Globals::mqtt.setServer(mqttServer.c_str(), mqttPort);
  Globals::mqtt.setCallback([](char*, byte*, unsigned int){});
  getPermanentMacFull();
  if (!Globals::mqtt.connected() && mqttServer.length()) {
    delay(0);
    String clientId = String("thcs_") + (Globals::chipIdStr.length() ? Globals::chipIdStr : "esp");
    bool ok;
    String willTopic = Globals::baseTopic + "/status";
    if (mqttUsername.length()) {
      ok = Globals::mqtt.connect(clientId.c_str(), mqttUsername.c_str(), mqttPassword.c_str(),
                                 willTopic.c_str(), 0, true, "offline");
    } else {
      ok = Globals::mqtt.connect(clientId.c_str(),
                                 willTopic.c_str(), 0, true, "offline");
    }
    delay(0);
    if (ok) {
      Globals::mqtt.publish(willTopic.c_str(), "online", true);
      mqttWasConnected = true;
      if (haEnabled) {
        String mac6 = getPermanentMac6();
        String prefix = (sensorType == SENSOR_MEC20) ? "mec20/" : "thcs/";
        String haStateBase = prefix + mac6 + "/sensor";
        announceHA(mac6, haStateBase);
      }
    }
  }
}

static bool connectClient() {
  String willTopic = Globals::baseTopic + "/status";
  delay(0);
  String clientId = String("thcs_") + (Globals::chipIdStr.length() ? Globals::chipIdStr : "esp");
  bool ok;
  if (mqttUsername.length()) {
    ok = Globals::mqtt.connect(clientId.c_str(), mqttUsername.c_str(), mqttPassword.c_str(),
                               willTopic.c_str(), 1, true, "offline");
  } else {
    ok = Globals::mqtt.connect(clientId.c_str(), willTopic.c_str(), 1, true, "offline");
  }
  delay(0);
  if (ok) {
    Globals::mqtt.publish(willTopic.c_str(), "online", true);
    DynamicJsonDocument d(256);
    d["fw"] = FW_NAME; d["ver"] = FW_VERSION; d["build"] = (uint32_t)BUILD_UNIX_TIME;
    d["chipid"] = Globals::chipIdStr; d["mac"] = Globals::macStr;
    d["timezone_offset"] = gmtOffset_sec; d["dst_offset"] = daylightOffset_sec;
    String meta; serializeJson(d, meta);
    Globals::mqtt.publish((Globals::baseTopic + "/meta").c_str(), meta.c_str(), true);
    if (haEnabled) {
      String mac6 = getPermanentMac6();
      String prefix = (sensorType == SENSOR_MEC20) ? "mec20/" : "thcs/";
      String haStateBase = prefix + mac6 + "/sensor";
      announceHA(mac6, haStateBase);
    }
  }
  return ok;
}

void loop() {
  using namespace WifiManager;
  if (wifiState != WIFI_STA_CONNECTED || WiFi.status() != WL_CONNECTED) {
    if (Globals::mqtt.connected()) Globals::mqtt.disconnect();
    return;
  }
  const bool scanningIdf = WifiScan::isScanning();
  const bool scanningArduino = (WiFi.scanComplete() == -1);
  if (scanningIdf || scanningArduino) {
    nextMqttAttemptAt = millis() + MQTT_RETRY_MS;
    if (!Globals::mqtt.connected() && (millis() - lastScanDeferLog >= 10000)) {
      lastScanDeferLog = millis();
      Globals::webDebug("[MQTT] Deferring connect: WiFi scan in progress");
    }
    return;
  }
  if (!mqttEnabled) {
    if (Globals::mqtt.connected()) {
      String willTopic = Globals::baseTopic + "/status";
      Globals::mqtt.publish(willTopic.c_str(), "offline", true);
      Globals::mqtt.disconnect();
      Globals::webDebug("[MQTT] Disabled by user: offline + disconnect", true);
    }
    return;
  }
  if (Globals::mqtt.connected()) {
    Globals::mqtt.loop();
    return;
  }
  if (millis() < nextMqttAttemptAt) return;
  nextMqttAttemptAt = millis() + MQTT_RETRY_MS;
  if (mqttServer.length()) Globals::mqtt.setServer(mqttServer.c_str(), mqttPort);
  Globals::webDebug("[MQTT] Attempting connect to " + mqttServer + ":" + String(mqttPort));
  delay(0);
  if (connectClient()) {
    Globals::webDebug("[MQTT] Connected", true);
    mqttWasConnected = true;
  } else {
    int state = Globals::mqtt.state();
    if (state == 4 || state == 5) {
      Globals::webDebug("[MQTT] Auth failed rc=" + String(state) + " backoff 30s", true);
      nextMqttAttemptAt = millis() + MQTT_AUTH_FAIL_BACKOFF_MS;
    } else {
      Globals::webDebug("[MQTT] Connect failed rc=" + String(state));
      nextMqttAttemptAt = millis() + MQTT_RETRY_MS;
    }
  }
  delay(0);
}

void publishIfNeeded() {
  if (!mqttEnabled) return;
  if (!Globals::mqtt.connected()) return;
  uint32_t now = millis();
  if (now - lastMqttPub < sensorIntervalMs) return;
  lastMqttPub = now;

  float vwc = ActiveSensor::getVWC();
  float tempC = ActiveSensor::getTempC();
  float pwec = ActiveSensor::getPwec();
  float ecF = ActiveSensor::getECuS();
  int ec = Utils::safeRoundToInt(ecF, 0, 1000000);
  int raw = (int)roundf(ActiveSensor::getRawEC());
  bool err = ActiveSensor::hasError();

  // Delay first publish until we have a valid reading
  if (!sSeenValidReading) {
    if (err || !ActiveSensor::heartbeat()) {
      return;
    } else {
      sSeenValidReading = true;
    }
  }

  bool shouldPublish = (publishMode == PUBLISH_ALWAYS);
  if (!shouldPublish) {
    if (isnan(sPrevVwc) || isnan(sPrevTempC) || isnan(sPrevPwec) || isnan(sPrevECuS)) {
      shouldPublish = true;
    } else {
      float dvwc  = fabsf(vwc  - sPrevVwc);
      float dtemp = fabsf(tempC - sPrevTempC);
      float dpwec = fabsf(pwec - sPrevPwec);
      float dec   = fabsf((float)ec - sPrevECuS);
      shouldPublish = (dvwc  >= deltaVWC) ||
                      (dtemp >= deltaTempC) ||
                      (dec   >= deltaECuS) ||
                      (dpwec >= deltaPWEC);
    }
  }
  if (!shouldPublish) return;

  publishStateForHA(vwc, tempC, pwec, ec, raw, err);

  sPrevVwc   = vwc;
  sPrevTempC = tempC;
  sPrevPwec  = pwec;
  sPrevECuS  = (float)ec;
}

} // namespace MQTTManager
