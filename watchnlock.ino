#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>
#define USE_TFT_ESPI_LIBRARY
#include "lv_xiao_round_screen.h"

#include "I2C_BM8563.h"
// #include "NotoSansBold15.h"
#include "./assets.h"

I2C_BM8563 rtc(I2C_BM8563_DEFAULT_ADDRESS, Wire);
I2C_BM8563_TimeTypeDef timeStruct;
I2C_BM8563_DateTypeDef dateStruct;

TFT_eSprite face = TFT_eSprite(&tft);

#define COLOR_THEME   0xd6ba //
#define COLOR_MID 0x10A2
#define COLOR_LIGHT 0x3186

#define FACE_W 240
#define FACE_H 240

// Sampling frequency
#define NUM_ADC_SAMPLE 100           

#define RP2040_VREF 3080
#define BATTERY_DEFICIT_VOL 500
#define BATTERY_FULL_VOL 950

const unsigned int MAX_MESSAGE_LENGTH = 120;

int32_t battery_level_percent(void)
{
  int32_t mvolts = 0;
  int32_t adc_raw = 0;
  for(int8_t i=0; i<NUM_ADC_SAMPLE; i++){
    adc_raw += analogRead(D0);
  }
  adc_raw /= NUM_ADC_SAMPLE;
  mvolts = RP2040_VREF * adc_raw / (1<<12);
  return adc_raw;
  int32_t level = (mvolts - BATTERY_DEFICIT_VOL) * 100 / (BATTERY_FULL_VOL-BATTERY_DEFICIT_VOL); // 1850 ~ 2100
  level = (level<0) ? 0 : ((level>100) ? 100 : level); 
  return level ;
}

int32_t getBat()
{
  int lvl = battery_level_percent();

  if (lvl < 10) {
    return 0;
  }
  if (lvl < 30) {
    return 1;
  }
  if (lvl < 60) {
    return 2;
  }
  return 3;
}

// Time for next tick
uint32_t targetTime = 0;

lv_coord_t touchX, touchY;
// =========================================================================
// Setup
// =========================================================================
void setup() {
  Serial.begin(115200);
  Serial.println("Booting...");

  pinMode(D7, INPUT_PULLUP); 
  pinMode(D6, OUTPUT);
  pinMode(D0, INPUT);

  tft.init();

  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  face.createSprite(FACE_W, FACE_H);

  // face.loadFont(NotoSansBold15);

  Wire.begin();
  rtc.begin();
  syncTime();

  renderFace();

  analogReadResolution(12);

  tft.writecommand(0x29);

}

// =========================================================================
// Loop
// =========================================================================

int lastTouch = -1;

