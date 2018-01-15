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
*       File: HT2CRYPT.H                                                    *
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
#ifndef __HT2CRYPT_H
#define __HT2CRYPT_H


#ifdef  __C51__

/* 8051-C (Keil) specific definitions/pragmas. */

#define BYTE         unsigned char           /* unsigned 8-bit value       */
#define BYTECONST    const BYTE code         /* array of BYTEs in ROM/CODE */
#define BYTEPTR      BYTE idata *            /* pointer to BYTEs in RAM    */
#define GETBIT(a,b)  (bit)((a) & (1<<(b) ))  /* fetch bit "b" out of "a"   */
#define TEST(a)      (BYTE)((bit)(a))        /* convert boolean to 00h/01h */

#pragma save
#pragma regparms     /* specify register based parameter passing.          */

#else

/* Standard ANSI-C definitions. (For different microcontroller             */
/* architectures, add your definitions here.)                              */

#define BYTE         unsigned char           /* unsigned 8-bit value       */
#define BYTECONST    const BYTE              /* array of BYTEs in ROM/CODE */
#define BYTEPTR      BYTE *                  /* pointer to BYTEs in RAM    */
#define GETBIT(a,b)  (BYTE)(((a) & (1<<(b) ))!=0)/*fetch bit "b" out of "a"*/
#define TEST(a)      (BYTE)((a) != 0)        /* convert boolean to 00h/01h */

#endif


/****************************************************************************
*                                                                           *
* Declarations in HT2CRYPT.C                                                *
*                                                                           *
****************************************************************************/

extern BYTE ident[4];        /* Transponder identifier */
extern BYTE secret_key[6];   /* Secret Key */

extern BYTE t[2];  /* The 48 bit ...     */
extern BYTE s[4];  /* ... shift register */


/****************************************************************************
*                                                                           *
* Function prototypes in HT2CRYPT.C                                         *
*                                                                           *
****************************************************************************/

extern BYTE function_bit(void)  __attribute__((section (".RFID")));
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



extern void shift_reg( BYTE shift_bit )  __attribute__((section (".RFID")));
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



extern BYTE feed_back(void)  __attribute__((section (".RFID")));
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



extern void Oneway1( BYTEPTR addr_rand ) __attribute__((section (".RFID")));
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
*   The global variables HT2ident and HT2secretkey are used for identifier, *
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



extern void Oneway2( BYTEPTR addr, BYTE length )  __attribute__((section (".RFID")));
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



#ifdef  __C51__      /* 8051-C specific definitions/pragmas. */
#pragma restore
#endif

#endif /* __HT2CRYPT_H */
