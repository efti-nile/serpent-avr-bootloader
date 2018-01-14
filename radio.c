#include "global.h"
#include "radio.h"
#include "Ht2crypt.h"

char BiteCounter;
char Message[100];
char AnswerLength = 0;
unsigned char data_block[8];
volatile char State = 0;

void PCF_GPIO_init(void) {
  
}

unsigned char GetTransponder(void) {
  char i, j, KeyType, DeviceType;

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
  //*    if (!Send_ReadPage(3)) return(res_UnknownKey);               //Ð§Ð¸Ñ‚Ð°ÐµÐ¼ Ð¿Ð°Ñ€Ð¾Ð»ÑŒ Ñ‚Ñ€Ð°Ð½ÑÐ¿Ð¾Ð½Ð´ÐµÑ€Ð°
  //*    ArrayToMem(Message, data_block, 5, 32);
  //*    Oneway2(data_block, 32);
  //*    if (memcmp(data_block+1, My_Trans_Password, 3) != 0) return(res_UnknownKey);   //ÐÐµ ÑÐ¾Ð²Ð¿Ð°Ð» Ð¿Ð°Ñ€Ð¾Ð»ÑŒ


  if (!Send_ReadPage(6)) {
    return(res_NoKey);  //Ð§Ð¸Ñ‚Ð°ÐµÐ¼ ÑÑ‚Ñ€Ð°Ð½Ð¸Ñ†Ñƒ ÐºÐ¾Ð½Ñ„Ð¸Ð³ÑƒÑ€Ð°Ñ†Ð¸Ð¸
  }

  SecCounter = 0;

  /*
  for (i=0;i<4;i++)
    __EEWrite(i, ident[i]);       //Ð¡Ð¾Ñ…Ñ€Ð°Ð½ÑÐµÐ¼ ÐºÐ»ÑŽÑ‡
  Beep(1);
  return(res_NoKey);
  */

  ArrayToMem(Message, data_block, 5, 32);
  Oneway2(data_block, 32);
  KeyType = data_block[2];
  DeviceType = data_block[3];
  //  if (!Send_ReadPage(7)) return(res_NoKey);                   //Ð§Ð¸Ñ‚Ð°ÐµÐ¼ ÑÑ‚Ñ€Ð°Ð½Ð¸Ñ†Ñƒ Ñ ÑÐµÑ€Ð¸Ð¹Ð½Ñ‹Ð¼ Ð½Ð¾Ð¼ÐµÑ€Ð¾Ð¼ - Ð´Ð¾Ð»Ð¶ÐµÐ½ Ð±Ñ‹Ñ‚ÑŒ Ð½Ð¾Ð¼ÐµÑ€ ÐºÑ€Ð°ÑÐ½Ð¾Ð³Ð¾ ÐºÐ»ÑŽÑ‡Ð°
  //  ArrayToMem(Message, data_block, 5, 32);
  //  Oneway2(data_block, 32);

  //  KeyType = kt_WorkKey;
  if (KeyType == kt_TestKey) {
    return(res_TestKey);  //Ð¢ÐµÑÑ‚Ð¾Ð²Ñ‹Ð¹ ÐºÐ»ÑŽÑ‡
  }

  if (KeyType == kt_UniKey) {
    //    if (!memcmp(ident, KeyDrebezg, 4)) return(res_Drebezg);
    return(res_MasterKey);            //Ð£Ð½Ð¸Ð²ÐµÑ€ÑÐ°Ð»ÑŒÐ½Ñ‹Ð¹ ÐºÐ»ÑŽÑ‡
  }

  //  if (DeviceType != dt_DeviceType) return(res_UnknownKey);    //ÐšÐ»ÑŽÑ‡ Ð¾Ñ‚ Ð´Ñ€ÑƒÐ³Ð¾Ð³Ð¾ ÑƒÑÑ‚Ñ€Ð¾Ð¹ÑÑ‚Ð²Ð°
  if (KeyType == kt_RedKey) {

#ifndef SKIP_RED_KEY_CHECK
    if (memcmp(ident, RedKey, 4)) {
      return(res_UnknownKey);  //ÐšÐ»ÑŽÑ‡ Ð¾Ñ‚ Ð´Ñ€ÑƒÐ³Ð¾Ð³Ð¾ ÑƒÑÑ‚Ñ€Ð¾Ð¹ÑÑ‚Ð²Ð°
    }
#endif

    return(res_RedKey);               //ÐšÑ€Ð°ÑÐ½Ñ‹Ð¹ ÐºÐ»ÑŽÑ‡
  }

  if (CheckFlag(AddWorkKey) && CheckFlag(TransNo)) {          //Ð ÐµÐ¶Ð¸Ð¼ Ð¾Ð±ÑƒÑ‡ÐµÐ½Ð¸Ñ ÐºÐ»ÑŽÑ‡ÐµÐ¹
    __watchdog_reset();

    if ((KeyType == kt_FreeKey) || (KeyType == kt_WorkKey)) { //ÐŸÐ¾Ð´Ñ…Ð¾Ð´ÑÑ‰Ð¸Ð¹ Ð´Ð»Ñ Ð¾Ð±ÑƒÑ‡ÐµÐ½Ð¸Ñ ÐºÐ»ÑŽÑ‡ÐµÐ¹
      if (LearnKeysCount >= MaxKeyCounter) {
        return(res_UnknownKey);  //Ð¡Ð»Ð¸ÑˆÐºÐ¾Ð¼ Ð¼Ð½Ð¾Ð³Ð¾ ÐºÐ»ÑŽÑ‡ÐµÐ¹
      }

      for (j = 0; j < LearnKeysCount; j++)                    //ÐŸÑ€Ð¾Ð²ÐµÑ€ÐºÐ° Ð½Ðµ Ð¿Ð¾Ð´Ð½Ð¾ÑÐ¸Ð»Ð¸ Ð»Ð¸ ÑƒÐ¶Ðµ ÐºÐ»ÑŽÑ‡
        for (i = 0; i < 4; i++) {
          TempKey[i] = __EERead(0x30 + (4 * j) + i);        //Ð§Ð¸Ñ‚Ð°ÐµÐ¼ ÐºÐ»ÑŽÑ‡ Ð¸Ð· Ð²Ñ€ÐµÐ¼ÐµÐ½Ð½Ð¾Ð³Ð¾ Ñ…Ñ€Ð°Ð½Ð¸Ð»Ð¸Ñ‰Ð°

          if (!memcmp(ident, TempKey, 4)) {
            return(res_UnknownKey);  //ÐšÐ»ÑŽÑ‡ ÑƒÐ¶Ðµ Ð¿Ð¾Ð´Ð½Ð¾ÑÐ¸Ð»Ð¸
          }
        }

      if (!Send_WritePage(7, RedKey)) {
        return(res_UnknownKey);  //Ð’ 7 ÑÑ‚Ñ€Ð°Ð½Ð¸Ñ†Ñƒ Ð¿Ð¸ÑˆÐµÐ¼ ÑÐµÑ€Ð¸Ð¹Ð½Ñ‹Ð¹ Ð½Ð¾Ð¼ÐµÑ€ - Ð½Ð¾Ð¼ÐµÑ€ ÐºÑ€Ð°ÑÐ½Ð¾Ð³Ð¾ ÐºÐ»ÑŽÑ‡Ð°
      }

      if (!Send_ConfigPage()) {
        return(res_UnknownKey);  //Ð’ 6 ÑÑ‚Ñ€Ð°Ð½Ð¸Ñ†Ñƒ Ð¿Ð¸ÑˆÐµÐ¼ ÐºÐ¾Ð½Ñ„Ð¸Ð³ÑƒÑ€Ð°Ñ†Ð¸ÑŽ ÐºÐ»ÑŽÑ‡Ð°
      }

      for (i = 0; i < 4; i++) {
        __EEWrite(0x30 + (4 * LearnKeysCount) + i, ident[i]);  //Ð¡Ð¾Ñ…Ñ€Ð°Ð½ÑÐµÐ¼ ÐºÐ»ÑŽÑ‡ Ð²Ð¾ Ð²Ñ€ÐµÐ¼ÐµÐ½Ð½Ð¾Ð¼ Ñ…Ñ€Ð°Ð½Ð¸Ð»Ð¸Ñ‰Ðµ
      }

      LearnKeysCount++;
      Beep(LearnKeysCount);
      MinCounter = 0;
      SecCounter = 0;
      MilSecCounter = 0;            //Ð¡Ð±Ñ€Ð¾Ñ ÑÑ‡ÐµÑ‚Ñ‡Ð¸ÐºÐ¾Ð² Ð²Ñ€ÐµÐ¼ÐµÐ½Ð¸

      ClearFlag(TransNo);
      return(res_FreeKey);
    } else {
      return(res_UnknownKey);
    }
  }


#ifndef STATICKEY                                         //Ð•ÑÐ»Ð¸ Ð¿Ñ€Ð¾Ð³Ñ€Ð°Ð¼Ð¼Ð¸Ñ€ÑƒÐµÐ¼Ñ‹Ðµ ÐºÐ»ÑŽÑ‡Ð¸

  if (KeyType == kt_WorkKey)
#endif
  {
    /*
        if (!memcmp(ident, KeyInvClose, 4)) return(res_InvClose);
        if (!memcmp(ident, KeyInvOpen, 4)) return(res_InvOpen);
        if (!memcmp(ident, KeyInvPark, 4)) return(res_InvPark);

        if (!memcmp(ident, KeyUseClose, 4)) return(res_UseClose);
        if (!memcmp(ident, KeyUseOpen, 4)) return(res_UseOpen);
        if (!memcmp(ident, KeyUsePark, 4)) return(res_UsePark);
    */
    for (i = 0; i < KeysCount; i++) {
      if (!memcmp(ident, Keys[i], 4)) {
        return(res_WorkKey);  //Ð Ð°Ð±Ð¾Ñ‡Ð¸Ð¹ ÐºÐ»ÑŽÑ‡
      }
    }

    return(res_UnknownKey);                                   //ÐšÐ»ÑŽÑ‡ Ð¾Ñ‚ Ð´Ñ€ÑƒÐ³Ð¾Ð³Ð¾ ÑƒÑÑ‚Ñ€Ð¾Ð¹ÑÑ‚Ð²Ð°
  }

  return(res_NoKey);
}

