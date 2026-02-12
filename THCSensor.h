/*
  THCSensor.h
  Non-blocking Modbus polling for original THCS sensor.
*/

#ifndef THCSENSOR_H
#define THCSENSOR_H

#include <Arduino.h>

namespace THCSensor {
  void begin();    // Initializes state + (now) sets RS485 baud back to 4800
  void loop();     // Advances Modbus state machine non-blocking

  float getVWC();
  float getTempC();
  float getPwec();
  float getECuS();
  float getRawEC();

  bool hasError();
  bool heartbeat();
}

#endif