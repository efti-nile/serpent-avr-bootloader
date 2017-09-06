// CRC-8 - based on the CRC8 formulas by Dallas/Maxim
// Code released under the therms of the GNU GPL 3.0 license
#include "crc8.h"

unsigned char crc8(uint8_t crc, const uint8_t *data, uint8_t len) {
  while (len--) {
    unsigned char extract = *data++;
    for (unsigned char tempI = 8; tempI; tempI--) {
      unsigned char sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) {
        crc ^= 0x8C;
      }
      extract >>= 1;
    }
  }
  return crc;
}