void memcpy(unsigned char Target[], unsigned char const Source[], char Count) {
  for (char i = 0; i < Count; i++) {
    Target[i] = Source[i];
  }
}

char memcmp(unsigned char const Target[], unsigned char const Source[], char Count) {
  for (char i = 0; i < Count; i++)
    if (Target[i] != Source[i]) {
      return 1;
    }
    
  return 0;
}


void MemToArray(unsigned char Mem[], char Array[], char First, char Count) {
  char j = 0;
  char k = First - 1;
  char i;
  
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
  char TempByte = 0;
  char j = 0;
  char k = 0;
  char i;
  
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
  char TempByte = 0;
  char j = 0;
  char k = 0;
  char i;
  
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
  //  char DisLp1, DisSmart;
  
  FilterH = 1;
  FilterL = 1;
  Gain1 = 1;
  Gain0 = 0;
  Hysteresis = 0;
  //  DisLp1 = 0, DisSmart = 0;
  
  //  char Dop = 0;
  //  Dop = (1<<(Gain1*(3-8)+8)) | (1<<(Gain0*(2-8)+8)) | (1<<(FilterH*(1-8)+8)) | (1<<(FilterL*(0-8)+8));
  SetConfigPage(0, (1 << (Gain1 * (3 - 8) + 8)) | (1 << (Gain0 * (2 - 8) + 8)) | (1 << (FilterH * (1 - 8) + 8)) | (1 << (FilterL * (0 - 8) + 8)));
  delay_ms(1);
  SetConfigPage(1, (1 << (Hysteresis * (1 - 8) + 8)));
  delay_ms(1);
  SetConfigPage(2, 0x00);
  delay_ms(1);
  SetConfigPage(3, 0x03);
  //  SetConfigPage(3, (1<<(DisLp1*(3-8)+8)) | (1<<(DisSmart*(2-8)+8)) | (1<<(1*(1-8)+8)) | (1<<(1*(0-8)+8)));
  delay_ms(5);
  
  char dop = SendToReaderCommand(0x08);   //Читаем фазу
  dop <<= 1;                              //Умножаем на 2
  dop += Offset;                          //Добавляем задержку
  dop |= (1 << 7);
  dop &= ~(1 << 6);        //Готовим команду
  SendToReaderNoAnswer(dop);              //SetSampleTime
  delay_ms(5);
  
  SetConfigPage(2, 0x0B);
  delay_ms(5);
  SetConfigPage(2, 0x08);
  delay_ms(1);
  SetConfigPage(2, 0x00);
  delay_ms(1);
}