void loop() {
  if (chsc6x_is_pressed()) {
    chsc6x_get_xy(&touchX, &touchY);
    if (touchY > 180) {
      lastTouch = 4;
    }
    if (lastTouch >= 0) {
      lastTouch = 4;
    }
  }

  if (lastTouch >= 0) {
    lastTouch--;
  }


  if(lastTouch > 0){
    face.fillSprite(TFT_WHITE);
    face.pushSprite(0, 0, TFT_TRANSPARENT);
  } else {
    if (targetTime < millis()) {
      targetTime = millis() + 1000;

      syncTime();
      renderFace();
    }
  }

  if (Serial.available()) {
    static char message[MAX_MESSAGE_LENGTH];
    static unsigned int message_pos = 0;
    while (Serial.available() > 0)
    {
      char inByte = Serial.read();
      if ( inByte != '\n' && (message_pos < MAX_MESSAGE_LENGTH - 1) )
      {
        message[message_pos] = inByte;
        message_pos++;
      }
    }

    message[message_pos] = '\0';
    Serial.printf("echo %s\n", message);

    static char time[10];

    if (prefix("SETMIN=", message)) {
        rtc.getTime(&timeStruct);
      Serial.println("SET MIN CMD");
      strncpy(time, message+7, strlen(message));
      timeStruct.minutes = atoi(time);
      rtc.setTime(&timeStruct);
      Serial.printf("CONFIRM MIN TO %i", atoi(time));
    }


    if (prefix("SETHRS=", message)) {
        rtc.getTime(&timeStruct);
      Serial.println("SET HRS CMD");
      strncpy(time, message+7, strlen(message));
      timeStruct.hours = atoi(time);
      rtc.setTime(&timeStruct);
      Serial.printf("CONFIRM HRS TO %i", atoi(time));
    }


    if (prefix("SETSEC=", message)) {
      rtc.getTime(&timeStruct);
      Serial.println("SET SEC CMD");
      strncpy(time, message+7, strlen(message));
      timeStruct.seconds = atoi(time);
      rtc.setTime(&timeStruct);
      Serial.printf("CONFIRM SEC TO %i", atoi(time));
    }

    if (prefix("SETDAY=", message)) {
      Serial.println("SET DAY CMD");
      rtc.getDate(&dateStruct);
      strncpy(time, message+7, strlen(message));
      dateStruct.date = atoi(time);
      rtc.setDate(&dateStruct);
      Serial.printf("CONFIRM DAY TO %i", atoi(time));
    }

    if (prefix("SETMON=", message)) {
      Serial.println("SET MON CMD");
      rtc.getDate(&dateStruct);
      strncpy(time, message+7, strlen(message));
      dateStruct.month = atoi(time);
      rtc.setDate(&dateStruct);
      Serial.printf("CONFIRM MON TO %i", atoi(time));
    }

    if (prefix("SETYRS=", message)) {
      Serial.println("SET YRS CMD");
      rtc.getDate(&dateStruct);
      strncpy(time, message+7, strlen(message));
      dateStruct.year = atoi(time);
      rtc.setDate(&dateStruct);
      Serial.printf("CONFIRM YRS TO %i", atoi(time));
    }

    if (prefix("GETDATE", message)) {
      rtc.getDate(&dateStruct);
      rtc.getTime(&timeStruct);
      Serial.printf("%04d/%02d/%02d %02d:%02d:%02d\n",
                dateStruct.year,
                dateStruct.month,
                dateStruct.date,
                timeStruct.hours,
                timeStruct.minutes,
                timeStruct.seconds
               );
    }

    if (prefix("GETMOON", message)) {
      rtc.getDate(&dateStruct);
      rtc.getTime(&timeStruct);
      Serial.printf("Moon: %i\n", getMoon());
    }

    if (prefix("GETBAT", message)) {
      int bat = battery_level_percent();
      Serial.printf("Battery: %i\n", bat);
    }

    syncTime();

    message_pos = 0;
  }
}

bool prefix(const char *pre, const char *str)
{
    return strncmp(pre, str, strlen(pre)) == 0;
}

