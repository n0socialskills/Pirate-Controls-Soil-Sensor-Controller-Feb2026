#include "System.h"
#include "Globals.h"
#include "Config.h"
#include <LittleFS.h>
#include "WifiManager.h"

void factoryReset() {
  Globals::webDebug("[System] Factory Reset initiated", true);
  
  Globals::prefs.begin(NS_WIFI, false);
  if (!Globals::prefs.clear()) Globals::webDebug("[System] WARNING: Failed to clear NS_WIFI", true);
  Globals::prefs.end();

  Globals::prefs.begin(NS_MQTT, false);
  if (!Globals::prefs.clear()) Globals::webDebug("[System] WARNING: Failed to clear NS_MQTT", true);
  Globals::prefs.end();

  Globals::prefs.begin(NS_SENSOR, false);
  if (!Globals::prefs.clear()) Globals::webDebug("[System] WARNING: Failed to clear NS_SENSOR", true);
  Globals::prefs.end();

  Globals::prefs.begin(NS_SYSTEM, false);
  if (!Globals::prefs.clear()) Globals::webDebug("[System] WARNING: Failed to clear NS_SYSTEM", true);
  Globals::prefs.end();

  if (!Globals::ensureFS()) {                        // [UPDATED] avoid repeated begin(true)
    Globals::webDebug("[System] WARNING: LittleFS not mounted during factoryReset", true);
  }

  if (LittleFS.exists(HISTORY_FILE)) {
    if (!LittleFS.remove(HISTORY_FILE)) {
      Globals::webDebug("[System] WARNING: Failed to remove history file", true);
    } else {
      Globals::webDebug("[System] History file removed", true);
    }
  } else {
    Globals::webDebug("[System] History file does not exist (OK)", true);
  }

  Globals::webDebug("[System] Factory reset complete. Restarting...", true);
  delay(500);
  ESP.restart();
}