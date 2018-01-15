/****************************************************************************
*                                                                           *
*              Copyright (C),  Philips Semiconductors BU ID                 *
*                                                                           *
*  All rights are reserved. Reproduction in whole or in part is prohibited  *
*           without the written consent of the copyright owner.             *
*                                                                           *
*  Philips reserves the right to make changes without notice at any time.   *
* Philips makes no warranty, expressed, implied or statutory, including but *
* not limited to any implied warranty of merchantibility or fitness for any *
*   particular purpose, or that the use will not infringe any third party   *
* patent, copyright or trademark. Philips must not be liable for any loss or*
*                      damage arising from its use.                         *
*                                                                           *
*****************************************************************************
*                                                                           *
*       File: HT2CRYPT.C                                                    *
*       Author:  F. Boeh, J. Nowottnick                                     *
*                                                                           *
*****************************************************************************
*      DATE      *          CHANGES DONE                       *    BY      *
*****************************************************************************
*                *                                             *            *
*  Mar 9, 1998   * Start implementation                        * Boeh       *
*  Mar 18, 1998  * First release                               * Boeh       *
*  June 12, 2001 * Remote mode description added               * Nowottnick *
*  June 13, 2001 * Automatic target detection                  * Boeh       *
****************************************************************************/

#include "ht2crypt.h"


#ifdef  __C51__      /* 8051-C specific definitions/pragmas. */
#pragma regparms     /* specify register based parameter passing. */
#endif


/****************************************************************************
* Table which contains the EXOR value of a 4 bit input.                     *
****************************************************************************/
BYTECONST exor_table[16] = {
  0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0
} ;



/****************************************************************************
* Non-linear functions F0, F1 and F2.                                       *
*                                                                           *
* The logic "one" entries of F0 and F1 have multiple bits set which         *
* makes it unnecessary to shift the values when combining the results       *
* to the input vector for F2.                                               *
* (Only bit masking necessary, see function_bit(). )                        *
****************************************************************************/
#define A (1+16)
#define B (2+4+8)

BYTECONST F0_table[16] = {
  A, 0, 0, A, A, A, A, 0, 0, 0, A, A, 0, A, 0, 0
};

BYTECONST F1_table[16] = {
  B, 0, 0, 0, B, B, B, 0, 0, B, B, 0, 0, B, B, 0
};

BYTECONST F2_table[32] = {
  1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0,
  1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0
};

#undef A
#undef B



/****************************************************************************
* Main parameters for HITAG2 cryptographic algorithm.                       *
****************************************************************************/
BYTE ident[4];      /* Transponder identifier */
BYTE secret_key[6] =                 {0x5D, 0x93, 0x81, 0x52, 0x5F, 0x7A};
//BYTE secret_key[6]=                 {0x4E, 0x80, 0x92, 0x41, 0x4C, 0x69};

BYTE t[2];  /* The 48 bit ...     */
BYTE s[4];  /* ... shift register */




