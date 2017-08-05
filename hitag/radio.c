#include "global.h"
#include "radio.h"
#include "Ht2crypt.h"

unsigned char BiteCounter;
unsigned char BitMessage[100];
unsigned char AnswerLength = 0;
unsigned char data_block[8];
volatile unsigned char State = 0;
unsigned char Message[27];

void MemToArray(unsigned char Mem[],
                unsigned char Array[],
                unsigned char First,
                unsigned char Count) {
  unsigned char j = 0;
  unsigned char k = First - 1;
  unsigned char i;

  for (i = (First - 1) * 8; i < (First + Count - 1) * 8; i++) {
    if (BitTest(Mem[k], (7 - j))) {
      Array[i] = 1;
    } else {
      Array[i] = 0;
    }

    if (++j == 8) {
      j = 0;
      k++;
    }
  }
}

//Копирует Count битов из битового массива Array в массив байтов Mem, начиная с бита First
void ArrayToMem(unsigned char Array[],
                unsigned char Mem[],
                unsigned char First,
                unsigned char Count) {
  unsigned char TempByte = 0;
  unsigned char j = 0;
  unsigned char k = 0;
  unsigned char i;

  for (i = 0; i < Count; i++) {
    if (Array[First + i] == 1) {
      BitOn(TempByte, (7 - j));
    }

    if (++j == 8) {
      Mem[k++] = TempByte;
      TempByte = 0;
      j = 0;
    }
  }

  if (j != 0) {
    Mem[k] = TempByte;
  }
}

//Копирует Count битов из битового массива Array в массив байтов Mem, начиная с бита First, в обратном порядке
void ArrayToMemRevers(unsigned char Array[],
                      unsigned char Mem[],
                      unsigned char First,
                      unsigned char Count) {
  unsigned char TempByte = 0;
  unsigned char j = 0;
  unsigned char k = 0;
  unsigned char i;

  for (i = Count; i > 0; i--) {
    if (Array[First + i - 1] == 1) {
      BitOn(TempByte, (7 - j));
    }

    if (++j == 8) {
      Mem[k++] = TempByte;
      TempByte = 0;
      j = 0;
    }
  }

  if (j != 0) {
    Mem[k] = TempByte;
  }
}

void InitRadio(void) {
  unsigned char Offset = 0x3F;
  unsigned char FilterH, FilterL;
  unsigned char Gain1, Gain0;
  unsigned char Hysteresis;
  //  unsigned char DisLp1, DisSmart;

  FilterH = 1;
  FilterL = 1;
  Gain1 = 1;
  Gain0 = 0;
  Hysteresis = 0;
  //  DisLp1 = 0, DisSmart = 0;

  //  unsigned char Dop = 0;
  //  Dop = (1<<(Gain1*(3-8)+8)) | (1<<(Gain0*(2-8)+8)) | (1<<(FilterH*(1-8)+8)) | (1<<(FilterL*(0-8)+8));
  SetConfigPage(0, (1 << (Gain1 * (3 - 8) + 8)) | (1 << (Gain0 * (2 - 8) + 8)) | (1 << (FilterH * (1 - 8) + 8)) | (1 << (FilterL * (0 - 8) + 8)));
  _delay_ms(1);
  SetConfigPage(1, (1 << (Hysteresis * (1 - 8) + 8)));
  _delay_ms(1);
  SetConfigPage(2, 0x00);
  _delay_ms(1);
  SetConfigPage(3, 0x03);
  //  SetConfigPage(3, (1<<(DisLp1*(3-8)+8)) | (1<<(DisSmart*(2-8)+8)) | (1<<(1*(1-8)+8)) | (1<<(1*(0-8)+8)));
  _delay_ms(5);

  unsigned char dop = SendToReaderCommand(0x08);   //Читаем фазу
  dop <<= 1;                              //Умножаем на 2
  dop += Offset;                          //Добавляем задержку
  dop |= (1 << 7);
  dop &= ~(1 << 6);        //Готовим команду
  SendToReaderNoAnswer(dop);              //SetSampleTime
  _delay_ms(5);

  SetConfigPage(2, 0x0B);
  _delay_ms(5);
  SetConfigPage(2, 0x08);
  _delay_ms(1);
  SetConfigPage(2, 0x00);
  _delay_ms(1);
}

