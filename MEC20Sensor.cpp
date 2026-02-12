#include "MEC20Sensor.h"
#include "Globals.h"
#include "Config.h"
#include "Utils.h"       // [UPDATED] use shared CRC and median helpers

static bool  sError = true;
static bool  sHeartbeat = false;
static float sVWC = 0.0f;
static float sTempC = 0.0f;
static float sPwec = 0.0f;
static float sECuS = 0.0f;
static float sRawEC = 0.0f;

static float ecRawSamples[3] = {0.0f,0.0f,0.0f};
static int   ecIdx = 0;

static uint32_t lastPoll = 0;
static const uint32_t MEC20_MIN_POLL_MS = 200; // [ADDED] avoid hammering bus

// [ADDED] Suppress noisy RX-short logs for first N ms after boot (unless debugMode enabled)
static const uint32_t STARTUP_QUIET_MS = 15000; //  s to suppress startup RX-short messages

// [ADDED] Allow one immediate retry when a single timeout occurs (prevents transient misses)
static uint8_t g_retryCount = 0; // 0 = no retry yet, 1 = already retried for current cycle

// [UPDATED] State machine to avoid blocking delays (non-blocking RX/TX)
enum MEC20State { M_IDLE, M_WAIT_POST_TX, M_RX_ACCUM, M_PROCESS };
static MEC20State mState = M_IDLE;
static uint32_t   mStateStart = 0;
static uint8_t    respBuf[11];
static size_t     respCount = 0;

static const uint8_t REQ_TEMPLATE[8] = {0x01,0x04,0x00,0x00,0x00,0x03,0x00,0x00}; // last two bytes are CRC