BYTE function_bit(void)
/****************************************************************************
*                                                                           *
* Description:                                                              *
*   Computes the result of the non-linear function F2= f(t,s) for both      *
*   oneway functions 1 and 2.                                               *
*                                                                           *
* Parameters: none                                                          *
*                                                                           *
* Return: Result of F2, either 0 or 1.                                      *
*                                                                           *
****************************************************************************/
{
  BYTE F01_index;  /* Index to tables F0 and F1 */
  BYTE F2_index;   /* Index to table F2 */
  
  F01_index  = GETBIT( t[0], 1 );
  F01_index <<= 1;
  F01_index |= GETBIT( t[0], 2 );
  F01_index <<= 1;
  F01_index |= GETBIT( t[0], 4 );
  F01_index <<= 1;
  F01_index |= GETBIT( t[0], 5 );
  F2_index  = F0_table[ F01_index ] & (BYTE)0x01;
  
  F01_index  = GETBIT( t[1], 0 );
  F01_index <<= 1;
  F01_index |= GETBIT( t[1], 1 );
  F01_index <<= 1;
  F01_index |= GETBIT( t[1], 3 );
  F01_index <<= 1;
  F01_index |= GETBIT( t[1], 7 );
  F2_index |= F1_table[ F01_index ] & (BYTE)0x02;
  
  F01_index  = GETBIT( s[1], 5 );
  F01_index <<= 1;
  F01_index |= GETBIT( s[0], 0 );
  F01_index <<= 1;
  F01_index |= GETBIT( s[0], 2 );
  F01_index <<= 1;
  F01_index |= GETBIT( s[0], 6 );
  F2_index |= F1_table[ F01_index ] & (BYTE)0x04;
  
  F01_index  = GETBIT( s[2], 6 );
  F01_index <<= 1;
  F01_index |= GETBIT( s[1], 0 );
  F01_index <<= 1;
  F01_index |= GETBIT( s[1], 2 );
  F01_index <<= 1;
  F01_index |= GETBIT( s[1], 3 );
  F2_index |= F1_table[ F01_index ] & (BYTE)0x08;
  
  F01_index  = GETBIT( s[3], 1 );
  F01_index <<= 1;
  F01_index |= GETBIT( s[3], 3 );
  F01_index <<= 1;
  F01_index |= GETBIT( s[3], 4 );
  F01_index <<= 1;
  F01_index |= GETBIT( s[2], 5 );
  F2_index |= F0_table[ F01_index ] & (BYTE)0x10;
  
  return F2_table[ F2_index ];
}




void shift_reg( BYTE shift_bit )
/****************************************************************************
*                                                                           *
* Description:                                                              *
*   Performs the shift operation of t,s. Both registers are shifted left    *
*   by one bit (with carry propagation from s to t), the feedback given by  *
*   "shift_bit" is inserted into s.                                         *
*                                                                           *
* Parameters:                                                               *
*   shift_bit: Bit to be shifted into the register, must be 0 or 1.         *
*                                                                           *
* Return: none                                                              *
*                                                                           *
****************************************************************************/
{
  t[0] <<= 1;
  t[0] |= GETBIT( t[1], 7 );
  
  t[1] <<= 1;
  t[1] |= GETBIT( s[0], 7 );
  
  s[0] <<= 1;
  s[0] |= GETBIT( s[1], 7 );
  
  s[1] <<= 1;
  s[1] |= GETBIT( s[2], 7 );
  
  s[2] <<= 1;
  s[2] |= GETBIT( s[3], 7 );
  
  s[3] <<= 1;
  s[3] |= shift_bit;
}



BYTE feed_back(void)
/****************************************************************************
*                                                                           *
* Description:                                                              *
*   Calculates the feedback for oneway function 2 by EXORing the specified  *
*   bits from the shift register t,s.                                       *
*                                                                           *
* Parameters: none                                                          *
*                                                                           *
* Return: Result of feedback calculation, either 0 or 1.                    *
*                                                                           *
****************************************************************************/
{
  BYTE sum;
  
  /* Fetch the relevant bits from shift register, perform first EXOR: */
  sum = (t[0] & (BYTE)0xB3) ^ (t[1] & (BYTE)0x80) ^
        (s[0] & (BYTE)0x83) ^ (s[1] & (BYTE)0x22) ^
        (s[3] & (BYTE)0x73);
        
  /* EXOR all 8 bits of "sum" and return the result: */
  return exor_table[ sum % 16 ] ^ exor_table[ sum / 16 ];
}