void FastSetPhase(void) {             //Быстрая настройка приемника
  _delay_us(320);
  SetConfigPage(2, 0x0A); //  SendToReaderNoAnswer(0x6A);
  _delay_us(400);
  SetConfigPage(2, 0x0B); //  SendToReaderNoAnswer(0x6B);
  _delay_us(320);
  SetConfigPage(2, 0x00); //  SendToReaderNoAnswer(0x60);
}

void SetPhase(void) {                 //Настройка приемника
}


unsigned char Send_StartAuthent(void) {
  for (unsigned char i = 0; i < 3; i++) {
    Send_Start();               //Отправка 11000
    TCNT = 0;
    OCRA = Delay_32bit;
    TIFR = (1 << OCFA);
    TIMSK = (1 << OCIEA); //Запуск таймера для контроля длительности приема

    FastSetPhase();

    SendToReader_Read();        //Переключаем на прием

    Init_Receiver();            //Сброс приемника в начальное состояние
    AnswerLength = 37;
    State = st_WaitAnswer;
    do {} while (State == st_WaitAnswer);

    _delay_us(10 * 8);

    if (State == st_OkReceiveAnswer) {
      SetConfigPage(2, 0x09);
      return 1;
    }
  }

  return 0;
}

void Send_Start(void) {
  Message[0] = 1;
  Message[1] = 1;
  Message[2] = 0;
  Message[3] = 0;
  Message[4] = 0;//      MemToArray(0xC0, Message, 1, 8);
  SendToReader_Write();                   //Переключаем на передачу
  SendToReader(5);                        //Посылка транспондеру 5 бит
  SendToReader_Stop();                    //Выключаем режим передачи
}

void Send_Stop(void) {
  _delay_ms(1);
  Send_Start();
  _delay_ms(1);
}

unsigned char Send_Password(void) {
  _delay_us(T_wait2 * 8);  //Ждем T_wait2
  SendToReader_Write();           //Переключаем на передачу
  SendToReader(64);             //Посылка транспондеру 64 бит криптованной посылки
  //  SendToReader(32);               //Посылка транспондеру 32 бит пароля станции
  SendToReader_Stop();            //Выключаем режим передачи

  TCNT = 0;
  OCRA = Delay_32bit;
  TIFR = (1 << OCFA);
  TIMSK = (1 << OCIEA); //Запуск таймера для контроля длительности приема

  FastSetPhase();
  SendToReader_Read();        //Переключаем на прием
  Init_Receiver();            //Сброс приемника в начальное состояние
  AnswerLength = 37;
  State = st_WaitAnswer;
  do {} while (State == st_WaitAnswer);

  _delay_us(10 * 8);
  SetConfigPage(2, 0x09); //  SendToReaderNoAnswer(0x69);

  if (State == st_OkReceiveAnswer) {
    return 1;
  } else {
    return 0;
  }
}

