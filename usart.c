// Simple USART driver written for ATmega328
#include "usart.h"

// Transmission complete flag
unsigned char USART_tx_complete = 1;

// Receiver circular buffer
unsigned char cb_rx[USART_CIRBUF_RX_LEN];
unsigned int head_rx = 0, tail_rx = 0;

// Transmitter circular buffer
unsigned char cb_tx[USART_CIRBUF_TX_LEN];
unsigned int head_tx = 0, tail_tx = 0;

// Initializes USART for ATmega328. !Needs reset values
// in USART control registers!
void USART_init(void){
  DDRD |= 1 << PD1; // Config TXD pin as output
  // UART receive interrupt: enable; UART receiver: Enable; UART transmitter: Enable.
  UCSR0B = 1 << RXCIE0 | 1 << TXCIE0 | 1 << RXEN0 | 1 << TXEN0;
  UBRR0L = F_CPU / 16 / USART_BAUD - 1; // 9600 @ F_CPU = 16MHz and UBRR0L = 103
}

// Default ISR for receiver
#ifdef USART_DEFAULT_RX_IRC
ISR(USART_RX_vect) {
  cb_rx[head_rx] = UDR0;
  head_rx = (head_rx + 1) & USART_CIRBUF_RX_LEN_MASK;
  if (head_rx == tail_rx ) {
    tail_rx = (tail_rx + 1) & USART_CIRBUF_RX_LEN_MASK;
  }
}
#endif

// Default ISR for transmitter
#ifdef USART_DEFAULT_TX_IRC
ISR(USART_TX_vect) {
  if (tail_tx != head_tx) {
    UDR0 = cb_tx[tail_tx];
    tail_tx = (tail_tx + 1) & USART_CIRBUF_TX_LEN_MASK;
  } else {
    USART_tx_complete = 1;
  }
}
#endif

// Get bytes from RX circular buffer. Returns number of read bytes.
unsigned char USART_rx_buf_get(unsigned char *src, unsigned char num) {
  unsigned char num_read = 0;
  while (tail_rx != head_rx && num_read < num) {
    *(src + num_read++) = cb_rx[tail_rx];
    tail_rx = (tail_rx + 1) & USART_CIRBUF_RX_LEN_MASK;
  }
  return num_read;
}

// Put in bytes RX circular buffer. Returns number of written bytes.
unsigned char USART_rx_buf_put(const unsigned char *src, unsigned char num) {
  unsigned char num_written = 0;
  while (num_written < num) {
    cb_rx[head_rx] = *(src + num_written++);
    head_rx = (head_rx + 1) & USART_CIRBUF_RX_LEN_MASK;
    if (head_rx == tail_rx ) {
      tail_rx = (tail_rx + 1) & USART_CIRBUF_RX_LEN_MASK;
    }
  }
  return num_written;
}

// Checks if RX circular buffer is empty
unsigned char USART_is_rx_buf_emtpty(void) {
  return tail_rx != head_rx;
}

// Purges RX circular buffer
void USART_rx_buf_purge(void){
  tail_rx = head_rx;
}

// Get bytes from TX circular buffer. Returns number of read bytes.
unsigned char USART_tx_buf_get(unsigned char *src, unsigned char num) {
  unsigned char num_read = 0;
  while (tail_tx != head_tx && num_read < num) {
    *(src + num_read++) = cb_tx[tail_tx];
    tail_tx = (tail_tx + 1) & USART_CIRBUF_TX_LEN_MASK;
  }
  return num_read;
}

// Put in bytes RX circular buffer. Returns number of written bytes.
unsigned char USART_tx_buf_put(const unsigned char *src, unsigned char num){
  unsigned char num_written = 0;
  while (num_written < num) {
    cb_tx[head_tx] = *(src + num_written++);
    asm("cli");
    head_tx = (head_tx + 1) & USART_CIRBUF_TX_LEN_MASK;
    if (head_tx == tail_tx ) {
      tail_tx = (tail_tx + 1) & USART_CIRBUF_TX_LEN_MASK;
    }
    asm("sei");
  }
  // Start transmission if yet not started
  if (USART_tx_complete && num_written) {
    USART_tx_buf_get((unsigned char *)&UDR0, 1);
    USART_tx_complete = 0;
  }
  return num_written;
}

// Checks if TX circular buffer is empty
unsigned char USART_is_tx_buf_emtpty(void){
  return tail_tx == head_tx;
}
