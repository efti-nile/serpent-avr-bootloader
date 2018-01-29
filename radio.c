// #include "global.h"
#include "radio.h"
#include "Ht2crypt.h"

unsigned char RedKey[4];              //Красный ключ
unsigned char KeysCount = 0;          //Кол-во рабочих ключей
unsigned char LearnKeysCount = 0;     //Кол-во ключей при обучении
unsigned char Keys[MaxKeysCount][4];  //Рабочие ключи
unsigned char TempKey[4];             //Временный ключ
volatile char BiteCounter;
volatile char Message[100];
volatile char AnswerLength = 0;
volatile unsigned char data_block[8];
volatile char State = 0;

void RFID_TIMER2_COMPA_ISR (void)  {
  if (State != st_WaitAnswer) {
    return;
  }

  asm("cli");

  if (BiteCounter == (AnswerLength - 1)) {
    if (CheckFlag(SecondFront)) {   //Если второй фронт с начали бита
      if (CheckFlag(CurentBit)) {   //Проверка флага что принимаем
        Message[BiteCounter] = 1;
        } else {
        Message[BiteCounter] = 0;
      }
    }

    BiteCounter++;
    PCICR &= ~(1 << PCIE0);
    State = st_OkReceiveAnswer; //Успешно приняли ответ
    TIMSK0 = 0;             //Остановка таймера для контроля длительности приема
    SendToReader_Stop();
    } else {
    Init_Manchester();  //Сброс алгоритма приема манчестера
  }

  asm("sei");
}

void RFID_TIMER1_COMPA_ISR (void) {
  if (Counter1++ > MaxCount1) {
    Counter1 = 0;

    if (MilSecCounter++ > MaxMilSecCounter) {
      MilSecCounter = 0;

      if (SecCounter++ > MaxSecCount) {
        SecCounter = 0;
      }
    }
  }
}

void RFID_TIMER0_COMPA_ISR (void) {
  TIMSK0 = 0; //Остановка таймера для контроля длительности приема
  State = st_TimeOut;
  SendToReader_Stop();            //Выключаем режим приема
}

void RFID_PCINT0_ISR (void) {
  if (State != st_WaitAnswer) {
    return;
  }

  asm("cli");
  char Count = TCNT2;
  TCNT2 = 0;

  if (!CheckFlag(StartMessage)) {
    SetFlag(StartMessage);  //Первый бит сообщения
    } else {    //Идет передача
    if (Count < 15) {
      Init_Manchester();  //Сброс алгоритма приема манчестера
      } else if (Count < 48) {
      if (CheckFlag(SecondFront)) { //Если второй фронт с начали бита
        //        Message[BiteCounter++] = CheckFlag(CurentBit);
        if (CheckFlag(CurentBit)) {   //Проверка флага что принимаем
          Message[BiteCounter] = 1;
          } else {
          Message[BiteCounter] = 0;
        }

        BiteCounter++;
        ClearFlag(SecondFront);   //Первый фронт за бит
        } else {
        SetFlag(SecondFront);  //Второй фронт за бит
      }
      } else if (Count < 80) {
      //      Message[BiteCounter++] = CheckFlag(CurentBit);
      //      Flags ^= CurentBit;              //Дергаем портом
      if (CheckFlag(CurentBit)) {   //Проверка флага что принимаем
        Message[BiteCounter] = 1;
        ClearFlag(CurentBit);
        } else {
        Message[BiteCounter] = 0;
        SetFlag(CurentBit);
      }

      BiteCounter++;
      SetFlag(SecondFront);
      } else {
      Init_Manchester();  //Сброс алгоритма приема манчестера
    }

    if (BiteCounter == AnswerLength) {     //Конец посылки
      PCICR &= ~(1 << PCIE0);
      State = st_OkReceiveAnswer; //Успешно приняли ответ
      TIMSK0 = 0;             //Остановка таймера для контроля длительности приема
      SendToReader_Stop();
    }

  }

  asm("sei");
}

void RFID_Disable(void) {
  SendToReaderNoAnswer(0x51);     //Page1  - отключили транспондер

  ClearFlag(AddWorkKey);
  ClearFlag(AddRedKey);
  ClearFlag(TransNo);
  // SetMonoFlag(RadioOff);

  // MonoState = mst_Wait;
}

void RFID_Enable(void) {
  OCR1A = OCR1A_RFID_max;
  SendToReaderNoAnswer(0x50);                   //Page1  - включили транспондер
}