unsigned char Send_ConfigPage() {      //В 6 страницу пишем конфигурацию ключа
  _delay_us(T_wait2 * 8);  //Ждем T_wait2
  unsigned char Command[2];
  Command[0] = 0xB2;
  Command[1] = 0x6C;

  memcpy(data_block, Command, 2);
  Oneway2(data_block, 15);
  MemToArray(data_block, BitMessage, 1, 2);

  SendToReader_Write();           //Переключаем на передачу
  SendToReader(15);               //Посылка транспондеру 15 бит
  SendToReader_Stop();            //Выключаем режим передачи

  //  ClearMessage();
  TCNT = 0;
  OCRA = Delay_16bit;
  TIFR = (1 << OCFA);
  TIMSK = (1 << OCIEA); //Запуск таймера для контроля длительности приема
  FastSetPhase();
  SendToReader_Read();        //Переключаем на прием
  Init_Receiver();            //Сброс приемника в начальное состояние
  AnswerLength = 15;
  State = st_WaitAnswer;
  do {} while (State == st_WaitAnswer);

  if (State != st_OkReceiveAnswer) {
    return 0;
  }

  _delay_us(10 * 8);
  SetConfigPage(2, 0x09); //  SendToReaderNoAnswer(0x69);

  //Проверка ответа
  unsigned char Ans[2];
  ArrayToMem(BitMessage, data_block, 5, 10);
  Oneway2(data_block, 10);
  memcpy(Ans, data_block, 2);
  Ans[1] &= ~0x3F;                  //Очистка правых 6 бит
  Command[1] &= ~0x3F;              //Очистка правых 6 бит

  if (memcmp(Ans, Command, 2) != 0) {
    return 0;
  }

  _delay_us(T_wait2 * 8);  //Ждем T_wait2
  data_block[0] = 0;
  data_block[1] = 0;
  data_block[2] = kt_WorkKey;
  data_block[3] = dt_DeviceType;
  Oneway2(data_block, 32);
  MemToArray(data_block, BitMessage, 1, 4);

  SendToReader_Write();           //Переключаем на передачу
  SendToReader(32);               //Посылка транспондеру 32 бит
  SendToReader_Stop();            //Выключаем режим передачи

  FastSetPhase();

  _delay_us(T_prog * 8);  //Ждем T_prog
  _delay_us(T_wait2 * 8);  //Ждем T_wait2

  return 1;
}

unsigned char Send_WritePage(unsigned char Nomber, unsigned char TempPage[]) {
  _delay_us(T_wait2 * 8);  //Ждем T_wait2

  unsigned char Command[2];
  unsigned char TempByte;
  TempByte = Nomber << 3;
  TempByte |= (1 << 7) | (1 << 1);
  TempByte &= ~((1 << 6) | (1 << 2));
  TempByte |= (Nomber >> 2) ^ 0x01;
  Command[0] = TempByte;
  TempByte = (Nomber << 6) ^ 0xC0;
  TempByte |= (1 << 5);
  TempByte &= ~(1 << 4);
  TempByte |= (Nomber << 1);
  Command[1] = TempByte;

  memcpy(data_block, Command, 2);
  Oneway2(data_block, 15);
  MemToArray(data_block, BitMessage, 1, 2);

  SendToReader_Write();           //Переключаем на передачу
  SendToReader(15);               //Посылка транспондеру 15 бит
  SendToReader_Stop();            //Выключаем режим передачи

  //  ClearMessage();
  TCNT = 0;
  OCRA = Delay_16bit;
  TIFR = (1 << OCFA);
  TIMSK = (1 << OCIEA); //Запуск таймера для контроля длительности приема
  FastSetPhase();
  SendToReader_Read();        //Переключаем на прием
  Init_Receiver();            //Сброс приемника в начальное состояние
  AnswerLength = 15;
  State = st_WaitAnswer;
  do {} while (State == st_WaitAnswer);

  if (State != st_OkReceiveAnswer) {
    return 0;
  }

  _delay_us(10 * 8);
  SetConfigPage(2, 0x09); //  SendToReaderNoAnswer(0x69);

  //Проверка ответа
  unsigned char Ans[2];
  ArrayToMem(BitMessage, data_block, 5, 10);
  Oneway2(data_block, 10);
  memcpy(Ans, data_block, 2);
  Ans[1] &= ~0x3F;                  //Очистка правых 6 бит
  Command[1] &= ~0x3F;              //Очистка правых 6 бит

  if (memcmp(Ans, Command, 2) != 0) {
    return 0;
  }

  _delay_us(T_wait2 * 8);  //Ждем T_wait2
  memcpy(data_block, TempPage, 4);
  Oneway2(data_block, 32);
  MemToArray(data_block, BitMessage, 1, 4);

  SendToReader_Write();           //Переключаем на передачу
  SendToReader(32);               //Посылка транспондеру 32 бит
  SendToReader_Stop();            //Выключаем режим передачи

  FastSetPhase();

  _delay_us(T_prog * 8);  //Ждем T_prog
  _delay_us(T_wait2 * 8);  //Ждем T_wait2

  return 1;
}


