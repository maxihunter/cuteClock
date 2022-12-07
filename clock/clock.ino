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

const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
const byte sunrise_prerun[] = { /*15*/ 5, 30, 60 };
const byte sunrise_multip[] = { /*17*/ 50, 8, 4 };

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
void handleAlarm(void);

int readBtnStatus(void);
int (*ledAction[16])(void);

tmElements_t tm;

int eepromAddr = 0;
byte oper_mode = 0; // 0 - normal mode
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
  hourAlarmMode = 12;
  night_hours = 21;
  alarm_hours = 8;
  alarm_minutes = 0;
  //ledMode = 3;

  int alarm_time = 0;

  //EEPROM.write(eepromAddr,1);
  //alarmMode = (hourAlarmMode >> 2);

  /* // get the date and time the compiler was run
  if (getDate(__DATE__) && getTime(__TIME__)) {
    if (RTC.write(tm)) {
      // error !!! config = true;
    }
  } */

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

  //if (1/*digitalRead(BTN_CFG_BAT) == LOW*/) {
  if (digitalRead(BTN_CFG_BAT) == LOW) {
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
}
void loop() {

  static byte dots_counter = 0;
  static byte value_set = 0;

  //BTN handling
  short btn_bat = !digitalRead(BTN_CFG_BAT);
  short btn_snooze = !digitalRead(BTN_SNOOZE);
  short btn_mode = !digitalRead(BTN_MODE);
  short btn_set = !digitalRead(BTN_SET);
  delay(1);
  //Serial.println(ledMode);
  if (btn_mode) {
    counter = 0;
    if (!oper_mode || oper_mode == m_end) {
      oper_mode = m_hour_set;
      value_set = tm.Hour;
    } else {
      oper_mode++;
    }
    if (oper_mode == m_minutes_set) {
      value_set = tm.Minute;
    }
  }
  if (btn_bat) {
    oper_mode = m_batt_check;
  }
  if (btn_set && oper_mode != m_idle) {
    value_set++;
    if ((oper_mode == m_hour_set && value_set > 23) ||
        (oper_mode == m_minutes_set && value_set > 59)) {
      value_set = 0;
    }
    counter = 0;
  }
  if (counter > 30 && oper_mode != m_idle) {
    Serial.println("MODE OFF");
    oper_mode = m_idle;
  }

  counter += 1;
  dots_counter += 1;
  
  alarm_time = (alarm_hours * 60) + alarm_minutes;
  int curr_minutes = (tm.Hour * 60) + tm.Minute;

  if (alarm_time == curr_minutes && !alarmState) {
    alarmState = 2;
  }
  if (btn_snooze && alarmState == 2) {
    alarmMode = 0;
  }
  if (alarmMode) {
    handleAlarm();
  }
  // if ledMode is 0 - leds are disabled
  if (alarmMode && (night_hours <= tm.Hour || alarm_time >= curr_minutes)) {
    stripSunrisemode(alarmMode, curr_minutes, alarm_time);
    strip.show();
  } else if (ledMode) {
    (*ledAction[ledMode - 1])();
    strip.show();
  }

  /*if (digitalRead(BTN_SNOOZE) == LOW) {
    if (alarmState == 2) {
      alarmState = 1;
      alarm_minutes += 10;
      if (alarm_minutes > 59) {
        alarm_minutes -= 60;
        alarm_hours++;
        if (alarm_hours > 23) {
          alarm_hours = 0;
        }
      }
    }
  }*/
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

  if (oper_mode == m_hour_set) {
    if (dots_counter%2) {
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
    if (dots_counter%2) {
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
        Serial.println("The DS1307 is stopped.  Please run the SetTime");
      } else {
        Serial.println("DS1307 read error!  Please check the circuitry.");
      }
    }
  }
}

int readBtnStatus(void) {
  int btn = !digitalRead(BTN_CFG_BAT);
  btn |= (!digitalRead(BTN_CFG_BAT) < 1);
  #define BTN_SET 9
#define BTN_MODE 10
#define BTN_SNOOZE 11
#define BTN_CFG_BAT 12
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

void stripStaticCustom(void){
  for (byte i = 0; i < NUMLEDS; i++) {
    leds[i] = mHSV(hsvMode, 255, 255);
  }
};
void stripPulseCustom(void){
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
    leds[i] = mRGB(255, 0, 0);
  }
}

void stripSunrisemode(byte alarmMode, int curr_minutes, int alarm_time) {
  /*short minute = tm.Minute;
  short hour = tm.Hour;
  int current_time = (hour*60)+(minute);
  int alarm_time = (alarm_hours*60)+(alarm_minutes);*/

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

void handleAlarm(void) {
  // run alarmMode sound;;;
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

void configMode(void) {
  Serial.println("OK");
  char incomingByte = 0;
  String input;
  while(1) {
    if (Serial.available() > 0) {  //если есть доступные данные
        // считываем байт
        input = Serial.readString();
		if (input.startsWith("T")) {
			Serial.println("TESTOK");
			continue;
		}
		if (input.startsWith("t=")) {
			if (!(RTC.read(tm))) {
				Serial.println("Error");
				continue;
			}
			if (!getTime(input.c_str()+2)) {
				Serial.println("Error");
				continue;
			}
			if (RTC.write(tm)) {
				Serial.println("Error");
				continue;
			}
			Serial.println("OK");
			continue;
		}
		if (input.startsWith("d=")) {
			if (!(RTC.read(tm))) {
				Serial.println("Error");
				continue;
			}
			if (!getDate(input.c_str()+2)) {
				Serial.println("Error");
				continue;
			}
			if (RTC.write(tm)) {
				Serial.println("Error");
				continue;
			}
			Serial.println("OK");
			continue;
		}
		if (input.startsWith("l=") && input.length() > 3) {
			byte led_m = input.c_str()[2];
			EEPROM.write(eepromAddr, led_m-48);
			Serial.println("OK");
			continue;
		}
		if (input.startsWith("l=") && input.length() > 3) {
			byte led_m = input.c_str()[2];
			EEPROM.write(eepromAddr, led_m);
			Serial.println("OK");
			continue;
		}
		if (input.startsWith("h=") && input.length() > 5) {
			char* led_m = &(input.c_str()[2]);
			byte hsv = atoi(led_m);
			EEPROM.write(eepromAddr+1, led_m);
			Serial.println("OK");
			continue;
		}
		
/*byte ledMode = 0;
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
byte counter = 0;
byte alarmMode = 0;*/
        Serial.println("unknown command");
    }
  }
      //Serial.print("Ok, getTime at counter = ");
      //Serial.print(counter);
      //Serial.println();
      //Serial.print("Ok, getTime at counter = ");
      //Serial.print(counter);
      //Serial.println();
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
}
