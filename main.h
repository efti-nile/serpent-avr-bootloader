#ifndef MAIN_H_
#define MAIN_H_

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "cbc-serpent.h"
#include "usart.h"
#include "crc32.h"
#include "crc8.h"
#include "avr-flash.h"

///
// LIN-messaging constans
#define LIN_ADD_LEN 1
#define LIN_DATALEN_LEN 1
#define LIN_CMD_LEN 1
#define LIN_DATA_MAXLEN (SPM_PAGESIZE + CIPH_BLOCK_LEN) // CONFIGURABLE
#define LIN_CRC_LEN 1
#define LIN_MSG_MAXLEN (LIN_ADD_LEN + LIN_DATALEN_LEN + \
  LIN_CMD_LEN + LIN_DATA_MAXLEN + LIN_CRC_LEN)

typedef enum {
  CMD_SEND_BTLDR_VERS = 0x01,
  CMD_WRITE_FLASH_PAGE,
  CMD_INIT_CIPHER,
  CMD_NOTHING_TO_DO
} cmd_opcode_t;

typedef enum {
  ANS_CIPHER_INITIALIZED = 0x01,
  ANS_FLASH_PAGE_WRITTEN,
  ANS_BOOTLOADER_VERSION
} ans_opcode_t;

typedef struct {
  cmd_opcode_t opcode;
  uint8_t datalen;
  uint8_t data[LIN_DATA_MAXLEN];
} cmd_t;

#define LIN_ADD 0x02 // CONFIGURABLE Device LIN-address

void init(void);
void send_ans(ans_opcode_t opcode, const uint8_t *data, uint8_t datalen);
void parse_rx_buf(cmd_t *pcmd);
uint8_t verify_fw(void);

#endif
