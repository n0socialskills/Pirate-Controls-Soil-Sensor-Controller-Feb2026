#include "ActiveSensor.h"
#include "Config.h"
#include "Globals.h"
#include "THCSensor.h"
#include "MEC20Sensor.h" // [ADDED]
#include "MQTTManager.h" // [ADDED] reset MQTT delta cache on driver switch

namespace ActiveSensor {

// [UPDATED] function pointer facade to avoid repeated sensorType branches on every getter
static void (*_begin)() = nullptr;
static void (*_loop)() = nullptr;
static float (*_getVWC)() = nullptr;
static float (*_getTempC)() = nullptr;
static float (*_getPwec)() = nullptr;
static float (*_getECuS)() = nullptr;
static float (*_getRawEC)() = nullptr;
static bool  (*_hasError)() = nullptr;
static bool  (*_heartbeat)() = nullptr;

void begin() {
  if (sensorType == SENSOR_MEC20) {
    Globals::webDebug("[ActiveSensor] Init MEC20 driver", true);
    _begin = MEC20Sensor::begin;
    _loop = MEC20Sensor::loop;
    _getVWC = MEC20Sensor::getVWC;
    _getTempC = MEC20Sensor::getTempC;
    _getPwec = MEC20Sensor::getPwec;
    _getECuS = MEC20Sensor::getECuS;
    _getRawEC = MEC20Sensor::getRawEC;
    _hasError = MEC20Sensor::hasError;
    _heartbeat = MEC20Sensor::heartbeat;
  } else {
    Globals::webDebug("[ActiveSensor] Init THCS driver", true);
    _begin = THCSensor::begin;
    _loop = THCSensor::loop;
    _getVWC = THCSensor::getVWC;
    _getTempC = THCSensor::getTempC;
    _getPwec = THCSensor::getPwec;
    _getECuS = THCSensor::getECuS;
    _getRawEC = THCSensor::getRawEC;
    _hasError = THCSensor::hasError;
    _heartbeat = THCSensor::heartbeat;
  }
  MQTTManager::resetPublishCache();          // [ADDED] ensure next MQTT publish is not suppressed after sensor swap
  if (_begin) _begin(); // [UPDATED] call through pointer for consistency
}

void loop() {
  if (_loop) _loop();
}

float getVWC()    { return _getVWC ? _getVWC() : 0.0f; }
float getTempC()  { return _getTempC ? _getTempC() : 0.0f; }
float getPwec()   { return _getPwec ? _getPwec() : 0.0f; }
float getECuS()   { return _getECuS ? _getECuS() : 0.0f; }
float getRawEC()  { return _getRawEC ? _getRawEC() : 0.0f; }
bool  hasError()  { return _hasError ? _hasError() : true; }
bool  heartbeat() { return _heartbeat ? _heartbeat() : false; }

}