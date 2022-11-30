#include <EEPROM.h>
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <TM1637.h>

const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

tmElements_t tm;

int eepromAddr = 0;
int ledMode = 0;

// пример работы с лентой
#define LED_PIN 4   // пин ленты
#define NUMLEDS 12  // кол-во светодиодов

#define ORDER_GRB  // порядок цветов ORDER_GRB / ORDER_RGB / ORDER_BRG

#define COLOR_DEBTH 2  // цветовая глубина: 1, 2, 3 (в байтах)
// на меньшем цветовом разрешении скетч будет занимать в разы меньше места,
// но уменьшится и количество оттенков и уровней яркости!

// ВНИМАНИЕ! define настройки (ORDER_GRB и COLOR_DEBTH) делаются до подключения библиотеки!
#include <microLED.h>

LEDdata leds[NUMLEDS];                   // буфер ленты типа LEDdata (размер зависит от COLOR_DEBTH)
microLED strip(leds, NUMLEDS, LED_PIN);  // объект лента

int8_t DispMSG[] = { 1, 2, 3, 4 };  // Настройка символов для последующего вывода на дислей
//Определяем пины для подключения к плате Arduino
#define CLK 2
#define DIO 3
//Создаём объект класса TM1637, в качестве параметров
//передаём номера пинов подключения
TM1637 tm1637(CLK, DIO);
void setup() {
  ledMode = EEPROM.read(eepromAddr);
  ledMode = 6;
  Serial.begin(9600);
  while (!Serial) ; // wait for Arduino Serial Monitor
  delay(200);
  //EEPROM.write(eepromAddr,1);
  /*bool parse=false;
  bool config=false;

  // get the date and time the compiler was run
  if (getDate(__DATE__) && getTime(__TIME__)) {
    parse = true;
    // and configure the RTC with this info
    if (RTC.write(tm)) {
      config = true;
    }
  }

  if (parse && config) {
    Serial.print("DS1307 configured Time=");
    Serial.print(__TIME__);
    Serial.print(", Date=");
    Serial.println(__DATE__);
  } else if (parse) {
    Serial.println("DS1307 Communication Error :-{");
    Serial.println("Please check your circuitry");
  } else {
    Serial.print("Could not parse info from the compiler, Time=\"");
    Serial.print(__TIME__);
    Serial.print("\", Date=\"");
    Serial.print(__DATE__);
    Serial.println("\"");
  }*/



  strip.setBrightness(20);  // яркость (0-255)
  // яркость применяется при выводе .show() !

  strip.clear();  // очищает буфер
  // применяется при выводе .show() !
  strip.show();  // выводим изменения на ленту

  //Инициализация модуля
  tm1637.init();
  //Установка яркости горения сегментов
  /*
     BRIGHT_TYPICAL = 2 Средний
     BRIGHT_DARKEST = 0 Тёмный
     BRIGHTEST = 7      Яркий
  */
  tm1637.set(5);
  tm1637.display(DispMSG);
}
void loop() {
  static byte counter = 0;
  static byte dots_counter = 0;

  //Задержка
  delay(1);

  counter += 1;
  dots_counter += 1;
  // if ledMode is 0 - leds are disabled
  if (ledMode) {
    fsmTick(counter);
    strip.show();
  }

#if 0
  if (DispMSG[3] == 9) {
    DispMSG[3] = 0;
    if (DispMSG[2] == 5) {
      DispMSG[2] = 0;
        if (DispMSG[1] == 9 && DispMSG[0] < 2) {
          DispMSG[1] = 0;
          DispMSG[0] += 1;
        } else if (DispMSG[1] == 3 && DispMSG[0] == 2) {
          DispMSG[1] = 0;
          DispMSG[0] = 0;
        } else {
          DispMSG[1] += 1;
        }
    } else {
      DispMSG[2] += 1;
    }
  } else {
    DispMSG[3] += 1;
  }
#endif
  if (dots_counter > 250) {
    dots_counter = 0;
    if (RTC.read(tm)) {

      tm1637.point(tm.Second % 2);
      DispMSG[3] = tm.Minute % 10;
      if (tm.Minute > 9) {
        DispMSG[2] = tm.Minute / 10;
      } else {
        DispMSG[2] = 0;
      }
      DispMSG[1] = tm.Hour % 10;
      if (tm.Hour > 9) {
        DispMSG[0] = tm.Hour / 10;
      } else {
        DispMSG[0] = 0;
      }
      tm1637.display(DispMSG);

    Serial.print("Ok, Time = ");
    print2digits(ledMode);
    Serial.println();
    /*Serial.write(':');
    print2digits(tm.Minute);
    Serial.write(':');
    print2digits(tm.Second);
    Serial.print(", Date (D/M/Y) = ");
    Serial.print(tm.Day);
    Serial.write('/');
    Serial.print(tm.Month);
    Serial.write('/');
    Serial.print(tmYearToCalendar(tm.Year));
    Serial.println();*/
    } else {
      if (RTC.chipPresent()) {
        //Serial.println("The DS1307 is stopped.  Please run the SetTime");
      } else {
        //Serial.println("DS1307 read error!  Please check the circuitry.");
      }
    }
  }
}

