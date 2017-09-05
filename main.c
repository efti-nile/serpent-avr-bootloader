#include "main.h"
#include "version-string.inc"

volatile uint8_t is_cmd_received = 0;

int main(void) {
  cmd_t cmd;
  init();
  while (1) {
    if (is_cmd_received) {
      // DDRD |= 0x08; PORTD ^= 0x08;
      parse_rx_buf(&cmd);
      switch (cmd.cmd_opcode) {
        case CMD_SEND_BTLDR_VERS: {
          uint8_t crc8_ = crc8(version_string, sizeof(version_string));
          USART_tx_buf_put(version_string, sizeof(version_string));
          USART_tx_buf_put(&crc8_, 1);
          break;
        }
        case CMD_WRITE_FLASH_PAGE: {
          cbc_decrypt(cmd.data, cmd.datalen / CIPH_BLOCK_LEN);
          uint32_t *pcrc32_ = (uint32_t *)(cmd.data + AVR_FLASH_PAGESIZE);
          uint16_t *ppage_no = (uint16_t *)(cmd.data + AVR_FLASH_PAGESIZE + sizeof(*pcrc32_));
          if (*pcrc32_ == crc32(cmd.data, AVR_FLASH_PAGESIZE)) {
            boot_program_page(*ppage_no, cmd.data);
          }
          // CRC8
          break;
        }
        case CMD_INIT_CIPHER: {
          cbc_init();
          break;
        }
        case CMD_NOTHING_TO_DO:
        default:
          break;
      }
      USART_rx_buf_purge();
      is_cmd_received = 0;
    }
    USART_tx_buf_put(version_string, sizeof(version_string));
    _delay_ms(500);
  }
  return 0;
}

void init(void) {
  USART_init();
  TIMSK1 |= OCIE1A; // enable timer 0 overflow interrupt
  asm("sei");
}

void parse_rx_buf(cmd_t *pcmd) {
  uint8_t num_bytes_wrote, msg_datalen, msg_len, msg_crc8;
  uint8_t buf[LIN_MSG_MAXLEN];
  num_bytes_wrote = USART_rx_buf_get(buf, LIN_MSG_MAXLEN);
  msg_datalen = buf[1];
  msg_len = LIN_MSG_MAXLEN - LIN_DATA_MAXLEN + msg_datalen;
  if (num_bytes_wrote == msg_len) {
    msg_crc8 = buf[msg_len - 1];
    if (msg_crc8 == crc8(buf, msg_len - 1)) {
      pcmd->cmd_opcode = (cmd_opcode_t) buf[2];
      pcmd->datalen = msg_datalen;
      memcpy(buf + 3, pcmd->data, msg_datalen);
    }
  }
  pcmd->cmd_opcode = CMD_NOTHING_TO_DO; // Parsing failed
}


ISR(TIMER1_COMPA_vect) {
  TCCR1B &= 0xF8; // stop timer
  TCNT1 = 0x0000; // zero timer
  TIFR1 |= OCF1A; // clear timer 1 comparator interrupt flag
  is_cmd_received = 1;
}

ISR(USART_RX_vect) {
  uint8_t rx_byte = UDR0;
  if ((TCCR1B & 0x07) == 0x00) { // if timer doesn't run
    if (rx_byte == LIN_ADD) { // if device address received
      OCR1A =  (uint8_t)(((double)LIN_MSG_MAXLEN * 2.0 * 10.0 / (double)USART_BAUD)\
        * ((double)F_CPU / 65536.0 * 1024.0)); // set timeout
      TCCR1B |= 0x05; // start timer at F_CPU/1024. Overflow every ~16 ms @ F_CPU == 16 MHz
    }
  } else {
    USART_rx_buf_put((uint8_t const *)&rx_byte, 1);
  }
}
