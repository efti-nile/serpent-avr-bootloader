#ifndef RADIO_H
#define RADIO_H

#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define TestPin(ADDRESS,BIT) ((ADDRESS) & (1<<(BIT)))
#define BitTest(x, y) ((x) & (1<<(y)))   //Проверка бита y в байте x
#define BitOn(x, y) ((x) |= (1<<(y)))    //Установка бита y в байте x


#define MaxKeysCount 3                //Максимальное кол-во рабочих ключей


#define OCR1A_RFID_max 1599

//Счетчики времени
volatile unsigned char Counter1;
#define MaxCount1 100
volatile unsigned char MilSecCounter;
#define MaxMilSecCounter 100
volatile unsigned char SecCounter;    //Счетчик секунд
#define MaxSecCount 60
volatile unsigned char MinCounter;    //Счетчик минут
#define MaxMinCount 60

#define MaxKeyCounter 3

volatile unsigned char Flags;
#define StartMessage 0x01         //0 бит -
#define SecondFront 0x02          //1 бит -
#define CurentBit 0x04            //2 бит -
#define AddRedKey 0x08            //3 бит - Ждем добавления красного ключа
#define AddWorkKey 0x10           //4 бит - Ждем добавления рабочего ключа
#define NoRedKey 0x20             //5 бит - Нет красного ключа
#define NoKey 0x40                //6 бит - Нет рабочего ключа
#define TransNo 0x80              //7 бит - Пауза без транспондера

#define SetFlag(y) (Flags |= (y))     //Установка бита y в байте Flags
#define ClearFlag(y) (Flags &= (~y))  //Сброс бита y в байте Flags
#define CheckFlag(y) (Flags & (y))    //Проверка бита y в байте Flags

// GetTransponderansponder return values
#define res_NoKey 0x00
#define res_UnknownKey 0x01
#define res_WorkKey 0x02
#define res_RedKey 0x03
#define res_MasterKey 0x04
#define res_FreeKey 0x05
#define res_TestKey 0x06
#define res_NeedStop 0x09

#define res_InvClose 0x0A
#define res_InvOpen 0x0B
#define res_InvPark 0x0C

#define res_UseClose 0x1A
#define res_UseOpen 0x1B
#define res_UsePark 0x1C

#define res_Drebezg 0x2A

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

extern volatile char BiteCounter;
extern volatile char Message[100];
extern volatile char AnswerLength;
extern volatile unsigned char data_block[8];

//unsigned char const My_ISK[6]=                 {0x4E, 0x80, 0x92, 0x41, 0x4C, 0x69};
//unsigned char const My_Defaul_Base_Password[4]={0x4E, 0x80, 0x92, 0x41}; //Defaul Read-write device password
//*unsigned char const My_Trans_Password[3]=      {0x1D, 0x0C, 0x07};
//unsigned char Random[4]=                 {0x5A, 0xAB, 0xDC, 0x0F}; // Fictive random number

void memcpy_(unsigned char Target[], unsigned char const Source[], char Count) __attribute__((section (".RFID")));
char memcmp_(unsigned char const Target[], unsigned char const Source[], char Count) __attribute__((section (".RFID")));
void MemToArray(unsigned char Mem[], char Array[], char First, char Count) __attribute__((section (".RFID")));
void ArrayToMem(char Array[], unsigned char Mem[], char First, char Count) __attribute__((section (".RFID")));
void ArrayToMemRevers(char Array[], unsigned char Mem[], char First, char Count) __attribute__((section (".RFID")));

uint32_t RFID_GetRedKeyID(void) __attribute__((section (".RFID")));

void RFID_Disable(void) __attribute__((section (".RFID")));
void RFID_Enable(void) __attribute__((section (".RFID")));
void RFID_Init(void) __attribute__((section (".RFID")));
void InitRadio(void) __attribute__((section (".RFID")));
char Send_StartAuthent(void) __attribute__((section (".RFID")));
void SetPhase(void) __attribute__((section (".RFID")));
void FastSetPhase(void) __attribute__((section (".RFID")));
void Send_Start(void) __attribute__((section (".RFID")));
void Send_Stop(void) __attribute__((section (".RFID")));
char Send_Password(void) __attribute__((section (".RFID")));
char Send_ConfigPage() __attribute__((section (".RFID")));
char Send_WritePage(char Nomber, unsigned char TempPage[]) __attribute__((section (".RFID")));
char Send_ReadPage(char Nomber) __attribute__((section (".RFID")));

void RFID_TIMER2_COMPA_ISR (void) __attribute__((section (".RFID")));
void RFID_TIMER1_COMPA_ISR (void) __attribute__((section (".RFID")));
void RFID_TIMER0_COMPA_ISR (void) __attribute__((section (".RFID")));
void RFID_PCINT0_ISR (void) __attribute__((section (".RFID")));

void SendToReader_Start(void) __attribute__((section (".RFID")));
char SendToReaderCommand(char Command) __attribute__((section (".RFID")));
void SendToReaderNoAnswer(char Command) __attribute__((section (".RFID")));
void SetConfigPage(char Page, char Params) __attribute__((section (".RFID")));
void SendToReader_Read(void) __attribute__((section (".RFID")));
void SendToReader_Write(void) __attribute__((section (".RFID")));
void SendToReader_Stop(void) __attribute__((section (".RFID")));
void SendToReader(char Nomber) __attribute__((section (".RFID")));


void Init_Manchester(void) __attribute__((section (".RFID")));
void Init_Receiver(void) __attribute__((section (".RFID")));


#define T_poweUp 315
#define T_wait1 202
#define T_wait2 90
#define T_prog 614


extern volatile char State;

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
