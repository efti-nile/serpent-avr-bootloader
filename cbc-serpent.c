#include "cbc-serpent.h"
#include "secret.inc"

static serpent_ctx_t ctx;

static unsigned char _buf1[CIPH_BLOCK_LEN];
static unsigned char _buf2[CIPH_BLOCK_LEN];
static unsigned char *acc = _buf1, *buf = _buf2;

void cbc_decrypt(unsigned char *src, unsigned char num_blocks_to_decrypt) {
  unsigned char *swap, i;
  for (i = 0; i < num_blocks_to_decrypt; i++) {
    memcpy(buf, src + i*CIPH_BLOCK_LEN, CIPH_BLOCK_LEN);
    serpent_dec(src + i*CIPH_BLOCK_LEN, &ctx);
    memxor(src + i*CIPH_BLOCK_LEN, acc, CIPH_BLOCK_LEN);
    swap = acc;
    acc = buf;
    buf = swap;
  }
}

void cbc_init(void) {
  serpent_init(key, SERPENT_KEY256, &ctx);
  memcpy(acc, iv, CIPH_BLOCK_LEN);
}
