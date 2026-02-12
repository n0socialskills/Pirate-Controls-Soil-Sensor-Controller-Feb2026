#pragma once
#include <Arduino.h>

namespace MQTTManager {
  bool announceHA(const String& haBaseId, const String& stateBase);
  void begin();
  void loop();
  void publishIfNeeded();
  void resetPublishCache(); // [ADDED] clears delta-mode previous values
}