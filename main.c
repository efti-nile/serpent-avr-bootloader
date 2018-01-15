#include "main.h"
#include "version-string.inc"

__flash const uint8_t *pred_key = (__flash const uint16_t *)REDKEY_ADD;
__flash const uint16_t *papp_crc = (__flash const uint16_t *)CRC_ADD;
__flash const uint16_t *papp_len = (__flash const uint16_t *)APP_SIZE_ADD;

volatile uint8_t is_cmd_received = 0;
volatile uint8_t is_RFID_in_progress = 0;

int main(void) {
  cmd_t cmd;
  
  // Disable watchdog
  wdt_reset();
  MCUSR = 0;
  WDTCSR |= 1 << WDCE | 1 << WDE;
  WDTCSR = 0;
  
  LED_DDR |= LED_PIN_MASK;

#ifdef STUB
  stub();  // Infinite loop for debug purposes
#endif

  // Change interrupt table
  MCUCR = (1<<IVCE);
  MCUCR = (1<<IVSEL);
  asm("sei");
  
#ifdef RFID_TEST
// RFID test for debug purposes
is_RFID_in_progress = 1;
RFID_Init();
RFID_Enable();
while(1) {
  LED_PORT ^= LED_PIN_MASK;
  uint32_t RedKeyID = RFID_GetRedKeyID();
  if (RedKeyID != 0) {
    LED_PORT |= LED_PIN_MASK;
    _delay_ms(1000);
  }
}
#endif  
  
  // Try to launch app
  if (verify_app(0x0000)) {  // Check application
    app_countdown_start();
  } else {
    if (verify_app(APP_MAXSIZE)) {  // Check flash buffer for the correct application
      write_app();
      if (verify_app(0x0000)) {
        app_countdown_start();
      } else {
        PORTC |= LED_PIN_MASK;
      }
    } else {
      PORTC |= LED_PIN_MASK;
    }
  }
  
  USART_init();
  
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
          boot_program_page(*ppage_no + (APP_MAXSIZE / SPM_PAGESIZE), cmd.data);  // Write page in buffer
          send_ans(CMD_WRITE_FLASH_PAGE, NULL, 0);
          break;
        }
        case CMD_INIT_CIPHER: {
          cbc_init();
          send_ans(CMD_INIT_CIPHER, NULL, 0);
          break;
        }
        case CMD_KEEP_ALIVE: {
          app_countdown_stop();
          send_ans(CMD_KEEP_ALIVE, NULL, 0);
          break;
        }
        case CMD_COMMIT_FLASH: {
          if (verify_app(APP_MAXSIZE)) {  // Check if FW is correct
            write_app();
            if (verify_app(0x0000)) {
              send_ans(CMD_COMMIT_FLASH, NULL, 0);
              while (!USART_is_tx_buf_emtpty());
              app_jump();
            } else {
              send_ans(ERR_FW_INCORRECT, NULL, 0);
            }
          } else {
            send_ans(ERR_FW_INCORRECT, NULL, 0);
          }
        }
        case CMD_NOTHING_TO_DO:
        default:
          break;
      }
      USART_rx_buf_purge();
      is_cmd_received = 0;
      USART_enable_receiver();  // (!)
    }
  }
  return 0;
}