void FastSetPhase(void) {             //Быстрая настройка приемника
  delay_us(320);
  SetConfigPage(2, 0x0A); //  SendToReaderNoAnswer(0x6A);
  delay_us(400);
  SetConfigPage(2, 0x0B); //  SendToReaderNoAnswer(0x6B);
  delay_us(320);
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
    
    delay_us(10 * 8);
    
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
  delay_ms(1);
  Send_Start();
  delay_ms(1);
}

char Send_Password(void) {
  delay_us(T_wait2 * 8);  //Ждем T_wait2
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
  
  delay_us(10 * 8);
  SetConfigPage(2, 0x09); //  SendToReaderNoAnswer(0x69);
  
  if (State == st_OkReceiveAnswer) {
    return 1;
  } else {
    return 0;
  }
}

char Send_ConfigPage() {      //В 6 страницу пишем конфигурацию ключа
  delay_us(T_wait2 * 8);  //Ждем T_wait2
  unsigned char Command[2];
  Command[0] = 0xB2;
  Command[1] = 0x6C;
  
  memcpy(data_block, Command, 2);
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
  
  delay_us(10 * 8);
  SetConfigPage(2, 0x09); //  SendToReaderNoAnswer(0x69);
  
  //Проверка ответа
  unsigned char Ans[2];
  ArrayToMem(Message, data_block, 5, 10);
  Oneway2(data_block, 10);
  memcpy(Ans, data_block, 2);
  Ans[1] &= ~0x3F;                  //Очистка правых 6 бит
  Command[1] &= ~0x3F;              //Очистка правых 6 бит
  
  if (memcmp(Ans, Command, 2) != 0) {
    return 0;
  }
  
  delay_us(T_wait2 * 8);  //Ждем T_wait2
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
  
  delay_us(T_prog * 8);  //Ждем T_prog
  delay_us(T_wait2 * 8);  //Ждем T_wait2
  
  return 1;
}

