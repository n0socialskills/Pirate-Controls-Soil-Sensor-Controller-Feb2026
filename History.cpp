#include <Arduino.h>
#include <LittleFS.h>
#include <time.h>
#include <ArduinoJson.h>
#include "Globals.h"
#include "THCSensor.h"
#include "Config.h"
#include "History.h"
#include "ActiveSensor.h"

#define HISTORY_MAGIC    0x53434854UL
#define HISTORY_VERSION  0x0001

struct __attribute__((packed)) HistoryHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t reserved;
  uint32_t head;
  uint32_t count;
  uint32_t maxRecords;
  uint32_t interval_s;
};

struct __attribute__((packed)) HistoryRecord {
  uint32_t epoch;
  float    vwc;
  float    pwec;
  float    temp_c;
};

static HistoryHeader g_hdr;
static bool g_hdrLoaded = false;

static bool loadHeader() {
  if (!Globals::ensureFS()) {                         // use shared FS mount
    Globals::webDebug("[History] LittleFS mount failed", true);
    return false;
  }
  File f = LittleFS.open(HISTORY_FILE, "r");
  if (!f) {
    Globals::webDebug("[History] History file missing; need begin()", true);
    return false;
  }
  if (f.readBytes((char*)&g_hdr, sizeof(g_hdr)) != (int)sizeof(g_hdr)) {
    Globals::webDebug("[History] Failed to read header", true);
    f.close();
    return false;
  }
  f.close();
  if (g_hdr.magic != HISTORY_MAGIC || g_hdr.version != HISTORY_VERSION) {
    Globals::webDebug("[History] Bad header magic/version", true);
    return false;
  }
  g_hdrLoaded = true;
  return true;
}

static uint32_t getNowEpoch() {
  struct tm tmnow;
  time_t now;
  if (!getLocalTime(&tmnow, 1)) return 0;
  now = mktime(&tmnow);
  return (uint32_t)now;
}

void History::begin() {
  if (!Globals::ensureFS()) {
    Globals::webDebug("[History] LittleFS begin failed", true);
    return;
  }

  if (!LittleFS.exists(HISTORY_FILE)) {
    File f = LittleFS.open(HISTORY_FILE, "w");
    if (!f) {
      Globals::webDebug("[History] Failed to create history file", true);
      return;
    }
    HistoryHeader hdr;
    hdr.magic = HISTORY_MAGIC;
    hdr.version = HISTORY_VERSION;
    hdr.reserved = 0;
    hdr.head = 0;
    hdr.count = 0;
    hdr.maxRecords = (uint32_t)HISTORY_MAX_RECORDS;
    hdr.interval_s = HISTORY_INTERVAL_MS / 1000UL;
    f.write((const uint8_t*)&hdr, sizeof(hdr));
    HistoryRecord rec;
    memset(&rec, 0, sizeof(rec));
    for (uint32_t i = 0; i < hdr.maxRecords; ++i) {
      f.write((const uint8_t*)&rec, sizeof(rec));
    }
    f.close();
    g_hdr = hdr;
    g_hdrLoaded = true;
  } else {
    loadHeader();
  }
}

bool History::addPoint(float vwc, float pwec, float temp_c) {
  if (!g_hdrLoaded && !loadHeader()) return false;

  struct tm tmnow;
  if (!getLocalTime(&tmnow, 1)) {
    Globals::webDebug("[History] addPoint: time not synced", true);
    return false;
  }
  uint32_t epoch = (uint32_t)mktime(&tmnow);
  if (epoch == 0) return false;

  File f = LittleFS.open(HISTORY_FILE, "r+");
  if (!f) {
    Globals::webDebug("[History] addPoint: failed to open file", true);
    return false;
  }

  HistoryRecord rec;
  rec.epoch = epoch;
  rec.vwc = vwc;
  rec.pwec = pwec;
  rec.temp_c = temp_c;

  uint32_t idx = g_hdr.head % g_hdr.maxRecords;
  uint32_t pos = sizeof(HistoryHeader) + idx * sizeof(HistoryRecord);
  if (!f.seek(pos, SeekSet)) {
    f.close();
    Globals::webDebug("[History] addPoint: seek failed", true);
    return false;
  }
  if ((size_t)f.write((const uint8_t*)&rec, sizeof(rec)) != sizeof(rec)) {
    f.close();
    Globals::webDebug("[History] addPoint: write failed", true);
    return false;
  }

  g_hdr.head = (g_hdr.head + 1) % g_hdr.maxRecords;
  if (g_hdr.count < g_hdr.maxRecords) g_hdr.count++;

  if (!f.seek(0, SeekSet)) {
    f.close();
    Globals::webDebug("[History] addPoint: header seek failed", true);
    return false;
  }
  if ((size_t)f.write((const uint8_t*)&g_hdr, sizeof(g_hdr)) != sizeof(g_hdr)) {
    f.close();
    Globals::webDebug("[History] addPoint: header write failed", true);
    return false;
  }

  f.close();
  return true;
}

void History::clear() {
  if (!Globals::ensureFS()) return;
  if (LittleFS.exists(HISTORY_FILE)) {
    LittleFS.remove(HISTORY_FILE);
  }
  g_hdrLoaded = false;
  begin();
}

