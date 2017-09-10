#ifndef AVR_FLASH_H
#define AVR_FLASH_H

#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/boot.h>

#ifdef DEBUG
#define APP_OFFSET 0x7000
#else
#define APP_OFFSET 0x0000
#endif

void boot_program_page (uint16_t page_no, uint8_t *buf);

#endif