void RFID_Init(void) {
  DDRB = 1 << SCK | 1 << MOSI;
  PCMSK0 |= 1 << PCINT4;  // Interrupt for Rx
  
  //Таймер 2 - прием - передача
  TIMSK2 = (1 << OCIE2A);
  TCCR2A = 0x00;
  TCCR2B = (1 << CS22);                     //Предделитель 64 - для частоты 16 Mhz
  OCR2A = 100;                              //Контроль стопа

  //Таймер 1 - звук и счетчики длительности и времени
  TIMSK1 = (1 << OCIE1A);
  TCCR1A = 0;
  TCCR1B = (1 << WGM12) | (1 << CS10);
  
  //Таймер 0 - общая длительность приема
  TIMSK0 = 0;
  OCR0A = 0;
  TCCR0B = (1 << CS02) | (1 << CS00);  // 1024 prescaler    

  asm("sei");
  InitRadio();
  OCR1A = OCR1A_RFID_max;
  RFID_Disable();
}

uint32_t RFID_GetRedKeyID(void) {
  char KeyType;

  if (!Send_StartAuthent()) {
    return(res_NoKey);  // No any transponder
  }

  ArrayToMem(Message, ident, 5, 32);
  data_block[0] = TCNT0;
  data_block[1] = TCNT1;
  data_block[2] = TCNT2;
  data_block[3] = Counter1;    //Ð“ÐµÐ½ÐµÑ€Ð°Ñ†Ð¸Ñ Ð¿ÑÐµÐ²Ð´Ð¾ ÑÐ»ÑƒÑ‡Ð°Ð¹Ð½Ð¾Ð³Ð¾ Ñ‡Ð¸ÑÐ»Ð°
  data_block[4] = 0xFF;
  data_block[5] = 0xFF;
  data_block[6] = 0xFF;
  data_block[7] = 0xFF;
  Oneway1(data_block);
  Oneway2(data_block + 4, 32);
  MemToArray(data_block, Message, 1, 8);

  if (!Send_Password()) {
    return(res_NoKey);  //ÐÐµÑ‚ Ð·Ð½Ð°ÐºÐ¾Ð¼Ð¾Ð³Ð¾ Ñ‚Ñ€Ð°Ð½ÑÐ¿Ð¾Ð½Ð´ÐµÑ€Ð°
  }

  ArrayToMem(Message, data_block, 5, 32);
  Oneway2(data_block, 32);

  if (!Send_ReadPage(6)) {
    return(res_NoKey);  //Ð§Ð¸Ñ‚Ð°ÐµÐ¼ ÑÑ‚Ñ€Ð°Ð½Ð¸Ñ†Ñƒ ÐºÐ¾Ð½Ñ„Ð¸Ð³ÑƒÑ€Ð°Ñ†Ð¸Ð¸
  }

  SecCounter = 0;

  ArrayToMem(Message, data_block, 5, 32);
  Oneway2(data_block, 32);
  KeyType = data_block[2];
  
  if (KeyType == kt_RedKey) {
    return (uint32_t)(*((uint32_t *)ident));
  } else {
    return 0;
  }
}

void memcpy_(unsigned char Target[], unsigned char const Source[], char Count) {
  for (unsigned char i = 0; i < Count; i++) {
    Target[i] = Source[i];
  }
}

char memcmp_(unsigned char const Target[], unsigned char const Source[], char Count) {
  for (unsigned char i = 0; i < Count; i++)
    if (Target[i] != Source[i]) {
      return 1;
    }
    
  return 0;
}


