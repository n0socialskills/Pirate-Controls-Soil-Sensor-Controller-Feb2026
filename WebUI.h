#ifndef WEBUI_H
#define WEBUI_H

namespace WebUI {
  void begin();
  void loop();
  void sendStatusWS();
  void sendSensorWS();
  void sendConfigWS();
  void sendMetaWS();
}

#endif