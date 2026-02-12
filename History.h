#pragma once

#include <Arduino.h>

namespace History {
  void begin();
  bool addPoint(float vwc, float pwec, float temp_c);
  void clear();
  String getJSON(uint32_t hours);
  String getMetaJSON();
  void streamJSON(uint32_t hours, Print &out, size_t maxPoints = 0);
  void streamCSV(uint32_t hours, Print &out, size_t maxPoints = 0);
  void loop();
}