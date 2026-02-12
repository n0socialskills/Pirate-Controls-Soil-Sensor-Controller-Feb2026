#include "THCSensor.h"
#include "Globals.h"
#include "Config.h"
#include "Utils.h"      // [UPDATED] use shared median & CRC utilities

static const uint8_t MODBUS_REQ[8] = {0x01,0x03,0x00,0x00,0x00,0x03,0x05,0xCB};
static uint8_t modbusResp[11];

// enum SensorState { S_IDLE, S_WAIT, S_PROCESS };                // [REMOVED]
enum SensorState { S_IDLE, S_WAIT_POST_TX, S_WAIT, S_PROCESS };   // [ADDED] post-TX non-blocking wait state
static SensorState sState = S_IDLE;
static uint32_t sStateStart = 0;
const uint32_t SENSOR_WAIT_MS = 100;

static float soil_vwc = 0.0f;
static float soil_temp_c = 0.0f;
static float soil_pwec = 0.0f;
static float soil_ec_us = 0.0f;
static float soil_raw_ec = 0.0f;

static float ecRawSamples[3] = {0,0,0};
static int ecRawIndex = 0;
static bool sensorError = false;
static bool sHeartbeat = false;
static size_t sRxCount = 0;

void THCSensor::begin() {
  // [ADDED] Ensure RS485 baud set back to 4800 when THCS selected
  Globals::RS485.begin(4800, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN); // [ADDED]
  Globals::RS485.setTimeout(0);                                       // [ADDED]
  sState = S_IDLE;
  sStateStart = millis();
}

float THCSensor::getVWC() { return soil_vwc; }
float THCSensor::getTempC() { return soil_temp_c; }
float THCSensor::getPwec() { return soil_pwec; }
float THCSensor::getECuS() { return soil_ec_us; }
float THCSensor::getRawEC() { return soil_raw_ec; }
bool THCSensor::hasError() { return sensorError; }
bool THCSensor::heartbeat() { return sHeartbeat; }

void THCSensor::loop() {
  uint32_t now = millis();
  switch (sState) {
    case S_IDLE:
      if (now - sStateStart >= sensorIntervalMs) {
        digitalWrite(RS485_DE_PIN, HIGH);
        digitalWrite(RS485_RE_PIN, HIGH);
        delayMicroseconds(50);
        while (Globals::RS485.available()) Globals::RS485.read();
        Globals::RS485.write(MODBUS_REQ, sizeof(MODBUS_REQ));
        Globals::RS485.flush();
        // delay(POST_TX_DELAY_MS);                                      // [REMOVED] blocking
        sState = S_WAIT_POST_TX; sStateStart = now;                      // [ADDED] non-blocking post-TX wait
      }
      break;

    case S_WAIT_POST_TX:                                                // [ADDED]
      if (now - sStateStart >= POST_TX_DELAY_MS) {
        digitalWrite(RS485_DE_PIN, LOW);
        digitalWrite(RS485_RE_PIN, LOW);
        sRxCount = 0;
        sState = S_WAIT; sStateStart = now;
      }
      break;

    case S_WAIT:
      while (Globals::RS485.available() && sRxCount < sizeof(modbusResp)) {
        modbusResp[sRxCount++] = (uint8_t)Globals::RS485.read();
      }
      if (sRxCount >= 11) {
        sState = S_PROCESS;
      } else if (now - sStateStart >= SENSOR_WAIT_MS) {
        memset(modbusResp, 0, sizeof(modbusResp));
        sRxCount = 0;
        sensorError = true;
        Globals::webDebug(String("[THCSensor] RX TIMEOUT: no complete frame within ") + String(SENSOR_WAIT_MS) + " ms", true);
        sState = S_IDLE;
        sStateStart = now;
      }
      break;

    case S_PROCESS: {
      bool ok = false;
      if (modbusResp[0] == 0x01 && modbusResp[1] == 0x03 && modbusResp[2] == 0x06) {
        uint16_t crcCalc = Utils::modbusCRC16(modbusResp, 9);     // [UPDATED]
        uint16_t crcRecv = modbusResp[9] | (modbusResp[10] << 8);
        if (crcCalc == crcRecv) ok = true;
      }
      if (ok) {
        int16_t vwc10  = (int16_t)((modbusResp[3] << 8) | modbusResp[4]);
        int16_t temp10 = (int16_t)((modbusResp[5] << 8) | modbusResp[6]);
        uint16_t rawEC = (uint16_t)((modbusResp[7] << 8) | modbusResp[8]);

        soil_vwc = (float)vwc10 / 10.0f;
        soil_temp_c = (float)temp10 / 10.0f;
        soil_raw_ec = (float)rawEC;

        if (ecRawSamples[0] == 0.0f && ecRawSamples[1] == 0.0f && ecRawSamples[2] == 0.0f && soil_raw_ec > 0.0f) {
          ecRawSamples[0] = ecRawSamples[1] = ecRawSamples[2] = soil_raw_ec;
          ecRawIndex = 0;
        } else {
          ecRawSamples[ecRawIndex] = soil_raw_ec;
          ecRawIndex = (ecRawIndex + 1) % 3;
        }

        float medRaw = Utils::median3(ecRawSamples[0], ecRawSamples[1], ecRawSamples[2]); // [UPDATED]
        float ecCal = EC_SLOPE * medRaw + EC_INTERCEPT;
        if (EC_TEMP_COEFF != 0.0f && soil_temp_c != 25.0f) {
          ecCal = ecCal / (1.0f + EC_TEMP_COEFF * (soil_temp_c - 25.0f));
        }
        soil_ec_us = ecCal;

        float epsilon_bulk = 1.3088f + 0.1439f * soil_vwc + 0.0076f * soil_vwc * soil_vwc;
        float epsilon_pore = 80.3f - 0.37f * (soil_temp_c - 20.0f);
        float epsilon_offset = 4.1f;
        if (epsilon_bulk > epsilon_offset) {
          float pw_ec_us = (epsilon_pore * soil_ec_us) / (epsilon_bulk - epsilon_offset);
          soil_pwec = pw_ec_us / 1000.0f;
        } else soil_pwec = 0.0f;
        soil_pwec = constrain(soil_pwec, 0.0f, 20.0f);

        if (sensorError) sensorError = false;
        sHeartbeat = !sHeartbeat;
      } else {
        uint16_t crcCalc = Utils::modbusCRC16(modbusResp, 9);       // [UPDATED]
        uint16_t crcRecv = modbusResp[9] | (modbusResp[10] << 8);
        sensorError = true;
        Globals::webDebug(String("[THCSensor] CRC FAIL calc=") + String(crcCalc) + " recv=" + String(crcRecv), true);
      }
      sState = S_IDLE;
      sStateStart = now;
      break;
    }

    default:
      sState = S_IDLE; sStateStart = now;
      break;
  }
}