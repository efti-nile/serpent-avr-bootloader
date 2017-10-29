#ifndef AVR_FLASH_H
#define AVR_FLASH_H

#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/boot.h>

void boot_program_page (uint16_t page_no, uint8_t *buf);

#endif
