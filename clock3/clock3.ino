#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> 

#include <DS3231.h>

#include <Wire.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit)) 
#endif

// Init the DS3231 using the hardware interface
DS3231  rtc(SDA, SCL);

Adafruit_MPR121 cap = Adafruit_MPR121();

uint16_t lasttouched = 0;
uint16_t currtouched = 0;

bool isLedOn = false, firstIn = true, waiting = false, waiting2 = false;
unsigned long trigger_time, trigger_time2;
int pir = 49, ir = 53; 
bool isClockOn = false;
unsigned long clockTimer, clockOnDuration = 120000;
int ledBrightness = 255;
int ledPin = 12, lcd_power = 51, lcd_status = 0;

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4

// Agar warna mudah dimengerti (Human Readable color):
#define TFT_BLACK   0x0000
#define TFT_BLUE    0x001F
#define TFT_RED     0xF800
#define TFT_GREEN   0x07E0
#define TFT_CYAN    0x07FF
#define TFT_MAGENTA 0xF81F
#define TFT_YELLOW  0xFFE0
#define TFT_WHITE   0xFFFF
#define TFT_GREY    0x5AEB

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

float sx = 0, sy = 1, mx = 1, my = 0, hx = -1, hy = 0;    
float sdeg = 0, mdeg = 0, hdeg = 0;
uint16_t osx = 120, osy = 120, omx = 120, omy = 120, ohx = 120, ohy = 120; 
int16_t x0 = 0, x1 = 0, yy0 = 0, yy1 = 0, x00 = 0, yy00 = 0;
uint32_t targetTime = 0, timer = 0;                    

uint16_t xpos; 
uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';
}
uint8_t hh = 0, mm = 42, ss = 0; 
boolean initial = 1;
char d;

void setup(void) {
   Serial.begin(9600);
    if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 found!");
  pinMode(ir, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(pir, INPUT);
  pinMode(lcd_power, OUTPUT);
  
  digitalWrite(lcd_power, HIGH);
  delay(1000);
  tft.reset();  
  tft.begin(0x9341); 
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);// background color
  tft.setCursor(50, 200);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(6);
  tft.print("RAW PRO");
  tft.setCursor(130, 400);
  tft.setTextSize(2);
  tft.print("Made in");
  tft.setCursor(100, 420);
  tft.print("Saudi Arabia");
  delay(10000);
  digitalWrite(lcd_power, HIGH);
  lcd_status = 1;
  isClockOn = true;
  clockTimer = millis();
  initializeClock();
}

