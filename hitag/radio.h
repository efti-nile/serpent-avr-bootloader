#ifndef RADIO_H
#define RADIO_H

#include <avr/io.h>
#include <util/delay.h>
#include <string.h> // memcpy, memcmp

#include "Ht2crypt.h"

#define TimeAntena 30

//Настройка приема под конктретный таймер
#define TCNT TCNT0
#define OCRA OCR0A
#define TIFR TIFR0
#define OCFA OCF0A
#define TIMSK TIMSK0
#define OCIEA OCIE0A

/*
#define TCNT TCNT1
#define OCRA OCR1A
#define TIFR TIFR1
#define OCFA OCF1A
#define TIMSK TIMSK1
#define OCIEA OCIE1A
*/

#define Delay_16bit 100
#define Delay_32bit 190
#define Delay_64bit 255

#define SPI_HalfStrobe 8
#define SPI_QartStrobe 4

#define RadioPort PORTB
#define RadioDDR DDRB
#define MOSI PB3
#define MISO PB4
#define SCK PB5

extern unsigned char BiteCounter;
extern unsigned char BitMessage[100];
extern unsigned char AnswerLength;
extern unsigned char data_block[8];

//unsigned char const My_ISK[6]=                 {0x4E, 0x80, 0x92, 0x41, 0x4C, 0x69};
//unsigned char const My_Defaul_Base_Password[4]={0x4E, 0x80, 0x92, 0x41}; //Defaul Read-write device password
//*unsigned char const My_Trans_Password[3]=      {0x1D, 0x0C, 0x07};
//unsigned char Random[4]=                 {0x5A, 0xAB, 0xDC, 0x0F}; // Fictive random number

//void memcpy(unsigned char Target[], unsigned char const Source[], unsigned char Count);
//unsigned char memcmp(unsigned char const Target[], unsigned char const Source[], unsigned char Count);
void MemToArray(unsigned char Mem[], unsigned char Array[], unsigned char First, unsigned char Count);
void ArrayToMem(unsigned char Array[], unsigned char Mem[], unsigned char First, unsigned char Count);
void ArrayToMemRevers(unsigned char Array[], unsigned char Mem[], unsigned char First, unsigned char Count);


void InitRadio(void);
unsigned char Send_StartAuthent(void);
void SetPhase(void);
void FastSetPhase(void);
void Send_Start(void);
void Send_Stop(void);
unsigned char Send_Password(void);
unsigned char Send_ConfigPage();
unsigned char Send_WritePage(unsigned char Nomber, unsigned char TempPage[]);
unsigned char Send_ReadPage(unsigned char Nomber);


void SendToReader_Start(void);
unsigned char SendToReaderCommand(unsigned char Command);
void SendToReaderNoAnswer(unsigned char Command);
void SetConfigPage(unsigned char Page, unsigned char Params);
void SendToReader_Read(void);
void SendToReader_Write(void);
void SendToReader_Stop(void);
void SendToReader(unsigned char Nomber);


void Init_Manchester(void);
void Init_Receiver(void);


#define T_poweUp 315
#define T_wait1 202
#define T_wait2 90
#define T_prog 614


extern volatile unsigned char State;

#define st_StartAuthent 1
#define st_ReceiveIdentifier 2
#define st_WaitReceiveIdentifier 3
#define st_OkReceiveIdentifier 4

#define st_TransmitPassword 10
#define st_WaitReceiveTAGPassword 11
#define st_OkReceiveTAGPassword 12

#define st_ReadPage 100
#define st_WaitReceiveData 101
#define st_OkReceiveData 102

#define st_WritePage 110
#define st_WaitReceiveAcknow 111
#define st_OkReceiveAcknow 112
#define st_WriteDaten 113

#define st_ReadPage2 150
#define st_WaitReceiveData2 151
#define st_OkReceiveData2 152

#define st_WaitTransmitRandom 21
#define st_TransmitRandom 22

#define st_WaitAnswer 201
#define st_OkReceiveAnswer 202
#define st_TimeOut 203

#define st_Wait 255

#define kt_FreeKey 0x00        //Чистый ключ
#define kt_WorkKey 0x01        //Рабочий ключ
#define kt_RedKey 0x02         //Красный ключ
#define kt_UniKey 0x07         //Универсальный ключ
#define kt_TestKey 0x08        //Тестовый ключ

#define dt_DeviceType 0x21     //Тип устройства для работы с ключами

#endif