void MemToArray(unsigned char Mem[], char Array[], char First, char Count) {
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
void ArrayToMem(char Array[], unsigned char Mem[], char First, char Count) {
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
void ArrayToMemRevers(char Array[], unsigned char Mem[], char First, char Count) {
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
  char FilterH, FilterL;
  char Gain1, Gain0;
  char Hysteresis;


  
  FilterH = 1;
  FilterL = 1;
  Gain1 = 1;
  Gain0 = 0;
  Hysteresis = 0;

  

  SetConfigPage(0, (1 << (Gain1 * (3 - 8) + 8)) | (1 << (Gain0 * (2 - 8) + 8)) | (1 << (FilterH * (1 - 8) + 8)) | (1 << (FilterL * (0 - 8) + 8)));
  _delay_ms(1);
  SetConfigPage(1, (1 << (Hysteresis * (1 - 8) + 8)));
  _delay_ms(1);
  SetConfigPage(2, 0x00);
  _delay_ms(1);
  SetConfigPage(3, 0x03);
  _delay_ms(5);
  
  char dop = SendToReaderCommand(0x08);   //Читаем фазу
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


char Send_StartAuthent(void) {
  for (char i = 0; i < 3; i++) {
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
  Message[4] = 0;
  SendToReader_Write();                   //Переключаем на передачу
  SendToReader(5);                        //Посылка транспондеру 5 бит
  SendToReader_Stop();                    //Выключаем режим передачи
}

void Send_Stop(void) {
  _delay_ms(1);
  Send_Start();
  _delay_ms(1);
}

char Send_Password(void) {
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

char Send_ConfigPage() {      //В 6 страницу пишем конфигурацию ключа
  _delay_us(T_wait2 * 8);  //Ждем T_wait2
  unsigned char Command[2];
  Command[0] = 0xB2;
  Command[1] = 0x6C;
  
  memcpy_(data_block, Command, 2);
  Oneway2(data_block, 15);
  MemToArray(data_block, Message, 1, 2);
  
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
  ArrayToMem(Message, data_block, 5, 10);
  Oneway2(data_block, 10);
  memcpy_(Ans, data_block, 2);
  Ans[1] &= ~0x3F;                  //Очистка правых 6 бит
  Command[1] &= ~0x3F;              //Очистка правых 6 бит
  
  if (memcmp_(Ans, Command, 2) != 0) {
    return 0;
  }
  
  _delay_us(T_wait2 * 8);  //Ждем T_wait2
  data_block[0] = 0;
  data_block[1] = 0;
  data_block[2] = kt_WorkKey;
  data_block[3] = dt_DeviceType;
  Oneway2(data_block, 32);
  MemToArray(data_block, Message, 1, 4);
  
  SendToReader_Write();           //Переключаем на передачу
  SendToReader(32);               //Посылка транспондеру 32 бит
  SendToReader_Stop();            //Выключаем режим передачи
  
  FastSetPhase();
  
  _delay_us(T_prog * 8);  //Ждем T_prog
  _delay_us(T_wait2 * 8);  //Ждем T_wait2
  
  return 1;
}

char Send_WritePage(char Nomber, unsigned char TempPage[]) {
  _delay_us(T_wait2 * 8);  //Ждем T_wait2
  
  unsigned char Command[2];
  char TempByte;
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
  
  memcpy_(data_block, Command, 2);
  Oneway2(data_block, 15);
  MemToArray(data_block, Message, 1, 2);
  
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
  ArrayToMem(Message, data_block, 5, 10);
  Oneway2(data_block, 10);
  memcpy_(Ans, data_block, 2);
  Ans[1] &= ~0x3F;                  //Очистка правых 6 бит
  Command[1] &= ~0x3F;              //Очистка правых 6 бит
  
  if (memcmp_(Ans, Command, 2) != 0) {
    return 0;
  }
  
  _delay_us(T_wait2 * 8);  //Ждем T_wait2
  memcpy_(data_block, TempPage, 4);
  Oneway2(data_block, 32);
  MemToArray(data_block, Message, 1, 4);
  
  SendToReader_Write();           //Переключаем на передачу
  SendToReader(32);               //Посылка транспондеру 32 бит
  SendToReader_Stop();            //Выключаем режим передачи
  
  FastSetPhase();
  
  _delay_us(T_prog * 8);  //Ждем T_prog
  _delay_us(T_wait2 * 8);  //Ждем T_wait2
  
  return 1;
}


char Send_ReadPage(char Nomber) {
  _delay_us(T_wait2 * 8);  //Ждем T_wait2
  unsigned char Command[2];
  char TempByte;
  
  TempByte = Nomber << 3;
  TempByte |= (1 << 7) | (1 << 6);
  TempByte &= ~((1 << 2) | (1 << 1));
  TempByte |= (Nomber >> 2) ^ 0x01;
  Command[0] = TempByte;
  TempByte = (Nomber << 6) ^ 0xC0;
  TempByte |= (1 << 5) | (1 << 4);
  TempByte |= (Nomber << 1);
  Command[1] = TempByte;
  
  memcpy_(data_block, Command, 2);
  Oneway2(data_block, 15);
  MemToArray(data_block, Message, 1, 2);
  
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

char SendToReaderCommand(char Command) {
  SendToReaderNoAnswer(Command);
  
  DDRB &= ~(1 << MISO);         //MISO вход
  char i = 0x80;
  char Answer = 0;
  
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

void SendToReaderNoAnswer(char Command) {
  SendToReader_Start();      //Старт
  char i = 0x80;
  
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

void SetConfigPage(char Page, char Params) {
  SendToReader_Start();      //Старт
  char Command = 0;
  Command = Params & 0x0F;
  Command += (Page << 4);
  Command |= (1 << 6);
  Command &= ~(1 << 7);
  
  char i = 0x80;
  
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

void SendToReader(char Nomber) {
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
  SetFlag(CurentBit);
  ClearFlag(SecondFront);
  ClearFlag(StartMessage);
  BiteCounter = 0;
}

void Init_Receiver(void) {
  PCIFR = 0;
  PCICR |= (1 << PCIE0);
  TCNT2 = 0;
  Init_Manchester();
}






