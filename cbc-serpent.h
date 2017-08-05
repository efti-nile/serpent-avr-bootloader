#ifndef CBC_SERPENT_H
#define CBC_SERPENT_H

#include <string.h>
#include "serpent/serpent.h"
#include "memxor/memxor.h"

#define CIPH_BLOCK_LEN (16)

void cbc_init(void);
void cbc_decrypt(unsigned char *src, unsigned char num_blocks_to_decrypt);

#endif
