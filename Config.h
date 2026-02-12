#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Build/Version
#define FW_NAME "PC_Soil_Sensor_Controller "
#define FW_VERSION "1.0"

#ifndef POST_TX_DELAY_MS
#define POST_TX_DELAY_MS 2
#endif

// Pins/Hardware
// RS485/Soil Sensor ESP32 S3 DevC Module Pin Setup
// RS485/Soil Sensor ESP32 30pin DevC Module Pin Setup

// NOTE: Update these to match your board wiring. Values below are for ESP32-S3 dev variants.
#define RS485_TX_PIN   16   //  ESP32-S3 TX2 20 ( 16 for ESP32s 30pin board)
#define RS485_RX_PIN   17   //  ESP32-S3 RX2 19 ( 17 for ESP32s 30pin board)
#define RS485_DE_PIN   18  //  ESP32-S3 DE  1 ( 18 for ESP32s 30pin board)
#define RS485_RE_PIN   19  //  ESP32-S3 RE  2 ( 19 for ESP32s 30pin board)
// Reset Button
#define FACTORY_RESET_PIN 13 // Active LOW, hold >= 10s

// OLED Display I2C Pins for ESP32-S3 (update if your board exposes different SDA/SCL)
#define I2C_SDA_PIN    21    // [UPDATED] ESP32-S3 8 ( 21 for ESP32s 30pin board) 
#define I2C_SCL_PIN    22    // [UPDATED] ESP32-S3 9 ( 22 for ESP32s 30pin board) 

// NTP
extern const char* ntpServer;

// Time defaults
extern long gmtOffset_sec;
extern int  daylightOffset_sec;

// WiFi AP/STA timings + defaults
extern const uint32_t WIFI_CONNECT_WINDOW_MS;
extern const uint32_t WIFI_AP_RETRY_PERIOD_MS;
extern const uint32_t AP_DISABLE_DELAY_MS;
extern const char* AP_SSID_DEFAULT;       // [UPDATED] "PC Sensor Controller Setup"
extern const char* AP_PASSWORD_DEFAULT;   // [UPDATED] "Controls"

// History storage
extern const uint32_t HISTORY_INTERVAL_MS;
extern const size_t   HISTORY_MAX_RECORDS;
extern const char*    HISTORY_FILE;

// MQTT defaults + HA + structured
extern String mqttServer;
extern int    mqttPort;
extern String mqttUsername;
extern String mqttPassword;
extern String mqttTopicUser;
extern bool   allowStructured;
extern bool   haEnabled;
extern bool   mqttEnabled;

// Calibration defaults
extern float EC_SLOPE;
extern float EC_INTERCEPT;
extern float EC_TEMP_COEFF;

// Fahrenheit toggle
extern bool useFahrenheit;

// Publish mode and thresholds
enum PublishMode { 
  PUBLISH_ALWAYS = 0,
  PUBLISH_DELTA  = 1
};
extern PublishMode publishMode;
extern float deltaVWC;
extern float deltaTempC;
extern float deltaECuS;
extern float deltaPWEC;

// Sensor poll interval (ms)
extern uint32_t sensorIntervalMs;

// Debug + WebSerial
extern bool debugMode;
extern bool webSerialEnabled;

// Preferences namespaces
extern const char* NS_WIFI;
extern const char* NS_MQTT;
extern const char* NS_SENSOR;
extern const char* NS_SYSTEM;

// Preferences loader
void loadPreferences();

// Sensor number used to suffix user MQTT topic as /sN
extern uint8_t sensorNumber;

// Human-friendly timezone name (persisted)
extern String tzName;

// Sensor type enum + global
enum SensorType {
  SENSOR_THCS = 0,
  SENSOR_MEC20 = 1
};
extern SensorType sensorType;

#endif
