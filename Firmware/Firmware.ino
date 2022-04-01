// Проект LD.CONNECT
// Автоматизированная беспроводная система сбора данных
// Прошивка Ардуино ProMicro v.0.2a
// Страница проекта:
// https://labdata.ru/projects/ld_connect
//
// (c) LabData.ru, 2022
//------------------------------------------------------
// Rev.1 27.03.2022 (0.2a) Исключен баг с отрицательными значениями с АЦП

#define DEBUG 0 // 1 для отладки, 0 для работы
#define VER 1
#define SPI_CLK  15
#define SPI_MISO 14
#define SPI_MOSI 16
#define ADC_CS   7
#define ADC_DRDY 2
#define AI0 A3
#define AI1 A2
#define AI2 A1
#define AI3 A0
#define IO0 21
#define IO1 20
#define IO2 19
#define IO3 18

#include <SPI.h>
#include "LD_Protocentral_ADS1220.h"
Protocentral_ADS1220 ads1220;
bool useBT = 0;
uint32_t ADC_t = 0;
uint32_t dt = 0;
byte ports[] = {0,0,0,0}; 
#include <avr/eeprom.h>
byte EEMEM aCH1; byte CH1;
byte EEMEM aCH2; byte CH2;
byte EEMEM aCH3; byte CH3;
byte EEMEM aCH4; byte CH4;
byte EEMEM aAUX1; byte AUX1;
byte EEMEM aAUX2; byte AUX2;
byte EEMEM aAUX3; byte AUX3;
byte EEMEM aAUX4; byte AUX4;
word EEMEM aTd;   word Td;
byte EEMEM aG;    byte G;
byte EEMEM aREG;  byte REG;
byte EEMEM aREF;  byte REF;
byte mode =0;
/* Режимы работы
 * 0 - Ожидание команд
 * 1 - Регистрация
 */
