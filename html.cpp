#include <Arduino.h>
#include "html.h"

const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Pirate Controls Sensor Controller</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="stylesheet" href="https://unpkg.com/uplot/dist/uPlot.min.css">
<script src="https://unpkg.com/uplot/dist/uPlot.iife.min.js"></script>
<style>
:root{
  --bg:#121212;--fg:#e0e0e0;--card:#1e1e1e;--border:#444;
  --primary:#3498db;--primary-hover:#2980b9;
  --ok:#4CAF50;--err:#F44336;--warn:#ff9800;
  --tab:#1a1a1a;--tab-active:#252525;
}
*{box-sizing:border-box}
body{margin:0;padding:16px;background:var(--bg);color:var(--fg);font-family:Arial,Helvetica,sans-serif}
.container{
  max-width:900px;margin:0 auto;background:var(--card);border-radius:8px;
  overflow:visible!important;
  box-shadow:0 2px 10px rgba(0,0,0,0.3)
}
.header{padding:14px 16px;background:var(--tab);border-bottom:1px solid #333;display:flex;align-items:center;justify-content:space-between}
.header h2{margin:0;font-size:18px}
.badge{padding:2px 8px;border-radius:4px;background:#555;color:#fff;font-size:12px;margin-left:6px}
.badge.ok{background:var(--ok)}
.badge.err{background:var(--err)}
.badge.warn{background:var(--warn);color:#fff}
.tabs{display:flex;background:var(--tab);border-bottom:1px solid #333;position:sticky;top:0;z-index:9999}
.tab{flex:1;text-align:center;padding:10px;cursor:pointer;border-bottom:3px solid transparent}
.tab.active{border-bottom-color:var(--primary);background:var(--tab-active);color:var(--primary);font-weight:bold}
.section{display:none;padding:16px}
.section.active{display:block}
.row{display:flex;justify-content:space-between;align-items:center;background:#222;margin:8px 0;padding:10px;border-radius:6px;border:1px solid var(--border)}
.label{font-weight:bold}
.unit{opacity:.7;margin-left:4px}
.controls{padding:12px;background:var(--tab);border-top:1px solid #333;display:flex;gap:8px;flex-wrap:wrap;justify-content:center}
button{background:var(--primary);border:none;color:#fff;padding:8px 12px;border-radius:4px;cursor:pointer;font-size:13px;transition:background 0.2s}
button:hover{background:var(--primary-hover)}
button.danger{background:var(--err)}
button.danger:hover{background:#d32f2f}
button.warn{background:var(--warn);color:#000}
button.warn:hover{background:#f9a825}
button:disabled{opacity:0.5;cursor:not-allowed}
.input{width:100%;padding:8px;border-radius:4px;border:1px solid var(--border);background:#111;color:#fff;font-size:13px}
select.input{cursor:pointer;padding:10px}
.group{border:1px solid var(--border);border-radius:6px;padding:12px;margin:12px 0;background:#1b1b1b}
.status{min-height:1.2em;margin-top:8px;font-weight:bold;text-align:center;padding:8px;border-radius:4px;background:#0a0a0a}
.status.ok{color:var(--ok)}.status.err{color:var(--err)}.status.info{color:var(--primary)}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:12px}
@media(max-width:600px){.grid{grid-template-columns:1fr}}
details{margin:8px 0}
details[open] summary{font-weight:bold}
details summary{cursor:pointer;padding:8px;background:#0a0a0a;border-radius:4px;border:1px solid var(--border);user-select:none}
details summary:hover{background:#111}
code,pre{background:#111;border:1px solid #333;border-radius:6px;padding:6px;display:block;max-height:200px;overflow:auto;font-size:11px}
.section-title{font-size:14px;font-weight:bold;margin-top:12px;margin-bottom:8px;color:var(--primary);text-transform:uppercase;letter-spacing:0.5px}
.button-group{display:flex;gap:6px;flex-wrap:wrap;justify-content:center;margin:8px 0}
.wifi-list{margin-top:8px;max-height:180px;overflow:auto;border:1px solid var(--border);padding:6px;border-radius:6px;background:#111}
.wifi-entry{padding:8px;border-bottom:1px solid #222;display:flex;justify-content:space-between;align-items:center;cursor:pointer}
.wifi-entry:hover{background:#151515}
.wifi-entry .ssid{font-weight:bold}
.wifi-entry .meta{opacity:0.8;font-size:12px}
.wifi-empty{opacity:0.8;padding:6px;text-align:center;color:#999}
.graph-disabled{opacity:0.55;pointer-events:none}
table.u-legend.u-inline.u-live,.u-legend.u-inline.u-live{
  display:flex!important;flex-wrap:wrap!important;gap:28px!important;justify-content:center!important;align-items:center!important;width:auto!important;table-layout:auto!important;white-space:nowrap!important;overflow:visible!important;max-width:none!important;box-sizing:border-box!important;position:relative!important;margin-top:8px!important;
}
table.u-legend.u-inline.u-live,.u-legend.u-inline.u-live{
  display:flex!important;
  flex-wrap:wrap!important;
  gap:28px!important;
  justify-content:center!important;
  align-items:center!important;
  width:100%!important;
  table-layout:auto!important;
  white-space:nowrap!important;
  overflow:visible!important;
  max-width:none!important;
  box-sizing:border-box!important;
  position:relative!important;
  margin-top:8px!important;
}

table.u-legend.u-inline.u-live>tbody,
table.u-legend.u-inline.u-live>tbody>tr,
table.u-legend.u-inline.u-live tr.u-series{
  display:inline-flex!important;
  flex-direction:row!important;
  align-items:center!important;
  justify-content:center!important;
  gap:8px!important;
  vertical-align:middle!important;
  margin:0!important;
  padding:0!important;
  border:none!important;
}

.u-legend.u-inline.u-live td,
.u-legend.u-inline.u-live th,
.u-legend.u-inline.u-live .u-label,
.u-legend.u-inline.u-live .u-value{
  display:inline-block!important;
  white-space:nowrap!important;
  overflow:visible!important;
  max-width:none!important;
  text-overflow:clip!important;
  word-break:normal!important;
  box-sizing:border-box!important;
  padding:0 4px!important;
}
/* Time line placeholder */
#chart-time{
  display:block;
  text-align:center;
  width:100%;
  margin:4px 0;
  font-size:12px;
}
.chart-wrap,#chart{overflow:visible!important;padding-bottom:56px!important}
@media(max-width:600px){
  table.u-legend.u-inline.u-live{gap:12px!important;padding-bottom:8px!important;white-space:normal!important}
  table.u-legend.u-inline.u-live>tbody>tr{display:inline-flex!important;flex-wrap:nowrap!important}
}
/* MEC20 Temp Coeff note style */
.mec20-note{font-size:12px;opacity:0.85;margin-top:4px;color:#ccc}
</style>
</head>
<body>
<div class="container">
  <div class="header">
    <h2>Pirate Controls Sensor Controller <span id="ap-badge" class="badge" style="display:none">AP Mode</span>
    <span id="sensor-type-badge" class="badge" style="background:#333">THCS</span> <!-- sensor type badge -->
    </h2>
    <div class="small">
      <span id="time-sync" class="badge">Time N/A</span>
      <span id="time-now" class="notice"></span>
      <span id="debug-badge" class="badge" style="display:none">DBG</span>
    </div>
  </div>
  <div class="tabs">
    <div class="tab active" data-tab="sensor">Sensor</div>
    <div class="tab" data-tab="graph">Graph</div>
    <div class="tab" data-tab="wifi">WiFi</div>
    <div class="tab" data-tab="mqtt">MQTT</div>
    <div class="tab" data-tab="cal">Calibration</div>
    <div class="tab" data-tab="system">System</div>
  </div>
  <div id="sensor" class="section active">
    <div class="row"><div class="label">Sensor Status</div><div><span id="sensor-status" class="badge">--</span></div></div>
    <div class="row"><div class="label">VWC</div><div><span id="vwc">--</span><span class="unit">%</span></div></div>
    <div class="row"><div class="label">Temp</div><div><span id="temp">--</span> <span id="temp-unit" class="unit">&deg;C</span></div></div>
    <div class="row"><div class="label">pwEC</div><div><span id="pwec">--</span> <span class="unit">dS/m</span></div></div>
    <div class="row"><div class="label">Bulk EC</div><div><span id="ec">--</span> <span class="unit">&micro;S/cm</span></div></div>
    <div class="row"><div class="label">Raw EC</div><div><span id="raw_ec">--</span></div></div>
    <div class="section-title">System Summary</div>
    <div class="row"><div class="label">Sensor #</div><div><span id="sensor-number">--</span></div></div>
    <div class="row"><div class="label">WiFi</div><div><span id="wifi-badge" class="badge">--</span> <span id="ip" class="notice"></span></div></div>
    <div class="row">
      <div class="label">MQTT</div>
      <div>
        <span id="mqtt-badge" class="badge">--</span>
        <span id="mqtt-topic-display" class="notice"></span>
      </div>
    </div>
    <div class="row"><div class="label">Uptime</div><div><span id="uptime">--</span> <span id="uptime-human" class="notice"></span></div></div>
    <div class="controls">
      <button onclick="confirmModal('Toggle Temperature Unit','Switch between Celsius and Fahrenheit?',toggleTempUnit)">°C/°F</button>
      <button onclick="confirmModal('Reboot Device','This will restart the device. Continue?',reboot)">Reboot</button>
    </div>
    <div id="sensor-status-msg" class="status"></div>
  </div>
  <div id="graph" class="section">
    <div class="controls">
      <button onclick="fetchChart(4)">4h</button>
      <button onclick="fetchChart(8)">8h</button>
      <button onclick="fetchChart(12)">12h</button>
      <button onclick="fetchChart(24)">24h</button>
      <button onclick="fetchChart(36)">36h</button>
    </div>
    <div class="chart-wrap" id="chart-wrap">
      <div id="chart" style="width:100%;height:320px;"></div>
    </div>
    <div id="graph-msg" class="status"></div>
    <div class="controls">
      <button onclick="downloadHistoryCsv()">Download 7d CSV</button>
    </div>
  </div>
  <div id="wifi" class="section">
    <div class="section-title">WiFi Status</div>
    <div class="row"><div class="label">WiFi Connection</div><div><span id="wifi2-badge" class="badge">--</span></div></div>
    <div class="row"><div class="label">Current IP</div><div><span id="ip2" class="notice">--</span></div></div>
    <div class="row"><div class="label">AP Mode</div><div><span id="ap-mode-badge" class="badge">--</span></div></div>
    <details>
      <summary>WiFi Settings</summary>
      <div class="grid" style="margin-top:8px">
        <div>
          <label>SSID</label>
          <input id="wifi-ssid" class="input" placeholder="MyWiFi" list="wifi-ssids">
          <datalist id="wifi-ssids"></datalist>
        </div>
        <div>
          <label>Password</label>
          <input id="wifi-pass" class="input" type="password" placeholder="••••••••" required aria-required="true">
        </div>
      </div>
      <div class="controls">
        <button onclick="confirmModal('Save WiFi Settings','Connect to this network and reboot?<br>SSID: <b>'+$('wifi-ssid').value+'</b>',saveWiFi)">Save & Reboot</button>
        <button onclick="confirmModal('Reconnect Now','Attempt to reconnect to WiFi immediately?',wifiRetry)">Reconnect</button>
        <button id="btn-wifi-scan" onclick="confirmModal('Scan Networks','Scan nearby WiFi networks and populate the SSID list?',wifiScan)">Scan Networks</button>
      </div>
      <div id="wifi-msg" class="status"></div>
      <div id="wifi-scan-info" class="small" style="margin-top:6px;display:block"></div>
      <div id="wifi-list" class="wifi-list" aria-live="polite">
        <div class="wifi-empty">No scan performed yet. Click "Scan Networks" to search for access points.</div>
      </div>
      <div class="small" style="margin-top:8px">If device enters AP Mode, connect to SSID <b>"PC Soil Sensor Setup"</b> (password <b>"Controls"</b>) and browse to <b>192.168.4.1</b>.</div>
    </details>
  </div>
  <div id="mqtt" class="section">
    <div class="section-title">MQTT Status</div>
    <div class="row"><div class="label">MQTT Connection</div><div><span id="mqtt2-badge" class="badge">--</span></div></div>
    <div class="row"><div class="label">HA Discovery</div><div><span id="ha-badge" class="badge">--</span></div></div>
    <div class="row"><div class="label">User Topic</div><div><span id="mqtt-topic2" class="notice">--</span></div></div>
    <div class="row" id="ha-row" style="display:none;"><div class="label">HA Discovery Topic</div><div><span id="ha-topic" class="notice">--</span></div></div>
    <details>
      <summary>MQTT Connection</summary>
      <div class="grid" style="margin-top:8px">
        <div><label>Server</label><input id="mqtt-server" class="input" placeholder="192.168.1.50"></div>
        <div><label>Port</label><input id="mqtt-port" class="input" type="number" placeholder="1883"></div>
        <div><label>Username</label><input id="mqtt-user" class="input" placeholder="(optional)"></div>
        <div>
          <label>Allow anonymous</label>
          <input id="mqtt-anon" type="checkbox" aria-label="Allow anonymous MQTT (no password)">
          <span class="small" style="opacity:0.8;margin-left:6px">Allow anonymous (no password)</span>
        </div>
        <div>
          <label>Password</label>
          <input id="mqtt-pass" class="input" type="password" placeholder="(optional)">
        </div>
      </div>
      <div class="controls">
        <button onclick="confirmModal('Save MQTT Settings','Connect to MQTT broker?<br>Server: <b>'+$('mqtt-server').value+':'+$('mqtt-port').value+'</b>',saveMqttConn)">Save & Reconnect</button>
        <button id="btn-mqtt-toggle" onclick="confirmModal('Toggle MQTT','Enable/Disable MQTT connection and publishing?',toggleMqtt)">Enable MQTT</button>
      </div>
    </details>
    <details>
      <summary>User Topic</summary>
      <div style="margin-top:8px">
        <label>MQTT Topic</label>
        <input id="mqtt-topic-input" class="input" placeholder="sensor/thcs" style="margin-top:4px;margin-bottom:8px">
        <div class="controls"><button onclick="confirmModal('Save MQTT Topic','Update MQTT topic?',saveMqttTopic)">Save Topic</button></div>
      </div>
    </details>
    <details>
      <summary>Home Assistant Discovery</summary>
      <div style="margin-top:8px">
        <div class="row"><div class="label">MQTT Server</div><div><span id="mqtt-server-status" class="notice">--</span></div></div>
        <div class="controls" style="margin-bottom:12px">
          <button onclick="confirmModal('Re-Announce to HA','Re-publish discovery configs to Home Assistant?',haAnnounce)">Re-publish discovery Config</button>
          <button id="ha-toggle-btn" onclick="confirmModal('Toggle HA Discovery','Enable/disable Home Assistant discovery?',toggleHA)">Enable HA Discovery</button>
        </div>
        <div class="small"><b>Note:</b> When enabled, device automatically publishes configs to Home Assistant MQTT discovery topics.</div>
      </div>
    </details>
    <details>
      <summary>Publish Mode</summary>
      <div style="margin-top:8px">
        <div style="margin-bottom:12px">
          <label style="display:block;margin-bottom:8px"><input type="radio" name="pubmode" value="always" onchange="updateDeltaDisplay()"> <b>Always</b> - Publish every sensor reading</label>
          <label style="display:block"><input type="radio" name="pubmode" value="delta" onchange="updateDeltaDisplay()"> <b>On Change</b> - Publish only when value changes beyond threshold</label>
        </div>
        <div id="delta-grid" style="display:none;margin-top:12px">
          <div class="grid">
            <div><label>VWC Δ (%)</label><input id="d-vwc" class="input" type="number" step="0.01" value="0.10"></div>
            <div><label>Temp Δ (°C)</label><input id="d-temp" class="input" type="number" step="0.1" value="0.20"></div>
            <div><label>EC Δ (µS/cm)</label><input id="d-ec" class="input" type="number" step="1" value="5"></div>
            <div><label>pwEC Δ (dS/m)</label><input id="d-pwec" class="input" type="number" step="0.01" value="0.02"></div>
          </div>
        </div>
        <div class="controls"><button onclick="confirmModal('Save Publish Mode','Update publish mode and thresholds?',savePublishMode)">Save Settings</button></div>
        <div class="small" style="margin-top:8px"><b>Example:</b> If VWC Δ = 0.10%, the device will only publish when VWC changes by 0.10% or more.</div>
      </div>
    </details>
    <div id="mqtt-msg" class="status"></div>
  </div>
  <div id="cal" class="section">
    <div class="section-title">Calibration</div>
    <div class="group">
      <div class="section-title" style="margin-top:0;margin-bottom:6px;color:#fff">Calibration Status</div>
      <div class="row"><div class="label">EC Slope</div><div><span id="cal-status-slope" class="notice">--</span></div></div>
      <div class="row"><div class="label">EC Intercept (µS/cm)</div><div><span id="cal-status-intercept" class="notice">--</span></div></div>
      <div class="row"><div class="label">Temp Coeff (/°C)</div><div><span id="cal-status-tc" class="notice">--</span></div></div>
      <div class="row">
        <div class="label">Temp</div>
        <div>
          <span id="cal-temp">--</span>
          <span id="cal-temp-unit" class="unit">&deg;C</span>
        </div>
      </div>
    </div>
    <details style="margin-top:8px">
      <summary>Calibration</summary>
      <div class="grid" style="margin-top:8px">
        <div>
          <label>EC Slope</label>
          <input id="cal-slope" class="input" type="number" step="0.0001" placeholder="e.g. 1.0000" value="">
        </div>
      </div>
      <div class="grid" style="margin-top:8px">
        <div>
          <label>EC Intercept (µS/cm)</label>
          <input id="cal-intercept" class="input" type="number" step="0.1" placeholder="e.g. 0.0" value="">
        </div>
      </div>
      <div class="grid" style="margin-top:8px">
        <div>
          <label>Temp Coefficient (per °C)</label>
          <input id="cal-tc" class="input" type="number" step="0.001" placeholder="e.g. 0.019" value="">
          <div id="mec20-tc-note" class="mec20-note" style="display:none">
            For MEC20: sensor internal EC temp coefficient is fixed at 2%/°C (ECTEMPCOFF). Firmware value here is disabled.
          </div>
        </div>
      </div>
      <div class="controls" style="margin-top:8px">
        <button onclick="confirmModal('Save Calibration','Apply EC slope/intercept and temperature coefficient?',saveCal)">Save Calibration</button>
      </div>
      <div id="cal-msg" class="status"></div>
    </details>
    <details>
      <summary>Two-Point Calibration Guide</summary>
      <div style="margin-top:8px;line-height:1.5">
        <div class="small" style="margin-bottom:6px">
          Use two buffer solutions and the measured raw EC readings in those buffers to compute slope and intercept.
        </div>
        <div style="margin-bottom:8px">
          <label style="margin-right:12px">
            <input type="radio" name="cal-method" value="quick" checked onchange="toggleCalMethod()"> No Temp Compensation (No TC)
          </label>
          <label>
            <input type="radio" name="cal-method" value="chart" onchange="toggleCalMethod()"> Manual Temp Compensation (Manual TC)
          </label>
          <div class="small" style="margin-top:6px;opacity:0.9"></div>
        </div>
        <div class="grid">
          <div>
            <label>Low Buffer Solution — µS (@25 °C)</label>
            <input id="buf-low" class="input" type="number" step="0.1" value="84" style="margin-top:4px">
          </div>
          <div>
            <label>High Buffer Solution — µS (@25 °C)</label>
            <input id="buf-high" class="input" type="number" step="0.1" value="1413" style="margin-top:4px">
          </div>
          <div>
            <label>Low RawEC Reading</label>
            <input id="raw-low" class="input" type="number" step="0.1" placeholder="raw_low" style="margin-top:4px">
          </div>
          <div>
            <label>High RawEC Reading</label>
            <input id="raw-high" class="input" type="number" step="0.1" placeholder="raw_high" style="margin-top:4px">
          </div>
          <div>
            <label>TDC Low µS Buffer Solution µS</label>
            <input id="buf-low-tdc" class="input" type="number" step="0.1" placeholder="uS value from TDC" style="margin-top:4px;display:none">
          </div>
          <div>
            <label>TDC High µS Buffer Solution</label>
            <input id="buf-high-tdc" class="input" type="number" step="0.1" placeholder="uS value from TDC" style="margin-top:4px;display:none">
          </div>
        </div>
        <div class="controls" style="margin-top:8px">
          <button onclick="calculateCalibration()">Calculate</button>
          <button onclick="confirmModal('Apply Calculated Calibration','Copy calculated slope/intercept to inputs so you can Save calibration?',applyCalculatedCal)">Copy to Inputs</button>
        </div>
        <div style="margin-top:8px">
          <div class="small"><b>Calculated:</b></div>
          <div class="row" style="margin-top:6px">
            <div class="label">Slope</div><div><span id="calc-slope" class="notice">--</span></div>
          </div>
          <div class="row" style="margin-top:6px">
            <div class="label">Intercept (µS/cm)</div><div><span id="calc-intercept" class="notice">--</span></div>
          </div>
          <div id="calc-msg" class="status" style="margin-top:6px"></div>
        </div>
        <div class="small" style="margin-top:8px">
          Steps:<br>
          1) Place probe in low buffer and record raw EC.<br>
          2) Place probe in high buffer and record raw EC.<br>
          3) Optionally use bottle TDC µS values for measurement temperature and enter in TDC fields (Manual TC).<br>
          4) Press Calculate.<br>
          5) Click "Copy to Inputs" then Save Calibration.
        </div>
      </div>
    </details>
  </div>
  <div id="system" class="section">
    <div class="section-title">System Summary</div>
    <div class="row"><div class="label">Uptime</div><div><span id="sys-uptime">--</span> <span id="sys-uptime-human" class="notice"></span></div></div>
    <div class="row"><div class="label">WiFi</div><div><span id="sum-wifi-badge" class="badge">--</span> <span id="sum-ip" class="notice"></span></div></div>
    <div class="row"><div class="label">MQTT</div><div><span id="sum-mqtt-badge" class="badge">--</span></div></div>
    <div class="row"><div class="label">Sensor #</div><div><span id="sum-sensor-number">--</span></div></div>
    <div class="row"><div class="label">WebSerial</div><div><span id="webserial-badge" class="badge">--</span></div></div>
    <div class="row"><div class="label">Heap</div><div>free:<span id="sys-heap">--</span> min:<span id="sys-heap-min">--</span></div></div>
    <div class="row"><div class="label">Timezone</div><div><span id="sum-timezone">--</span></div></div>
    <details>
      <summary>Sensor Configuration</summary>
      <div style="margin-top:8px">
        <div class="section-title">Sensor Configuration</div>
        <div style="margin-bottom:12px">
          <label>Sensor # (1–255)</label>
          <input id="sensor-id" class="input" type="number" min="1" max="255" value="1" style="margin-top:4px;margin-bottom:8px">
          <div class="small">On save, device appends /sN to the user MQTT topic.</div>
          <div class="controls"><button onclick="confirmModal('Set Sensor Number','Change sensor number to: '+$('sensor-id').value+'?',saveSensorNumber)">Save Sensor #</button></div>
        </div>
        <div style="margin-bottom:12px">
          <label>Sensor Poll Interval (ms)</label>
          <input id="sensor-ms" class="input" type="number" min="1000" max="60000" step="500" value="5000" style="margin-top:4px;margin-bottom:8px">
          <div class="small">Time between sensor reads (1000–60000 ms) Default 5000ms = 5sec.</div>
          <div class="controls"><button onclick="confirmModal('Save Poll Interval','Update sensor poll interval?',saveSensorInterval)">Save Poll Interval</button></div>
        </div>
        <div style="margin-bottom:12px">
          <label>Sensor Type</label>
          <select id="sensor-type-select" class="input" style="margin-top:4px;margin-bottom:8px">
            <option value="thcs">THCS (Default)</option>
            <option value="mec20">MEC20</option>
          </select>
          <div class="small">Select the active sensor hardware. Switching reconfigures RS485 baud automatically.</div>
          <div class="controls"><button onclick="confirmModal('Save Sensor Type','Switch active sensor driver to: '+$('sensor-type-select').value+'?',saveSensorType)">Save Sensor Type</button></div>
        </div>
      </div>
    </details>
    <details>
      <summary style="margin-top:16px">System Configuration</summary>

      <div style="margin-top:8px">
        <div class="section-title">Timezone</div>
        <div class="grid" style="margin-top:8px">
          <div>
            <label>Timezone</label>
            <select id="tz-simple-select" class="input" style="margin-top:4px"></select>
          </div>
          <div>
            <label>Daylight Saving</label>
            <div style="display:flex;align-items:center;gap:8px;margin-top:6px">
              <input id="dst-toggle" type="checkbox" aria-label="Enable DST">
              <span id="tz-effective" class="small">Effective: UTC+00:00</span>
            </div>
          </div>
        </div>
        <div class="controls" style="margin-top:8px">
          <button onclick="saveTime()">Save Timezone</button>
        </div>
      </div>

      <div style="margin-top:8px"></div>

      <div class="controls">
        <button onclick="confirmModal('Toggle Temperature Unit','Switch between Celsius and Fahrenheit?',toggleTempUnit)">°C/°F</button>
        <button onclick="confirmModal('Reboot Device','This will restart the device. Continue?',reboot)">Reboot</button>
        <button id="webserial-btn" onclick="confirmModal('Toggle WebSerial','Enable/disable WebSerial console?',toggleWebSerial)">Enable WebSerial</button>
        <button id="debug-btn" onclick="confirmModal('Toggle Debug Mode','Enable/disable verbose Debug Mode?<br><b>Note:</b> May increase logging and reduce performance if left on.',toggleDebugMode)">Enable Debug Mode</button>
        <button class="warn" onclick="confirmModal('Clear History','Erase all stored historical data?<br><b>This cannot be undone.</b>',clearHistory)">Clear History</button>
        <button class="danger" onclick="confirmModal('Factory Reset','<b>Warning:</b> This will erase ALL settings and history.<br>Device will reboot in AP mode.<br>Continue?',factoryReset)">Factory Reset</button>
      </div>
    </details>
    <div class="small" id="meta" style="margin-top:16px;padding-top:12px;border-top:1px solid #333">FW: --, Build: --, ChipID: --, MAC: --, @n0socialskills</div>
  </div>
</div>
<div id="modal" style="display:none;position:fixed;inset:0;background:rgba(0,0,0,.7);align-items:center;justify-content:center;z-index:1000;backdrop-filter:blur(2px)">
  <div style="background:#1f1f1f;padding:20px;border-radius:8px;max-width:450px;width:95%;border:1px solid #333;box-shadow:0 4px 20px rgba(0,0,0,0.5)">
    <h3 id="modal-title" style="margin-top:0;margin-bottom:12px;color:var(--primary)">Confirm</h3>
    <div id="modal-body" style="margin:8px 0 20px 0;color:#ccc;line-height:1.5"></div>
    <div style="display:flex;gap:8px;justify-content:flex-end">
      <button onclick="modalConfirm()" id="modal-ok" style="background:var(--ok)">OK</button>
      <button onclick="modalClose()" style="background:#666">Cancel</button>
    </div>
  </div>
</div>
<script>
let ws,useCelsius=true,configLoaded=false,pendingAction=null,lastStatus={};
let haEnabledState=null,webserialState=null,debugModeState=null;
let uplotChart=null,historyAcc={t:[],vwc:[],temp:[],pwec:[]},historyUpdateScheduled=false,lastHistoryHours=24;
let currentSensorType='thcs'; // track active sensor type client-side

function $(id){return document.getElementById(id)}
function setBadge(el,ok){if(!el)return;el.className='badge '+(ok?'ok':'err');el.textContent=ok?'OK':'--'}
function confirmModal(title,body,action){const mt=$('modal-title'),mb=$('modal-body');if(mt)mt.textContent=title;if(mb)mb.innerHTML=body;pendingAction=action;const md=$('modal');if(md)md.style.display='flex'}
function modalClose(){const md=$('modal');if(md)md.style.display='none';pendingAction=null}
function modalConfirm(){if(pendingAction)pendingAction();modalClose()}
function updateDeltaDisplay(){const mode=document.querySelector('input[name="pubmode"]:checked')?.value||'always',deltaGrid=$('delta-grid');if(deltaGrid)deltaGrid.style.display=mode==='delta'?'block':'none'}

function setupTabs(){
  const tabsContainer=document.querySelector('.tabs');if(!tabsContainer)return;
  tabsContainer.addEventListener('click',e=>{
    const t=e.target.closest('.tab');if(!t||!tabsContainer.contains(t))return;
    const targetName=t.dataset.tab;if(!targetName)return;
    document.querySelectorAll('.tab').forEach(x=>x.classList.remove('active'));
    document.querySelectorAll('.section').forEach(x=>x.classList.remove('active'));
    t.classList.add('active');
    const section=document.getElementById(targetName);if(section)section.classList.add('active');
    if(targetName==='graph'){if(!uplotChart)initChart();fetchChart(8);}
  });
  document.querySelectorAll('.tab').forEach(tabEl=>{
    tabEl.setAttribute('role','tab');
    tabEl.tabIndex=0;
    tabEl.addEventListener('keydown',ev=>{
      if(ev.key==='Enter'||ev.key===' '){ev.preventDefault();tabEl.click();}
    });
  });
}

function initChart(){
  const el=$('chart');if(!el)return;
  const opts={
    width:el.clientWidth||800,
    height:320,
    title:'Soil History',
    tzDate:ts=>new Date(ts),
    scales:{x:{time:true},vwc:{auto:true},temp:{auto:true},pwec:{auto:true}},
    series:[
      {},
      {label:'VWC (%)',scale:'vwc',stroke:'#3498db',width:1,value:(u,v)=>v==null?'':v.toFixed(1)},
      {label:'Temp',scale:'temp',stroke:'#F44336',width:1,value:(u,v)=>v==null?'':v.toFixed(1)},
      {label:'pwEC',scale:'pwec',stroke:'#4CAF50',width:1,value:(u,v)=>v==null?'':v.toFixed(1)}
    ],
    axes:[
      {stroke:'#aaa',grid:{stroke:'#2b2b2b'}},
      {scale:'vwc',label:'VWC (%)',stroke:'#aaa',grid:{stroke:'#2b2b2b'}},
      {scale:'temp',label:'Temperature',stroke:'#aaa',side:1},
      {scale:'pwec',label:'pwEC (dS/m)',stroke:'#aaa',side:1}
    ]
  };
  uplotChart=new uPlot(opts,[[],[],[],[]],el);
  setTimeout(cleanupLegend,120);
}

function clearHistoryAcc(){historyAcc.t.length=0;historyAcc.vwc.length=0;historyAcc.temp.length=0;historyAcc.pwec.length=0}
function scheduleHistoryUpdate(){if(historyUpdateScheduled)return;historyUpdateScheduled=true;setTimeout(()=>{historyUpdateScheduled=false;updateChartFromAcc()},200)}

function cleanupLegend(){
  try{
    const legend=document.querySelector('.u-legend.u-inline.u-live')||document.querySelector('.u-legend');if(!legend)return;
    legend.querySelectorAll('td,th,.u-label,.u-value').forEach(el=>{
      const txt=(el.textContent||'').trim();
      if(txt===':'||txt==='')el.style.display='none';else el.style.display='';
    });
  }catch(e){console.debug('cleanupLegend error',e);}
}

function updateChartFromAcc(){
  if(!uplotChart)initChart();
  if(!uplotChart)return;
  const n=historyAcc.t.length;
  if(n===0){
    const gm=$('graph-msg');
    if(gm){gm.className='status info';gm.textContent='No history to display yet. Ensure time is synced (Time OK badge) and let the device run for a few minutes.';}
    uplotChart.setData([[],[],[],[]]);
    setTimeout(cleanupLegend,60);
    return;
  }
  const gm=$('graph-msg');if(gm)gm.textContent='';
  uplotChart.setData([historyAcc.t,historyAcc.vwc,historyAcc.temp,historyAcc.pwec]);
  setTimeout(cleanupLegend,60);
}

function handleHistoryChunk(d){
  let arr=d.data;
  if(typeof arr==='string'){try{arr=JSON.parse(arr)}catch(e){arr=[]}}
  if(!Array.isArray(arr)||arr.length===0)return;
  arr.forEach(p=>{
    const tMs=(p.t||0)*1000;
    historyAcc.t.push(tMs);
    historyAcc.vwc.push(p.vwc!==undefined?Number(p.vwc):null);
    if(p.temp!==undefined){
      const tc=Number(p.temp);
      historyAcc.temp.push(useCelsius?tc:(tc*9/5+32));
    }else historyAcc.temp.push(null);
    historyAcc.pwec.push(p.pwec!==undefined?Number(p.pwec):null);
  });
  scheduleHistoryUpdate();
}
function handleHistoryEnd(_d){updateChartFromAcc()}
function downloadHistoryCsv(){window.open('/download/history?hours=168','_blank')}

function fetchChart(hours){
  lastHistoryHours=hours||24;
  clearHistoryAcc();
  const gm=$('graph-msg');
  if(gm){gm.className='status info';gm.textContent='Loading history...'}
  if(ws&&ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify({type:'get_chart_data',hours}));
}

function connectWS(){
  const proto=location.protocol==='https:'?'wss:':'ws:';
  const url=proto+'//'+location.host+'/ws';
  ws=new WebSocket(url);
  ws.onopen=()=>{
    ws.send(JSON.stringify({type:'get_config'}));
    ws.send(JSON.stringify({type:'get_status'}));
    ws.send(JSON.stringify({type:'request_meta'}));
    setInterval(()=>{if(ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify({type:'ping',t:Date.now()}))},10000);
  };
  ws.onmessage=ev=>{
    try{
      const d=JSON.parse(ev.data);
      switch(d.type){
        case 'sensor':updateSensor(d);break;
        case 'status':updateStatus(d);break;
        case 'config_update':loadConfig(d);break;
        case 'history_chunk':handleHistoryChunk(d);break;
        case 'history_end':handleHistoryEnd(d);break;
        case 'meta':updateMeta(d);break;
        case 'sensor_type_update':
          toast(d.message||'Sensor type updated','info');
          currentSensorType=d.sensorType||'thcs';
          updateSensorTypeBadge();
          updateCalTempCoeffUI();
          break;
        case 'ap_state':
        case 'wifi_update':
        case 'mqtt_update':
        case 'mqtt_topic_update':
        case 'calibration_update':
        case 'temp_unit_changed':
        case 'time_update':
        case 'ota_auth_update':
        case 'sensor_interval_update':
        case 'publish_mode_update':
        case 'history_cleared':
        case 'ha_state':
        case 'reboot_initiated':
        case 'reset_initiated':
          toast(d.message||'OK','info');break;
        case 'debug_mode_state':
          debugModeState=!!d.enabled;updateDebugButton();setDebugBadge();toast(d.message||(debugModeState?'Debug Mode enabled':'Debug Mode disabled'),'info');break;
        case 'webserial_state':
          webserialState=!!d.enabled;updateWebSerialButton();setWebSerialBadge();toast(d.message||(webserialState?'WebSerial enabled':'WebSerial disabled'),'info');break;
        case 'mqtt_state':
          lastStatus.mqtt_enabled=!!d.enabled;updateMqttControls();toast(d.message||(lastStatus.mqtt_enabled?'MQTT enabled':'MQTT disabled'),'info');break;
        case 'pong':break;
        case 'error':toast(d.message||'Error','err');break;
      }
    }catch(e){console.error('WS error',e);}
  };
  ws.onclose=()=>setTimeout(connectWS,1500);
}

function updateSensor(d){
  const num=v=>{const n=parseFloat(v);return isNaN(n)?NaN:n};
  const vwc=num(d.soil_hum),tmp=num(d.soil_temp),pwec=num(d.soil_pw_ec),ec=num(d.soil_ec),raw=num(d.raw_ec);
  if(!isNaN(vwc)&&$('vwc'))$('vwc').textContent=vwc.toFixed(1);
  if(!isNaN(tmp)&&$('temp')){
    if(useCelsius){
      $('temp').textContent=tmp.toFixed(1);
      if($('temp-unit')) $('temp-unit').innerHTML='&deg;C';
      if($('cal-temp')) $('cal-temp').textContent = tmp.toFixed(1);
      if($('cal-temp-unit')) $('cal-temp-unit').innerHTML = '&deg;C';
    } else {
      $('temp').textContent=(tmp*9/5+32).toFixed(1);
      if($('temp-unit')) $('temp-unit').innerHTML='&deg;F';
      if($('cal-temp')) $('cal-temp').textContent = (tmp*9/5+32).toFixed(1);
      if($('cal-temp-unit')) $('cal-temp-unit').innerHTML = '&deg;F';
    }
  }
  if(!isNaN(pwec)&&$('pwec'))$('pwec').textContent=pwec.toFixed(2);
  if(!isNaN(ec)&&$('ec'))$('ec').textContent=ec.toFixed(0);
  if(!isNaN(raw)){
    if($('raw_ec'))$('raw_ec').textContent=raw.toFixed(0);
    const calRaw=$('cal-status-raw');if(calRaw)calRaw.textContent=raw.toFixed(0);
  }
  const s=$('sensor-status');
  if(s){s.className='badge '+(d.sensor_error?'err':'ok');s.textContent=d.sensor_error?'ERROR':'OK'}
}

function updateStatus(d){
  lastStatus=d;
  if(d.sensor_type){
    currentSensorType=String(d.sensor_type);
    updateSensorTypeBadge();
    updateCalTempCoeffUI();
  }
  try{
    if($('wifi-badge'))setBadge($('wifi-badge'),!!d.wifi_connected);
    if($('mqtt-badge'))setBadge($('mqtt-badge'),!!d.mqtt_connected);
    if($('ip'))$('ip').textContent=d.ip||'';
    const effTopic=d.mqtt_topic||'';
    if($('mqtt-topic-display'))$('mqtt-topic-display').textContent=effTopic;
    try{
      const inputEl=$('mqtt-topic-input');
      if(inputEl&&document.activeElement!==inputEl)inputEl.value=effTopic;
    }catch(ex){}
    if($('uptime'))$('uptime').textContent=(typeof d.uptime_sec==='number'?d.uptime_sec:Number(d.uptime||0))+'s';
    if($('uptime-human'))$('uptime-human').textContent=d.uptime_human||'';
    if($('ap-badge'))$('ap-badge').style.display=d.ap_active?'inline-block':'none';
    const ts=$('time-sync');
    if(ts){ts.textContent=d.time_synced?'Time OK':'Time N/A';ts.className='badge '+(d.time_synced?'ok':'err')}
    const tnow=$('time-now');if(tnow)tnow.textContent=d.time_str||'';
    useCelsius=!(d.useFahrenheit===true);
    if(typeof d.sensor_number==='number'&&$('sensor-number'))$('sensor-number').textContent=d.sensor_number;
    debugModeState=!!(d.debug_mode===true);setDebugBadge();updateDebugButton();
    if(typeof d.sensor_number==='number'&&$('sum-sensor-number'))$('sum-sensor-number').textContent=d.sensor_number;
    if($('sum-wifi-badge'))setBadge($('sum-wifi-badge'),!!d.wifi_connected);
    if($('sum-ip'))$('sum-ip').textContent=d.ip||'';
    if($('sum-mqtt-badge'))setBadge($('sum-mqtt-badge'),!!d.mqtt_connected);
    if($('sys-heap'))$('sys-heap').textContent=d.heap_free??0;
    if($('sys-heap-min'))$('sys-heap-min').textContent=d.heap_min_free??d.heap_min??0;
    if($('sys-uptime'))$('sys-uptime').textContent=(d.uptime_seconds??d.uptime_sec??0)+'s';
    if($('sys-uptime-human'))$('sys-uptime-human').textContent=d.uptime_human||'';
    const wOn=!!(d.webserial_enabled||d.webSerialEnabled);webserialState=wOn;setWebSerialBadge();updateWebSerialButton();
    if(typeof d.ecSlope!=='undefined'&&$('cal-status-slope'))$('cal-status-slope').textContent=String(d.ecSlope);
    if(typeof d.ecIntercept!=='undefined'&&$('cal-status-intercept'))$('cal-status-intercept').textContent=String(d.ecIntercept);
    if(typeof d.ecTempCoeff!=='undefined'&&$('cal-status-tc'))$('cal-status-tc').textContent=String(d.ecTempCoeff);
    if(typeof d.raw_ec!=='undefined'&&$('cal-status-raw'))$('cal-status-raw').textContent=String(d.raw_ec);
    if($('sum-timezone')){
      if(d.tz_name||d.tz_abbrev||d.tz_offset_str){
        const name=d.tz_name||'',abbr=d.tz_abbrev||'',off=d.tz_offset_str||'';
        $('sum-timezone').textContent=name?`${name} — ${abbr||off}`:(abbr||off||'--');
      }else if(typeof d.timezone_offset!=='undefined'){
        const fmtSecs=secs=>{
          const s=Number(secs)||0,sign=s>=0?'+':'-',abs=Math.abs(s),h=Math.floor(abs/3600),m=Math.round((abs%3600)/60);
          if(m===60)return`${sign}${h+1}h`;
          if(m===0)return`${sign}${h}h`;
          return m===0?`${sign}${h}h`:`${sign}${h}h${m}m`;
        };
        $('sum-timezone').textContent=`GMT ${fmtSecs(d.timezone_offset)}, DST ${fmtSecs(d.dst_offset||0)}`;
      }else $('sum-timezone').textContent='--';
    }
    if(typeof d.modbus_id!=='undefined'&&$('sum-modbus-id'))$('sum-modbus-id').textContent=d.modbus_id;
    if(typeof d.modbus_baud!=='undefined'&&$('sum-modbus-baud'))$('sum-modbus-baud').textContent=d.modbus_baud;
    if($('wifi2-badge'))setBadge($('wifi2-badge'),!!d.wifi_connected);
    if($('ip2'))$('ip2').textContent=d.ip||'--';
    const apOn=!!(d.ap_active);const apBadge=$('ap-mode-badge');
    if(apBadge){apBadge.className='badge '+(apOn?'ok':'err');apBadge.textContent=apOn?'Enabled':'Disabled'}
    const m2=$('mqtt2-badge');if(m2){m2.className='badge '+(d.mqtt_connected?'ok':'err');m2.textContent=d.mqtt_connected?'Connected':'--'}
    if($('mqtt-topic2'))$('mqtt-topic2').textContent=d.mqtt_topic||'--';
    const ha=!!(d.ha_enabled);const haRow=$('ha-row');
    if(haRow){
      if(ha){
        haRow.style.display='flex';
        const serverHa=d.ha_topic||null;
        if(serverHa&&$('ha-topic'))$('ha-topic').textContent=serverHa;
        else if($('ha-topic')){
          const baseId=(d.mac&&String(d.mac).length)?String(d.mac).replace(/:/g,'').slice(-6).toUpperCase():(d.chipid||'');
          $('ha-topic').textContent='thcs/'+baseId+'/sensor';
        }
      }else haRow.style.display='none';
    }
    const hab=$('ha-badge');if(hab){hab.className='badge '+(ha?'ok':'err');hab.textContent=ha?'Enabled':'Disabled'}
    haEnabledState=ha;
    if($('ha-toggle-btn'))$('ha-toggle-btn').textContent=ha?'Disable HA Discovery':'Enable HA Discovery';
    if($('mqtt-server-status'))$('mqtt-server-status').textContent=d.mqtt_server||'--';
    lastStatus.mqtt_enabled=!!d.mqtt_enabled;updateMqttControls();
  }catch(ex){console.error('updateStatus error:',ex);}
}

function loadConfig(c){
  if(!configLoaded){
    try{
      if($('wifi-ssid'))$('wifi-ssid').value=c.ssid||'';
      if($('mqtt-server'))$('mqtt-server').value=c.mqttServer||'';
      if($('mqtt-port'))$('mqtt-port').value=c.mqttPort||'';
      if($('mqtt-user'))$('mqtt-user').value=c.mqttUsername||'';
      const currentTopic=c.mqttTopic||'';
      if($('mqtt-topic-input'))$('mqtt-topic-input').value=currentTopic;
      if($('mqtt-topic-display'))$('mqtt-topic-display').textContent=currentTopic;
      if($('cal-slope'))$('cal-slope').value=(c.ecSlope??'').toString();
      if($('cal-intercept'))$('cal-intercept').value=(c.ecIntercept??'').toString();
      if($('cal-tc'))$('cal-tc').value=(c.ecTempCoeff??'').toString();
      if($('cal-status-slope'))$('cal-status-slope').textContent=c.ecSlope!==undefined?String(c.ecSlope):'--';
      if($('cal-status-intercept'))$('cal-status-intercept').textContent=c.ecIntercept!==undefined?String(c.ecIntercept):'--';
      if($('cal-status-tc'))$('cal-status-tc').textContent=c.ecTempCoeff!==undefined?String(c.ecTempCoeff):'--';
      document.querySelectorAll('input[name="pubmode"]').forEach(r=>r.checked=(r.value===(c.publishMode||'always')));
      updateDeltaDisplay();
      if($('d-vwc'))$('d-vwc').value=c.deltaVWC??0.1;
      if($('d-temp'))$('d-temp').value=c.deltaTemp??0.2;
      if($('d-ec'))$('d-ec').value=c.deltaEC??5;
      if($('d-pwec'))$('d-pwec').value=c.deltaPWEC??0.02;
      setTimezoneUIFromValues((c.tzOffset??0),(c.dstOffset??0));
      if($('sensor-ms'))$('sensor-ms').value=c.sensorIntervalMs??5000;
      webserialState=!!c.webSerialEnabled;
      debugModeState=!!c.debugMode;
      try{
        const anon=!!(c.mqttAllowAnonymous||c.mqttAllowAnon||false),ma=$('mqtt-anon');
        if(ma)ma.checked=anon;
      }catch(e){}
      if(typeof c.sensorNumber==='number'&&$('sensor-id'))$('sensor-id').value=c.sensorNumber;
      if(c.sensorType && $('sensor-type-select')){
        currentSensorType=String(c.sensorType);
        $('sensor-type-select').value=currentSensorType;
        updateSensorTypeBadge();
        updateCalTempCoeffUI();
      }
      updateWebSerialButton();
      updateDebugButton();setDebugBadge();
      configLoaded=true;
    }catch(ex){console.error('loadConfig error:',ex);}
  }
}

function updateMeta(m){
  try{
    const metaEl=$('meta');
    if(metaEl)metaEl.textContent=`FW: ${m.fw||'-'} ${m.ver||''}, Build: ${m.build||''}, ChipID: ${m.chipid||'-'}, MAC: ${m.mac||'-'}, @n0socialskills`;
  }catch(e){console.error('updateMeta error',e);}
}

function toast(msg,cls='info'){
  const el=document.querySelector('.section.active .status')||$('sensor-status-msg');
  if(!el)return;el.className='status '+cls;el.textContent=msg;setTimeout(()=>{if(el.textContent===msg)el.textContent=''},4000);
}

function openWebSerial(){window.open('/webserial','_blank')}

function toggleTempUnit(){
  if(ws&&ws.readyState===WebSocket.OPEN){
    useCelsius=!useCelsius;
    ws.send(JSON.stringify({type:'temp_unit',fahrenheit:!useCelsius}));
  }
}

const TZ_CHOICES=[
  {key:'UTC',label:'UTC (UTC±00:00)',gmt:0,dst:0},
  {key:'UK',label:'UK — GMT/BST (UTC+00:00 / UTC+01:00)',gmt:0,dst:3600},
  {key:'WEU',label:'Western Europe — WET/WEST (UTC+00:00 / UTC+01:00)',gmt:0,dst:3600},
  {key:'CEU',label:'Central Europe — CET/CEST (UTC+01:00 / UTC+02:00)',gmt:3600,dst:3600},
  {key:'EEU',label:'Eastern Europe — EET/EEST (UTC+02:00 / UTC+03:00)',gmt:7200,dst:3600},
  {key:'MSK',label:'Moscow — MSK (UTC+03:00)',gmt:10800,dst:0},
  {key:'IL',label:'Israel — IST/IDT (UTC+02:00 / UTC+03:00)',gmt:7200,dst:3600},
  {key:'SAST',label:'South Africa — SAST (UTC+02:00)',gmt:7200,dst:0},
  {key:'PKT',label:'Pakistan — PKT (UTC+05:00)',gmt:18000,dst:0},
  {key:'IST',label:'India — IST (UTC+05:30)',gmt:19800,dst:0},
  {key:'NPT',label:'Nepal — NPT (UTC+05:45)',gmt:20700,dst:0},
  {key:'BST8',label:'China/Singapore — CST/SGT (UTC+08:00)',gmt:28800,dst:0},
  {key:'JST',label:'Japan — JST (UTC+09:00)',gmt:32400,dst:0},
  {key:'AEST',label:'Australia East — AEST/AEDT (UTC+10:00 / UTC+11:00)',gmt:36000,dst:3600},
  {key:'ACST',label:'Australia Central — ACST/ACDT (UTC+09:30 / UTC+10:30)',gmt:34200,dst:3600},
  {key:'NZ',label:'New Zealand — NZST/NZDT (UTC+12:00 / UTC+13:00)',gmt:43200,dst:3600},
  {key:'ART',label:'Argentina — ART (UTC−03:00)',gmt:-10800,dst:0},
  {key:'BRT',label:'Brazil — BRT (UTC−03:00)',gmt:-10800,dst:0},
  {key:'ATL',label:'Atlantic — AST/ADT (UTC−04:00 / UTC−03:00)',gmt:-14400,dst:3600},
  {key:'NFLD',label:'Newfoundland — NST/NDT (UTC−03:30 / UTC−02:30)',gmt:-12600,dst:3600},
  {key:'EST',label:'Eastern — EST/EDT (UTC−05:00 / UTC−04:00)',gmt:-18000,dst:3600},
  {key:'CST',label:'Central — CST/CDT (UTC−06:00 / UTC−05:00)',gmt:-21600,dst:3600},
  {key:'MST',label:'Mountain — MST/MDT (UTC−07:00 / UTC−06:00)',gmt:-25200,dst:3600},
  {key:'AZ',label:'Arizona — MST (no DST) (UTC−07:00)',gmt:-25200,dst:0},
  {key:'PST',label:'Pacific — PST/PDT (UTC−08:00 / UTC−07:00)',gmt:-28800,dst:3600},
  {key:'AK',label:'Alaska — AKST/AKDT (UTC−09:00 / UTC−08:00)',gmt:-32400,dst:3600},
  {key:'HST',label:'Hawaii — HST (no DST) (UTC−10:00)',gmt:-36000,dst:0}
];

function formatOffset(sec){
  const s=Number(sec)||0,sign=s>=0?'+':'-',a=Math.abs(s),hh=Math.floor(a/3600),mm=Math.floor((a%3600)/60);
  return`UTC${sign}${String(hh).padStart(2,'0')}:${String(mm).padStart(2,'0')}`;
}
function populateTimezoneDropdown(){
  const sel=$('tz-simple-select');if(!sel)return;
  sel.innerHTML='';
  TZ_CHOICES.forEach(c=>{
    const opt=document.createElement('option');
    opt.value=c.key;opt.textContent=c.label;sel.appendChild(opt);
  });
  sel.value='EST';
  if($('dst-toggle'))$('dst-toggle').checked=false;
  updateEffectiveLabel();
}
function findChoiceByGmt(gmt){return TZ_CHOICES.find(c=>c.gmt===gmt)||TZ_CHOICES[0]}
function setTimezoneUIFromValues(gmtOffset,dstOffset){
  const sel=$('tz-simple-select'),dst=$('dst-toggle'),choice=findChoiceByGmt(Number(gmtOffset)||0);
  if(sel)sel.value=choice.key;
  if(dst)dst.checked=!!(Number(dstOffset)||0);
  updateEffectiveLabel();
}
function updateEffectiveLabel(){
  const sel=$('tz-simple-select'),dst=$('dst-toggle'),badge=$('tz-effective');
  if(!sel||!dst||!badge)return;
  const choice=TZ_CHOICES.find(c=>c.key===sel.value)||TZ_CHOICES[0],effective=choice.gmt+(dst.checked?(choice.dst||0):0);
  badge.textContent=`Effective: ${formatOffset(effective)}`;
}
function saveTime(){
  const sel=$('tz-simple-select'),dstEl=$('dst-toggle');
  if(!sel||!dstEl){toast('Timezone controls missing','err');return;}
  const choice=TZ_CHOICES.find(c=>c.key===sel.value)||TZ_CHOICES[0],gmt=choice.gmt,dst=dstEl.checked?(choice.dst||0):0;
  const payload={type:'update_time',gmt_offset_sec:gmt,dst_offset_sec:dst,tz_name:choice.label};
  if(ws&&ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify(payload));
}
function reboot(){if(ws&&ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify({type:'reboot_device'}))}
function factoryReset(){if(ws&&ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify({type:'factory_reset'}))}
function wifiRetry(){if(ws&&ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify({type:'wifi_retry_now'}))}
function saveWiFi(){
  const ssid=$('wifi-ssid')?$('wifi-ssid').value.trim():'',pass=$('wifi-pass')?$('wifi-pass').value:'';
  if(!ssid){toast('SSID required','err');return;}
  if(!pass||pass.trim().length===0){toast('WiFi password required','err');return;}
  if(ws&&ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify({type:'update_wifi',ssid,password:pass}));
}
function saveMqttConn(){
  const server=$('mqtt-server')?$('mqtt-server').value.trim():'',port=Number($('mqtt-port')?$('mqtt-port').value.trim():0),user=$('mqtt-user')?$('mqtt-user').value.trim():'',pass=$('mqtt-pass')?$('mqtt-pass').value:'',allowAnon=!!($('mqtt-anon')&&$('mqtt-anon').checked);
  if(!server||!(port>0&&port<65536)){toast('Invalid MQTT host/port','err');return;}
  if(!allowAnon&&(!pass||pass.trim().length===0)){toast('MQTT password required (unless "Allow anonymous" enabled)','err');return;}
  if(ws&&ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify({type:'update_mqtt',server,port,user,pass,allow_anonymous:allowAnon}));
}
function saveMqttTopic(){
  const topic=$('mqtt-topic-input')?$('mqtt-topic-input').value.trim():'';
  if(!topic){toast('Topic required','err');return;}
  if(ws&&ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify({type:'update_mqtt_topic',topic}));
  try{const disp=$('mqtt-topic-display');if(disp)disp.textContent=topic;}catch(e){}
}
function toggleHA(){
  const next=!(haEnabledState===true);haEnabledState=next;
  if($('ha-toggle-btn'))$('ha-toggle-btn').textContent=next?'Disable HA Discovery':'Enable HA Discovery';
  if(ws&&ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify({type:'enable_ha',enabled:next}));
}
function haAnnounce(){if(ws&&ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify({type:'ha_announce_now'}))}
function savePublishMode(){
  const mode=document.querySelector('input[name="pubmode"]:checked')?.value||'always',vwc=Number($('d-vwc')?$('d-vwc').value:0),tt=Number($('d-temp')?$('d-temp').value:0),ec=Number($('d-ec')?$('d-ec').value:0),pw=Number($('d-pwec')?$('d-pwec').value:0);
  if(ws&&ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify({type:'update_publish_mode',mode,vwc_delta:vwc,temp_delta:tt,ec_delta:ec,pwec_delta:pw}));
}
function saveSensorNumber(){
  const el=$('sensor-id'),v=el?parseInt(el.value,10):NaN;
  if(!(v>=1&&v<=255)){toast('Sensor # must be 1..255','err');return;}
  if(ws&&ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify({type:'set_sensor_number',id:v}));
}
function saveSensorInterval(){
  const el=$('sensor-ms'),ms=el?parseInt(el.value,10):NaN;
  if(Number.isNaN(ms)||ms<1000||ms>60000){toast('Interval 1000..60000 ms','err');return;}
  if(ws&&ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify({type:'update_sensor_interval',ms}));
}
function saveSensorType(){
  const sel=$('sensor-type-select');if(!sel){toast('Sensor type control missing','err');return;}
  const val=sel.value;
  if(ws&&ws.readyState===WebSocket.OPEN){
    ws.send(JSON.stringify({type:'set_sensor_type',sensor_type:val}));
  }
}
function updateWebSerialButton(){
  const btn=$('webserial-btn');if(!btn)return;
  const on=!!webserialState;btn.textContent=on?'Disable WebSerial':'Enable WebSerial';
}
function setWebSerialBadge(){
  const b=$('webserial-badge');if(!b)return;
  const on=!!webserialState;b.className='badge '+(on?'ok':'err');b.textContent=on?'Enabled':'Disabled';
}
function updateDebugButton(){
  const btn=$('debug-btn');if(!btn)return;
  const on=!!debugModeState;btn.textContent=on?'Disable Debug Mode':'Enable Debug Mode';
}
function setDebugBadge(){
  const b=$('debug-badge');if(!b)return;
  const on=!!debugModeState;b.style.display=on?'inline-block':'none';
}
function toggleDebugMode(){
  debugModeState=!(debugModeState===true);updateDebugButton();setDebugBadge();
  if(ws&&ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify({type:'toggle_debug_mode',enabled:debugModeState}));
}
function toggleMqtt(){
  const next=!(lastStatus.mqtt_enabled===true);
  if(ws&&ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify({type:'toggle_mqtt',enabled:next}));
}
function updateMqttControls(){
  const btn=$('btn-mqtt-toggle');if(btn)btn.textContent=(lastStatus.mqtt_enabled===true)?'Disable MQTT':'Enable MQTT';
  const badge=$('mqtt2-badge');if(!badge)return;
  if(typeof lastStatus.mqtt_enabled==='boolean'&&lastStatus.mqtt_enabled===false&&!lastStatus.mqtt_connected){
    badge.className='badge err';badge.textContent='--';
  }else{
    const conn=!!lastStatus.mqtt_connected;badge.className='badge '+(conn?'ok':'err');badge.textContent=conn?'Connected':'--';
  }
}
function clearHistory(){if(ws&&ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify({type:'clear_history'}))}
function saveCal(){
  const slope=parseFloat(($('cal-slope')||{}).value),intercept=parseFloat(($('cal-intercept')||{}).value),tc=parseFloat(($('cal-tc')||{}).value);
  if([slope,intercept,tc].some(v=>Number.isNaN(v))){toast('Invalid calibration values','err');return;}
  if(ws&&ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify({type:'update_calibration',slope,intercept,tempcoeff:tc}));
}
function toggleCalMethod(){
  const method=document.querySelector('input[name="cal-method"]:checked')?.value||'quick',lowTdc=$('buf-low-tdc'),highTdc=$('buf-high-tdc'),msgEl=$('calc-msg');
  if(lowTdc)lowTdc.style.display=method==='chart'?'':'none';
  if(highTdc)highTdc.style.display=method==='chart'?'':'none';
  if(msgEl){
    msgEl.className='status info';
    msgEl.textContent=method==='chart'?'Manual TC selected: provide TDC µS values if available.':'No TC selected.';
    setTimeout(()=>{if(msgEl&&msgEl.textContent)msgEl.textContent=''},2500);
  }
}
function calculateCalibration(){
  const elBufLow=$('buf-low'),elBufLowTdc=$('buf-low-tdc'),elRawLow=$('raw-low'),elBufHigh=$('buf-high'),elBufHighTdc=$('buf-high-tdc'),elRawHigh=$('raw-high'),msgEl=$('calc-msg'),outSlope=$('calc-slope'),outIntercept=$('calc-intercept');
  if(!elBufLow||!elRawLow||!elBufHigh||!elRawHigh||!outSlope||!outIntercept){
    if(msgEl){msgEl.className='status err';msgEl.textContent='Calibration inputs missing'}
    return;
  }
  const rawLow=parseFloat(elRawLow.value),rawHigh=parseFloat(elRawHigh.value);
  if(isNaN(rawLow)||isNaN(rawHigh)||Math.abs(rawHigh-rawLow)<1e-6){
    if(msgEl){msgEl.className='status err';msgEl.textContent='Invalid raw readings.'}
    return;
  }
  const method=document.querySelector('input[name="cal-method"]:checked')?.value||'quick';
  const bufLowRef=parseFloat(elBufLow.value),bufHighRef=parseFloat(elBufHigh.value);
  if(isNaN(bufLowRef)||isNaN(bufHighRef)){
    if(msgEl){msgEl.className='status err';msgEl.textContent='Enter buffer EC (µS @25°C) values.'}
    return;
  }
  let useLow=bufLowRef,useHigh=bufHighRef;
  if(method==='chart'){
    const lowTdcVal=elBufLowTdc?parseFloat(elBufLowTdc.value):NaN,highTdcVal=elBufHighTdc?parseFloat(elBufHighTdc.value):NaN;
    if(!Number.isNaN(lowTdcVal))useLow=lowTdcVal;
    if(!Number.isNaN(highTdcVal))useHigh=highTdcVal;
  }
  if(Number.isNaN(useLow)||Number.isNaN(useHigh)){
    if(msgEl){msgEl.className='status err';msgEl.textContent='Buffer µS values invalid.'}
    return;
  }
  const slope=(useHigh-useLow)/(rawHigh-rawLow),intercept=useLow-slope*rawLow;
  outSlope.textContent=isFinite(slope)?slope.toFixed(2):'--';
  outIntercept.textContent=isFinite(intercept)?intercept.toFixed(2):'--';
  if(msgEl){
    msgEl.className='status info';
    msgEl.textContent=method==='chart'?'Calculated using Manual TC.':'Calculated using No TC.';
    setTimeout(()=>{if(msgEl&&msgEl.textContent)msgEl.textContent=''},3000);
  }
  outSlope.dataset.val=String(slope);
  outIntercept.dataset.val=String(intercept);
  window._calculatedSlope = slope;
  window._calculatedIntercept = intercept;
}
function applyCalculatedCal(){
  const outSlope=$('calc-slope'),outIntercept=$('calc-intercept'),elSlope=$('cal-slope'),elIntercept=$('cal-intercept');
  if(!outSlope||!outIntercept||!elSlope||!elIntercept)return;
  let s=window._calculatedSlope,i=window._calculatedIntercept;
  if(typeof s==='undefined'||typeof i==='undefined'){
    s=outSlope.dataset?outSlope.dataset.val:undefined;
    i=outIntercept.dataset?outIntercept.dataset.val:undefined;
  }
  if(typeof s==='undefined'||typeof i==='undefined'){
    confirmModal('No calculation','No calculated slope/intercept found. Run "Calculate" first.',()=>{});
    return;
  }
  const sNum=Number(s),iNum=Number(i);
  if(Number.isFinite(sNum)&&Number.isFinite(iNum)){
    elSlope.value=sNum.toFixed(2);
    elIntercept.value=iNum.toFixed(2);
    toast('Calculated slope/intercept copied into inputs. Click Save Calibration to persist.','info');
  }else toast('Calculated values invalid','err');
}
function downloadHistory(){
  const btn=$('btn-download-history');if(!btn)return;
  btn.disabled=true;
  let win=window.open('about:blank','_blank');
  if(!win){
    toast('Popup blocked. Please allow popups for this site and try again','err');
    btn.disabled=false;
    return;
  }
  fetch('/api/history/meta')
    .then(r=>r.json().catch(()=>({count:0})))
    .then(j=>{
      if(j&&j.count>0)win.location='/download/history';
      else{
        try{win.close()}catch(_){}
        toast('No history available','err');
      }
    })
    .catch(_=>{win.location='/download/history'})
    .finally(()=>{btn.disabled=false});
}
function clearLogs(){
  const btn=$('btn-clear-logs');if(!btn)return;
  btn.disabled=true;
  fetch('/api/logs/clear',{method:'POST'})
    .then(r=>{
      if(r.ok)toast('Logs cleared','info');else toast('Clear logs failed','err');
      btn.disabled=false;
    })
    .catch(e=>{
      toast('Clear logs error: '+e,'err');
      btn.disabled=false;
    });
}
function updateLoggingButton(enabled){
  const btn=$('btn-toggle-logging');if(btn)btn.textContent=enabled?'Disable File Logging':'Enable File Logging';
}
function updateLoggingStatus(){
  fetch('/api/logging')
    .then(r=>{
      if(!r.ok){updateLoggingButton(false);return null;}
      return r.json();
    })
    .then(j=>{if(j)updateLoggingButton(!!j.enabled)})
    .catch(()=>{updateLoggingButton(false)});
}
function wifiScan(){
  const btn=$('btn-wifi-scan'),info=$('wifi-scan-info'),statusEl=$('wifi-msg'),list=$('wifi-list');
  if(!btn||!info||!statusEl||!list)return;
  btn.disabled=true;
  statusEl.className='status info';statusEl.textContent='Starting scan...';
  info.textContent='';list.innerHTML='<div class="wifi-empty">Scanning for networks…</div>';
  fetch('/api/wifi_scan_start',{method:'POST'})
    .then(r=>r.json().catch(()=>({started:false})))
    .then(res=>{
      const msg=(res&&res.message)?String(res.message):(res.started?'Scanning for networks...':'Failed to start scan');
      statusEl.className=res.started?'status info':'status err';
      statusEl.textContent=msg;
      if(res.started)setTimeout(pollWifiScanStatus,800);
      else{list.innerHTML='<div class="wifi-empty">Scan failed to start</div>';btn.disabled=false;}
    })
    .catch(err=>{
      statusEl.className='status err';statusEl.textContent='Scan request failed';
      list.innerHTML='<div class="wifi-empty">Scan request failed</div>';
      btn.disabled=false;
    });
}
function pollWifiScanStatus(){
  const info=$('wifi-scan-info'),statusEl=$('wifi-msg'),dl=$('wifi-ssids'),list=$('wifi-list');
  if(!statusEl||!list)return;
  fetch('/api/wifi_scan_status')
    .then(r=>r.json())
    .then(j=>{
      if(j.status==='scanning'){
        statusEl.className='status info';statusEl.textContent='Scanning for networks...';
        setTimeout(pollWifiScanStatus,800);
        return;
      }
      if(j.status==='failed'){
        statusEl.className='status err';statusEl.textContent='Scan failed';
        list.innerHTML='<div class="wifi-empty">Scan failed</div>';
        if(info)info.textContent='';
        return;
      }
      if(j.status==='done'&&Array.isArray(j.aps)){
        j.aps.sort((a,b)=>(b.rssi||0)-(a.rssi||0));
        if(dl)dl.innerHTML='';
        if(j.aps.length===0){
          list.innerHTML='<div class="wifi-empty">No networks found</div>';
          if(info)info.textContent='No networks found';
          statusEl.className='status warn';
          statusEl.textContent='Scan complete: 0 networks';
        }else{
          list.innerHTML='';
          let added=0;
          j.aps.forEach(ap=>{
            if(!ap||!ap.ssid)return;
            const ssid=String(ap.ssid).trim();
            if(!ssid)return;
            added++;
            if(dl){
              const opt=document.createElement('option');
              opt.value=ssid;
              opt.title=`RSSI ${ap.rssi||'N/A'} ch ${ap.chan||'N/A'} ${ap.enc||''}`;
              dl.appendChild(opt);
            }
            const entry=document.createElement('div');
            entry.className='wifi-entry';
            entry.setAttribute('role','button');
            entry.onclick=()=>selectSsid(ssid);
            const left=document.createElement('div');
            const ss=document.createElement('span');
            ss.className='ssid';ss.textContent=ssid;
            left.appendChild(ss);
            if(ap.enc&&ap.enc!=='OPEN'){
              const lock=document.createElement('span');
              lock.className='lock';lock.textContent='🔒';
              left.appendChild(lock);
            }
            const right=document.createElement('div');
            right.className='meta';
            right.textContent=`${ap.rssi??'N/A'} dBm • ch ${ap.chan??'N/A'}${ap.bssid?' • '+ap.bssid:''}`;
            entry.appendChild(left);
            entry.appendChild(right);
            list.appendChild(entry);
          });
          if(info)info.textContent=`Found ${added} network${added!==1?'s':''}. Click an entry to use its SSID.`;
          statusEl.className='status info';
          statusEl.textContent=`Scan complete: ${added} networks`;
        }
      }else if(j.status==='idle'&&!Array.isArray(j.aps)){
        statusEl.className='status warn';statusEl.textContent='No scan results (try again)';
        list.innerHTML='<div class="wifi-empty">No results</div>';
        if(info)info.textContent='';
      }else{
        statusEl.className='status err';statusEl.textContent='Scan failed or returned no results';
        list.innerHTML='<div class="wifi-empty">Scan failed</div>';
        if(info)info.textContent='';
      }
    })
    .catch(err=>{
      const statusEl2=$('wifi-msg');
      if(statusEl2){statusEl2.className='status err';statusEl2.textContent='Error polling scan status'}
      const list2=$('wifi-list');
      if(list2)list2.innerHTML='<div class="wifi-empty">Error fetching results</div>';
    })
    .finally(()=>{
      const btn2=$('btn-wifi-scan');
      if(btn2)btn2.disabled=false;
    });
}
function selectSsid(ssid){
  const input=$('wifi-ssid');if(!input)return;
  input.value=ssid;toast('SSID selected: '+ssid,'info');
  const pass=$('wifi-pass');if(pass)pass.focus();
}

function updateSensorTypeBadge(){
  const b=$('sensor-type-badge');if(!b)return;
  b.textContent=(currentSensorType==='mec20'?'MEC20':'THCS');
  b.className='badge '+(currentSensorType==='mec20'?'warn':'ok');
}

function updateCalTempCoeffUI(){
  const tc=$('cal-tc'),note=$('mec20-tc-note');
  if(!tc||!note)return;
  if(currentSensorType==='mec20'){
    tc.disabled=true;
    note.style.display='block';
  }else{
    tc.disabled=false;
    note.style.display='none';
  }
}

document.addEventListener('DOMContentLoaded',()=>{
  setupTabs();
  connectWS();
  populateTimezoneDropdown();
  const sel=$('tz-simple-select'),dst=$('dst-toggle');
  if(sel)sel.addEventListener('change',updateEffectiveLabel);
  if(dst)dst.addEventListener('change',updateEffectiveLabel);
  document.querySelectorAll('details').forEach(d=>{if(d.hasAttribute('open'))d.removeAttribute('open')});
  try{toggleCalMethod()}catch(e){}
});

function toggleWebSerial(){
  webserialState=!(webserialState===true);
  updateWebSerialButton();setWebSerialBadge();
  if(webserialState===true){
    const host=(lastStatus&&lastStatus.ip)?lastStatus.ip:location.host,proto=location.protocol==='https:'?'https:':'http:',url=proto+'//'+host+'/webserial';
    let win=null;
    try{win=window.open('about:blank','_blank')}catch(e){win=null;}
    if(win){
      try{win.location=url}
      catch(e){
        try{win.close()}catch(_){}
        console.error('Failed to navigate WebSerial tab',e);
      }
    }else toast('Unable to open WebSerial tab (popup blocked).','warn');
  }
  if(ws&&ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify({type:'toggle_webserial',enabled:webserialState}));
}
</script>
</body>
</html>)rawliteral";

extern const size_t index_html_len = sizeof(index_html) - 1;