unsigned char Send_ReadPage(unsigned char Nomber) {
  _delay_us(T_wait2 * 8);  //Ждем T_wait2
  unsigned char Command[2];
  unsigned char TempByte;

  TempByte = Nomber << 3;
  TempByte |= (1 << 7) | (1 << 6);
  TempByte &= ~((1 << 2) | (1 << 1));
  TempByte |= (Nomber >> 2) ^ 0x01;
  Command[0] = TempByte;
  TempByte = (Nomber << 6) ^ 0xC0;
  TempByte |= (1 << 5) | (1 << 4);
  TempByte |= (Nomber << 1);
  Command[1] = TempByte;

  memcpy(data_block, Command, 2);
  Oneway2(data_block, 15);
  MemToArray(data_block, BitMessage, 1, 2);

  SendToReader_Write();           //Переключаем на передачу
  SendToReader(15);               //Посылка транспондеру 15 бит
  SendToReader_Stop();            //Выключаем режим передачи

  TCNT = 0;
  OCRA = Delay_32bit;
  TIFR = (1 << OCFA);
  TIMSK = (1 << OCIEA); //Запуск таймера для контроля длительности приема
  FastSetPhase();
  SendToReader_Read();        //Переключаем на прием
  Init_Receiver();            //Сброс приемника в начальное состояние
  AnswerLength = 37;
  State = st_WaitAnswer;
  do {} while (State == st_WaitAnswer);

  if (State != st_OkReceiveAnswer) {
    return 0;
  }

  _delay_us(T_wait2 * 8);  //Ждем T_wait2
  return 1;
}

//***************************************************
//************Настройка базовой станции**************
//***************************************************

void SendToReader_Start(void) {
  DDRB |= (1 << MOSI);          //MOSI - выход

  _delay_us(SPI_QartStrobe);
  PORTB |= (1 << SCK);

  if (TestPin(PORTB, MOSI)) {
    PORTB &= ~(1 << MOSI);
  }

  _delay_us(SPI_HalfStrobe);
  PORTB |= (1 << MOSI);
  _delay_us(SPI_HalfStrobe);
  PORTB &= ~(1 << SCK);

  _delay_us(SPI_QartStrobe);
}

unsigned char SendToReaderCommand(unsigned char Command) {
  SendToReaderNoAnswer(Command);

  DDRB &= ~(1 << MISO);         //MISO вход
  unsigned char i = 0x80;
  unsigned char Answer = 0;

  do { //Читаем результат
    _delay_us(SPI_QartStrobe);
    PORTB |= (1 << SCK);
    _delay_us(SPI_HalfStrobe);

    if (TestPin(PINB, MISO)) {
      Answer |= i;
    }

    PORTB &= ~(1 << SCK);
    _delay_us(SPI_QartStrobe);
    i >>= 1;
  } while (i > 0);

  return(Answer);
}

void SendToReaderNoAnswer(unsigned char Command) {
  SendToReader_Start();      //Старт
  unsigned char i = 0x80;

  do { //Шлем команду
    if (Command & i) {
      PORTB |= (1 << MOSI);
    } else {
      PORTB &= ~(1 << MOSI);
    }

    _delay_us(SPI_QartStrobe);
    PORTB |= (1 << SCK);
    _delay_us(SPI_HalfStrobe);
    PORTB &= ~(1 << SCK);
    _delay_us(SPI_QartStrobe);

    i >>= 1;
  } while (i > 0);
}