void Oneway1( BYTEPTR addr_rand )
/****************************************************************************
*                                                                           *
* Description:                                                              *
*   Performs the initialization phase of the HITAG2/HITAG2+ cryptographic   *
*   algorithm. This function can be used to implement the cryptographic     *
*   protocol in transponder mode (HITAG2/HITAG2+) and remote mode (HITAG2+) *
*   Depending on the mode of operation, the initialization is done with     *
*   different input parameters.                                             *
*                                                                           *
*   a) Transponder Mode:                                                    *
*   The feedback registers are loaded with identifier and immobilizer       *
*   secret key, and oneway function 1 is executed 32 times thereafter.      *
*   The global variables ident and secretkey are used for identifier,       *
*   immobilizer secret key, the random number is given as an input          *
*   to the function.                                                        *
*                                                                           *
*   b) Remote Mode:                                                         *
*   The feedback registers are loaded with identifier and remote            *
*   secret key, and oneway function 1 is executed 32 times thereafter.      *
*   The global variables HT2ident and HT2secretkey are used for identifier, *
*   remote secret key. The sequence increment (28bit) and the command ID    *
*   (4bit) are given together as 32bit input data block to the function.    *
*                                                                           *
*   The function outputs the initialized global shift registers t,s.        *
*                                                                           *
* Parameters:                                                               *
*   addr_rand: Pointer to a memory area of 4 bytes that contains the        *
*              random number (transponder mode)                             *
*              or sequence increment + command ID (remote mode)             *
*              to be used for the initialization.                           *
*                                                                           *
* Return: none                                                              *
*                                                                           *
****************************************************************************/
{
  BYTE bit_mask;  /* Used to fetch single bits of random/sec_key.*/
  BYTE byte_cnt;  /* Byte counter for random/secret key.         */
  BYTE fb;        /* Feedback bit for oneway function 1.         */
  
  for (unsigned char i = 0; i <= 5; i++) {
    secret_key[i] = secret_key[i] ^ 0x13;
  }
  
  /* Initialise oneway function 1 with identifier and parts of secret key */
  t[0] = ident[0];
  t[1] = ident[1];
  s[0] = ident[2];
  s[1] = ident[3];
  s[2] = secret_key[4];
  s[3] = secret_key[5];
  
  
  /* Perform 32 times oneway function 1 (nonlinear feedback) */
  byte_cnt = 0;
  bit_mask = 0x80; /* Setup bit mask: MSB of first byte */
  
  do {
    /* One round of oneway function 1: */
    fb = function_bit()
         ^ TEST( (secret_key[byte_cnt] ^ addr_rand[byte_cnt]) & bit_mask );
    shift_reg( fb );
    
    /* Advance to next bit of random number / secret key: */
    bit_mask >>= 1;
    
    if( bit_mask == 0 ) {
      bit_mask = 0x80;
      byte_cnt++;
    }
  } while(byte_cnt < 4);
}



void Oneway2( BYTEPTR addr, BYTE length )
/****************************************************************************
*                                                                           *
* Description:                                                              *
*   Performs the encryption respective decryption of a given data block     *
*   by repeatedly executing oneway function 2 and Exclusive-Oring with the  *
*   generated cipher bits. The computation is repeated for the number       *
*   of bits as specified by "length". The global shift register contents    *
*   after the initialization by Oneway1(), or the current contents after    *
*   the last call of Oneway2() is used as start condition.                  *
*                                                                           *
* Parameters:                                                               *
*   addr:   Pointer to a memory area that contains the data to be           *
*           encrypted or decrypted.                                         *
*   length: Number of bits to encrypt / decrypt.                            *
*                                                                           *
* Return: none                                                              *
*                                                                           *
****************************************************************************/
{
  BYTE bit_mask; /* Mask for current bit of data block. */
  BYTE bitval;
  
  bit_mask = 0x80; /* Setup bit mask: MSB of first byte */
  
  do {
  
    /* Calculate cipher bit and perform EXOR on data bit: */
    /* NOTE: Timing invariant implementation (uses multiplication). */
    /* NOTE: Possible performance degradation on other machines.    */
    bitval = (BYTE)((function_bit() ^ TEST( *addr & bit_mask )) * bit_mask);
    *addr = (*addr & (BYTE)~bit_mask) | bitval;
    
    shift_reg( feed_back() );
    
    bit_mask >>= 1;
    
    if( bit_mask == 0 ) {
      bit_mask = 0x80;
      addr++;
    }
    
    length--;
  } while( length );
}