char Send_WritePage(char Nomber, unsigned char TempPage[]) {
  delay_us(T_wait2 * 8);  //Ждем T_wait2
  
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
  
  memcpy(data_block, Command, 2);
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
  
  delay_us(10 * 8);
  SetConfigPage(2, 0x09); //  SendToReaderNoAnswer(0x69);
  
  //Проверка ответа
  unsigned char Ans[2];
  ArrayToMem(Message, data_block, 5, 10);
  Oneway2(data_block, 10);
  memcpy(Ans, data_block, 2);
  Ans[1] &= ~0x3F;                  //Очистка правых 6 бит
  Command[1] &= ~0x3F;              //Очистка правых 6 бит
  
  if (memcmp(Ans, Command, 2) != 0) {
    return 0;
  }
  
  delay_us(T_wait2 * 8);  //Ждем T_wait2
  memcpy(data_block, TempPage, 4);
  Oneway2(data_block, 32);
  MemToArray(data_block, Message, 1, 4);
  
  SendToReader_Write();           //Переключаем на передачу
  SendToReader(32);               //Посылка транспондеру 32 бит
  SendToReader_Stop();            //Выключаем режим передачи
  
  FastSetPhase();
  
  delay_us(T_prog * 8);  //Ждем T_prog
  delay_us(T_wait2 * 8);  //Ждем T_wait2
  
  return 1;
}


char Send_ReadPage(char Nomber) {
  delay_us(T_wait2 * 8);  //Ждем T_wait2
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
  
  memcpy(data_block, Command, 2);
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
  
  delay_us(T_wait2 * 8);  //Ждем T_wait2
  return 1;
}




//***************************************************
//************Настройка базовой станции**************
//***************************************************

