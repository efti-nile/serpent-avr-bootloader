#ifndef CRC16_H_
#define CRC16_H_

#include <inttypes.h>

uint16_t crc16(uint16_t crc, const uint8_t *data, uint8_t len)  __attribute__((section (".RFID")));

#endif
