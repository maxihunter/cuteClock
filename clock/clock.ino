#include <EEPROM.h>
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <TM1637.h>

enum operation_mode {
  m_idle,
  m_batt_check,
  m_hour_set,
  m_minutes_set,
  m_end
};

#define CLOCK_VERSION "1.0"
#define CLOCK_REVISION "A"

#define RTC_STOPPED "RTC is stopped"
#define RTC_FAILURE "RTC read error"

const char* monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
const byte sunrise_prerun[] = { 5, 30, 60 };
const byte sunrise_multip[] = { 50, 8, 4 };

void configMode(void);

void stripRollingRainbow(void);
void stripStaticRainbow(void);
void stripCircleRainbow(void);
void stripPulseRainbow(void);
void stripArrowDots(void);
void stripArrowOverlap(void);
void stripArrowDotsSec(void);
void stripArrowOverlapSec(void);
void stripStaticCustom(void);
void stripPulseCustom(void);
void stripStaticRED(void);

void stripSunrisemode(byte alarmMode, int curr_minutes, int alarm_time);
void handleAlarm(int curr_minutes);

int readBtnStatus(void);
#define EFFECTS_SIZE 10
int (*ledAction[EFFECTS_SIZE])(void);

tmElements_t tm;

/* Configuration variables */
int eepromAddr = 0;
byte oper_mode = 0;  // 0 - normal mode
                     // 1 set hours mode
                     // 2 set minutes mode
byte ledMode = 0;
byte ledBrightness = 0;
byte hsvMode = 0;
// XXXX XXXX - byte data
// 3210
// 01 - normal alarm mode (0 - off, 1 - on LED, 2 - sound alarm)
// 32 - sunrise mode (0 - off, othen prerun: 1 - 15 minute; 2 - 30 minutes; 3 - 60 minutes)
byte hourAlarmMode = 0;
byte night_hours = 0;
byte alarm_hours = 0;
byte alarm_minutes = 0;
byte time_format = 0;

/* Runtime state variables */
byte counter = 0;
byte alarmMode = 0;
int alarm_time = 0;
byte alarmState = 0;


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

int8_t DispMSG[] = { 0, 5, 0, 0 };  // Настройка символов для последующего вывода на дислей
//Определяем пины для подключения к плате Arduino
#define CLK 2
#define DIO 3
//Создаём объект класса TM1637, в качестве параметров
//передаём номера пинов подключения

#define BTN_SET 9
#define BTN_MODE 10
#define BTN_SNOOZE 11
#define BTN_CFG_BAT 12
#define SPEAKER_PIN 6

short cfg_bat_btn = 1;

