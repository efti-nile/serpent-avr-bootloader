#include "crc16.h"

// CRC-16/USB
//  Poly: 0x8005
//  Init: 0xFFFF
//  Checksum for "123456789":

uint16_t crc16(uint16_t crc, const uint8_t *data, uint8_t len) {
  while (len--) {
    crc ^= *data++;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = crc >> 1 ^ 0xA001 * (crc & 1);
    }
  }
  return crc;
}
