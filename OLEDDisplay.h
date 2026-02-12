#ifndef OLEDDISPLAY_H
#define OLEDDISPLAY_H

#include <Arduino.h>

namespace OLEDDisplay {
  void begin();
  void loop();
  void setApNotice(bool on);
  void setShowFactoryHold(bool on, uint32_t startMs = 0);
}

#endif