TM1637 tm1637(CLK, DIO);
void setup() {
  ledAction[0] = stripRollingRainbow;
  ledAction[1] = stripStaticRainbow;
  ledAction[2] = stripCircleRainbow;
  ledAction[3] = stripPulseRainbow;
  ledAction[4] = stripArrowDots;
  ledAction[5] = stripArrowOverlap;
  ledAction[6] = stripArrowDotsSec;
  ledAction[7] = stripArrowOverlapSec;
  ledAction[8] = stripStaticCustom;
  ledAction[9] = stripPulseCustom;

  ledMode = EEPROM.read(eepromAddr);
  hsvMode = EEPROM.read(eepromAddr + 1);
  hourAlarmMode = EEPROM.read(eepromAddr + 2);
  night_hours = EEPROM.read(eepromAddr + 3);
  alarm_hours = EEPROM.read(eepromAddr + 4);
  alarm_minutes = EEPROM.read(eepromAddr + 5);
  time_format = EEPROM.read(eepromAddr + 6);

  hourAlarmMode = 12;
  night_hours = 21;
  alarm_hours = 8;
  alarm_minutes = 0;
  ledMode = 3;

  int alarm_time = alarm_time = (alarm_hours * 60) + alarm_minutes;

  //EEPROM.write(eepromAddr,1);
  alarmMode = (hourAlarmMode >> 2);

  // get the date and time the compiler was run
  /*if (getDate(__DATE__) && getTime(__TIME__)) {
    if (RTC.write(tm)) {
      // error !!! config = true;
    }
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

  pinMode(BTN_CFG_BAT, INPUT_PULLUP);
  pinMode(BTN_SNOOZE, INPUT_PULLUP);
  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(BTN_SET, INPUT_PULLUP);
  RTC.read(tm);

  if (1 /*digitalRead(BTN_CFG_BAT) == LOW*/) {
    //if (digitalRead(BTN_CFG_BAT) == LOW) {
    // enter config mode
    Serial.begin(9600);
    while (!Serial)
      ;  // wait for Arduino Serial Monitor
    delay(200);
    for (int i = 0; i < NUMLEDS; i++) {
      leds[i] = mRGB(0, 0, 0);
    }
    leds[0] = mRGB(255, 0, 0);
    leds[1] = mRGB(255, 0, 0);
    leds[11] = mRGB(255, 0, 0);
    strip.show();
    DispMSG[0] = 0x10;
    DispMSG[1] = 0x10;
    DispMSG[2] = 0x10;
    DispMSG[3] = 0x10;
    tm1637.display(DispMSG);
    Serial.println("Ready");
    configMode();
  }
  Serial.begin(9600);
  while (!Serial)
    ;  // wait for Arduino Serial Monitor
  delay(200);
  Serial.println("Ready");
}

void loop() {
  // init local static variables for ticks counting
  static byte dots_counter = 0;
  static byte value_set = -1;
  counter += 1;
  dots_counter += 1;

  //BTN handling
  short btn_bat = !digitalRead(BTN_CFG_BAT);
  short btn_snooze = !digitalRead(BTN_SNOOZE);
  short btn_mode = !digitalRead(BTN_MODE);
  short btn_set = !digitalRead(BTN_SET);

  // Buttons logic block
  delay(1);
  if (btn_mode) {
    counter = 0;
    if (!oper_mode || oper_mode == m_end) {
      oper_mode = m_hour_set;
      if (value_set != -1) {
        tm.Minute = value_set;
      }
      value_set = tm.Hour;
    } else {
      oper_mode++;
    }
    if (oper_mode == m_minutes_set) {
      if (value_set != -1) {
        tm.Hour = value_set;
      }
      value_set = tm.Minute;
    }
  }
  if (btn_bat) {
    oper_mode = m_batt_check;
  }
  if (btn_set && oper_mode != m_idle) {
    value_set++;
    if ((oper_mode == m_hour_set && value_set > 23) || (oper_mode == m_minutes_set && value_set > 59)) {
      value_set = 0;
    }
    counter = 0;
  }
  // Quit setup clock mode if timeout was reached
  if (counter > 30 && oper_mode != m_idle) {
    Serial.println("MODE OFF");
    alarm_time = alarm_time = (alarm_hours * 60) + alarm_minutes;
    if (RTC.write(tm)) {
      Serial.println("RTC write error!");
    }
    oper_mode = m_idle;
  }
  // Stop active alarm if snooze button was pressed
  if (btn_snooze && alarmState == 2) {
    alarmState = 0;
    analogWrite(SPEAKER_PIN, 0);
  }

  // Hourly beep block
  if (alarmState == 2) {
    if (dots_counter % 2) {
      analogWrite(SPEAKER_PIN, 125);
    } else {
      analogWrite(SPEAKER_PIN, 0);
    }
    delay(100);
  }

  // Handle alarm
  int curr_minutes = (tm.Hour * 60) + tm.Minute;
  if (hourAlarmMode) {
    handleAlarm(curr_minutes);
  }
  // if ledMode is 0 - leds are disabled
  // Handle sunrise mode - adjust WHITE LED brightness
  // base on time to the alarm
  if (alarmMode && (night_hours <= tm.Hour || alarm_time >= curr_minutes)) {
    stripSunrisemode(alarmMode, curr_minutes, alarm_time);
    strip.show();
  } else if (ledMode && ledMode <= EFFECTS_SIZE) {
    (*ledAction[ledMode - 1])();
    strip.show();
  }

  // Show Battery power if battery button was pressed
  if (oper_mode == m_batt_check) {
    DispMSG[0] = 0x11;
    DispMSG[1] = 0x11;
    DispMSG[2] = 8;
    DispMSG[3] = 5;
    tm1637.display(DispMSG);
    delay(2000);
    oper_mode = m_idle;
    return;
  }

  // Show clock setup cycle if mode button was pressed
  if (oper_mode == m_hour_set) {
    DispMSG[3] = tm.Minute % 10;
    if (tm.Minute > 9) {
      DispMSG[2] = tm.Minute / 10;
    } else {
      DispMSG[2] = 0;
    }
    if (dots_counter % 2) {
      DispMSG[0] = 0x11;
      DispMSG[1] = 0x11;
    } else {
      DispMSG[1] = value_set % 10;
      if (value_set > 9) {
        DispMSG[0] = value_set / 10;
      } else {
        DispMSG[0] = 0;
      }
    }
    tm1637.display(DispMSG);
    delay(200);
    return;
  }
  if (oper_mode == m_minutes_set) {
    DispMSG[1] = tm.Hour % 10;
    if (tm.Hour > 9) {
      DispMSG[0] = tm.Hour / 10;
    } else {
      DispMSG[0] = 0;
    }
    if (dots_counter % 2) {
      DispMSG[2] = 0x11;
      DispMSG[3] = 0x11;
    } else {
      DispMSG[3] = value_set % 10;
      if (value_set > 9) {
        DispMSG[2] = value_set / 10;
      } else {
        DispMSG[2] = 0;
      }
    }
    tm1637.display(DispMSG);
    delay(200);
    return;
  }

  // TODO: Alarm setup cycle

  // Show clock time on LED display
  if (dots_counter > 20 && oper_mode == m_idle) {
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
    } else {
      if (RTC.chipPresent()) {
        Serial.println(RTC_STOPPED);
      } else {
        Serial.println(RTC_FAILURE);
      }
    }
  }
}

