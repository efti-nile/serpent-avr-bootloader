#include "avr-flash.h"

// Used source from http://www.nongnu.org/avr-libc/user-manual/group__avr__boot.html
void boot_program_page (uint16_t page_no, uint8_t *buf) {
  uint8_t sreg;
  uint16_t page = page_no * SPM_PAGESIZE;
  // Disable interrupts.
  sreg = SREG;
  cli();
  eeprom_busy_wait();
  boot_page_erase(page);
  boot_spm_busy_wait();  // Wait until the memory is erased.
  for (uint8_t i = 0; i < SPM_PAGESIZE; i += 2) {
      // Set up little-endian word.
      uint16_t w = *buf++;
      w += (*buf++) << 8;

      boot_page_fill (page + i, w);
  }
  boot_page_write (page);     // Store buffer in flash page.
  boot_spm_busy_wait();       // Wait until the memory is written.
  // Re-enable RWW-section again. We need this if we want to jump back
  // to the application after boot loading.
  boot_rww_enable ();
  // Re-enable interrupts (if they were ever enabled).
  SREG = sreg;
}
