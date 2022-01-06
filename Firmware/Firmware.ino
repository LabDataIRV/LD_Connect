#define VER 1
#define SPI_CLK  15
#define SPI_MISO 14
#define SPI_MOSI 16
#define ADC_CS   7
#define ADC_DRDY 2
#define AI0 A0
#define AI1 A1
#define AI2 A2
#define AI3 A3
#define IO0 18
#define IO1 19
#define IO2 20
#define IO3 21

#include <SPI.h>
#include "Protocentral_ADS1220.h"
#define VREF         2.048            // Internal reference of 2.048V
#define VFSR         VREF/PGA
#define FULL_SCALE   (((long int)1<<23)-1)
//byte PGA = PGA_GAIN_1; // Programmable Gain = 1,2,4,8,16,32,64,128
//byte FIR = FIR_OFF; // FIR_OFF, FIR_5060, FIR_50Hz, FIR_60Hz
Protocentral_ADS1220 ads1220;
//int32_t adc_data;
//uint8_t  Td = 10;
uint32_t ADC_t = 0;
uint32_t dt = 0;
byte ports[] = {0,0,0,0}; 
#include <avr/eeprom.h>
byte EEMEM aСH1; byte CH1;
byte EEMEM aСH2; byte CH2;
byte EEMEM aСH3; byte CH3;
byte EEMEM aСH4; byte CH4;
byte EEMEM aAUX1; byte AUX1;
byte EEMEM aAUX2; byte AUX2;
byte EEMEM aAUX3; byte AUX3;
byte EEMEM aAUX4; byte AUX4;
word EEMEM aTd;   word Td;
byte EEMEM aG;    byte G;
byte EEMEM aREG;  byte REG;
byte EEMEM aREF;  byte REF;

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
  REF = 0; // 5 D
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
  if (bitRead(tAUX1,0) == 1)||(bitRead(tAUX1,1) == 1)||(bitRead(tAUX1,4) == 1) error = 1;
  if (bitRead(tAUX2,0) == 1)||(bitRead(tAUX2,1) == 1)||(bitRead(tAUX2,4) == 1) error = 1;
  if (bitRead(tAUX3,0) == 1)||(bitRead(tAUX3,1) == 1)||(bitRead(tAUX3,4) == 1) error = 1;
  if (bitRead(tAUX4,0) == 1)||(bitRead(tAUX4,1) == 1)||(bitRead(tAUX4,4) == 1) error = 1;
  if (tTd < 1)) error = 1;
  if ((tREF < 0)||(tREF > 1)) error = 1;
  if (!((tG == 1)||(tG == 2)||(tG == 4)||(tG == 8)||(tG == 16)||(tG == 32)||(tG == 64)||(tG == 128))) error = 1; //1,2,4,8,16,32,64,128
  if (error) { // записываем по умолчанию
    SaveConfig();
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
byte mode =0;
/* Режимы работы
 * 0 - Ожидание команд
 * 1 - Регистрация
 */
void setup() {
  default_settings();
  Serial.begin(9600);
  //while (!Serial) {
  //  ; // wait for serial port to connect. Needed for native USB port only
  //}
  checkEEPROM(); // проверка корректности памяти устройства
  ADCinit();
}

void loop() {

//SINGLE-ENDED INPUTS 
//MUX_SE_CH0 
//MUX_SE_CH1 
//MUX_SE_CH2 
//MUX_SE_CH3
  if (Serial.available()){
    char CMD = Serial.read();
    if (CMD == 'M') { Текущий режим
      Serial.print(mode);
    }
    if (CMD == 'R') { Состояние регистров
      Serial.print("CH1");
      Serial.print(bitRead(CH1,0));
      Serial.print(bitRead(CH1,1));
      Serial.print(bitRead(CH1,2));
      Serial.print(bitRead(CH1,3));
      Serial.print("CH2");
      Serial.print(bitRead(CH2,0));
      Serial.print(bitRead(CH2,1));
      Serial.print(bitRead(CH2,2));
      Serial.print(bitRead(CH2,3));
      Serial.print("CH3");
      Serial.print(bitRead(CH3,0));
      Serial.print(bitRead(CH3,1));
      Serial.print(bitRead(CH3,2));
      Serial.print(bitRead(CH3,3));
      Serial.print("CH4");
      Serial.print(bitRead(CH4,0));
      Serial.print(bitRead(CH4,1));
      Serial.print(bitRead(CH4,2));
      Serial.print(bitRead(CH4,3));
      Serial.print("AUX1");
      Serial.print(bitRead(AUX1,0));
      Serial.print(bitRead(AUX1,1));
      Serial.print(bitRead(AUX1,2));
      Serial.print(bitRead(AUX1,3));
      Serial.print("AUX2");
      Serial.print(bitRead(AUX2,0));
      Serial.print(bitRead(AUX2,1));
      Serial.print(bitRead(AUX2,2));
      Serial.print(bitRead(AUX2,3));
      Serial.print("AUX3");
      Serial.print(bitRead(AUX3,0));
      Serial.print(bitRead(AUX3,1));
      Serial.print(bitRead(AUX3,2));
      Serial.print(bitRead(AUX3,3));
      Serial.print("AUX4");
      Serial.print(bitRead(AUX4,0));
      Serial.print(bitRead(AUX4,1));
      Serial.print(bitRead(AUX4,2));
      Serial.print(bitRead(AUX4,3));
      Serial.print("Td");
      Serial.print(Td);
      Serial.print("G");
      Serial.print(G);
      Serial.print("REG");
      Serial.print(REG);
      Serial.print("REF");
      Serial.print(REF);
    }
    if (CMD == 'G') { // Запуск регистрации
      mode = 1;
      ads1220.Start_Conv();
      Serial.print(mode);
    }
    if (CMD == 'H') { // Остановка регистрации
      mode = 0;
      ads1220.Reset();
      ADCinit();
      Serial.print(mode);
    }
    if (CMD == 'S') { // Остановка регистрации
      mode = 0;
      ads1220.Reset();
      ADCinit();
      Serial.print(mode);
    }
    if (CMD == 'C') { // Настройка параметров
       char CMD = Serial.read(); //Тип параметра
       if (CMD == 'A'){ // Вход АЦП
        
       }
       if (CMD == 'P'){ // Вход Ардуино
        
       }
       if (CMD == 'T'){ // Период дискретизации
        
       }
       if (CMD == 'G'){ // Вход усиление
        
       }
       if (CMD == 'F'){ // Опорное напряжение
        
       }
       if (CMD == 'M'){ // Автостарт
         
       }     
    }
    if (CMD == 'P') { // Задаем порты
       char CMD = Serial.read(); //Номер порта
       byte Val = Serial.parseInt(); //Номер порта
       if (CMD == '1') ports[0] = Val; // Порт P1
       if (CMD == '2') ports[1] = Val; // Порт P2
       if (CMD == '3') ports[2] = Val; // Порт P3
       if (CMD == '4') ports[3] = Val; // Порт P4 
    }
    
  }
  if ((millis()-ADC_t >=Td)&&(mode==1)) {
    dt = micros();
    ads1220.select_mux_channels(MUX_SE_CH0);
    //while ((digitalRead(ADC_DRDY)) == HIGH){ ;  }
    //int32_t adc_data=ads1220.Read_WaitForData();
    int32_t adc_data=ads1220.Read_SingleShot_SingleEnded_WaitForData(MUX_SE_CH0);
    Serial.print(micros()-dt);
    //Serial.print(adc_data);
    Serial.print(",");
    ads1220.select_mux_channels(MUX_SE_CH1);
    //while ((digitalRead(ADC_DRDY)) == HIGH){ ;  }
    //adc_data=ads1220.Read_WaitForData();
    adc_data=ads1220.Read_SingleShot_SingleEnded_WaitForData(MUX_SE_CH1);
    Serial.print(micros()-dt);
    //Serial.print(adc_data);
    Serial.print(",");
    ads1220.select_mux_channels(MUX_SE_CH2);
    //while ((digitalRead(ADC_DRDY)) == HIGH){ ;  }
    //adc_data=ads1220.Read_WaitForData();
    adc_data=ads1220.Read_SingleShot_SingleEnded_WaitForData(MUX_SE_CH2);
    Serial.print(micros()-dt);
    //Serial.print(adc_data);
    Serial.print(",");
    ads1220.select_mux_channels(MUX_SE_CH3);
    //while ((digitalRead(ADC_DRDY)) == HIGH){ ;  }
    //adc_data=ads1220.Read_WaitForData();
    adc_data=ads1220.Read_SingleShot_SingleEnded_WaitForData(MUX_SE_CH3);
    Serial.println(micros()-dt); 
    //Serial.print(adc_data);
    /*Serial.print(",");
    adc_data = analogRead(AI0);
    adc_data = map(adc_data,0, 1023,0,FULL_SCALE);
    Serial.print(adc_data);
    Serial.print(",");
    adc_data = analogRead(AI1);
    adc_data = map(adc_data,0, 1023,0,FULL_SCALE);
    Serial.print(adc_data);
    Serial.print(",");
    adc_data = analogRead(AI2);
    adc_data = map(adc_data,0, 1023,0,FULL_SCALE);
    Serial.print(adc_data);
    Serial.print(",");
    adc_data = analogRead(AI3);
    adc_data = map(adc_data,0, 1023,0,FULL_SCALE);
    Serial.print(adc_data);
    */
    Serial.println();
    ADC_t = millis(); 
  }
    
}