String History::getJSON(uint32_t hours) {
  String out;
  if (!g_hdrLoaded && !loadHeader()) return "[]";
  if (g_hdr.count == 0 || g_hdr.interval_s == 0) return "[]";

  uint32_t retention_h = (g_hdr.maxRecords * g_hdr.interval_s) / 3600UL;
  if (hours == 0 || hours > retention_h) hours = retention_h;

  uint32_t nowEpoch = getNowEpoch();
  if (nowEpoch == 0) return "[]";
  uint32_t fromEpoch = nowEpoch - hours * 3600UL;

  File f = LittleFS.open(HISTORY_FILE, "r");
  if (!f) return "[]";
  if (!f.seek(sizeof(HistoryHeader), SeekSet)) {
    f.close();
    return "[]";
  }

  uint32_t validCount = g_hdr.count;
  uint32_t maxRecords = g_hdr.maxRecords;
  uint32_t oldestIndex = (g_hdr.head + maxRecords - validCount) % maxRecords;

  HistoryRecord rec;
  bool first = true;
  out += "[";

  for (uint32_t i = 0; i < validCount; ++i) {
    uint32_t idx = (oldestIndex + i) % maxRecords;
    uint32_t pos = sizeof(HistoryHeader) + idx * sizeof(HistoryRecord);
    if (!f.seek(pos, SeekSet)) break;
    if (f.readBytes((char*)&rec, sizeof(rec)) != (int)sizeof(rec)) break;
    if (rec.epoch < fromEpoch) continue;

    if (!first) out += ",";
    first = false;

    out += "{\"t\":";
    out += String(rec.epoch);
    out += ",\"vwc\":";
    out += String(rec.vwc, 3);
    out += ",\"pwec\":";
    out += String(rec.pwec, 3);
    out += ",\"temp\":";
    out += String(rec.temp_c, 2);
    out += "}";
  }

  out += "]";
  f.close();
  return out;
}

String History::getMetaJSON() {
  DynamicJsonDocument d(256);
  d["type"] = "history_meta";
  if (!g_hdrLoaded && !loadHeader()) {
    d["interval_s"] = 0;
    d["max_records"] = 0;
    d["retention_h"] = 0;
    d["count"] = 0;
  } else {
    d["interval_s"] = g_hdr.interval_s;
    d["max_records"] = g_hdr.maxRecords;
    uint32_t retention_h = (g_hdr.maxRecords * g_hdr.interval_s) / 3600UL;
    d["retention_h"] = retention_h;
    d["count"] = g_hdr.count;
  }
  String out;
  serializeJson(d, out);
  return out;
}

void History::streamJSON(uint32_t hours, Print &out, size_t maxPoints) {
  String arr = getJSON(hours);
  out.print(arr);
}

void History::streamCSV(uint32_t hours, Print &out, size_t maxPoints) {
  // CSV header
  out.println("Date,Time,VWC,pwEC,TempC,TempF");
  if (!g_hdrLoaded && !loadHeader()) return;
  if (g_hdr.count == 0 || g_hdr.interval_s == 0) return;

  uint32_t retention_h = (g_hdr.maxRecords * g_hdr.interval_s) / 3600UL;
  if (hours == 0 || hours > retention_h) hours = retention_h;
  if (hours > 168) hours = 168;

  uint32_t nowEpoch = getNowEpoch();
  if (nowEpoch == 0) return;
  uint32_t fromEpoch = nowEpoch - hours * 3600UL;

  File f = LittleFS.open(HISTORY_FILE, "r");
  if (!f) return;
  if (!f.seek(sizeof(HistoryHeader), SeekSet)) {
    f.close(); return;
  }

  uint32_t valid = g_hdr.count;
  uint32_t maxR = g_hdr.maxRecords;
  uint32_t oldest = (g_hdr.head + maxR - valid) % maxR;
  HistoryRecord rec;
  size_t emitted = 0;
  if (maxPoints == 0) maxPoints = 4000;

  for (uint32_t i=0;i<valid && emitted<maxPoints;i++){
    uint32_t idx = (oldest + i) % maxR;
    uint32_t pos = sizeof(HistoryHeader) + idx * sizeof(HistoryRecord);
    if (!f.seek(pos, SeekSet)) break;
    if (f.readBytes((char*)&rec, sizeof(rec)) != (int)sizeof(rec)) break;
    if (rec.epoch < fromEpoch) continue;

    time_t t = (time_t)rec.epoch;
    struct tm tmrec;
    if (!localtime_r(&t, &tmrec)) gmtime_r(&t, &tmrec);

    char datebuf[16], timebuf[16];
    strftime(datebuf, sizeof(datebuf), "%Y-%m-%d", &tmrec);
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", &tmrec);

    out.print(datebuf); out.print(','); out.print(timebuf); out.print(',');
    out.print(rec.vwc,3); out.print(',');
    out.print(rec.pwec,3); out.print(',');
    out.print(rec.temp_c,2); out.print(',');
    float tf = rec.temp_c * 9.0f / 5.0f + 32.0f;
    out.println(tf,2);
    emitted++;
    if (emitted % 50 == 0) delay(0);
  }
  f.close();
}

void History::loop() {
  uint32_t now = millis();
  if (now >= Globals::nextHistoryAt) {
    struct tm tmnow;
    if (!ActiveSensor::hasError() && getLocalTime(&tmnow, 1)) {
      addPoint(ActiveSensor::getVWC(), ActiveSensor::getPwec(), ActiveSensor::getTempC());
    }
    Globals::nextHistoryAt += HISTORY_INTERVAL_MS;
  }
}