void fsmTick(const short counter) {
  short second = tm.Second/5;
  short minute = tm.Minute/5;
  short hour = tm.Hour < 12 ? tm.Hour : tm.Hour-12;;
  short minute_c;
  short hour_c;
  if (ledMode < 6) {
  switch (ledMode) {
    case 1:  // Rolling rainbow
      for (byte i = 0; i < NUMLEDS; i++) {
        //strip.setHSV(i, counter + i * (255 / NUMLEDS), 255, 255);
        leds[i] = mHSV(counter + i * (255 / (NUMLEDS)), 255, 255);
      }
      break;
    case 2:  // Static rainbow
      for (byte i = 0; i < NUMLEDS; i++) {
        leds[i] = mHSV(counter + (255 / (NUMLEDS)), 255, 255);
      }
      break;
    case 3:  // Dots Arrow
      for (byte i = 0; i < NUMLEDS; i++) {
        leds[i] = mRGB(0, 0, 0);
      }
      if (minute == hour) {
        leds[minute] = mRGB(255, 0, 255);
      } else {
        leds[minute] = mRGB(255, 0, 0);
        leds[hour] = mRGB(0, 0, 255);
      }
      break;
    case 4:  // Line Arrow
      minute_c = tm.Minute/5;
      hour_c = 0;
      for (int i = 0; i < NUMLEDS; i++) {
        minute_c = minute > i ? 255 : 0;
        hour_c = hour > i ? 255 : 0;

        leds[i] = mRGB(minute_c, 0, hour_c);
      }
      break;
    case 5:  // Line overlaped Arrow
      minute_c = tm.Minute/5;
      hour_c = 0;
      short maximum = minute > hour ? minute : hour;
      
      for (int i = 0; i < NUMLEDS; i++) {
        /*if (minute > hour) {
          minute_c = 0;
          hour_c = 255;
        } else {
          minute_c = 0;
          hour_c = 255;
        }*/
        if (maximum > i) {
          leds[i] = mRGB((hour_c-minute_c) < i ? 0: 255 *(hour_c-minute_c) , 0, (minute_c-hour_c) < i? 255 *(minute_c-hour_c):0);
        } else {
          leds[i] = mRGB(0, 0, 0);
        }
      }
      break;
    case 7:  // Line seconds
      for (int i = 0; i < NUMLEDS; i++) {
        if (i) {
          leds[i] = mRGB(255, 255, 255);
        } else {
          leds[i] = mRGB(0, 0, 0);
        }
      }
      break;
    case 30:  // Static RED
      for (int i = 0; i < NUMLEDS; i++) {
        leds[i] = mRGB(255, 0, 0);
      }
      break;
  }
  } else {
    switch (ledMode) {
    case 6:  // Line seconds
      for (int i = 0; i < NUMLEDS; i++) {
        if (i) {
          leds[i] = mRGB(255, 255, 255);
        } else {
          leds[i] = mRGB(0, 0, 0);
        }
      }
      break;
    case 30:  // Static RED
      for (int i = 0; i <= NUMLEDS; i++) {
        leds[i] = mRGB(255, 0, 0);
      }
      break;
  }
  }

}

bool getTime(const char *str) {
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

bool getDate(const char *str) {
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}
