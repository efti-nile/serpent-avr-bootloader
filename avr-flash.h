#ifndef AVR_FLASH_H
#define AVR_FLASH_H

#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/boot.h>

#define AVR_FLASH_PAGESIZE (256)

void boot_program_page (uint32_t page, uint8_t *buf);

#endif