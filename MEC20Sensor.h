#ifndef MEC20SENSOR_H
#define MEC20SENSOR_H

#include <Arduino.h>

/*
  MEC20Sensor.h
  Driver for MEC20 Soil Sensor (Temp / VWC / EC).
  Reads registers via Modbus RTU Function 0x04:
    0x0000: Temperature (INT16) -> /100 °C
    0x0001: VWC (UINT16) -> /100 %
    0x0002: EC  (UINT16) -> µS/cm (no scaling)
  Slave address assumed 1. Baud 9600 8N1.
*/

namespace MEC20Sensor {
  void begin();   // Sets RS485 to 9600 8N1, initializes state
  void loop();    // Performs polling when interval elapsed

  float getVWC();
  float getTempC();
  float getPwec();
  float getECuS();
  float getRawEC();

  bool hasError();
  bool heartbeat();
}

#endif