# PC_THCS_V1 — ESP32 + THCS/MEC20 Soil Sensor Controller

> **Status:** BETA — Not Released  
> **Firmware:** `PC_THCS_V1` (Pirate Controls)  
> **Current firmware README version:**   
> **Author / Repo:** [@n0socialskills](https://github.com/n0socialskills) · [PC-THCS-V1-ESP32](https://github.com/n0socialskills/PC-THCS-V1-ESP32)  
> **Build timestamp token:** `BUILD_UNIX_TIME` (see `Config.h`)

---

## Attribution / Upstream References

This firmware is a modification of work found at:

- <https://github.com/kromadg/soil-sensor>  
- <https://www.beanbasement.nl/threads/diy-low-cost-tdr-moisture-content-temp-ec-substrate-sensor.18337/>  
- <https://scienceinhydroponics.com/2023/01/connecting-a-low-cost-tdr-moisture-content-ec-temp-sensor-to-a-nodemcuv3.html>

Thanks to those authors and communities for the original research and examples.

> **Disclaimer:** This code is provided **"AS IS"** without warranty of any kind, express or implied. Use at your own risk.

---

## Status: BETA — Not Released

- This firmware and Web UI are currently in **beta**. Use in **test environments only**.  
- Data formats and features may change before an official release.  
- Users and integrators should expect occasional breaking changes during the beta.  
- Please report issues and feedback via the repository issue tracker: https://github.com/n0socialskills/PC-THCS-V1-ESP32/issues

---

## Summary of Key Updates  

- Added MEC20 RS485 soil sensor support alongside the original THCS driver.  
- Introduced ActiveSensor router so THCS / MEC20 drivers can be selected at runtime.  
- Sensor Type selection is available in the Web UI (System → Sensor Configuration).  
- MEC20 driver implemented as a non‑blocking state machine and uses 9600 8N1.  
- THCS driver uses 4800 8N1; both drivers now configure the RS485 UART during driver begin().  
- RS485 UART initialization moved out of Globals::begin(); ActiveSensor/driver begin() fully configures UART to avoid interim wrong-baud reads during boot.  
- WiFiScan verbose esp-idf debug prints (Serial.printf) are gated by Debug Mode to reduce serial noise in production; enable Debug Mode to see detailed scan diagnostics.  
- History file capacity now uses the global Config constant (HISTORY_MAX_RECORDS) — removed duplicate hardcoded capacity in History.cpp.  
- Consolidated Modbus CRC and median-of-3 helpers into Utils.

---

## Table of Contents

1. [Quick Start](#1--quick-start)  
2. [Hardware, Pinout & Wiring](#2--hardware-pinout)  
3. [Build & Flash (Arduino / PlatformIO / Docker)](#3--build--flash-detailed)  
4. [Flashing & First Boot (Provisioning)](#4--flashing--first-boot-provisioning)  
5. [Web UI — Quick Walkthrough](#5--web-ui--quick-walkthrough)  
6. [HTTP API (Reference)](#6--http-api--reference)  
7. [WebSocket API (Reference)](#7--websocket-api--reference)  
8. [MQTT Topics & Payloads](#8--mqtt-topics--payloads)  
9. [History Format (LittleFS)](#9--history-format-littlefs)  
10. [Logging & Troubleshooting](#10--logging--troubleshooting)  
11. [WiFi Scan Behavior & Known Quirks](#11--wifi-scan-behavior--known-quirks)  
12. [Timezone UX (Dropdown + DST Toggle)](#12--timezone-ux-simplified-dropdown--always-editable-dst-toggle-new)  
13. [Multiple Sensors (Notes)](#13--multiple-sensors-notes)  
14. [Developer Guide: Architecture & Important Files](#14--developer-guide-architecture--important-files)  
15. [How to Extend / Add Features](#15--how-to-extend--add-features)  
16. [Tests & Debugging](#16--tests--debugging)  
17. [Contributing](#17--contributing)  
18. [License](#18--license)  
19. [Changelog](#19--changelog-highlights)  
20. [Contact & Credits](#20--contact--credits)  
21. [TODO (Planned Work)](#21--todo-planned-work)  
22. [Appendix: Scripts & Examples](#22--appendix-scripts--examples)

---

## 1 — Quick Start

### Minimum Hardware

- ESP32‑S3 (or compatible ESP32 variant tested with this repo)  
- RS485 transceiver (TTL ↔ differential), e.g. MAX485 module  
- THC‑S conductivity sensor (RS485) ComTopWin THCS Sensor — OR — MEC20 Soil Sensor (RS485)  
- SH1106 128×64 I2C OLED 1.3" display

### Basic Steps

1. **Build firmware** (see [Section 3](#3--build--flash-detailed)).  
2. **Flash** firmware to ESP32.  
3. Power the device. On first boot, the device creates AP **`THCS_SETUP`** (password **`Controls`**).  
4. Connect to `http://192.168.4.1` and provision your Wi‑Fi SSID/password.  
5. After provisioning, the device attempts STA connection, syncs time (NTP), and begins normal operation.  
6. Visit the Web UI for sensor readings, MQTT config, Wi‑Fi scan, history, and system controls.  
7. In System → Sensor Configuration choose the physical sensor type (THCS or MEC20). The selected driver will initialize the RS485 UART with the correct baud/parity.

### Quick `curl` Examples

Replace `<ip>` with device IP:

- Status:  
  `curl http://<ip>/api/status`

- Start Wi‑Fi scan:  
  `curl -X POST http://<ip>/api/wifi_scan_start`

- Poll scan status:  
  `curl http://<ip>/api/wifi_scan_status`

- Download 7‑day history CSV (for spreadsheets):  
  `curl -o history.csv "http://<ip>/download/history?hours=168"`

- Reboot device (via WebSocket – see WS section for details):  
  `wscat -c ws://<ip>/ws -x '{"type":"reboot_device"}'`

Factory reset and other destructive actions are intended to be triggered via the Web UI or WebSocket (`type:"factory_reset"`); no unauthenticated HTTP POST endpoint is exposed for them in this codebase.

---

## 2 — Hardware, Pinout & Wiring 

### Hardware Required

- ESP32‑S3 Development Boards with Expansion Adapter N16R8 KIT A  
  <https://www.aliexpress.com/item/1005007319706057.html>  
- ESP32‑WROOM‑32D/32U 30‑pin module with breakout board  
  <https://www.aliexpress.com/item/1005006422498371.html>  
- ComWinTop THC‑S RS485 Sensor  
  <https://www.aliexpress.com/item/1005001524845572.html>  
- **OR** a compatible **TH3001** (Temp/Humidity) RS485 Sensor  
- 1.3" OLED Display Module  
  <https://www.aliexpress.com/item/1005006127524245.html>  
- MAX485 RS485 module  
  <https://www.aliexpress.com/item/1005003204223371.html>  
- Momentary self-reset button (no light, 3–6 V, 12 mm) for **Factory Reset / AP double‑tap+hold**  
  <https://www.aliexpress.com/item/1005002153423900.html>  
- MEC20 Soil Sensor (RS485) can be substituted for THCS; connects to same RS485 bus (see Section 21 notes).

### Optional Hardware

- 2.54 mm / 0.1" pitch PCB screw terminal block connector, 4‑pin  
- Micro‑USB extension waterproof cable (USB 2.0 Micro‑5pin male/female)  
- SP13 waterproof IP68 4‑pin cable connectors  
- 0.96" OLED display module SSD1306 I2C/IIC/SPI 128×64  
- Small case  
- USB cable with charging and data  
- Step drill bit  
- Soldering iron, flux, solder

### Sensor Manual

- ComWinTop THC‑S Manual:  
  <https://dl.artronshop.co.th/CWT-RS485-Soil/THC-S%20manual.pdf>

---

### Pin Mapping — ESP32‑S3 Dev (Primary, matches `Config.h`)

```cpp
#define RS485_TX_PIN   20   // ESP32-S3 TX2
#define RS485_RX_PIN   19   // ESP32-S3 RX2
#define RS485_DE_PIN   1    // ESP32-S3 DE
#define RS485_RE_PIN   2    // ESP32-S3 RE
#define FACTORY_RESET_PIN 13

#define I2C_SDA_PIN    8    // ESP32-S3 SDA
#define I2C_SCL_PIN    9    // ESP32-S3 SCL
```

- RS485 TX (UART2): `GPIO20`  
- RS485 RX (UART2): `GPIO19`  
- RS485 DE (Driver Enable): `GPIO1`  
- RS485 RE (Receiver Enable): `GPIO2`  
- Factory reset / AP button (active LOW): `GPIO13`  
- OLED SH1106 I2C: SDA=`GPIO8`, SCL=`GPIO9`

### Alternate Mapping — ESP32 (Legacy 30‑pin Dev Boards)

If you are using an ESP32 30‑pin variant (older dev boards), use these mappings instead; update `Config.h` comments/code accordingly:

- RS485 TX (UART2): `GPIO16`   // ESP32 30‑pin TX2  
- RS485 RX (UART2): `GPIO17`   // ESP32 30‑pin RX2  
- RS485 DE (Driver Enable): `GPIO18`  
- RS485 RE (Receiver Enable): `GPIO19`  
- OLED I2C SDA: `GPIO21`  
- OLED I2C SCL: `GPIO22`

> **Note:** On ESP32‑S3 boards, verify your selected IO pins are not in use by flash/PSRAM for your module.

---

### I2C Initialization (Important)

The `u8g2` display object (`Globals::u8g2`) is constructed with `I2C_SDA_PIN` and `I2C_SCL_PIN`, but on ESP32 it is recommended to explicitly configure `Wire` as well:

Call:

```cpp
Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
```

in `setup()` after `Serial.begin()` and before `OLEDDisplay::begin()` / `Globals::u8g2.begin()`.

Example snippet from your `setup()` (you can adapt):

```cpp
Serial.begin(115200);
delay(100);
// [RECOMMENDED] Initialize I2C explicitly on ESP32 / ESP32-S3
Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);  // SDA=GPIO8, SCL=GPIO9 (S3 dev example)

Globals::begin();
loadPreferences();
ActiveSensor::begin(); // driver will configure RS485 baud
OLEDDisplay::begin();
```

---

### Recommended RS485 Wiring

- ESP32 UART2 TX (GPIO20 or legacy 16) → DI (transceiver)  
- ESP32 UART2 RX (GPIO19 or legacy 17) ← RO (transceiver)  
- DE/RE connected to their dedicated GPIOs (or tied together and driven from one pin if you modify the code).  
- RS485 A/B ↔ sensor RS485 A/B lines (matching polarity).  
- Common ground between ESP32 and RS485 transceiver.  
- Termination and bias resistors as recommended by your RS485 transceiver and cable length.

---

### Power

- Use a regulated 5 V (for sensor + RS485 module) and 3.3 V for ESP32, following board specs.  
- ESP32 I/O is **3.3 V only** — do **not** drive pins with 5 V logic.

---

## 3 — Build & Flash (Detailed)

### Requirements

- Arduino‑ESP32 toolchain **or** PlatformIO  
- Libraries (non‑exhaustive list used by this firmware):
  - `ArduinoJson` v6  
  - `AsyncTCP`  
  - `ESPAsyncWebServer`  
  - `WebSerial`  
  - `PubSubClient`  
  - `U8g2`  
  - `Preferences` (esp32 core)  
  - `LittleFS` support for ESP32 (`LittleFS.h`)  

### Partitions CSV note (important)

- The ESP32 core looks for a sketch‑local `partitions.csv`.  
- If you keep multiple partition CSVs (e.g. `partitions_4mb_factory.csv`, `partitions_16mb_8mb.csv`), copy or symlink the desired one to `partitions.csv` before building.  
- **Ensure the partition scheme provides at least 2MB for the app and 2MB for flash/LittleFS** to accommodate the firmware and data storage (e.g., history files).

### A) Arduino CLI / IDE / Cloud

- Select board:

  - For ESP32‑S3 dev variants: **“ESP32S3 Dev Module”** (or vendor‑specific S3 variant).
  - For legacy 30‑pin ESP32: `ESP32 Dev Module`.

- Install required libraries via Library Manager or CLI.  
- Build and **Upload** (baud e.g. `921600`).  
- Serial monitor: **115200 baud**.

### B) PlatformIO

Example `platformio.ini` (conceptual):

```ini
[env:esp32s3-dev]
platform = espressif32
board = esp32s3dev
framework = arduino
monitor_speed = 115200
build_flags =
  -D ARDUINO_USB_CDC_ON_BOOT=1
  -D CONFIG_ARDUINO_LOOP_STACK_SIZE=16384
lib_deps =
  bblanchon/ArduinoJson
  me-no-dev/AsyncTCP
  me-no-dev/ESP Async WebServer
  olikraus/U8g2
```

Commands:

- Build: `pio run`  
- Upload: `pio run --target upload`  
- Monitor: `pio device monitor --baud 115200`

### C) Containerized (Optional)

Use Docker to reproduce builds in CI:

```dockerfile
FROM platformio/platformio-core:latest
WORKDIR /project
COPY . /project
RUN pio run
```

Adapt for Arduino CLI or a custom runner as needed.

---

## 4 — Flashing & First Boot (Provisioning)

- After flashing, on **first boot** when there are no stored Wi‑Fi credentials:
  - `WifiManager::begin()` starts **AP mode** (`THCS_SETUP` / `Controls`) by default.
  - The OLED shows AP mode info (SSID, password, IP).

- Connect to `THCS_SETUP` and browse to `http://192.168.4.1`.

- In the Web UI (Wi‑Fi tab):

  - Enter your STA SSID + password.  
  - Click **Save & Reboot**.

- On reboot, the device will:

  - Attempt STA connection (`WIFI_STA_TRYING`).  
  - If STA connects:
    - NTP is configured via `configTime(gmtOffset_sec, daylightOffset_sec, ntpServer)`.  
    - AP is disabled after a short delay (`AP_DISABLE_DELAY_MS`).
  - If STA fails:
    - Remains in/returns to AP mode and periodically retries STA (`WIFI_AP_RETRY_PERIOD_MS`).

- You can also:

  - Trigger **Wi‑Fi retry** via Web UI / WebSocket (`type:"wifi_retry_now"`).  
  - Trigger **factory reset** via Web UI / WebSocket (`type:"factory_reset"`), which:
    - Clears Wi‑Fi, MQTT, sensor, system NVS namespaces.  
    - Removes `/hist_v1.bin` history file.  
    - Restarts into AP mode provisioning.

---

## 5 — Web UI — Quick Walkthrough

The Web UI is a single‑page app embedded in firmware (`html.cpp` → `index_html[]` served from `PROGMEM`). It communicates via WebSocket (`/ws`) and a few HTTP endpoints.

### Tabs

1. **Sensor**

   - Shows current sensor readings:
     - VWC (`getVWC()` in %)  
     - Temperature (`getTempC()`; UI optionally shows °C or °F)  
     - pwEC (`getPwec()` in dS/m)  
     - Bulk EC (`getECuS()` in µS/cm)  
     - Raw EC (`getRawEC()` count before calibration)  
   - Sensor status badge (OK / ERROR) driven by `ActiveSensor::hasError()`.  
   - System summary:
     - Sensor # (for `/sN` MQTT suffix)  
     - Wi‑Fi + IP status  
     - MQTT status + effective MQTT topic  
     - Uptime (seconds + human‑readable string)  
   - Controls:
     - Toggle temperature unit (°C / °F)  
     - Reboot device  

2. **Graph**

   - Requests historical data via WebSocket (`type:"get_chart_data", hours:...`).  
   - Firmware side (`WebUI::wsOnEvent` + `History` / LittleFS) streams history as multiple **chunked** messages:
     - `type:"history_chunk"` with small JSON `data` arrays.  
     - Final `type:"history_end"` message indicates completion and total points.  
   - Graph is rendered using [uPlot](https://github.com/leeoniya/uPlot):
     - X axis: time.  
     - Series: VWC (%), Temperature (°C/°F), pwEC (dS/m).  
   - There is a **Graph enabled** UI toggle:
     - Backed by WebSocket message `{"type":"toggle_history","enabled":true|false}`.  
     - Enabled/disabled state persisted under NVS (`NS_SYSTEM` key `history_ui_enabled`).  
     - When disabled, device **still records history** in `/hist_v1.bin`; only UI graph is disabled.  
   - Button to download **7‑day CSV**: calls `/download/history?hours=168` (see [History](#9--history-format-littlefs)).

3. **WiFi**

   - Status summary:
     - STA vs AP mode, IP address, retry countdown.  
   - Wi‑Fi settings:
     - SSID + password.  
     - Buttons: Save & Reboot, Reconnect.  
   - **Wi‑Fi scan**:
     - “Scan Networks” button calls `/api/wifi_scan_start`.  
     - Polls `/api/wifi_scan_status` to get results.  
     - Clickable list populates the SSID input.  
   - Short help text for AP mode (SSID + IP).

4. **MQTT**

   - Status:
     - MQTT connected?  
     - HA Discovery enabled?  
     - Effective MQTT user topic and HA discovery topic.  
   - MQTT Connection:
     - Server/port, username/password, “Allow anonymous” helper.  
     - Save & Reconnect.  
     - MQTT enable/disable toggle (`mqttEnabled` persisted in NVS and enforced in `MQTTManager::loop()`).  
   - User Topic:
     - Base MQTT topic (default `sensor/thcs`).  
     - When Sensor # is set, firmware appends `/sN` in user publishes.  
   - Home Assistant Discovery:
     - Enable/disable HA discovery (persisted as `ha_enabled`).  
     - Re‑publish discovery configs now (`ha_announce_now`).  
   - Publish Mode:
     - “Always” vs “On Change (delta)” selection and per‑field thresholds:
       - VWC Δ (%)  
       - Temp Δ (°C)  
       - EC Δ (µS/cm)  
       - pwEC Δ (dS/m)  
     - UI sends `{"type":"update_publish_mode"}` with thresholds; firmware currently:
       - Persists `publishMode`, `deltaVWC`, `deltaTempC`, `deltaECuS`, `deltaPWEC` in NVS (`NS_MQTT`).  
       - **Note:** `MQTTManager::publishIfNeeded()` in this beta still publishes every interval; delta thresholds are stored and surfaced in UI but not yet enforced in the MQTT loop.  <!-- [ADDED] -->

5. **Calibration**

   - Shows current calibration:
     - EC Slope (`EC_SLOPE`)  
     - EC Intercept (`EC_INTERCEPT`)  
     - Temp Coeff (`EC_TEMP_COEFF`)  
     - Current temperature (°C/°F) and raw EC.  
   - Calibration inputs:
     - EC Slope, EC Intercept (µS/cm), Temp Coefficient (`per °C`).  
     - Saved via WebSocket message `{"type":"update_calibration","slope","intercept","tempcoeff"}` and persisted in NVS (`NS_SENSOR`).  
   - Two‑Point Calibration Guide (matches UI + `Sensor Calibration Guide.md`):
     - Mode radio group:
       - **No Temp Compensation (No TC)**:  
         - Use `"Low Buffer Solution — µS (@25 °C)"` (`buf-low`) and `"High Buffer Solution — µS (@25 °C)"` (`buf-high`) directly.  
       - **Manual Temp Compensation (Manual TC)**:  
         - Optional TDC fields `"TDC Low µS Buffer Solution µS"` (`buf-low-tdc`) and `"TDC High µS Buffer Solution"` (`buf-high-tdc`) let you paste bottle‑chart values at your measurement temperature and compute compensated slope/intercept.  
     - Behavior (as implemented in `calculateCalibration()` in `html.cpp`):  
       - Reads `raw-low`, `raw-high` (raw EC).  
       - Uses `buf-low`/`buf-high` as reference µS.  
       - If `cal-method == "chart"`, prefers `buf-*-tdc` values **when present**; otherwise falls back to reference µS.  
       - Computes:  
         - `slope = (useHigh - useLow) / (rawHigh - rawLow)`  
         - `intercept = useLow - slope * rawLow`  
       - Displays slope/intercept and status message:  
         - “Calculated using Manual TC.” when Manual TC used.  
         - “Calculated using No TC.” in No‑TC mode.  
     - “Copy to Inputs” button copies results into the main calibration inputs so they can be saved.

6. **System**

   - System summary:
     - Uptime (seconds + human)  
     - Wi‑Fi and MQTT badges  
     - Sensor #  
     - WebSerial Enabled/Disabled  
     - Heap free/min values  
     - Timezone summary (name/offset)  
   - Sensor configuration:
     - Sensor # (1–255; persisted and used as `/sN` suffix).  
     - Sensor poll interval (ms, 1000–60000). Persisted to `sensor_int_ms` and used by `THCSensor` state machine + MQTT loop interval.  
   - System configuration:
     - Timezone dropdown + DST toggle (see section 12).  
     - Temperature unit toggle.  
     - WebSerial enable/disable.  
     - Debug Mode enable/disable.  
     - History clear.  
     - Factory Reset.  
   - Footer shows firmware name, version, build, chip ID and MAC.

---

## 6 — HTTP API — Reference

**Base URL:** `http://<device_ip>/`

### 6.1 Root & Static

#### `GET /`

- Serves the embedded Web UI HTML (`index_html[] PROGMEM`).

---

### 6.2 Status & System

#### `GET /api/status`

Returns a JSON snapshot of basic device state (examples):

```json
{
  "fw": "THCS_V1_ESP32 (Pirate Controls) ",
  "ver": "1.0",
  "wifi_connected": true,
  "ip": "10.0.0.42",
  "mqtt_connected": true,
  "mqtt_enabled": true,
  "uptime_s": 90,
  "uptime_human": "1m 30s",
  "history_enabled": true,
  "sensor_type": "mec20"
}
```

Currently `/api/status` is primarily used by the Web UI and tools for quick health checks.

---

### 6.3 Wi‑Fi Scan & Status

#### `POST /api/wifi_scan_start`

Starts a Wi‑Fi scan:

- Preferred path: **esp‑idf async scan** via `WifiScan::startAsync()` (non‑blocking).  
- Fallback: synchronous `WiFi.scanNetworks(false, true)` if IDF scan cannot be started.

Example responses:

```json
{ "started": true, "message": "Async scan started" }

{ "started": true, "message": "Synchronous scan completed", "count": 19 }

{ "started": false, "message": "Failed to start scan" }
```

#### `GET /api/wifi_scan_status`

Check scan status and/or results:

- While scanning:

```json
{ "status": "scanning" }
```

- When done (IDF or fallback):

```json
{
  "status": "done",
  "count": 12,
  "aps": [
    {
      "ssid": "MyWiFi",
      "rssi": -50,
      "chan": 6,
      "bssid": "AA:BB:CC:DD:EE:FF",
      "enc": "SECURED"
    }
  ]
}
```

- Idle / failure cases:

```json
{ "status": "idle" }

{ "status": "failed", "error": "..." }
```

Implementation details:

- IDF path uses `esp_wifi_scan_start` + `WIFI_EVENT_SCAN_DONE` and defers AP record reads to the main loop via `WifiScan::poll()` to avoid heavy work in the event callback.  
- Fallback path uses Arduino `WiFi.scanNetworks()` when IDF record retrieval keeps reporting 0 APs or fails repeatedly.  
- `WifiScan::buildStatusJSON()` encapsulates caching logic for `/api/wifi_scan_status`.  
- Note: verbose esp-idf debug prints from WiFiScan (esp_wifi return codes, record counts, handler registration status) are only emitted when Debug Mode is enabled to avoid serial clutter in production.

---

### 6.4 History

#### `GET /api/history/meta`

Returns metadata about the history ring buffer:

```json
{
  "type": "history_meta",
  "interval_s": 300,
  "max_records": 4096,
  "retention_h": 341,
  "count": 432
}
```

- `interval_s` – sampling interval in seconds (derived from `HISTORY_INTERVAL_MS`).  
- `max_records` – record capacity of the file (uses `HISTORY_MAX_RECORDS` from `Config.h`).  
- `retention_h` – theoretical history window (`max_records * interval_s / 3600`).  
- `count` – how many records are currently valid.

#### `GET /download/history`

Streams history as **CSV** using `History::streamCSV()`.

- Query parameter `hours` (optional): look‑back window, clamped internally to a maximum of `168` (7 days):

  - Example: `/download/history?hours=72`.

- Output headers:

  - `Content-Type: text/csv; charset=utf-8`  
  - `Content-Disposition: attachment; filename="history.csv"`

- CSV columns (as implemented):

```text
Date,Time,VWC,pwEC,TempC,TempF
2025-02-10,13:00:00,24.123,0.157,21.37,70.46
...
```

Implementation notes:

- `streamCSV()` always writes the header line, even if there is no data.  
- For each record, it converts epoch to local time using `localtime_r` (or `gmtime_r` fallback).  
- Temperature in Fahrenheit is computed as `(C * 9 / 5) + 32` and printed with 2 decimal places.  
- A safety cap of 7 days (~2016 points at 5‑minute interval) is enforced internally.

---

### 6.5 Other HTTP

- There are no unauthenticated HTTP endpoints for reboot or factory reset in this code; those actions are performed via WebSocket messaging from the Web UI.

---

## 7 — WebSocket API — Reference

**Endpoint:** `ws://<device_ip>/ws`

- Protocol: plain WebSocket; messages are JSON objects.
- Both client and server always include a `"type"` field.
- Maximum inbound message size is limited to ~4 KiB; oversized messages yield an error.

### 7.1 Messages: Client → Server

A non‑exhaustive set (matching `WebUI.cpp`):

- `{"type": "get_status"}`  
- `{"type": "get_config"}`  
- `{"type": "request_meta"}`  
- `{"type": "get_chart_data", "hours": 24}`  
- `{"type": "get_history_meta"}`  
- `{"type": "debug_add_point"}`

- `{"type": "update_wifi", "ssid": "...", "password": "..."}`  
- `{"type": "wifi_retry_now"}`

- `{"type": "update_mqtt", "server":"...", "port":1883, "user":"...", "pass":"...", "allow_anonymous":false}`  
- `{"type": "update_mqtt_topic", "topic": "sensor/thcs"}`  
- `{"type": "toggle_mqtt", "enabled": true}`

- `{"type": "set_sensor_number", "id": 1}`  
- `{"type": "set_mqtt_features", "allow_structured": true}`  
- `{"type": "update_sensor_interval", "ms": 5000}`

- `{"type": "enable_ha", "enabled": true}`  
- `{"type": "ha_announce_now"}`

- `{"type": "update_calibration", "slope": 1.00, "intercept": 0.0, "tempcoeff": 0.0}`

- `{"type": "temp_unit", "fahrenheit": true}`

- `{"type": "update_time", "gmt_offset_sec": -14400, "dst_offset_sec": 3600, "tz_name": "Atlantic — AST/ADT (UTC−04:00 / UTC−03:00)"}`

- `{"type": "toggle_history", "enabled": true}`  
- `{"type": "clear_history"}`

- `{"type": "reboot_device"}`  
- `{"type": "factory_reset"}`

- `{"type": "toggle_debug_mode", "enabled": true}`  
- `{"type": "toggle_webserial", "enabled": true}`

- `{"type": "ping", "t": 1700000000000}`

- `{"type":"set_sensor_type","sensor_type":"thcs"}` or `{"type":"set_sensor_type","sensor_type":"mec20"}`

### 7.2 Messages: Server → Client

From `WebUI.cpp`:

- `type: "status"` — device status for UI:

  - `wifi_connected`, `wifi_mode`, `ap_active`, `ip`, `rssi`,  
    `mqtt_connected`, `mqtt_enabled`, `mqtt_topic`,  
    `uptime_sec`, `uptime_human`, `since_reset_sec`,  
    `debug_mode`, `heap_free`, `heap_min`, `psram_free`,  
    `useFahrenheit`, `structured_enabled`, `ha_enabled`,  
    `publish_mode`, `thresholds`, `timezone_offset`, `dst_offset`,  
    `tz_name`, `tz_offset_str`, `history_enabled`,  
    `sensor_interval_ms`, `sensor_number`, `chipid`, `mac`, `ha_topic`,  
    `time_synced`, `time_str`, `webserial_enabled`,  
    `sensor_type`

- `type: "sensor"` — live sensor data:

  - `soil_hum`, `soil_temp`, `soil_pw_ec`, `soil_ec`, `raw_ec`, `sensor_error`.

- `type: "config_update"` — config snapshot:

  - `ssid`, `mqttServer`, `mqttPort`, `mqttUsername`, `mqttTopic`,  
    `ecSlope`, `ecIntercept`, `ecTempCoeff`,  
    `useFahrenheit`, `allowStructured`, `haEnabled`, `mqttEnabled`,  
    `publishMode`, `deltaVWC`, `deltaTemp`, `deltaEC`, `deltaPWEC`,  
    `tzOffset`, `dstOffset`, `sensorIntervalMs`,  
    `webSerialEnabled`, `debugMode`, `sensorNumber`, `historyEnabled`,  
    `tzName`, `tzOffsetStr`, `sensorType`.

- `type: "meta"` — firmware/meta info (`WebUI::sendMetaWS()`):

  - `fw`, `ver`, `build`, `chipid`, `mac`.

- `type: "history_chunk"` — streaming history for charts:

  - `idxStart`, `idxEnd`, `hours`, `data` (serialized JSON array of points `{t, vwc, pwec, temp}`).

- `type: "history_end"` — indicates no more chunks:

  - `idxTotal`, `hours`, `message` (optional status string).

- Other result/ack types:

  - `wifi_update`, `mqtt_update`, `mqtt_topic_update`,  
    `mqtt_state`, `mqtt_features_update`,  
    `sensor_number_update`, `sensor_interval_update`,  
    `ha_state`,  
    `calibration_update`,  
    `temp_unit_changed`,  
    `time_update`,  
    `history_state`, `history_cleared`,  
    `reboot_initiated`, `reset_initiated`,  
    `debug_mode_state`,  
    `webserial_state`,  
    `error`,  
    `pong`.

### 7.3 Notes

- WebSocket messages are limited to 4096 bytes in size (`WS_MAX_MESSAGE_SIZE`).  
- History streaming is chunked to avoid long blocking operations and oversized WS frames.  
- Time‑dependent features (history, NTP) rely on `getLocalTime()`; when time is not synced, history endpoints will return empty windows or “time not synced” messages.

---

## 8 — MQTT Topics & Payloads

### 8.1 MQTT Connection

- Configurable via Web UI and persisted to NVS (`NS_MQTT` namespace):
  - `mqttServer` (string)  
  - `mqttPort` (int)  
  - `mqttUsername`, `mqttPassword`  
  - `mqtt_enabled` (bool toggle)  
  - `allow_structured`, `ha_enabled`  
- `MQTTManager::begin()` sets basic parameters and attempts an initial connection.  
- `MQTTManager::loop()` handles reconnection with backoff and defers connection attempts during Wi‑Fi scans.  
- When MQTT is disabled in UI, the firmware:
  - Publishes `offline` to the status topic.  
  - Disconnects the client and stops attempting new connections until re‑enabled.

### 8.2 Device Identity & Base Topics

- Device ID / topics:

  - `Globals::chipIdStr` — derived from the Wi‑Fi MAC tail (for UI + meta).  
  - Permanent MAC from `ESP.getEfuseMac()` is used to build:
    - `MAC6` = last 6 uppercase hex chars (no colons).  
  - Base topics:
    - `Globals::baseTopic = "thcs/" + chipIdStr` (for status/meta).  
    - HA state topic: `thcs/<MAC6>/sensor`.

### 8.3 Published Topics

#### 8.3.1 Status / Meta

- LWT / availability:
  - Topic: `thcs/<chipId>/status`  
  - Payloads: `online`, `offline` (retained).

- Meta:
  - Topic: `thcs/<chipId>/meta` (retained).  
  - Payload JSON includes fw, ver, build, chipid, mac, timezone offsets.

#### 8.3.2 Sensor State JSON (User Topic + HA State Topic)

- **User topic**: configurable `mqttTopicUser` (e.g. `sensor/thcs`). If `sensorNumber` is 1–255, firmware appends `/sN`:

  - Example: `sensor/thcs/s1`.

- **HA state topic**: `thcs/<MAC6>/sensor`.

Both receive the same **compact JSON** payload from `publishStateForHA()` including:

```json
{
  "vwc": 25.4,
  "temp": 22.1,
  "pwec": 0.16,
  "ec": 412,
  "temp_f": 71.8,
  "raw_ec": 410,
  "error": false,
  "uptime_str": "00:00:22:41",
  "sensor_type": "mec20"
}
```

Notes:

- `pwec` is rounded to 2 decimal places.  
- `ec` is bulk EC in µS/cm (integer).  
- `temp_f` is °F rounded to one decimal.  
- `uptime_str` is `DD:HH:MM:SS`.  
- `sensor_type` is included to indicate the active driver; Home Assistant templates ignore unknown fields by default, but downstream integrations can use this to differentiate sensor models.

#### 8.3.3 Structured Subtopics

If `allowStructured == true`, values are also published separately under:

- User structured base: `<mqttTopicUser>[/sN]`  
- HA structured base: `thcs/<MAC6>/sensor`

Each gets:

- `/vwc` — `"25.4"` (1 decimal)  
- `/temp` — `"22.1"` (1 decimal)  
- `/temp_f` — `"71.8"` (1 decimal)  
- `/ec` — `"412"` (integer)  
- `/pwec` — `"0.16"` (2 decimals, pore‑water EC in dS/m)  
- `/uptime_str` — `"00:00:22:41"`

### 8.4 Home Assistant Discovery

- Discovery topics: `homeassistant/sensor/<unique_id>/config` (retained).  
- `unique_id` uses the `thcs_<MAC6>_*` pattern.

Each discovery payload includes:

- `availability_topic: thcs/<chipId>/status` with `payload_available = "online"` and `payload_not_available = "offline"`.  
- `device` block with:
  - `identifiers = ["thcs_<MAC6>"]`  
  - `manufacturer = "Pirate Controls"`  
  - `model = "THCS V1 ESP32"`  
  - `name = "THCS Sensor <MAC6>"`  
  - `sw_version = "<FW_NAME> <FW_VERSION>"`

---

## 9 — History Format (LittleFS)

- File: `/hist_v1.bin` on `LittleFS`.  
- Managed by `History.cpp`.

### 9.1 File Layout

Header (`HistoryHeader`, packed):

```c
struct HistoryHeader {
  uint32_t magic;      // 0x53434854 "THCS"
  uint16_t version;    // 0x0001
  uint16_t reserved;   // reserved for alignment/future use
  uint32_t head;       // next write index
  uint32_t count;      // number of valid records
  uint32_t maxRecords; // total slots (from Config::HISTORY_MAX_RECORDS)
  uint32_t interval_s; // sampling interval in seconds
};
```

Record (`HistoryRecord`, packed):

```c
struct HistoryRecord {
  uint32_t epoch;  // Unix time
  float    vwc;    // %
  float    pwec;   // dS/m
  float    temp_c; // °C
};
```

### 9.2 Behavior

- `History::begin()`:

  - Mounts `LittleFS`.  
  - If `/hist_v1.bin` does not exist:
    - Creates it.  
    - Writes header with `magic`, `version`, `maxRecords` (value from `HISTORY_MAX_RECORDS` in `Config.h`), and `interval_s` from `HISTORY_INTERVAL_MS / 1000UL`.  
    - Pre‑allocates `maxRecords` zeroed `HistoryRecord`s.

- `History::addPoint(...)`:

  - Ensures header is loaded/valid.  
  - Requires valid local time via `getLocalTime()`.  
  - Retrieves epoch with `mktime`.  
  - Writes the new record at `head`, increments `head`, and updates `count`.  
  - Rewrites the header at file position 0.

- `History::clear()`:

  - Deletes `/hist_v1.bin` from `LittleFS`.  
  - Recreates via `begin()`.

- `History::getJSON(hours)`:

  - Returns a JSON array string with records newer than `now - hours*3600`.  
  - Clamps `hours` to actual retention.  
  - Iterates through ring buffer in chronological order.

- `History::streamJSON(hours, Print &out, size_t maxPoints)`:

  - Streams JSON array directly into a `Print` to avoid large allocations.  
  - Capped to 36 hours (~432 points at 5‑minute interval) for charting.  
  - Used by internal WebSocket streaming logic.

- `History::streamCSV(hours, Print &out, size_t maxPoints)`:

  - See section 6.4.  
  - Writes `Date,Time,VWC,pwEC,TempC,TempF` header and per‑record lines.  
  - Uses `localtime_r` (fallback `gmtime_r`) to convert epoch.  

- `History::loop()`:

  - Uses `Globals::nextHistoryAt` and `HISTORY_INTERVAL_MS` to decide when to log a new point.  
  - Only logs if time is synced and `ActiveSensor::hasError()` returns false.

---

## 10 — Logging & Troubleshooting

### 10.1 Logging

- Serial (`Serial.println`) used throughout.  
- `Globals::webDebug(String, bool force=false)` logs conditionally based on `debugMode`:

  - If `force == true` or `debugMode == true`, prints to Serial; if WebSerial started, also mirrors to WebSerial.

- WebSerial:

  - When `webSerialEnabled == true`, `Globals::ensureWebSerial()` sets up WebSerial on `Globals::server`.  
  - Can be toggled from the Web UI (System tab) and persisted to NVS.  
  - Open via browser at `/webserial` (UI button opens a new tab pointing there).

- WiFiScan verbose debug:
  - The `WifiScan` module emits additional `Serial.printf` debug (esp-idf rc codes, record counts) when `debugMode` is enabled. This avoids noisy logs during normal operation but provides deeper diagnostics when needed.

### 10.2 Troubleshooting Checklist

- **Wi‑Fi not connecting:**

  - Check SSID/password.  
  - Ensure device is in 2.4 GHz range.  
  - Use Web UI Wi‑Fi page to retry or reset to AP mode (button double‑tap+hold, see below).  

- **Wi‑Fi scan shows 0 APs:**

  - The firmware retries IDF `scan_get_ap_records` a few times, then falls back to a synchronous scan.  
  - Check logs for `[WiFiScan]` messages and temporary 0‑AP conditions.  
  - Enable Debug Mode to see esp-idf debug prints for deeper diagnosis.

- **MQTT not connecting:**

  - Confirm broker host/port and credentials.  
  - Verify `mqttEnabled` toggle is ON.  
  - Check logs for `"[MQTT]"` messages (auth failure, backoff, etc.).  

- **No sensor readings / frequent errors:**

  - Check RS485 wiring and baud rate (THCS = 4800, MEC20 = 9600, both 8N1).  
  - Verify DE/RE pin mapping in `Config.h` matches hardware.  
  - `THCSensor` and `MEC20Sensor` both log detailed debug when CRC fails and when RX times out.
  - Look for `[THCSensor]` or `[MEC20Sensor]` debug logs about MODBUS timeouts or CRC fails.  

- **History empty or chart doesn’t show:**

  - Ensure time is synced (`Time OK` badge).  
  - Ensure Graph UI is enabled (System / Status / History enabled).  
  - Wait at least one full interval (`HISTORY_INTERVAL_MS`).  
  - Use `{"type":"debug_add_point"}` WebSocket message to inject a point for testing.  

- **OLED blank:**

  - Confirm I2C wiring and `Wire.begin()` call.  
  - Check for conflicts with other I2C devices or pins.  

- **Linker error `undefined reference` for functions like `Utils::uptimeHuman`:**

  - Ensure all `.cpp` files (including `Utils.cpp`) are in the same sketch folder (Arduino) or under `src/` (PlatformIO) so they are compiled.  
  - Do not rename files without updating corresponding includes.

- **AP / Factory reset button behavior:**

  - Single long hold (~10 s) → **Factory Reset** (clears NVS + history, restarts).  
  - Double‑tap quickly, then hold for 10 s → **Start AP** even when STA is configured.  
  - OLED shows explicit messages for AP start and factory reset countdown.

---

## 11 — WiFi Scan Behavior & Known Quirks

From `WifiScan.*`:

- Primary scan path:

  1. Register IDF event handler for `WIFI_EVENT_SCAN_DONE`.  
  2. Call `esp_wifi_scan_start(&cfg, false)` to begin an async scan.  
  3. On `SCAN_DONE`, schedule a deferred read (no heavy work inside the handler).  
  4. `WifiScan::poll()` is called from the main loop to:
     - Call `esp_wifi_scan_get_ap_num()` and `esp_wifi_scan_get_ap_records()` once per attempt.  
     - Cache minimal info in a `std::vector<APEntry>`: `ssid`, `rssi`, `chan`, `bssid`, `enc`.  
     - Mark `sCacheReady = true` once data is cached or an unrecoverable error occurs.  

- Fallback path:

  - If IDF record retrieval repeatedly yields `num == 0` or fails:
    - The firmware runs a single **Arduino synchronous scan** (`WiFi.scanNetworks(false, true)`).  
    - Caches results into the same vector.  
    - Ends the scan session.

- Status semantics:

  - `WifiScan::getStatus()` returns `IDLE`, `SCANNING`, `DONE`, `FAILED`.  
  - HTTP `/api/wifi_scan_status` returns `"status": "scanning" | "done" | "idle" | "failed"` and optionally `aps[]`.  

Known edges:

- Some IDF/Arduino combinations momentarily report `0` APs even when APs exist. The firmware retries a few times before falling back.  
- The scan API ensures the AP/STA mode is preserved (uses AP+STA if AP is currently active for provisioning).  
- The scan session lifecycle is explicit; SCAN_DONE from fallback Arduino scans is ignored so it doesn’t interfere with IDF logic.

---

## 12 — Timezone UX: Simplified Dropdown + Always‑Editable DST Toggle (NEW)

The Web UI presents:

- A curated timezone dropdown (`TZ_CHOICES` in `html.cpp`), each with:

  - `key`, `label`, `gmt` (base offset seconds), `dst` (typical DST offset).  

UI behavior:

- Displays “Effective: UTC±HH:MM” with `gmt + (dst_enabled ? dst : 0)`.  
- On **Save Timezone**, sends:

```json
{
  "type": "update_time",
  "gmt_offset_sec": <choice.gmt>,
  "dst_offset_sec": <dst_enabled ? choice.dst : 0>,
  "tz_name": "<choice.label>"
}
```

Firmware behavior:

- WebUI handler updates:
  - `gmtOffset_sec` and `daylightOffset_sec` globals.  
  - Optional `tzName` string.  
  - Persists them to NVS under `NS_SYSTEM`.  
  - Calls `configTime(gmtOffset_sec, daylightOffset_sec, ntpServer)`.

- Status JSON includes:
  - `timezone_offset`, `dst_offset`, `tz_name`, `tz_offset_str`.

---

## 13 — Multiple Sensors (Notes)

Current support:

- Single physical THCS sensor on RS485 bus by default.  
- Optionally MEC20 driver for MEC20 sensors (selected via UI).  
- A configurable **Sensor #** (1–255) stored in NVS:
  - Used to suffix the user MQTT topic with `/sN`.  
  - Exposed in Web UI and status JSON.

Planned / conceptual extensions (TODO):

- Support multiple RS485 sensors (addressing / bus scheduling).  
- Per‑sensor calibration profiles.  
- Per‑sensor HA discovery topics.

---

## 14 — Developer Guide: Architecture & Important Files

### 14.1 High‑Level Architecture

Main file:

- `THCS_MEC20_Sensor_V1.ino`

  - `setup()`:

    - Initializes RS485 control pins + factory reset pin.  
    - Starts Serial and logs boot banner + build info (`BUILD_UNIX_TIME`).  
    - Calls `Globals::begin()` (FS, identity).  
      - NOTE: Globals::begin() intentionally no longer initializes RS485 UART — the active sensor driver (THCS or MEC20) initializes UART in its begin() to guarantee correct baud/parity after sensor type selection.  
    - Calls `loadPreferences()` to load Wi‑Fi, MQTT, calibration, system prefs.  
    - Logs select configuration values (e.g. `sensorIntervalMs`, `publishMode`).  
    - Initializes subsystems:

      - `ActiveSensor::begin()`  ← active driver will set RS485 baud/parity
      - `OLEDDisplay::begin()`  
      - `History::begin()`  
      - `WifiManager::begin()`  
      - `WebUI::begin()`  
      - `MQTTManager::begin()`  
      - `WifiScan::begin()`  

    - Sets `Globals::nextHistoryAt` to a time aligned with `HISTORY_INTERVAL_MS`.  
    - Calls `configTime(gmtOffset_sec, daylightOffset_sec, ntpServer)`.  
    - Logs completion.

  - `loop()`:

    - `WifiManager::loop()`  
    - `WifiScan::poll()`  
    - `MQTTManager::loop()`  
    - `ActiveSensor::loop()`  
    - `MQTTManager::publishIfNeeded()`  
    - `WebUI::loop()`  
    - `History::loop()`  
    - `OLEDDisplay::loop()`  
    - `Globals::handleScheduled()`  
    - `Globals::ws.cleanupClients()`.

### 14.2 Modules

- **Config** (`Config.h/.cpp`):

  - Global configuration and NVS keys/namespaces.  

- **Globals** (`Globals.h/.cpp`):

  - Platform initialization (LittleFS, identity, server).  
  - Note: RS485 UART configuration is intentionally *not* done in Globals::begin() anymore — it's the responsibility of the active sensor driver (THCSSensor::begin or MEC20Sensor::begin) so the UART is always configured with the correct baud and parameters after sensor selection.

- **ActiveSensor** (`ActiveSensor.h/.cpp`):

  - Router facade that selects and calls the correct driver. Drivers implement begin(), loop(), getters and error/heartbeat.

- **THCSensor** (`THCSensor.h/.cpp`):

  - Non‑blocking Modbus state machine over RS485 at 4800 8N1. Uses Utils::modbusCRC16 and median helper.

- **MEC20Sensor** (`MEC20Sensor.h/.cpp`):

  - Non‑blocking Modbus state machine over RS485 at 9600 8N1. Uses Utils::modbusCRC16 and median helper. Firmware does not apply an additional EC temperature compensation to MEC20 EC measurements (MEC20 has internal compensation).

- **WifiScan** (`WifiScan.h/.cpp`):

  - Uses esp-idf async scan with SCAN_DONE handler and deferred record read in poll(). Falls back to Arduino sync scan. Verbose esp-idf debug printing is gated by `debugMode`.

- **MQTTManager**, **History**, **WebUI**, **OLEDDisplay**, **Utils**, **System**, etc. See code for details.

---

## 15 — How to Extend / Add Features

Typical pattern for new features:

1. **Config / storage:** add globals + persist in `loadPreferences()`.  
2. **Core logic:** implement in module, use `Globals::webDebug`.  
3. **Web API:** add WebSocket message type or HTTP route.  
4. **Web UI:** patch `html.cpp` (controls + handlers).  
5. **Testing:** manual tests + logs.

Notes:
- If adding a new sensor model, implement driver begin() to configure RS485 UART parameters and any register map. ActiveSensor uses the driver's begin() so RS485 will be correctly configured.

---

## 16 — Tests & Debugging

### 16.1 Manual Test Ideas

- **Wi‑Fi** provisioning & retries.  
- **Wi‑Fi scan** results & fallback. (Enable Debug Mode to view esp-idf debug prints.)  
- **MQTT** connect/publish & HA discovery.  
- **History** accumulation & chart rendering.  
- **Factory reset** via UI & button.  
- **Debug Mode/WebSerial** toggling.  

---

## 17 — Contributing

- Fork → branch → PR.  
- Keep PRs focused.  
- Include description, screenshots (UI), test steps, logs, MQTT samples.  
- Document NVS or format changes clearly.

---

## 18 — License

Recommended: **MIT**.

> **Action item:** Add a `LICENSE` file (MIT) to the repo root. Until then, the project should be treated as **all‑rights‑reserved** by default.

---

## 19 — Changelog (Highlights)

- Added MEC20 support and runtime sensor selection.  
- MEC20 driver refactored to non-blocking state machine.  
- Consolidated CRC/median helpers into Utils.  
- Fixed invalid default MQTT IP; adjusted default build timestamp.  
- History capacity now uses Config’s HISTORY_MAX_RECORDS; removed duplicate HISTORY_FILE macro.  
- RS485 UART initialization moved to driver begin() (ActiveSensor/driver) to avoid interim wrong-baud windows.  
- WiFiScan verbose debug prints gated by Debug Mode.

---

## 20 — Contact & Credits

**Author / Maintainer**

- Instagram: `@n0socialskills`  
- Email: `n0socialskills@protonmail.com`  
- Repo: [PC-THCS-V1-ESP32](https://github.com/n0socialskills/PC-THCS-V1-ESP32)

**Credits**

Uses:

- Arduino‑ESP32  
- MAX485 RS485 module  
- ComTopWin THCS‑S RS485 sensor  
- MEC20 RS485 sensor  
- ESP‑IDF  
- AsyncTCP / ESPAsyncWebServer  
- ArduinoJson  
- U8g2  
- PubSubClient  
- LittleFS  
- WebSerial

Upstream references:

- [kromadg/soil-sensor](https://github.com/kromadg/soil-sensor)  
- BeanBasement forum thread (DIY low‑cost TDR moisture sensor)  
- ScienceInHydroponics article on low‑cost TDR sensors

---

## 21 — TODO (Planned Work)

### Short‑Term (High Priority)

- [ ] Add OpenAPI 3.0 YAML for HTTP endpoints — P0  
- [ ] Add AsyncAPI / JSON message catalog for WebSocket messages — P0  
- [ ] Add CI pipeline (PlatformIO / GitHub Actions) — P0  
- [ ] Add `LICENSE` (MIT) and `CONTRIBUTING.md` to repo root — P0  
- [ ] Add tests for WiFi scan fallback — P1  
- [ ] Implement multi‑sensor config + per‑sensor calibration — P1  
- [ ] Wire `PublishMode` / delta thresholds into `MQTTManager::publishIfNeeded()` and document behavior changes — P1

### Medium‑Term

- [ ] Improve memory usage logging and soft‑fail strategies — P2  
- [ ] Consider optional file‑based logging with bounded size + download endpoint — P2  

---

## 22 — Appendix: Scripts & Examples

### 22.1 Wi‑Fi Scan

Start scan:

```bash
curl -X POST http://<ip>/api/wifi_scan_start
```

Poll results:

```bash
curl http://<ip>/api/wifi_scan_status
```

### 22.2 Status Snapshot

```bash
curl http://<ip>/api/status
```

### 22.3 History CSV Download

```bash
curl -o history.csv "http://<ip>/download/history?hours=168"
```

### 22.4 WebSocket Examples (using `wscat`)

- Get status and config:

```bash
wscat -c ws://<ip>/ws
> {"type":"get_status"}
> {"type":"get_config"}
```

- Reboot:

```bash
> {"type":"reboot_device"}
```

- Factory reset:

```bash
> {"type":"factory_reset"}
```

- Toggle MQTT:

```bash
> {"type":"toggle_mqtt","enabled":false}
```

- Request history data for last 24 hours:

```bash
> {"type":"get_chart_data","hours":24}
```

- Toggle Graph (history) UI:

```bash
> {"type":"toggle_history","enabled":true}
```

---

[END]