static void renderFace() {
  face.fillSprite(TFT_BLACK);

  int x=0;
  int y=0;
  int i=0;
  int p=0;
  int dStart = 5;
  int mStart = dStart+6;
  int bStart = mStart+7;
  int moon = getMoon();

  moon += 3;
  moon %= 8;

  int bat = getBat();
  int trinum = (dateStruct.date * moon) % 20;

  for (x=0; x<watchface_bg_width; x++) {
    for (y=0; y<watchface_bg_height; y++) {
      i = (y*watchface_bg_width) + x;
      p = pgm_read_byte_near(watchface_bg_pixels + i);
      switch (p) {
        case 1:
          face.drawPixel(x, y, COLOR_LIGHT);
          break;
        case 2:
          face.drawPixel(x, y, COLOR_MID);
          break;
        case 3: // AM
          if (timeStruct.hours <= 12) {
            face.drawPixel(x, y, COLOR_THEME);
          } else {
            face.drawPixel(x, y, COLOR_MID);
          }
          break;
        case 4: // THEME
          face.drawPixel(x, y, COLOR_THEME);
          break; 
        case 19:
          if (trinum == 0) {
            face.drawPixel(x, y, COLOR_THEME);
          }
          break;
        default:
          if (p >= dStart && p <= mStart) { // Weekdays
            if (dateStruct.weekDay == (p-dStart)) {
              face.drawPixel(x, y, COLOR_THEME);
            } else {
              face.drawPixel(x, y, COLOR_MID);
            }
          }

          if (p > mStart && p <= bStart) {
            if (moon < 4) {
              if (p > mStart + 4 - moon) {
                face.drawPixel(x, y, COLOR_THEME);
              }
            } else {
              if (p > mStart+(moon-4)) {
                face.drawPixel(x, y, COLOR_THEME);
              }
            }
          }

          if (p > bStart) {
            if (p < bStart+bat) {
              face.drawPixel(x, y, COLOR_THEME);
            } else {
              face.drawPixel(x, y, COLOR_MID);
            }
          }
        } 

    }
  }

  int numX = 11;
  int numY = 85;
  int numSpace = 34;

  int offset = numX;
  for (int i=0; i<8; i++) {
    int displayNum;
    int hours = timeStruct.hours % 12;
    if (hours == 0) {
      hours = 12; 
    }

    switch (i) {
      case 0:
        displayNum = hours / 10;
        if (displayNum == 0) {
          displayNum = -1;
        }
        break;
      case 1:
        displayNum = hours % 10;
        break;
      case 3:
        displayNum = timeStruct.minutes / 10;
        break;
      case 4:
        displayNum = timeStruct.minutes % 10;
        break;
      case 6:
        displayNum = timeStruct.seconds / 10;
        break;
      case 7:
        displayNum = timeStruct.seconds % 10;
        break;
    }

    if (i == 2 || i == 5) {
      offset += 9;
    } else {
      drawSevenSegment(displayNum, offset, numY);
      offset += numSpace;
    }
  }

  face.pushSprite(0, 0, TFT_TRANSPARENT);
}


const uint8_t sevenSegmentNumbers[70] = {
  1, 1, 1, 0, 1, 1, 1, // 0
  0, 0, 1, 0, 0, 1, 0, // 1
  1, 0, 1, 1, 1, 0, 1, // 2
  1, 0, 1, 1, 0, 1, 1, // 3
  0, 1, 1, 1, 0, 1, 0, // 4
  1, 1, 0, 1, 0, 1, 1, // 5
  1, 1, 0, 1, 1, 1, 1, // 6
  1, 0, 1, 0, 0, 1, 0, // 7
  1, 1, 1, 1, 1, 1, 1, // 8
  1, 1, 1, 1, 0, 1, 1, // 9
};

static void drawSevenSegment(int16_t n, int16_t x, int16_t y) {
  int digitalWidth = 24;
  int digitalHeight = 28;
  int digitalX = x;
  int digitalY = y;

  uint32_t onColor = COLOR_THEME;
  uint32_t offColor = COLOR_MID;

  if (n == -1) {
    onColor = offColor;
    n = 0;
  }

  drawHorizontalSegment(digitalX + 4, digitalY, digitalWidth, sevenSegmentNumbers[(n * 7) + 0] ? onColor : offColor);

  drawVerticalSegment(digitalX, digitalY + 4, digitalHeight, sevenSegmentNumbers[(n * 7) + 1] ? onColor : offColor);
  drawVerticalSegment(digitalX + digitalWidth + 3, digitalY + 4, digitalHeight, sevenSegmentNumbers[(n * 7) + 2] ? onColor : offColor);

  drawHorizontalSegment(digitalX + 4, digitalY + digitalHeight + 3, digitalWidth, sevenSegmentNumbers[(n * 7) + 3] ? onColor : offColor);

  drawVerticalSegment(digitalX, digitalY + 4 + digitalHeight + 3, digitalHeight, sevenSegmentNumbers[(n * 7) + 4] ? onColor : offColor);
  drawVerticalSegment(digitalX + digitalWidth + 3, digitalY + 4 + digitalHeight + 3, digitalHeight, sevenSegmentNumbers[(n * 7) + 5] ? onColor : offColor);

  drawHorizontalSegment(digitalX + 4, digitalY + digitalHeight + 3 + digitalHeight + 3, digitalWidth, sevenSegmentNumbers[(n * 7) + 6] ? onColor : offColor);
}