void loop() {
    
  touchLogic();
  
  if(isClockOn)
  {
    if(millis() - clockTimer < clockOnDuration) // Here insert Mosfet Control High digital write High
    {
      clockLogic();
    }
    else // digital write LOW
    {
      lcd_status = 0;
      isClockOn = false;
      tft.fillScreen(TFT_BLACK);// background color
    }
    
  }
  else
  {
    digitalWrite(lcd_power, LOW);
    if (digitalRead(pir) == HIGH) 
    { 
      digitalWrite(lcd_power, HIGH);
      lcd_status = 1;
      isClockOn = true;
      clockTimer = millis();
      initializeClock();
    }
  }
  
  LEDStripLogic();
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void LEDStripLogic()
{
  if(waiting)
  {
    if(millis() - trigger_time >= 20)
    {
      waiting = false;
      if(firstIn)
      {
        isLedOn = !isLedOn;
        firstIn = false;
      }
    }
  }
  else
  {
     if(digitalRead(ir) == 0)
    {
      trigger_time = millis();
      waiting = true;
    }
  }

  if(waiting2)
  {
    if(millis() - trigger_time2 >= 20)
    {
      waiting2 = false;
      firstIn = true;
    }
  }
  else
  {
     if(digitalRead(ir) == 1)
    {
      trigger_time2 = millis();
      waiting2 = true;
    }
  }

  if(isLedOn)
  {
    analogWrite(ledPin, ledBrightness);
  }
  else
  {
    analogWrite(ledPin, 0);
  }
}

void clockLogic()
{ 
  tft.setCursor(xpos-60, 50);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.print(rtc.getDateStr()); 
  
  if (targetTime < millis()) {
    targetTime = millis() + 1000;
    ss++;              
    if (ss == 60) {
      ss = 0;
      mm++;            
      if (mm > 59) {
        mm = 0;
        hh++;          
        if (hh > 23) {
          hh = 0;
        }
      }
    }
   
    sdeg = ss * 6;                     // 0-59 -> 0-354
    mdeg = mm * 6 + sdeg * 0.01666667; // 0-59 -> 0-360 - includes seconds, but these increments are not used
    hdeg = hh * 30 + mdeg * 0.0833333; // 0-11 -> 0-360 - includes minutes and seconds, but these increments are not used
    hx = cos((hdeg - 90) * 0.0174532925);
    hy = sin((hdeg - 90) * 0.0174532925);
    mx = cos((mdeg - 90) * 0.0174532925);
    my = sin((mdeg - 90) * 0.0174532925);
    sx = cos((sdeg - 90) * 0.0174532925);
    sy = sin((sdeg - 90) * 0.0174532925);

    if (ss == 0 || initial) {
      timer = millis();
      initial = 0;
      // Erase hour and minute hand positions every minute
      tft.drawTriangle(ohx, ohy, xpos - 5, tft.height()/2 - 5, xpos + 5, tft.height()/2 + 5, TFT_BLACK);
      ohx = hx * 72 + xpos + 1;
      ohy = hy * 72 + tft.height()/2;
      tft.drawTriangle(omx, omy, xpos - 5, tft.height()/2 - 5, xpos + 5, tft.height()/2 + 5, TFT_BLACK);
      omx = mx * 94 + xpos;
      omy = my * 94 + tft.height()/2; 
    }

    // Redraw new hand positions, hour and minute hands not erased here to avoid flicker
    tft.drawLine(osx, osy, xpos, tft.height()/2, TFT_BLACK);
    osx = sx * 100 + xpos + 1;
    osy = sy * 100 + tft.height()/2;
    tft.drawLine(osx, osy, xpos, tft.height()/2, TFT_WHITE);
    tft.drawTriangle(ohx, ohy, xpos - 5, tft.height()/2 - 5, xpos + 5, tft.height()/2 + 5, TFT_WHITE);
    tft.drawTriangle(omx, omy, xpos - 5, tft.height()/2 - 5, xpos + 5, tft.height()/2 + 5, TFT_WHITE);
    tft.fillCircle(xpos, tft.height()/2, 5, TFT_WHITE); 
  }
}

void touchLogic()
{
  currtouched = cap.touched();
  
  for (uint8_t i=0; i<12; i++) {
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) {
      if(i == 0)
      {
        ledBrightness = 80;
      }
      else if(i == 1)
      {
        ledBrightness = 200;
      }
      else if(i == 2)
      {
        ledBrightness = 255;
      }
    }
  }

  // reset our state
  lasttouched = currtouched;
  delay(100);
}

void initializeClock()
{
  tft.reset();  
  tft.begin(0x9341); 
  tft.setRotation(2);
  tft.setTextColor(TFT_WHITE);// text color
  tft.fillScreen(TFT_BLACK);// background color
  delay(2000);

  xpos = tft.width() / 2; 
   
  for (int i = 0; i < 360; i += 30) {
    sx = cos((i - 90) * 0.0174532925);
    sy = sin((i - 90) * 0.0174532925);
    x0 = sx * 144 + xpos;
    yy0 = sy * 144 + tft.height()/2;
    x1 = sx * 130 + xpos;
    yy1 = sy * 130 + tft.height()/2;

    tft.setCursor(x0-5, yy0);
    tft.setTextSize(3);

    switch(i)
    {
      case 0:
        tft.print("12");
        break;
      case 30:
        tft.print("1");
        break;
      case 60:
        tft.print("2");
        break;
      case 90:
        tft.print("3");  
        break;
      case 120:
        tft.print("4");  
        break;
        case 150:
        tft.print("5");
        break;
      case 180:
        tft.print("6");
        break;
      case 210:
        tft.print("7");
        break;
      case 240:
        tft.print("8");  
        break;
      case 270:
        tft.print("9");  
        break;
       case 300:
        tft.print("10");
        break;
      case 330:
        tft.print("11");
        break;
    }

  }
  
  for (int i = 0; i < 360; i += 6) {
    sx = cos((i - 90) * 0.0174532925);
    sy = sin((i - 90) * 0.0174532925);
    x0 = sx * 102 + xpos;
    yy0 = sy * 102 + 120;
    x00 = sx * 92 + xpos;
    yy00 = sy * 92 + 120;
  }

  targetTime = millis() + 1000;

  // Initialize the rtc object
  rtc.begin();
  String hours = getValue(rtc.getTimeStr(), ':', 0);
  hh  = atoi(hours.c_str ()); 
  String minutes = getValue(rtc.getTimeStr(), ':', 1);
  mm  = atoi(minutes.c_str ()); 
  String seconds = getValue(rtc.getTimeStr(), ':', 2);
  ss  = atoi(seconds.c_str ()); 
}
