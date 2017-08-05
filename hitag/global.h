#ifndef GLOBAL_H
#define GLOBAL_H

#include <avr/io.h>

#define LightPort PORTC
#define LightPin PC4

#define OutPort PORTC
//#define OutPin1 PC0               //��� 10 ������   �������� "Helo"
#define OutPin1 PC2               //��� 12 ������ �������� "Helo"
#define OutPin2 PC3               //������ ������

#define MotorPort PORTC
#define MotorPin1 PC1
//#define MotorPin2 PC2           //��� 10 ������
#define MotorPin2 PC0             //��� 12 ������

#define SoundPort PORTC
#define SoundPin PC5
#define SoundDelay 8

#define InPort PIND
#define PowerPin PIND6       //���� ���������
#define ButtonPin PIND5      //���� ��� ������ ��������� �������
#define ClosePin PIND7       //���� ��� �������� �� ������� ������� ������������
#define OpenPin PIND4        //���� ��� �������� �� ������� ������� ������������

#define InPortB PINB
#define ParkPin PINB1        //���� ��� ��������� "Park"
#define FreePin PINB2        //�������� ����

#define ControlPort PIND
//#define ControlPin1 PIND1       //��� 10 ������
//#define ControlPin2 PIND2       //��� 10 ������
#define ControlPin1 PIND2         //��� 12 ������
#define ControlPin2 PIND1         //��� 12 ������

#define Drebezg_ms 30
#define MotorTime 2

#define BitTest(x, y) ((x) & (1<<(y)))   //�������� ���� y � ����� x
#define BitOn(x, y) ((x) |= (1<<(y)))    //��������� ���� y � ����� x
#define BitOff(x, y) ((x) &= ~(1<<(y)))  //����� ���� y � ����� x

#define TestPin(ADDRESS,BIT) ((ADDRESS) & (1<<(BIT)))
#define TestPinInv(ADDRESS,BIT) (!((ADDRESS) & (1<<(BIT))))
#define PinOn(ADDRESS,BIT) ((ADDRESS) |= (1<<(BIT)))
#define PinOff(ADDRESS,BIT) ((ADDRESS) &= ~(1<<(BIT)))


void __EEWrite(char ADR, char VAL);
char __EERead(char ADR);
void __EEWrite16(unsigned int ADR, unsigned int VAL);
unsigned int __EERead16(unsigned int ADR);

char GetCRC8(unsigned char Source);
unsigned int GetCRC16(unsigned char *Source, unsigned char Count);

#define PageLength 128
#define PageLengthWord 64
extern unsigned char FlashPage[PageLength];         //����� ��� �������� ������

//#define MaxMessageLength 19                          //������������ ������ ��������� (12+7 ����)
#define MaxMessageLength 27                          //������������ ������ ��������� (20+7 ����)
extern unsigned char Message[MaxMessageLength];      //���������
extern unsigned char RezervMessage[MaxMessageLength];   //����� ��� ������� �������� ��� �������
extern unsigned char RezervMessageLength;              //������� ����� �������� ��� �������



unsigned int ReadFlashWord(unsigned int Address);
void ReadFlashPage(unsigned int Address);
void WriteFlashPage(unsigned int Address);



#endif
