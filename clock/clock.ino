#include <TM1637.h>

// пример работы с лентой
#define LED_PIN 4       // пин ленты
#define NUMLEDS 12       // кол-во светодиодов

#define ORDER_GRB       // порядок цветов ORDER_GRB / ORDER_RGB / ORDER_BRG

#define COLOR_DEBTH 2   // цветовая глубина: 1, 2, 3 (в байтах)
// на меньшем цветовом разрешении скетч будет занимать в разы меньше места,
// но уменьшится и количество оттенков и уровней яркости!

// ВНИМАНИЕ! define настройки (ORDER_GRB и COLOR_DEBTH) делаются до подключения библиотеки!
#include <microLED.h>

LEDdata leds[NUMLEDS];  // буфер ленты типа LEDdata (размер зависит от COLOR_DEBTH)
microLED strip(leds, NUMLEDS, LED_PIN);  // объект лента

int8_t DispMSG[] = {1, 2, 3, 4};   // Настройка символов для последующего вывода на дислей
int8_t is_dots = 0;

#define CLK 2
#define DIO 3

TM1637 tm1637(CLK, DIO);

int8_t mode = 2;

void setup()
{
  strip.setBrightness(30);    // яркость (0-255)
  // яркость применяется при выводе .show() !

  strip.clear();   // очищает буфер
  // применяется при выводе .show() !
  
  strip.show(); // выводим изменения на ленту
  
  //Инициализация модуля
  tm1637.init();
  //Установка яркости горения сегментов
  /*
     BRIGHT_TYPICAL = 2 Средний
     BRIGHT_DARKEST = 0 Тёмный
     BRIGHTEST = 7      Яркий
  */
  tm1637.set(6);
}

void loop()
{
  static byte counter = 0;
  tm1637.point(is_dots ? true : false);
  tm1637.display(DispMSG);

  if (mode == 0)
    for (byte i = 0; i < NUMLEDS; i++) {
      //strip.setHSV(i, counter + i * (255 / NUMLEDS), 255, 255);  // можно так
      leds[i] = mHSV(counter + i * (255 / NUMLEDS), 255, 255); // или в стиле fastLED
    }
  else if (mode == 1) 
    for (byte i = 0; i < NUMLEDS; i++) {
      //strip.setHSV(i, counter + i * (255 / NUMLEDS), 255, 255);  // можно так
      leds[i] = mHSV(255,counter + i * (255 / NUMLEDS), 255); // или в стиле fastLED
    }
    else if (mode == 2) 
    for (byte i = 0; i < NUMLEDS; i++) {
      if (i < 2 )
        continue;
      //strip.setHSV(i, counter + i * (255 / NUMLEDS), 255, 255);  // можно так
      leds[i] = mHSV(counter, 255, (counter + i * (200 / NUMLEDS))); // или в стиле fastLED
    }
  counter += 1;
  strip.show();
  
}