#define TRIANGLE_TOP_LENGTH 15
const uint8_t triangleTop[TRIANGLE_TOP_LENGTH] = {
  0, 0, 1, 0, 0,
  0, 1, 1, 1, 0,
  1, 1, 1, 1, 1
};

static void drawHorizontalSegment(int16_t x, int16_t y, int16_t w, uint32_t color) {
  int16_t h = 5;
  int16_t area = w*h;

  for (int16_t xi = 0; xi < w; xi++) {
    for (int16_t yi = 0; yi < h; yi++) {
      int16_t pixel = (xi * h) + yi;
      int16_t tx = x+xi;
      int16_t ty = y+yi;

      if (pixel < TRIANGLE_TOP_LENGTH) {
        if (triangleTop[pixel] == 1) {
          face.drawPixel(tx, ty, color);
        }
      }
      else if (area-pixel <= TRIANGLE_TOP_LENGTH) {
        pixel = area-pixel-1;

        if (triangleTop[pixel] == 1) {
          face.drawPixel(tx, ty, color);
        }
      } else {
        face.drawPixel(tx, ty, color);
      }
    }
  }
}

static void drawVerticalSegment(int16_t x, int16_t y, int16_t h, uint32_t color) {
  int16_t w = 5;
  int16_t area = w*h;

  for (int16_t xi = 0; xi < w; xi++) {
    for (int16_t yi = 0; yi < h; yi++) {
      int16_t pixel = (yi * w) + xi;
      int16_t tx = x+xi;
      int16_t ty = y+yi;

      if (pixel < TRIANGLE_TOP_LENGTH) {
        if (triangleTop[pixel] == 1) {
          face.drawPixel(tx, ty, color);
        }
      }
      else if (area-pixel <= TRIANGLE_TOP_LENGTH) {
        pixel = area-pixel-1;

        if (triangleTop[pixel] == 1) {
          face.drawPixel(tx, ty, color);
        }
      } else {
        face.drawPixel(tx, ty, color);
      }
    }
  }
}

int calculateMoonPhase(int year, int month, int day) {
    // Constants for the calculation
    int knownNewMoonYear = 2000;
    int knownNewMoonMonth = 1; // January
    int knownNewMoonDay = 6;   // 6th January 2000
    int daysInLunarCycle = 29; // Average lunar cycle length in days

    // Calculate the number of days from the known new moon date to the given date
    int totalDays = 0;

    // Calculate days from the known new moon year to the given year
    for (int y = knownNewMoonYear; y < year; y++) {
        totalDays += (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 366 : 365; // leap year check
    }

    // Calculate days from the known new moon month to the given month in the given year
    for (int m = knownNewMoonMonth; m < month; m++) {
        if (m == 2) {
            totalDays += (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 29 : 28; // February
        } else if (m == 4 || m == 6 || m == 9 || m == 11) {
            totalDays += 30; // April, June, September, November
        } else {
            totalDays += 31; // January, March, May, July, August, October, December
        }
    }

    // Add the days in the given month
    totalDays += (day - knownNewMoonDay);

    // Calculate the phase
    int phase = (totalDays % daysInLunarCycle + daysInLunarCycle) % daysInLunarCycle;

    return phase; // Phase ranges from 0 to 28
}

int getMoon() {
    int phase = calculateMoonPhase(dateStruct.year, dateStruct.month, dateStruct.date);
    switch (phase) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
            return 0; // New Moon
            break;
        case 5:
        case 6:
        case 7:
            return 1; // Waxing Crescent
            break;
        case 8:
        case 9:
        case 10:
            return 2; // First Quarter
            break;
        case 11:
        case 12:
        case 13:
            return 3; // Waxing Gibbous
            break;
        case 14:
            return 4; // Full Moon
            break;
        case 15:
        case 16:
        case 17:
            return 5; // Waning Gibbous
            break;
        case 18:
        case 19:
        case 20:
            return 6; // Last Quarter
            break;
        case 21:
        case 22:
        case 23:
            return 7; // Waning Crescent
            break;
        default:
            return 0;
    }
}

void syncTime(void){
  targetTime = millis() + 100;
  rtc.getTime(&timeStruct);
  rtc.getDate(&dateStruct);
}