void SetConfigPage(unsigned char Page, unsigned char Params) {
  SendToReader_Start();      //Старт
  unsigned char Command = 0;
  Command = Params & 0x0F;
  Command += (Page << 4);
  Command |= (1 << 6);
  Command &= ~(1 << 7);

  unsigned char i = 0x80;

  do { //Шлем команду
    if (Command & i) {
      PORTB |= (1 << MOSI);
    } else {
      PORTB &= ~(1 << MOSI);
    }

    _delay_us(SPI_QartStrobe);
    PORTB |= (1 << SCK);
    _delay_us(SPI_HalfStrobe);
    PORTB &= ~(1 << SCK);
    _delay_us(SPI_QartStrobe);

    i >>= 1;
  } while (i > 0);
}

void SendToReader_Read(void) {  // 111
  SendToReader_Start();      //Старт
  PORTB |= (1 << MOSI);

  _delay_us(SPI_QartStrobe);
  PORTB |= (1 << SCK);             //1
  _delay_us(SPI_HalfStrobe);
  PORTB &= ~(1 << SCK);
  _delay_us(SPI_HalfStrobe);

  PORTB |= (1 << SCK);             //1
  _delay_us(SPI_HalfStrobe);
  PORTB &= ~(1 << SCK);
  _delay_us(SPI_HalfStrobe);

  PORTB |= (1 << SCK);             //1
  _delay_us(SPI_HalfStrobe);
  PORTB &= ~(1 << SCK);
  _delay_us(SPI_QartStrobe);
}

void SendToReader_Write(void) { //110
  SendToReader_Start();      //Старт
  PORTB |= (1 << MOSI);

  _delay_us(SPI_QartStrobe);
  PORTB |= (1 << SCK);             //1
  _delay_us(SPI_HalfStrobe);
  PORTB &= ~(1 << SCK);
  _delay_us(SPI_HalfStrobe);

  PORTB |= (1 << SCK);             //1
  _delay_us(SPI_HalfStrobe);
  PORTB &= ~(1 << SCK);
  _delay_us(SPI_QartStrobe);

  PORTB &= ~(1 << MOSI);

  _delay_us(SPI_QartStrobe);
  PORTB |= (1 << SCK);             //0
  _delay_us(SPI_HalfStrobe);
  PORTB &= ~(1 << SCK);
  _delay_us(SPI_QartStrobe);
}

void SendToReader_Stop(void) {
  PORTB &= ~(1 << SCK); //SCK
  _delay_us(SPI_HalfStrobe);
  PORTB |= (1 << SCK);  //SCK
  _delay_us(SPI_HalfStrobe);
  PORTB &= ~(1 << SCK); //SCK

  PORTB |= (1 << MOSI);  //MOSI
}

void SendToReader(unsigned char Nomber) {
#define t_low 7
#define T_1 29
#define T_0 20
#define t_stop 40
  unsigned char i;

  for (i = 0; i < Nomber; i++) {
    PORTB ^= (1 << MOSI);             //Дергаем портом MOSI
    _delay_us(t_low * 8);              //Ждем время t_low
    PORTB ^= (1 << MOSI);             //Дергаем портом MOSI

    if (Message[i] == 1) {
      _delay_us((T_1 - t_low) * 8);  //Ждем время до полного T_1
    } else {
      _delay_us((T_0 - t_low) * 8);  //Ждем время до полного T_0
    }
  }

  //Фронт после передачи сообщения
  PORTB ^= (1 << MOSI);               //Дергаем портом MOSI
  _delay_us(t_low * 8);                //Ждем время t_low
  PORTB ^= (1 << MOSI);               //Дергаем портом MOSI
}

void Init_Manchester(void) {
  //**  SetFlag(CurentBit);
  //**  ClearFlag(SecondFront);
  //**  ClearFlag(StartMessage);
  BiteCounter = 0;
}

void Init_Receiver(void) {
  PCIFR = 0;
  PCICR |= (1 << PCIE0);
  TCNT2 = 0;
  Init_Manchester();
}
