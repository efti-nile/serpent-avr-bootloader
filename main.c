#include "main.h"
#include "version-string.inc"

__flash const uint8_t *pred_key = (__flash const uint16_t *)REDKEY_ADD;
__flash const uint16_t *papp_crc = (__flash const uint16_t *)CRC_ADD;
__flash const uint16_t *papp_len = (__flash const uint16_t *)APP_SIZE_ADD;

volatile uint8_t is_cmd_received = 0;

int main(void) {
  cmd_t cmd;
#ifdef STUB
  stub();  // Infinite loop for debug purpose
#endif
  init();
  if (verify_app()) {
    app_countdown_start();
  }
  while (1) {
    if (is_cmd_received) {
      parse_rx_buf(&cmd);
      switch (cmd.opcode) {
        case CMD_SEND_BTLDR_VERS: {
          send_ans(CMD_SEND_BTLDR_VERS, version_string, sizeof(version_string));
          break;
        }
        case CMD_WRITE_FLASH_PAGE: {
          cbc_decrypt(cmd.data, cmd.datalen / CIPH_BLOCK_LEN);
          uint16_t *ppage_no = (uint16_t *)(cmd.data + SPM_PAGESIZE);
          uint32_t *pcrc32 = (uint32_t *)(cmd.data + SPM_PAGESIZE + sizeof(*ppage_no));
          if (*pcrc32 == crc32(cmd.data, SPM_PAGESIZE + sizeof(*ppage_no))) {
            if (*ppage_no == REDKEY_PAGE) { // If received page rewrites device serial number...
              for (uint8_t i = 0; i < REDKEY_LEN; i++) { // ...copy serial number from device flash
                cmd.data[REDKEY_ADD % SPM_PAGESIZE + i] = *(pred_key + 1);
              }
            }
            boot_program_page(*ppage_no, cmd.data);
            send_ans(CMD_WRITE_FLASH_PAGE, NULL, 0);
          } else {
            send_ans(ERR_CRC32_INCORRECT, NULL, 0);
          }
          break;
        }
        case CMD_INIT_CIPHER: {
          cbc_init();
          send_ans(CMD_INIT_CIPHER, NULL, 0);
          break;
        }
        case CMD_KEEP_ALIVE: {
          app_countdown_stop();
          break;
        }
        case CMD_NOTHING_TO_DO:
        default:
          break;
      }
      USART_rx_buf_purge();
      is_cmd_received = 0;
      USART_enable_receiver();
    }
  }
  return 0;
}

void init(void) {
  MCUCR = (1<<IVCE);
  MCUCR = (1<<IVSEL);
  USART_init();
  asm("sei");
}

uint8_t verify_app(void) {
  uint16_t crc = 0xFFFF;
  if (*papp_len > APP_MAXSIZE) {
    return 0;
  }
  for (uint16_t add = 0; add < *papp_len; add += 2) {
    uint16_t buf;
    if (add == INFO_BEGIN) {
      add = INFO_END + 1;  // Skip INFO-block
    }
    buf = *((__flash const uint16_t *) add);
    crc = crc16(crc, (const uint8_t *)&buf, sizeof(uint16_t));
  }
  crc = crc >> 8 | crc << 8;
  return crc == *papp_crc;
}

void send_ans(cmd_opcode_t opcode, const uint8_t *data, uint8_t datalen) {
  uint8_t crc = 0x00, lin_add = LIN_ADD;
  USART_tx_buf_put((const uint8_t *)&lin_add, sizeof(lin_add));
  crc = crc8(crc, (const uint8_t *)&lin_add, sizeof(lin_add));
  USART_tx_buf_put((const uint8_t *)&datalen, sizeof(datalen));
  crc = crc8(crc, (const uint8_t *)&datalen, sizeof(datalen));
  USART_tx_buf_put((const uint8_t *)&opcode, sizeof(opcode));
  crc = crc8(crc, (const uint8_t *)&opcode, sizeof(opcode));
  USART_tx_buf_put(data, datalen);
  crc = crc8(crc, data, datalen);
  USART_tx_buf_put(&crc, 1);
}

void parse_rx_buf(cmd_t *pcmd) {
  uint8_t num_bytes_wrote, msg_datalen, msg_len, msg_crc8;
  uint8_t buf[LIN_MSG_MAXLEN];
  num_bytes_wrote = USART_rx_buf_get(buf, LIN_MSG_MAXLEN);
  msg_datalen = buf[1];
  msg_len = LIN_MSG_MAXLEN - LIN_DATA_MAXLEN + msg_datalen;
  if (num_bytes_wrote == msg_len) {
    msg_crc8 = buf[msg_len - 1];
    if (msg_crc8 == crc8(0x00, buf, msg_len - 1)) {
      pcmd->opcode = (cmd_opcode_t) buf[2];
      pcmd->datalen = msg_datalen;
      memcpy(pcmd->data, buf + 3, msg_datalen);
      return;
    }
  }
  pcmd->opcode = CMD_NOTHING_TO_DO; // Parsing failed
}

ISR(TIMER1_COMPA_vect) {
  TCCR1B &= ~0x07; // stop timer
  TCNT1 = 0x0000; // zero timer
  TIFR1 |= 1 << OCF1A; // clear timer 1 comparator interrupt flag
  is_cmd_received = 1;
  USART_disable_receiver();
}

ISR(USART_RX_vect) {
  uint8_t rx_byte = UDR0;
  if ((TCCR1B & 0x07) == 0x00) { // if timer doesn't run
    if (rx_byte == LIN_ADD) { // if device address received
	    // OCR1A = 4818;
      TIMSK1 |= 1 << OCIE1A; // enable timer A compare interrupt
      OCR1A =  (uint16_t)(((double)LIN_MSG_MAXLEN * 2.0 * 10.0 / (double)USART_BAUD)\
        * ((double)F_CPU / 1024.0)); // set timeout
      TCCR1B |= 0x05; // start timer at F_CPU/1024. Overflow every ~16 ms @ F_CPU == 16 MHz
      USART_rx_buf_put((uint8_t const *)&rx_byte, 1);
    }
  } else {
    USART_rx_buf_put((uint8_t const *)&rx_byte, 1);
  }
}

ISR(TIMER0_COMPA_vect) {
  static uint16_t countdown = APP_COUNTDOWN;
  if (countdown-- == 0) {
    DDRC |= 1 << PC2;
    PORTC |= 1 << PC2;
  }
  TCNT0 = 0x00;
}

void app_countdown_start(void) {
  TIMSK0 = 1 << OCIE0A;
  OCR0A = (uint8_t) ((double)F_CPU / 1024.0 / 100.0); // 10 ms @ F_CPU == 16 MHz && 1024 prescaler
  TCCR0B = 0x05; // start timer at F_CPU/1024
}

void app_countdown_stop(void) {
  TCCR0B = 0x00;
}

void stub(void) {
  MCUCR = (1<<IVCE);
  MCUCR = (1<<IVSEL);
  DDRC |= 1 << PC2 | 1 << PC3;
  asm("sei");
  TIMSK1 |= 1 << OCIE1A;  // enable timer 0 compare interrupt
  OCR1A = (uint16_t) ((double)F_CPU / 1024.0); // for 1 s @ F_CPU == 16 MHz && 1024 prescaler
  TCCR1B |= 0x05; // start timer at F_CPU/1024
  for (uint8_t i = 0; ; i++) {
    if (is_cmd_received) {
        is_cmd_received = 0;
        PORTC ^= 1 << PC3;
        TCCR1B |= 0x05;
    }
    if (!i) {
      PORTC ^=  1 << PC2;
    }
    _delay_ms(1);
  }
}
