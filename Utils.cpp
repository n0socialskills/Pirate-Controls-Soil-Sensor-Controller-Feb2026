#include "Utils.h"

String Utils::uptimeHuman(uint32_t sec) {
  uint32_t d = sec / 86400; sec %= 86400;
  uint32_t h = sec / 3600; sec %= 3600;
  uint32_t m = sec / 60; sec %= 60;
  char buf[48];
  if (d) snprintf(buf, sizeof(buf), "%ud %uh %um %us", d,h,m,sec);
  else if (h) snprintf(buf, sizeof(buf), "%uh %um %us", h,m,sec);
  else if (m) snprintf(buf, sizeof(buf), "%um %us", m,sec);
  else snprintf(buf, sizeof(buf), "%us", sec);
  return String(buf);
}

// [ADDED] Implementation of CRC16 for Modbus RTU
uint16_t Utils::modbusCRC16(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (int j = 0; j < 8; ++j) {
      if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
      else crc >>= 1;
    }
  }
  return crc;
}