void stripRollingRainbow(void) {
  for (byte i = 0; i < NUMLEDS; i++) {
    //strip.setHSV(i, counter + i * (255 / NUMLEDS), 255, 255);
    leds[i] = mHSV(counter + i * (255 / (NUMLEDS)), 255, 255);
  }
}
void stripCircleRainbow(void) {
  static short direction = 0;
  static short color = 0;
  //short pref = direction ? direction - counter : counter;
  for (byte i = 0; i < NUMLEDS; i++) {
    if (i * 21 < direction) {
      leds[i] = mHSV(color * 43, 255, 255);
    } else {
      leds[i] = mRGB(0, 0, 0);
    }
  }
  direction++;
  if (counter == 255) {
    direction = 0;
    color++;
    if (color > 5) {
      color = 0;
    }
  }
}
void stripStaticRainbow(void) {
  for (byte i = 0; i < NUMLEDS; i++) {
    leds[i] = mHSV(counter + (255 / (NUMLEDS)), 255, 255);
  }
}
void stripPulseRainbow(void) {
  static short direction = 0;
  static short color = 0;
  short pref = direction ? direction - counter : counter;
  for (byte i = 0; i < NUMLEDS; i++) {
    leds[i] = mHSV(color * 43, 255, pref);
  }
  //Serial.println(pref);
  delay(1);
  if (counter == 255) {
    if (direction == 0) {
      direction = 255;
    } else {
      direction = 0;
      color++;
      if (color > 5) {
        color = 0;
      }
    }
  }
}
void stripArrowDots(void) {
  short minute = tm.Minute / 5;
  short hour = tm.Hour < 12 ? tm.Hour : tm.Hour - 12;
  ;
  for (byte i = 0; i < NUMLEDS; i++) {
    leds[i] = mRGB(0, 0, 0);
  }
  if (minute == hour) {
    leds[minute] = mRGB(255, 0, 255);
  } else {
    leds[minute] = mRGB(255, 0, 0);
    leds[hour] = mRGB(0, 0, 255);
  }
}
void stripArrowOverlap(void) {
  short second = tm.Second / 5;
  short minute = tm.Minute / 5;
  short hour = tm.Hour < 12 ? tm.Hour : tm.Hour - 12;
  ;
  short minute_c;
  short hour_c;
  minute_c = tm.Minute / 5;
  hour_c = 0;
  for (int i = 0; i < NUMLEDS; i++) {
    minute_c = minute > i ? 255 : 0;
    hour_c = hour > i ? 255 : 0;
    leds[i] = mRGB(minute_c, 0, hour_c);
  }
}
void stripArrowDotsSec(void) {
  short second = tm.Second / 5;
  for (int i = 0; i < NUMLEDS; i++) {
    if (i == second) {
      leds[i] = mRGB(255, 125, 125);
    } else {
      leds[i] = mRGB(0, 0, 0);
    }
  }
}
void stripArrowOverlapSec(void) {
  short second = tm.Second / 5;
  for (int i = 0; i < NUMLEDS; i++) {
    if (i <= second) {
      leds[i] = mRGB(255, 25, 25);
    } else {
      leds[i] = mRGB(0, 0, 0);
    }
  }
}

