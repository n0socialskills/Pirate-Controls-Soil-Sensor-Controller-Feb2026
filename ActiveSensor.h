#ifndef ACTIVESENSOR_H
#define ACTIVESENSOR_H

#include "Config.h"

// [ADDED] Router facade for currently selected sensor (THCS or MEC20).
namespace ActiveSensor {
  void begin();   // [ADDED] Initialize correct driver
  void loop();    // [ADDED] Poll active sensor

  float getVWC();     // [%]
  float getTempC();   // [°C]
  float getPwec();    // [dS/m]
  float getECuS();    // [µS/cm calibrated]
  float getRawEC();   // [µS/cm raw]

  bool hasError();    // [ADDED] error flag from active driver
  bool heartbeat();   // [ADDED] toggles on successful update
}

#endif