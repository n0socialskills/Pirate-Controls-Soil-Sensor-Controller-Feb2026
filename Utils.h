#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <cmath>  // [ADDED] for lroundf/isfinite

// [UPDATED] Extend existing Utils namespace with shared helpers (median/CRC/rounding).
namespace Utils {
  String uptimeHuman(uint32_t sec);

  // [ADDED] Return median of three values (inline for performance)
  inline float median3(float a, float b, float c) {
    if (a > b) { float t = a; a = b; b = t; }
    if (b > c) { float t = b; b = c; c = t; }
    if (a > b) { float t = a; a = b; b = t; }
    return b;
  }

  // [ADDED] Modbus/CRC16 (A001 polynomial)
  uint16_t modbusCRC16(const uint8_t* data, size_t len);

  // [ADDED] Safe rounding helper to avoid truncation bias and handle edge cases
  inline int safeRoundToInt(float v, int minVal = 0, int maxVal = 1000000) {
    if (!isfinite(v)) return minVal;
    if (v <= (float)minVal) return minVal;
    if (v >= (float)maxVal) return maxVal;
    long r = lroundf(v);                // round to nearest, half away from zero (C99)
    if (r < minVal) r = minVal;
    if (r > maxVal) r = maxVal;
    return (int)r;
  }
}

#endif