void stripStaticCustom(void) {
  for (byte i = 0; i < NUMLEDS; i++) {
    leds[i] = mHSV(hsvMode, 255, 255);
  }
};
void stripPulseCustom(void) {
  static short direction = 0;
  short pref = direction ? direction - counter : counter;
  for (byte i = 0; i < NUMLEDS; i++) {
    leds[i] = mHSV(hsvMode, 255, pref);
  }
  delay(1);
  if (counter == 255) {
    if (direction == 0) {
      direction = 255;
    } else {
      direction = 0;
    }
  }
};

void stripStaticRED(void) {
  for (int i = 0; i < NUMLEDS; i++) {
    leds[i] = mHSV(hsvMode, 255, 255);
  }
}

void stripSunrisemode(byte alarmMode, int curr_minutes, int alarm_time) {
  // 76 - sunrise mode (0 - off, othen prerun: 1 - 15 minute; 2 - 30 minutes; 3 - 60 minutes)
  // 76
  // const byte sunrise_prerun[] = {15,30,60};
  // const byte sunrise_multip[] = {17,8,4};
  if ((curr_minutes + sunrise_prerun[alarmMode - 1] > alarm_time) && (night_hours > tm.Hour)) {
    short light = (curr_minutes + sunrise_prerun[alarmMode - 1] - alarm_time) * sunrise_multip[alarmMode - 1];
    if (light > 255)
      light = 255;
    for (int i = 0; i < NUMLEDS; i++) {
      leds[i] = mRGB(light, light, light);
    }
  } else {
    for (int i = 0; i < NUMLEDS; i++) {
      leds[i] = mRGB(0, 0, 0);
    }
  }
}

void handleAlarm(int curr_minutes) {
  if (alarm_time == curr_minutes && !alarmState) {
    alarmState = 2;
  }
  if (tm.Minute == 0 && tm.Second == 0) {
    // TODO: do hourly alarm speaker or light
    if (alarmState == 0) {
      analogWrite(SPEAKER_PIN, 125);
      delay(100);
      analogWrite(SPEAKER_PIN, 160);
      delay(100);
      analogWrite(SPEAKER_PIN, 0);
      alarmState = 1;
    }
  } else {
    alarmState = 0;
  }
}

