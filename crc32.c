#include "crc32.h"

// CRC-32/zlib
//  Poly: 0x04C11DB7
//  Init: 0xFFFFFFFF
//  Checksum for "123456789": 0xCBF43926

uint32_t crc32(const uint8_t *data, uint8_t len) {
  uint32_t crc = 0xFFFFFFFFU;
  while (len--) {
    crc ^= *data++;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = crc >> 1 ^ 0xEDB88320U * (crc & 1);
    }
  }
  return ~crc;
}