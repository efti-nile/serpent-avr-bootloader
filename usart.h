// Simple USART driver written for ATmega328
#ifndef USART_H_
#define USART_H_

#include <avr/io.h>
#include <avr/interrupt.h>

#define USART_BAUD (9600)
#define USART_CIRBUF_RX_LEN (512) // Must be power-of-two and less than 256!
#define USART_CIRBUF_TX_LEN (64) // Must be power-of-two and less than 256!

// #define USART_DEFAULT_RX_IRC // Uncomment to use default RX IRC
#define USART_DEFAULT_TX_IRC // Uncomment to use default TX IRC

#define USART_CIRBUF_RX_LEN_MASK (USART_CIRBUF_RX_LEN - 1)
#define USART_CIRBUF_TX_LEN_MASK (USART_CIRBUF_TX_LEN - 1)

void USART_init(void);

void USART_disable_receiver(void);
void USART_enable_receiver(void);
unsigned char USART_rx_buf_get(unsigned char *src, unsigned char num);
unsigned char USART_rx_buf_put(const unsigned char *src, unsigned char num);
unsigned char USART_is_rx_buf_emtpty(void);
void USART_rx_buf_purge(void);

unsigned char USART_tx_buf_get(unsigned char *src, unsigned char num);
unsigned char USART_tx_buf_put(const unsigned char *src, unsigned char num);
unsigned char USART_is_tx_buf_emtpty(void);
#endif
