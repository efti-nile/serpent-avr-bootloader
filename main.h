#ifndef MAIN_H_
#define MAIN_H_

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <avr/wdt.h>
#include "cbc-serpent.h"
#include "usart.h"
#include "crc32.h"
#include "crc16.h"
#include "crc8.h"
#include "avr-flash.h"
#include "radio.h"

__attribute__((used, section (".REDKEY"))) uint8_t red_key[4] = {0xE4, 0x68, 0xE9, 0x1A};
#define REDKEY_BTLDR_ADD 0x7FFC

#ifdef STUB
#warning Set STUB symbol. Bootloader want work properly.
#endif

#define APP_COUNTDOWN 500  // Delay before branch to the application, * 10 ms
#define RED_KEY_TIMEOUT (30*5)  // Timeout to check red key,  * 200 ms

///
// Pinout
#define LED_DDR DDRC
#define LED_PORT PORTC
#define LED_PIN_MASK (1 << 4)

///
// INFO-block
#define INFO_PAGE_NO 1
#define INFO_BEGIN 0x80
#define INFO_END (0xC0 - 1)
#define CRC_ADD 0xB4
#define APP_SIZE_ADD 0xB6
#define APP_MAXSIZE 12032
// Red key must be contained in one single flash page!
// Otherwise it won't be proper restored after firmware upgrade!
#define REDKEY_ADD 0xC0 // CONFIGURABLE 
#define REDKEY_LEN 4 // CONFIGURABLE 

///
// LIN-address
#define LIN_ADD 0x02 // CONFIGURABLE

///
// LIN-messaging constans
#define LIN_ADD_LEN 1
#define LIN_DATALEN_LEN 1
#define LIN_CMD_LEN 1
#define LIN_DATA_MAXLEN (SPM_PAGESIZE + CIPH_BLOCK_LEN)
#define LIN_CRC_LEN 1
#define LIN_MSG_MAXLEN (LIN_ADD_LEN + LIN_DATALEN_LEN + \
  LIN_CMD_LEN + LIN_DATA_MAXLEN + LIN_CRC_LEN)

typedef enum {
  CMD_SEND_BTLDR_VERS = 0x01,
  CMD_INIT_CIPHER,
  CMD_WRITE_FLASH_PAGE,
  ERR_CRC32_INCORRECT,
  CMD_NOTHING_TO_DO,
  CMD_KEEP_ALIVE,
  CMD_COMMIT_FLASH,
  ERR_FW_INCORRECT,
  CMD_CHECK_RED_KEY,
  ERR_RED_KEY_INCORRECT
} cmd_opcode_t;

typedef enum {
  RED_KEY_CORRECT = 49,
  RED_KEY_INCORRECT = 48
} data_constant_t;

typedef struct {
  cmd_opcode_t opcode;
  uint8_t datalen;
  uint8_t data[LIN_DATA_MAXLEN];
} cmd_t;

void send_ans(cmd_opcode_t opcode, const uint8_t *data, uint8_t datalen);
void parse_rx_buf(cmd_t *pcmd)  __attribute__((section (".RFID")));
uint8_t verify_app(uint16_t offset);
void write_app(void);
void app_countdown_start(void);
void app_countdown_stop(void);
void app_jump(void);
void stub(void);

#endif
