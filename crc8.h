// CRC-8 - based on the CRC8 formulas by Dallas/Maxim
// Code released under the therms of the GNU GPL 3.0 license
#ifndef CRC8_H_
#define CRC8_H_

#include <inttypes.h>

unsigned char crc8(uint8_t crc, const uint8_t *data, uint8_t len);
#endif