void MEC20Sensor::begin() {
  // [ADDED] Reconfigure RS485 for MEC20
  Globals::RS485.begin(9600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  Globals::RS485.setTimeout(0);
  sError = true;
  sHeartbeat = false;
  sVWC = sTempC = sPwec = sECuS = sRawEC = 0.0f;
  ecRawSamples[0] = ecRawSamples[1] = ecRawSamples[2] = 0.0f;
  ecIdx = 0;
  lastPoll = millis();
  mState = M_IDLE;
  mStateStart = millis();
  respCount = 0;
  g_retryCount = 0; // reset retry counter on begin
  Globals::webDebug("[MEC20Sensor] begin @9600 8N1");
}

void MEC20Sensor::loop() {
  uint32_t now = millis();

  // Throttle polls
  if (mState == M_IDLE && (now - lastPoll < max(sensorIntervalMs, MEC20_MIN_POLL_MS))) {
    return;
  }

  switch (mState) {
    case M_IDLE: {
      // Build request with CRC
      uint8_t req[8];
      memcpy(req, REQ_TEMPLATE, sizeof(REQ_TEMPLATE));
      uint16_t crc = Utils::modbusCRC16(req, 6);         // [UPDATED] shared CRC
      req[6] = crc & 0xFF;
      req[7] = (crc >> 8) & 0xFF;

      // Prepare for TX
      while (Globals::RS485.available()) Globals::RS485.read();
      digitalWrite(RS485_DE_PIN, HIGH);
      digitalWrite(RS485_RE_PIN, HIGH);
      delayMicroseconds(50);
      Globals::RS485.write(req, 8);
      Globals::RS485.flush();

      // Move to wait-post-tx (non-blocking)
      mState = M_WAIT_POST_TX;
      mStateStart = now;
      respCount = 0;
      lastPoll = now;
      // NOTE: do NOT reset g_retryCount here — resetting here would allow unlimited retries.
      // g_retryCount is reset on begin() and on a successful read only.
      break;
    }

    case M_WAIT_POST_TX: {
      // Wait POST_TX_DELAY_MS (non-blocking)
      if (now - mStateStart >= POST_TX_DELAY_MS) {
        digitalWrite(RS485_DE_PIN, LOW);
        digitalWrite(RS485_RE_PIN, LOW);
        mState = M_RX_ACCUM;
        mStateStart = now;
      }
      break;
    }

    case M_RX_ACCUM: {
      // Accumulate incoming bytes without delay() - keep responsive
      while (Globals::RS485.available() && respCount < sizeof(respBuf)) {
        respBuf[respCount++] = (uint8_t)Globals::RS485.read();
      }
      // If we have expected frame or timeout, move to process
      const uint32_t WAIT_MS = 200; // [UPDATED] increased slightly to tolerate slow startup replies
      if (respCount >= 11) {
        mState = M_PROCESS;
      } else if (now - mStateStart >= WAIT_MS) {
        // Timeout
        // If we haven't retried yet for this poll, schedule a single immediate retry.
        if (g_retryCount == 0) {
          g_retryCount = 1; // mark we've used our one retry
          // schedule immediate retry: go back to IDLE and allow loop() to send again quickly
          // set lastPoll so the interval check passes immediately
          lastPoll = millis() - max((uint32_t)MEC20_MIN_POLL_MS, (uint32_t)50);
          // clear rx buffer & return to IDLE to trigger new send on next loop iteration
          respCount = 0;
          mState = M_IDLE;
          mStateStart = now;
          if (debugMode) Globals::webDebug("[MEC20Sensor] RX timeout - scheduling 1 immediate retry", true);
        } else {
          // we've already retried once; treat as failure for this interval
          sError = true;
          // [UPDATED] Suppress noisy startup log messages until STARTUP_QUIET_MS has elapsed,
          // unless debugMode is enabled (so developers still see them when needed).
          if (debugMode || millis() > STARTUP_QUIET_MS) {
            Globals::webDebug(String("[MEC20Sensor] RX short: ") + String(respCount), true);
          }
          // reset and go back to idle (will be retried next normal interval)
          respCount = 0;
          mState = M_IDLE;
          mStateStart = now;
          // ensure next send won't be immediate (avoid tight retry loop)
          lastPoll = now;
          g_retryCount = 0; // reset retry state until next explicit send
        }
      }
      break;
    }

    case M_PROCESS: {
      // Validate length and CRC
      if (respCount < 11) {
        sError = true;
        // [UPDATED] Suppress noisy startup log messages as above
        if (debugMode || millis() > STARTUP_QUIET_MS) {
          Globals::webDebug(String("[MEC20Sensor] RX short during process: ") + String(respCount), true);
        }
        respCount = 0;
        mState = M_IDLE;
        mStateStart = now;
        break;
      }

      uint16_t crcRecv = (uint16_t)respBuf[9] | ((uint16_t)respBuf[10] << 8);
      uint16_t crcCalc = Utils::modbusCRC16(respBuf, 9); // [UPDATED]
      if (crcRecv != crcCalc) {
        sError = true;
        Globals::webDebug("[MEC20Sensor] CRC fail", true);
        respCount = 0;
        mState = M_IDLE;
        mStateStart = now;
        break;
      }

      if (respBuf[1] != 0x04 || respBuf[2] != 0x06) {
        sError = true;
        Globals::webDebug("[MEC20Sensor] Bad func/len", true);
        respCount = 0;
        mState = M_IDLE;
        mStateStart = now;
        break;
      }

      int16_t tRaw  = (int16_t)((respBuf[3] << 8) | respBuf[4]);
      uint16_t vRaw = (uint16_t)((respBuf[5] << 8) | respBuf[6]);
      uint16_t eRaw = (uint16_t)((respBuf[7] << 8) | respBuf[8]);

      sTempC = (float)tRaw / 100.0f;
      sVWC   = (float)vRaw / 100.0f;
      sRawEC = (float)eRaw;

      if (ecRawSamples[0] == 0.0f && ecRawSamples[1] == 0.0f && ecRawSamples[2] == 0.0f && sRawEC > 0.0f) {
        ecRawSamples[0] = ecRawSamples[1] = ecRawSamples[2] = sRawEC;
        ecIdx = 0;
      } else {
        ecRawSamples[ecIdx] = sRawEC;
        ecIdx = (ecIdx + 1) % 3;
      }

      float med = Utils::median3(ecRawSamples[0], ecRawSamples[1], ecRawSamples[2]); // [UPDATED]
      float ecCal = EC_SLOPE * med + EC_INTERCEPT;
      // if (EC_TEMP_COEFF != 0.0f && sTempC != 25.0f) {                     // [REMOVED] MEC20 has fixed internal TC
      //   ecCal = ecCal / (1.0f + EC_TEMP_COEFF * (sTempC - 25.0f));        // [REMOVED]
      // }                                                                    // [REMOVED]
      sECuS = ecCal;

      // Reuse THCS dielectric-based pwEC model
      float epsilon_bulk   = 1.3088f + 0.1439f * sVWC + 0.0076f * sVWC * sVWC;
      float epsilon_pore   = 80.3f   - 0.37f * (sTempC - 20.0f);
      float epsilon_offset = 4.1f;
      if (epsilon_bulk > epsilon_offset) {
        float pw_ec_us = (epsilon_pore * sECuS) / (epsilon_bulk - epsilon_offset);
        sPwec = pw_ec_us / 1000.0f;
      } else {
        sPwec = 0.0f;
      }
      sPwec = constrain(sPwec, 0.0f, 20.0f);

      sError = false;
      sHeartbeat = !sHeartbeat;

      // [ADDED] successful read: clear retry counter
      g_retryCount = 0;

      respCount = 0;
      mState = M_IDLE;
      mStateStart = now;
      break;
    }

    default:
      mState = M_IDLE;
      mStateStart = now;
      respCount = 0;
      break;
  }
}

float MEC20Sensor::getVWC()    { return sVWC; }
float MEC20Sensor::getTempC()  { return sTempC; }
float MEC20Sensor::getPwec()   { return sPwec; }
float MEC20Sensor::getECuS()   { return sECuS; }
float MEC20Sensor::getRawEC()  { return sRawEC; }
bool  MEC20Sensor::hasError()  { return sError; }
bool  MEC20Sensor::heartbeat() { return sHeartbeat; }