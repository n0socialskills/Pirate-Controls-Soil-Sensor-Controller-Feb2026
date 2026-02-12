#include "Config.h"
#include <Arduino.h>
#include <Preferences.h>
#include "Globals.h"
#include "WifiManager.h"

// NTP
const char* ntpServer = "pool.ntp.org";

// Time defaults
long gmtOffset_sec = -4L * 3600L;
int  daylightOffset_sec = 3600;

// WiFi timing/AP defaults
const uint32_t WIFI_CONNECT_WINDOW_MS = 180000;
const uint32_t WIFI_AP_RETRY_PERIOD_MS = 300000;
const uint32_t AP_DISABLE_DELAY_MS = 30000;
const char* AP_SSID_DEFAULT = "PC Sensor Controller Setup"; // [UPDATED]
const char* AP_PASSWORD_DEFAULT = "Controls";         // [UPDATED]

// History
const uint32_t HISTORY_INTERVAL_MS = 300000;
const size_t   HISTORY_MAX_RECORDS = 2016;
const char*    HISTORY_FILE = "/hist_v1.bin";

// MQTT defaults
String mqttServer   = "192.168.0.1";
int    mqttPort     = 1883;
String mqttUsername = "";
String mqttPassword = "";
String mqttTopicUser = "sensor/thcs";
bool   allowStructured = true;
bool   haEnabled = false;
bool   mqttEnabled = true;

// Calibration defaults
float EC_SLOPE = 1.00f;
float EC_INTERCEPT = 0.00f;
float EC_TEMP_COEFF = 0.00f;

// Fahrenheit toggle
bool useFahrenheit = false;

// Publish mode/thresholds
PublishMode publishMode = PUBLISH_ALWAYS;
float deltaVWC = 0.1f;
float deltaTempC = 0.2f;
float deltaECuS = 5.0f;
float deltaPWEC = 0.02f;

// Sensor poll interval
uint32_t sensorIntervalMs = 5000;
bool historyEnabled = true;

// Debug + WebSerial
bool debugMode = false;
bool webSerialEnabled = false;

// Sensor number default
uint8_t sensorNumber = 1;

// Timezone name
String tzName = "";

// Preferences namespaces
const char* NS_WIFI   = "wifi_prefs";
const char* NS_MQTT   = "mqtt_prefs";
const char* NS_SENSOR = "sensor_prefs";
const char* NS_SYSTEM = "system_prefs";

// Sensor type default
SensorType sensorType = SENSOR_THCS;

void loadPreferences() {
  using namespace WifiManager;

  // WiFi
  Globals::prefs.begin(NS_WIFI, true);
  WifiManager::staSSID = Globals::prefs.getString("ssid", "");
  WifiManager::staPASS = Globals::prefs.getString("password", "");
  Globals::prefs.end();

  // MQTT
  Globals::prefs.begin(NS_MQTT, true);
  mqttServer = Globals::prefs.getString("mqttServer", mqttServer);
  mqttPort = Globals::prefs.getInt("mqttPort", mqttPort);
  mqttUsername = Globals::prefs.getString("mqttUsername", mqttUsername);
  mqttPassword = Globals::prefs.getString("mqttPassword", mqttPassword);
  mqttTopicUser = Globals::prefs.getString("topic", mqttTopicUser);
  allowStructured = Globals::prefs.getBool("allow_structured", allowStructured);
  haEnabled = Globals::prefs.getBool("ha_enabled", haEnabled);
  mqttEnabled = Globals::prefs.getBool("mqtt_enabled", mqttEnabled);
  {
    String pm = Globals::prefs.getString("pub_mode", "always");
    publishMode = (pm=="delta")?PUBLISH_DELTA:PUBLISH_ALWAYS;
  }
  deltaVWC = Globals::prefs.getFloat("d_vwc", deltaVWC);
  deltaTempC = Globals::prefs.getFloat("d_temp", deltaTempC);
  deltaECuS = Globals::prefs.getFloat("d_ec", deltaECuS);
  deltaPWEC = Globals::prefs.getFloat("d_pwec", deltaPWEC);
  Globals::prefs.end();

  // Sensor / Calibration / Unit
  Globals::prefs.begin(NS_SENSOR, true);
  EC_SLOPE = Globals::prefs.getFloat("EC_SLOPE", EC_SLOPE);
  EC_INTERCEPT = Globals::prefs.getFloat("EC_INTERCEPT", EC_INTERCEPT);
  EC_TEMP_COEFF = Globals::prefs.getFloat("EC_TEMP_COEFF", EC_TEMP_COEFF);
  useFahrenheit = Globals::prefs.getBool("USE_FAHRENHEIT", useFahrenheit);
  sensorIntervalMs = Globals::prefs.getUInt("sensor_int_ms", sensorIntervalMs);
  sensorNumber = Globals::prefs.getUChar("sensor_number", sensorNumber);
  sensorType = (SensorType)Globals::prefs.getInt("sensor_type", (int)sensorType);
  Globals::prefs.end();

  // System
  long oldGmtOffset = gmtOffset_sec;
  int oldDstOffset = daylightOffset_sec;
  
  Globals::prefs.begin(NS_SYSTEM, true);
  gmtOffset_sec = (long)Globals::prefs.getInt("tz_offset", (int)gmtOffset_sec);
  daylightOffset_sec = Globals::prefs.getInt("dst_offset", daylightOffset_sec);
  debugMode = Globals::prefs.getBool("debug", debugMode);
  webSerialEnabled = Globals::prefs.getBool("webserial", webSerialEnabled);
  tzName = Globals::prefs.getString("tz_name", tzName.c_str());
  // historyEnabled key no longer used (graph always enabled)
  if (gmtOffset_sec == oldGmtOffset && !Globals::prefs.isKey("tz_offset")) {
    Globals::webDebug("[Config] TZ offset not in prefs; using default: " + String(gmtOffset_sec));
  }
  if (daylightOffset_sec == oldDstOffset && !Globals::prefs.isKey("dst_offset")) {
    Globals::webDebug("[Config] DST offset not in prefs; using default: " + String(daylightOffset_sec));
  }
  Globals::prefs.end();
 
  Globals::webDebug("[Config] Preferences loaded successfully");
}