void SendToReader_Start(void) {
  DDRB |= (1 << MOSI);          //MOSI - выход
  
  delay_us(SPI_QartStrobe);
  PORTB |= (1 << SCK);
  
  if (TestPin(PORTB, MOSI)) {
    PORTB &= ~(1 << MOSI);
  }
  
  delay_us(SPI_HalfStrobe);
  PORTB |= (1 << MOSI);
  delay_us(SPI_HalfStrobe);
  PORTB &= ~(1 << SCK);
  
  delay_us(SPI_QartStrobe);
}

char SendToReaderCommand(char Command) {
  SendToReaderNoAnswer(Command);
  
  DDRB &= ~(1 << MISO);         //MISO вход
  char i = 0x80;
  char Answer = 0;
  
  do { //Читаем результат
    delay_us(SPI_QartStrobe);
    PORTB |= (1 << SCK);
    delay_us(SPI_HalfStrobe);
    
    if (TestPin(PINB, MISO)) {
      Answer |= i;
    }
    
    PORTB &= ~(1 << SCK);
    delay_us(SPI_QartStrobe);
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
    
    delay_us(SPI_QartStrobe);
    PORTB |= (1 << SCK);
    delay_us(SPI_HalfStrobe);
    PORTB &= ~(1 << SCK);
    delay_us(SPI_QartStrobe);
    
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
    
    delay_us(SPI_QartStrobe);
    PORTB |= (1 << SCK);
    delay_us(SPI_HalfStrobe);
    PORTB &= ~(1 << SCK);
    delay_us(SPI_QartStrobe);
    
    i >>= 1;
  } while (i > 0);
}

void SendToReader_Read(void) {  // 111
  SendToReader_Start();      //Старт
  PORTB |= (1 << MOSI);
  
  delay_us(SPI_QartStrobe);
  PORTB |= (1 << SCK);             //1
  delay_us(SPI_HalfStrobe);
  PORTB &= ~(1 << SCK);
  delay_us(SPI_HalfStrobe);
  
  PORTB |= (1 << SCK);             //1
  delay_us(SPI_HalfStrobe);
  PORTB &= ~(1 << SCK);
  delay_us(SPI_HalfStrobe);
  
  PORTB |= (1 << SCK);             //1
  delay_us(SPI_HalfStrobe);
  PORTB &= ~(1 << SCK);
  delay_us(SPI_QartStrobe);
}

void SendToReader_Write(void) { //110
  SendToReader_Start();      //Старт
  PORTB |= (1 << MOSI);
  
  delay_us(SPI_QartStrobe);
  PORTB |= (1 << SCK);             //1
  delay_us(SPI_HalfStrobe);
  PORTB &= ~(1 << SCK);
  delay_us(SPI_HalfStrobe);
  
  PORTB |= (1 << SCK);             //1
  delay_us(SPI_HalfStrobe);
  PORTB &= ~(1 << SCK);
  delay_us(SPI_QartStrobe);
  
  PORTB &= ~(1 << MOSI);
  
  delay_us(SPI_QartStrobe);
  PORTB |= (1 << SCK);             //0
  delay_us(SPI_HalfStrobe);
  PORTB &= ~(1 << SCK);
  delay_us(SPI_QartStrobe);
}

void SendToReader_Stop(void) {
  PORTB &= ~(1 << SCK); //SCK
  delay_us(SPI_HalfStrobe);
  PORTB |= (1 << SCK);  //SCK
  delay_us(SPI_HalfStrobe);
  PORTB &= ~(1 << SCK); //SCK
  
  PORTB |= (1 << MOSI);  //MOSI
}

void SendToReader(char Nomber) {
#define t_low 7
#define T_1 29
#define T_0 20
#define t_stop 40
  char i;
  
  for (i = 0; i < Nomber; i++) {
    PORTB ^= (1 << MOSI);             //Дергаем портом MOSI
    delay_us(t_low * 8);              //Ждем время t_low
    PORTB ^= (1 << MOSI);             //Дергаем портом MOSI
    
    if (Message[i] == 1) {
      delay_us((T_1 - t_low) * 8);  //Ждем время до полного T_1
    } else {
      delay_us((T_0 - t_low) * 8);  //Ждем время до полного T_0
    }
  }
  
  //Фронт после передачи сообщения
  PORTB ^= (1 << MOSI);               //Дергаем портом MOSI
  delay_us(t_low * 8);                //Ждем время t_low
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