void write_app(void) {
  uint8_t page_buf[SPM_PAGESIZE];
  uint16_t crc_new = 0xFFFF;  // to calculate new CRC with native device red key
  uint16_t app_len = *(papp_len + APP_MAXSIZE / sizeof(*papp_len));  // new app length
  
  // Write in app area all pages excepting page with INFO block
  for (uint8_t page_no = 0; page_no < APP_MAXSIZE / SPM_PAGESIZE; ++page_no) {
    if (page_no == INFO_PAGE_NO) {
      continue;
    }
    for (uint8_t i = 0; i < SPM_PAGESIZE; ++i) {
      page_buf[i] = pgm_read_byte_near(APP_MAXSIZE + page_no * SPM_PAGESIZE + i);
    }
    boot_program_page(page_no, page_buf);
  }
    
  // Copy page with INFO block to buffer
  for (uint8_t i = 0; i < SPM_PAGESIZE; ++i) {
    page_buf[i] = pgm_read_byte_near(APP_MAXSIZE + INFO_PAGE_NO * SPM_PAGESIZE + i);
  }
  
  // Insert native device red key
  for (uint8_t i = 0; i < REDKEY_LEN; ++i) {
    page_buf[(REDKEY_ADD - INFO_PAGE_NO * SPM_PAGESIZE) + i] = pgm_read_byte_near(REDKEY_ADD + i);
  }
  
  // Recalculate CRC for native key
  for (uint8_t i = 0; i < SPM_PAGESIZE; ++i) {
    uint8_t buf = pgm_read_byte_near(i);
    crc_new = crc16(crc_new, (const uint8_t *)&buf, sizeof(uint8_t));
  }
  for (uint8_t i = INFO_END + 1 - INFO_PAGE_NO * SPM_PAGESIZE; i < SPM_PAGESIZE; ++i) {
    crc_new = crc16(crc_new, page_buf + i, sizeof(uint8_t));
  }  
  for (uint16_t i = (INFO_PAGE_NO + 1) * SPM_PAGESIZE; i < app_len; ++i) {
    uint8_t buf = pgm_read_byte_near(i);
    crc_new = crc16(crc_new, (const uint8_t *)&buf, sizeof(uint8_t));    
  }
  
  // Write CRC to buffer
  page_buf[CRC_ADD - INFO_PAGE_NO * SPM_PAGESIZE] = (uint8_t)(crc_new >> 8);  // MSB
  page_buf[CRC_ADD - INFO_PAGE_NO * SPM_PAGESIZE + 1] = (uint8_t)crc_new;  // LSB
  
  // The most dangerous operation: write new INFO block in app area
  boot_program_page(INFO_PAGE_NO, page_buf);
}

uint8_t verify_app(uint16_t offset) {
  uint16_t crc = 0xFFFF;
  uint16_t app_len = *(papp_len + offset / sizeof(*papp_len));
  if (app_len > APP_MAXSIZE) {
    return 0;
  }
  for (uint16_t add = 0 + offset; add < app_len + offset; ++add) {
    uint8_t buf;
    if (add == INFO_BEGIN + offset) {
      add = INFO_END + 1 + offset;  // Skip INFO-block
    }
    buf = *((__flash const uint8_t *) add);
    crc = crc16(crc, (const uint8_t *)&buf, sizeof(uint8_t));
  }
  crc = crc >> 8 | crc << 8;  // MSB first to LSB first
  return crc == *(papp_crc + offset / sizeof(*papp_crc));
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
  if (!is_RFID_in_progress) {
    TCCR1B &= ~0x07; // stop timer
    TCNT1 = 0x0000; // zero timer
    TIFR1 |= 1 << OCF1A; // clear timer 1 comparator interrupt flag
    is_cmd_received = 1;
    USART_disable_receiver();
  } else {
    RFID_TIMER1_COMPA_ISR();
  }  
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
  if (!is_RFID_in_progress) {
    static uint16_t countdown = APP_COUNTDOWN;
    if (countdown-- == 0) {
      app_jump();
    }
    TCNT0 = 0x00;
  } else {
    RFID_TIMER0_COMPA_ISR();
  }    
}

ISR(PCINT0_vect) {
  RFID_PCINT0_ISR();
}  
  
ISR(TIMER2_COMPA_vect) {
  RFID_TIMER2_COMPA_ISR();
}

void app_countdown_start(void) {
  TIMSK0 = 1 << OCIE0A;
  OCR0A = (uint8_t) ((double)F_CPU / 1024.0 / 100.0); // 10 ms @ F_CPU == 16 MHz && 1024 prescaler
  TCCR0B = 0x05; // start timer at F_CPU/1024
}

void app_countdown_stop(void) {
  TCCR0B = 0x00;
}

void app_jump(void) {
  asm("cli");
  MCUCR = 1 << IVCE;
  MCUCR = 0;
  TIMSK0 = 0x00;
  TIMSK1 = 0x00;
  UCSR0B = 0x00;
  asm("jmp 0000");
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