void default_settings(){ // Установить настройки по умолчанию без сохранения
  /* Структура выходов
   * бит 0 - направление (0 - вход, 1 - выход)
   * бит 1 - тип (0 - аналоговый, 1 - цифровой)
   * бит 2 - активация (0 - выключен, 1 - активен)
   * бит 3 - использовать как шину данных (0 - нет, 1 - да)
   */
  CH1 = 0;
  CH2 = 0;
  CH3 = 0;
  CH4 = 0;
  AUX1 = 0;
  AUX2 = 0;
  AUX3 = 0;
  AUX4 = 0;
  Td = 10; //мс 100 Гц
  G = 1;
  REG = 0; // откл. авто рег
  REF = 0; // 5 V
  // Доп входы цифровые
  bitWrite(AUX1, 1, 1);
  bitWrite(AUX2, 1, 1);
  bitWrite(AUX3, 1, 1);
  bitWrite(AUX4, 1, 1);
}
void SaveConfig(){ // Обновление настроек в памяти
  eeprom_update_byte(&aCH1, CH1);
  eeprom_update_byte(&aCH2, CH2);
  eeprom_update_byte(&aCH3, CH3);
  eeprom_update_byte(&aCH4, CH4);
  eeprom_update_byte(&aAUX1, AUX1);
  eeprom_update_byte(&aAUX2, AUX2);
  eeprom_update_byte(&aAUX3, AUX3);
  eeprom_update_byte(&aAUX4, AUX4);
  eeprom_update_word(&aTd, Td);
  eeprom_update_byte(&aG, G);
  eeprom_update_byte(&aREG, REG);
  eeprom_update_byte(&aREF, REF);
}
void checkEEPROM(){ // Проверка корректности настроек в памяти иначе записать по умолчанию
  byte tCH1 = eeprom_read_byte(&aCH1);
  byte tCH2 = eeprom_read_byte(&aCH2);
  byte tCH3 = eeprom_read_byte(&aCH3);
  byte tCH4 = eeprom_read_byte(&aCH4);
  byte tAUX1 = eeprom_read_byte(&aAUX1);
  byte tAUX2 = eeprom_read_byte(&aAUX2);
  byte tAUX3 = eeprom_read_byte(&aAUX3);
  byte tAUX4 = eeprom_read_byte(&aAUX4);
  word tTd = eeprom_read_word(&aTd);
  byte tG = eeprom_read_byte(&aG);
  byte tREG = eeprom_read_byte(&aREG);
  byte tREF = eeprom_read_byte(&aREF);
  bool error = 0;
  if ((bitRead(tCH1,0) == 1)||(bitRead(tCH1,1) == 1)||(bitRead(tCH1,3) == 1)) { 
    error = 1;
    if (DEBUG) Serial.println("Ошибка в CH1!");
  }
  if ((bitRead(tCH2,0) == 1)||(bitRead(tCH2,1) == 1)||(bitRead(tCH2,3) == 1)) { 
    error = 1;
    if (DEBUG) Serial.println("Ошибка в CH2!");
  }
  if ((bitRead(tCH3,0) == 1)||(bitRead(tCH3,1) == 1)||(bitRead(tCH3,3) == 1)) { 
    error = 1;
    if (DEBUG) Serial.println("Ошибка в CH3!");
  }
  if ((bitRead(tCH4,0) == 1)||(bitRead(tCH4,1) == 1)||(bitRead(tCH4,3) == 1)) { 
    error = 1;
    if (DEBUG) Serial.println("Ошибка в CH4!");
  }
  if (tTd < 1) { 
    error = 1;
    if (DEBUG) Serial.println("Ошибка в Td!");
  }
  if ((tREF < 0)||(tREF > 1)) { 
    error = 1;
    if (DEBUG) Serial.println("Ошибка в REF!");
  }
  if (!((tG == 1)||(tG == 2)||(tG == 4)||(tG == 8)||(tG == 16)||(tG == 32)||(tG == 64)||(tG == 128))) { 
    error = 1;
    if (DEBUG) Serial.println("Ошибка в G!");
  } //1,2,4,8,16,32,64,128
  if (error) { // записываем по умолчанию
    if (DEBUG) Serial.println("Ошибка в EEPROM!");
    SaveConfig();
  } else
  {
   CH1 = tCH1;
   CH2 = tCH2;
   CH3 = tCH3;
   CH4 = tCH4;
   AUX1 = tAUX1;
   AUX2 = tAUX2;
   AUX3 = tAUX3;
   AUX4 = tAUX4;
   Td = tTd;
   G = tG;
   REG = tREG;
   REF = tREF;
   if (DEBUG) Serial.println("Загрузка завершена");
  }
}
void ADCinit(){ // Инициализация АЦП
  ads1220.begin(ADC_CS,ADC_DRDY);
  ads1220.set_data_rate(DRT_2000SPS);//20,45,90,175,330,600,1000
  byte PGA = PGA_GAIN_1;
  switch (G){
    case 1:
      PGA = PGA_GAIN_1;
      break;
    case 2:
      PGA = PGA_GAIN_2;
      break;
    case 4:
      PGA = PGA_GAIN_4;
      break;
    case 8:
      PGA = PGA_GAIN_8;
      break;
    case 16:
      PGA = PGA_GAIN_16;
      break;
    case 32:
      PGA = PGA_GAIN_32;
      break;
    case 64:
      PGA = PGA_GAIN_64;
      break;
    case 128:
      PGA = PGA_GAIN_128;
      break;
  }
  ads1220.set_pga_gain(PGA); 
  ads1220.set_FIR_Filter(FIR_OFF);//FIR_OFF, FIR_5060, FIR_50Hz, FIR_60Hz    
  ads1220.set_conv_mode_continuous();
  ads1220.set_OperationMode(MODE_TURBO);
  ads1220.DRDYmode_default();
  //ads1220.Start_Conv();
  // настройка встроенного АЦП
  // настройка входов

  //настройка опорного напряжения
  if (REF==1) { 
    analogReference(INTERNAL);
  } else {
    analogReference(DEFAULT);
  }
}
void setup() {
  Serial.begin(9600);
  Serial.setTimeout(100);
  Serial1.begin(9600);
  Serial1.setTimeout(100);
  //while (!Serial) {
  //  ; 
  //}
  default_settings();
  checkEEPROM(); // проверка корректности памяти устройства
  ADCinit();
  if (REG == 1) { // Начать регистрацию автоматически
    ads1220.Start_Conv();
    mode=1; 
  }
}
void SendRegisters(){ // Отправить состояние регистров в порт
  if (useBT){
      Serial1.print("CH1 ");
      Serial1.print(bitRead(CH1,0));
      Serial1.print(bitRead(CH1,1));
      Serial1.print(bitRead(CH1,2));
      Serial1.println(bitRead(CH1,3));
      Serial1.print("CH2 ");
      Serial1.print(bitRead(CH2,0));
      Serial1.print(bitRead(CH2,1));
      Serial1.print(bitRead(CH2,2));
      Serial1.println(bitRead(CH2,3));
      Serial1.print("CH3 ");
      Serial1.print(bitRead(CH3,0));
      Serial1.print(bitRead(CH3,1));
      Serial1.print(bitRead(CH3,2));
      Serial1.println(bitRead(CH3,3));
      Serial1.print("CH4 "); 
      Serial1.print(bitRead(CH4,0));
      Serial1.print(bitRead(CH4,1));
      Serial1.print(bitRead(CH4,2));
      Serial1.println(bitRead(CH4,3));
      Serial1.print("AUX1 ");
      Serial1.print(bitRead(AUX1,0));
      Serial1.print(bitRead(AUX1,1));
      Serial1.print(bitRead(AUX1,2));
      Serial1.println(bitRead(AUX1,3));
      Serial1.print("AUX2 ");
      Serial1.print(bitRead(AUX2,0));
      Serial1.print(bitRead(AUX2,1));
      Serial1.print(bitRead(AUX2,2));
      Serial1.println(bitRead(AUX2,3));
      Serial1.print("AUX3 ");
      Serial1.print(bitRead(AUX3,0));
      Serial1.print(bitRead(AUX3,1));
      Serial1.print(bitRead(AUX3,2));
      Serial1.println(bitRead(AUX3,3));
      Serial1.print("AUX4 ");
      Serial1.print(bitRead(AUX4,0));
      Serial1.print(bitRead(AUX4,1));
      Serial1.print(bitRead(AUX4,2));
      Serial1.println(bitRead(AUX4,3));
      Serial1.print ("Td ");
      Serial1.println(Td);
      Serial1.print("G ");
      Serial1.println(G);
      Serial1.print("REG ");
      Serial1.println(REG);
      Serial1.print("REF ");
      Serial1.println(REF);
      Serial1.print("Ports:"); 
      Serial1.print(ports[0]); 
      Serial1.print(ports[1]);
      Serial1.print(ports[2]); 
      Serial1.print(ports[3]);
      Serial.println();
  } else {
      Serial.print("CH1 ");
      Serial.print(bitRead(CH1,0)); 
      Serial.print(bitRead(CH1,1)); 
      Serial.print(bitRead(CH1,2)); 
      Serial.println(bitRead(CH1,3));
      Serial.print("CH2 ");
      Serial.print(bitRead(CH2,0)); 
      Serial.print(bitRead(CH2,1)); 
      Serial.print(bitRead(CH2,2)); 
      Serial.println(bitRead(CH2,3));
      Serial.print("CH3 ");
      Serial.print(bitRead(CH3,0)); 
      Serial.print(bitRead(CH3,1)); 
      Serial.print(bitRead(CH3,2)); 
      Serial.println(bitRead(CH3,3));
      Serial.print("CH4 "); 
      Serial.print(bitRead(CH4,0)); 
      Serial.print(bitRead(CH4,1)); 
      Serial.print(bitRead(CH4,2)); 
      Serial.println(bitRead(CH4,3));
      Serial.print("AUX1 ");
      Serial.print(bitRead(AUX1,0)); 
      Serial.print(bitRead(AUX1,1)); 
      Serial.print(bitRead(AUX1,2)); 
      Serial.println(bitRead(AUX1,3));
      Serial.print("AUX2 ");
      Serial.print(bitRead(AUX2,0)); 
      Serial.print(bitRead(AUX2,1)); 
      Serial.print(bitRead(AUX2,2)); 
      Serial.println(bitRead(AUX2,3));
      Serial.print("AUX3 ");
      Serial.print(bitRead(AUX3,0)); 
      Serial.print(bitRead(AUX3,1)); 
      Serial.print(bitRead(AUX3,2)); 
      Serial.println(bitRead(AUX3,3));
      Serial.print("AUX4 ");
      Serial.print(bitRead(AUX4,0)); 
      Serial.print(bitRead(AUX4,1)); 
      Serial.print(bitRead(AUX4,2)); 
      Serial.println(bitRead(AUX4,3));
      Serial.print ("Td ");
      Serial.println(Td);
      Serial.print("G ");
      Serial.println(G);
      Serial.print("REG ");
      Serial.println(REG);
      Serial.print("REF ");
      Serial.println(REF);
      Serial.print("Ports:"); 
      Serial.print(ports[0]);  
      Serial.print(ports[1]); 
      Serial.print(ports[2]);  
      Serial.print(ports[3]); 
      Serial.println();      
  }
}
void SendMessage(char *Msg){
  if (useBT) 
            Serial1.println(Msg);
      else
            Serial.println(Msg);
}
void SerialCommand(){ // Обработка дистанционного управления
  if (Serial.available())  { useBT = 0; }
  if (Serial1.available()) { useBT = 1; }
  if (Serial.available()||Serial1.available()){
    char CMD, CMD1, CMD2, CMD3;
    byte Val;
    uint16_t Val16;
    if (useBT) 
      CMD = Serial1.read();
    else 
      CMD = Serial.read();
    if (CMD == 'M') 
      if (useBT) 
            Serial1.println(mode);  //Текущий режим
       else
            Serial.println(mode);  //Текущий режим
    if (CMD == 'R') SendRegisters();     //Состояние регистров
    if (CMD == 'L') {                    // Запуск регистрации
      mode = 1;
      ads1220.Start_Conv();
      if (useBT) 
            Serial1.println(mode);
      else
            Serial.println(mode);
    }
    if (CMD == 'H') {                    // Остановка регистрации
      mode = 0;
      ads1220.ads1220_Reset();
      ADCinit();
      if (useBT) 
            Serial1.println(mode);
      else
            Serial.println(mode);
    }
    if (CMD == 'S') {                    // Сохранение настроек
      SaveConfig();
      if (useBT) 
            Serial1.println("OK");
      else
            Serial.println("OK");
    }
    if (CMD == 'C') {                    // Настройка параметров
       if (useBT)
          CMD1 = Serial1.read(); //Тип параметра
       else
          CMD1 = Serial.read(); //Тип параметра
          
       if (CMD1 == 'A'){ // Вход АЦП
          if (useBT){
            CMD2 = Serial1.read(); //Номер порта
            CMD3 = Serial1.read(); //настройка
            Val = Serial1.parseInt(); // значение
          }else{
            CMD2 = Serial.read(); //Номер порта
            CMD3 = Serial.read(); //настройка
            Val = Serial.parseInt(); // значение
          }  
          switch (CMD2){ // выбор порта
            case '1':
              if (CMD3 == 'E') {
                bitWrite(CH1,2,constrain(Val, 0, 1));
                SendMessage("A1E");
              }
              //if (useBT)
              //  Serial1.println(CH1, BIN);
              //else
              //  Serial.println(CH1, BIN);
              break;
            case '2':
              if (CMD3 == 'E') {
                bitWrite(CH2,2,constrain(Val, 0, 1));
                SendMessage("A2E");
              }
              //if (useBT)
              //  Serial1.println(CH2, BIN);
              //else
              //  Serial.println(CH2, BIN);
              break;
            case '3':
              if (CMD3 == 'E') {
                bitWrite(CH3,2,constrain(Val, 0, 1));
                SendMessage("A3E");
              }
              //if (useBT)
              //  Serial1.println(CH3, BIN);
              //else
              //  Serial.println(CH3, BIN);
              break;
            case '4':
              if (CMD3 == 'E') {
                bitWrite(CH4,2,constrain(Val, 0, 1));
                SendMessage("A4E");
              }
              //if (useBT)
              //  Serial1.println(CH4, BIN);
              //else
              //  Serial.println(CH4, BIN);
              break;
          }
          PortsConfig();
       }
       if (CMD1 == 'P'){ // Вход Ардуино
         if (useBT){
          CMD2 = Serial1.read(); //Номер порта
          CMD3 = Serial1.read(); //настройка
          Val = Serial1.parseInt(); // значение
         } else {
          CMD2 = Serial.read(); //Номер порта
          CMD3 = Serial.read(); //настройка
          Val = Serial.parseInt(); // значение
         }
          switch (CMD2){ // выбор порта
            case '1': 
              if (CMD3 == 'D') {
                bitWrite(AUX1,0,constrain(Val, 0, 1));
                SendMessage("P1D");
              }
              if (CMD3 == '#') {
                bitWrite(AUX1,1,constrain(Val, 0, 1));
                SendMessage("P1A");
              }
              if (CMD3 == 'E') {
                bitWrite(AUX1,2,constrain(Val, 0, 1));
                SendMessage("P1E");
              }
              if (CMD3 == 'B') {
                bitWrite(AUX1,3,constrain(Val, 0, 1));
                SendMessage("P1B");
              }
              //if (useBT) Serial1.println(AUX1, BIN);
              //else       Serial.println(AUX1, BIN);
              break;
            case '2':
              if (CMD3 == 'D') {
                bitWrite(AUX2,0,constrain(Val, 0, 1));
                SendMessage("P2D");
              }
              if (CMD3 == '#') {
                bitWrite(AUX2,1,constrain(Val, 0, 1));
                SendMessage("P2A");
              }
              if (CMD3 == 'E') {
                bitWrite(AUX2,2,constrain(Val, 0, 1));
                SendMessage("P2E");
              }
              if (CMD3 == 'B') {
                bitWrite(AUX2,3,constrain(Val, 0, 1));
                SendMessage("P2B");
              }
              //if (useBT)   Serial1.println(AUX2, BIN);
              //else    Serial.println(AUX2, BIN);
              break;
            case '3':
              if (CMD3 == 'D') {
                bitWrite(AUX3,0,constrain(Val, 0, 1));
                SendMessage("P3D");
              }
              if (CMD3 == '#') {
                bitWrite(AUX3,1,constrain(Val, 0, 1));
                SendMessage("P3A");
              }
              if (CMD3 == 'E') {
                bitWrite(AUX3,2,constrain(Val, 0, 1));
                SendMessage("P3E");
              }
              if (CMD3 == 'B') {
                bitWrite(AUX3,3,constrain(Val, 0, 1));
                SendMessage("P3B");
              }
              //if (useBT)  Serial1.println(AUX3, BIN);
              //else        Serial.println(AUX3, BIN);
              break;
            case '4':
              if (CMD3 == 'D') {
                bitWrite(AUX4,0,constrain(Val, 0, 1));
                SendMessage("P4D");
              }
              if (CMD3 == '#') {
                bitWrite(AUX4,1,constrain(Val, 0, 1));
                SendMessage("P4A");
              }
              if (CMD3 == 'E') {
                bitWrite(AUX4,2,constrain(Val, 0, 1));
                SendMessage("P4E");
              }
              if (CMD3 == 'B') {
                bitWrite(AUX4,3,constrain(Val, 0, 1));
                SendMessage("P4B");
              }
              //if (useBT)   Serial1.println(AUX4, BIN);
              //else         Serial.println(AUX4, BIN);
              break;
          } 
          PortsConfig();
       }
       if (CMD1 == 'T'){ // Период дискретизации
          if (useBT){
            Val16 = Serial1.parseInt();  
            Serial1.println(Td);
          } else {
            Val16 = Serial.parseInt();  
            Serial.println(Td);
          }
          Td = constrain(Val16,1,65535);
       }
       if (CMD1 == 'G'){ // Вход усиление
        if (useBT){
          Val = Serial1.parseInt();  
          Serial1.println(Val);
        } else {
          Val = Serial.parseInt();  
          Serial.println(Val);
        }
          if (((Val == 1)||(Val == 2)||(Val == 4)||(Val == 8)||(Val == 16)||(Val == 32)||(Val == 64)||(Val == 128))) 
              G = Val;
          
          ADCinit();
       }
       if (CMD1 == 'F'){ // Опорное напряжение
          if (useBT){
              Val = Serial1.parseInt(); 
              Serial1.println(Val);
          }else {
            Val = Serial.parseInt(); 
            Serial.println(Val);
          }
          if (Val==0) 
            REF = 0;
          else
            REF = 1;
          
          ADCinit();
       }
       if (CMD1 == 'M'){ // Автостарт
          if (useBT){
            Val = Serial1.parseInt();
            Serial1.println(Val);
          }else {
            Val = Serial.parseInt();
            Serial.println(Val);
          }
          if (Val==0) 
            REG = 0;
          else
            REG = 1;
       }     
    }
    if (CMD == '!') {                    // Задаем порты
      if (useBT){
        CMD = Serial1.read(); //Номер порта
        Val = Serial1.parseInt(); //Значение порта
      } else {
        CMD = Serial.read(); //Номер порта
        Val = Serial.parseInt(); //Значение порта
      }
       if (CMD == '1') {
        ports[0] = Val; // Порт P1
       }
       if (CMD == '2') {
        ports[1] = Val; // Порт P2
       }
       if (CMD == '3') {
        ports[2] = Val; // Порт P3
       }
       if (CMD == '4') {
        ports[3] = Val; // Порт P4 
       }
    }
  }
}
void SendValue24(byte CH) { // Процедура измерения и отправки числа в порт
  int32_t adc_data=0;
  // измерение
  switch (CH){
    case 1: // CH1
    ads1220.select_mux_channels(MUX_SE_CH0); // Выюор 1 канала
    adc_data=ads1220.Read_WaitForData(); // Чтение измерения
    break;
    case 2: // CH2
    ads1220.select_mux_channels(MUX_SE_CH1); // Выюор 2 канала
    adc_data=ads1220.Read_WaitForData(); // Чтение измерения
    break;
    case 3: // CH3
    ads1220.select_mux_channels(MUX_SE_CH2); // Выюор 3 канала
    adc_data=ads1220.Read_WaitForData(); // Чтение измерения
    break;
    case 4: // CH4
    ads1220.select_mux_channels(MUX_SE_CH3); // Выюор 4 канала
    adc_data=ads1220.Read_WaitForData(); // Чтение измерения
    break;
  }
  if (useBT){
    Serial1.print(CH);
    Serial1.print(",");
    Serial1.println(adc_data);
  } else {
    Serial.print(CH);
    Serial.print(",");
    Serial.println(adc_data);
  }
}
void SendValue16(byte CH) { // Процедура измерения и отправки числа в порт
  uint16_t adc_data=0;
  // измерение
  switch (CH){
    case 1: // P1
    adc_data=analogRead(AI0); // Чтение измерения
    break;
    case 2: // P2
    adc_data=analogRead(AI1); // Чтение измерения
    break;
    case 3: // P3
    adc_data=analogRead(AI2); // Чтение измерения
    break;
    case 4: // P4
    adc_data=analogRead(AI3); // Чтение измерения
    break;
  }
  if (useBT){
    Serial1.print(CH+4);
    Serial1.print(",");
    Serial1.println(adc_data);
  } else {
    Serial.print(CH+4);
    Serial.print(",");
    Serial.println(adc_data);
  }
}
void SendValue8(byte CH) { // Процедура измерения и отправки оного числа в порт
  byte D_data=0;
  // измерение
  switch (CH){
    case 1: // P1
    D_data=digitalRead(IO0); // Чтение измерения
    break;
    case 2: // C2
    D_data=digitalRead(IO1); // Чтение измерения
    break;
    case 3: // C3
    D_data=digitalRead(IO2); // Чтение измерения
    break;
    case 4: // C4
    D_data=digitalRead(IO3); // Чтение измерения
    break;
  }
  if (useBT){
    Serial1.print(CH+4);
    Serial1.print(",");
    Serial1.println(D_data);
  } else {
    Serial.print(CH+4);
    Serial.print(",");
    Serial.println(D_data);
  }
}
void SendData() { // Чтение и отправка данных в порт
  uint16_t Period = millis()-dt;
  dt = millis();
  if (useBT){
    Serial1.print('t');
    Serial1.print(",");
    Serial1.println(Period);
  } else {
    Serial.print('t');
    Serial.print(",");
    Serial.println(Period);
  }
  if (bitRead(CH1,2)==1){ // если включен аналоговый вход
    SendValue24(1);
  }
  if (bitRead(CH2,2)==1){ // если включен аналоговый вход
    SendValue24(2);
  }
  if (bitRead(CH3,2)==1){ // если включен аналоговый вход
    SendValue24(3);
  }
  if (bitRead(CH4,2)==1){ // если включен аналоговый вход
    SendValue24(4);
  }
  if ((bitRead(AUX1,2)==1)&&(bitRead(AUX1,3)==0)&&(bitRead(AUX1,0)==0)){ // если включен доп вход
    if (bitRead(AUX1,1)==0){ // вход аналоговый
      SendValue16(1);  
    } else {                 // вход цифровой
      SendValue8(1);
    }
  }
  if ((bitRead(AUX2,2)==1)&&(bitRead(AUX2,3)==0)&&(bitRead(AUX2,0)==0)){ // если включен доп вход
    if (bitRead(AUX2,1)==0){ // вход аналоговый
      SendValue16(2);  
    } else {                 // вход цифровой
      SendValue8(2);
    }
  }
  if ((bitRead(AUX3,2)==1)&&(bitRead(AUX3,3)==0)&&(bitRead(AUX3,0)==0)){ // если включен доп вход
    if (bitRead(AUX3,1)==0){ // вход аналоговый
      SendValue16(3);  
    } else {                 // вход цифровой
      SendValue8(3);
    }
  }
  if ((bitRead(AUX4,2)==1)&&(bitRead(AUX4,3)==0)&&(bitRead(AUX4,0)==0)){ // если включен доп вход
    if (bitRead(AUX4,1)==0){ // вход аналоговый
      SendValue16(4);  
    } else {                 // вход цифровой
      SendValue8(4);
    }
  }
  
}
void PortsConfig() { // Настройка портов
  // P1
  if (bitRead(AUX1,3)==0){ //если не шина данных
    if (bitRead(AUX1,1)==0) // если аналоговый
      pinMode(AI0,INPUT);
    else { // если цифровой
      if (bitRead(AUX1,0)==0)  // вход
        pinMode(IO0,INPUT);
      else // выход
        pinMode(IO0,OUTPUT);
    }   
  }
  // P2
  if (bitRead(AUX2,3)==0){ //если не шина данных
    if (bitRead(AUX2,1)==0) // если аналоговый
      pinMode(AI1,INPUT);
    else { // если цифровой
      if (bitRead(AUX2,0)==0)  // вход
        pinMode(IO1,INPUT);
      else // выход
        pinMode(IO1,OUTPUT);
    }   
  }
   // P3
  if (bitRead(AUX3,3)==0){ //если не шина данных
    if (bitRead(AUX3,1)==0) // если аналоговый
      pinMode(AI2,INPUT);
    else { // если цифровой
      if (bitRead(AUX3,0)==0)  // вход
        pinMode(IO2,INPUT);
      else // выход
        pinMode(IO2,OUTPUT);
    }   
  }
  // P4
  if (bitRead(AUX4,3)==0){ //если не шина данных
    if (bitRead(AUX4,1)==0) // если аналоговый
      pinMode(AI3,INPUT);
    else { // если цифровой
      if (bitRead(AUX4,0)==0)  // вход
        pinMode(IO3,INPUT);
      else // выход
        pinMode(IO3,OUTPUT);
    }   
  }
}
void PortsControl() { // Управление состоянием портов
  if ((bitRead(AUX1,0)==1)&&(bitRead(AUX1,2)==1)&&(bitRead(AUX1,3)==0)){
    if (bitRead(AUX1,1)==1) digitalWrite(IO0,ports[0]);
    else analogWrite(IO0,ports[0]);
  }
  if ((bitRead(AUX2,0)==1)&&(bitRead(AUX2,2)==1)&&(bitRead(AUX2,3)==0)){
    if (bitRead(AUX2,1)==1) digitalWrite(IO1,ports[1]);
    else analogWrite(IO1,ports[1]);
  }
  if ((bitRead(AUX3,0)==1)&&(bitRead(AUX3,2)==1)&&(bitRead(AUX3,3)==0)){
    if (bitRead(AUX3,1)==1) digitalWrite(IO2,ports[2]);
    else analogWrite(IO2,ports[2]);
  }
  if ((bitRead(AUX4,0)==1)&&(bitRead(AUX4,2)==1)&&(bitRead(AUX4,3)==0)){
    if (bitRead(AUX4,1)==1) digitalWrite(IO3,ports[3]);
    else analogWrite(IO3,ports[3]);
  }
}
void loop() {
  SerialCommand();
  PortsControl();
  if ((millis()-ADC_t >=Td)&&(mode==1)) {
    SendData();
    ADC_t = millis(); 
  }  
}