bool getTime(const char* str) {
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

bool getDate(const char* str) {
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

void configMode(void) {
  Serial.println("OK");
  char incomingByte = 0;
  String input;
  while (1) {
    if (Serial.available() > 0) {
      input = Serial.readString();
      if (input.startsWith("T")) {
        Serial.println("TESTOK");
        continue;
      }
      if (input.startsWith("V")) {
        Serial.println(CLOCK_VERSION "|" CLOCK_REVISION);
        continue;
      }
      if (input.startsWith("t=")) {
        if (!(RTC.read(tm))) {
          Serial.println("Error: RTC failed");
          continue;
        }
        if (!getTime(input.c_str() + 2)) {
          Serial.println("Error: Wrong format");
          continue;
        }
        if (RTC.write(tm)) {
          Serial.println("Error: RTC failed");
          continue;
        }
        Serial.println("OK");
        continue;
      }
      if (input.startsWith("d=")) {
        if (!(RTC.read(tm))) {
          Serial.println("Error: RTC failed");
          continue;
        }
        if (!getDate(input.c_str() + 2)) {
          Serial.println("Error: Wrong format");
          continue;
        }
        if (RTC.write(tm)) {
          Serial.println("Error: RTC failed");
          continue;
        }
        Serial.println("OK");
        continue;
      }
      // led mode
      if (input.startsWith("l=") && input.length() > 3) {
        char* val = input.c_str();
        int led = atoi(val + 2);
        EEPROM.write(eepromAddr, led);
        //Serial.print(led);
        Serial.println("OK");
        continue;
      }
      // HSV for custom color mode
      if (input.startsWith("h=") && input.length() > 3) {
        char* val = input.c_str();
        int hsv = atoi(val + 2);
        EEPROM.write(eepromAddr + 1, hsv);
        Serial.print(hsv);
        Serial.println("OK");
        continue;
      }
      // Hourly beep
      if (input.startsWith("b=") && input.length() > 3) {
        char* val = input.c_str();
        int beep = atoi(val + 2);
        EEPROM.write(eepromAddr + 2, beep);
        //Serial.print(led);
        Serial.println("OK");
        continue;
      }
      // night not disturb hours
      if (input.startsWith("n=") && input.length() > 3) {
        char* val = input.c_str();
        int h_alarm = atoi(val + 2);
        EEPROM.write(eepromAddr + 3, h_alarm);
        //Serial.print(led);
        Serial.println("OK");
        continue;
      }
      // alarm hours
      if (input.startsWith("A=") && input.length() > 3) {
        char* val = input.c_str();
        int al_h = atoi(val + 2);
        EEPROM.write(eepromAddr + 4, al_h);
        //Serial.print(led);
        Serial.println("OK");
        continue;
      }
      // alarm minutes
      if (input.startsWith("a=") && input.length() > 3) {
        char* val = input.c_str();
        int al_m = atoi(val + 2);
        EEPROM.write(eepromAddr + 5, al_m);
        //Serial.print(led);
        Serial.println("OK");
        continue;
      }
      // 24 or 12 clock format
      if (input.startsWith("m=") && input.length() > 3) {
        char* val = input.c_str();
        int time = atoi(val + 2);
        EEPROM.write(eepromAddr + 6, time);
        //Serial.print(led);
        Serial.println("OK");
        continue;
      }
      if (input.startsWith("R")) {
        RTC.read(tm);
        char buf[256] = {};
        snprintf(buf, 256, "ROK|l=%d;h=%d;b=%d;n=%d;A=%d;a=%d;m=%d;t=%d:%d:%d;d=%d.%d.%d;",
                 ledMode, hsvMode, hourAlarmMode, night_hours, alarm_hours, alarm_minutes,
                 time_format, tm.Hour, tm.Minute, tm.Second, tm.Day, tm.Month, tm.Year);
        Serial.println(buf);
        continue;
      }
      if (input.startsWith("r")) {
        if (!RTC.read(tm)) {
          if (RTC.chipPresent()) {
            Serial.println(RTC_STOPPED);
          } else {
            Serial.println(RTC_FAILURE);
          }
        } else {
          Serial.println("OK");
        }
        continue;
      }
      Serial.println("unknown command");
    }
  }
  /* char buff[128] = {};
  snprintf(buff, 128, "Now %d:%d:%d %d.%d.%d\n",
            tm.Hour, tm.Minute, tm.Second, tm.Day, tm.Month, tmYearToCalendar(tm.Year));
  Serial.println(buff);
  */
}
