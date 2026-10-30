#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_AHTX0.h>
#include <ScioSense_ENS160.h>
#include "time.h"

// ====== WiFi & Time config ======
const char* ssid      = "";
const char* password  = "";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec      = 8 * 3600;   // GMT+8
const int   daylightOffset_sec = 0;

// ====== Pins ======
#define TFT_CS   9
#define TFT_DC   8
#define TFT_RST  7
#define TFT_MOSI 6
#define TFT_SCLK 5

#define ENC_A_PIN    10
#define ENC_B_PIN    20
#define ENC_BTN_PIN  21
#define KEY0_PIN     0

#define SDA_PIN 1
#define SCL_PIN 2

#define LED_PIN 4
#define BUZZ_PIN 3

// ====== TFT & Sensors ======
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST); // hardware SPI
Adafruit_AHTX0   aht;
ScioSense_ENS160 ens160(0x53);   // ENS160 I2C address 0x53

// ====== Colors ======
#define CYBER_BG      ST77XX_BLACK
#define CYBER_GREEN   0x07E0  
#define CYBER_ACCENT  0x07FF  
#define CYBER_LIGHT   0xFD20  
#define CYBER_BLUE    0x07FF  
#define CYBER_PINK    0xF81F

#define AQ_BAR_GREEN  0x07E0
#define AQ_BAR_YELLOW 0xFFE0
#define AQ_BAR_ORANGE 0xFD20
#define AQ_BAR_RED    0xF800
#define CYBER_DARK    0x4208

#ifndef PI
#define PI 3.1415926
#endif

// ====== Modes ======
enum UIMode {
  MODE_MENU = 0,
  MODE_CLOCK,
  MODE_POMODORO,
  MODE_ALARM,
  MODE_DVD,
  MODE_GAME
};
UIMode currentMode = MODE_CLOCK;       // khởi động vào CLOCK luôn
int menuIndex = 0;
const int MENU_ITEMS = 5;       // Monitor, Pomodoro, Alarm, DVD, Game

// ====== Pomodoro ======
enum PomodoroState {
  POMO_SELECT = 0,
  POMO_RUNNING,
  POMO_PAUSED,
  POMO_DONE
};
PomodoroState pomoState = POMO_SELECT;
int  pomoPresetIndex    = 0;                   // 0:5, 1:15, 2:30
const uint16_t pomoDurationsMin[3] = {5, 15, 30};
unsigned long pomoStartMillis = 0;
unsigned long pomoPausedMillis = 0;
int prevPomoRemainSec  = -1;
int prevPomoPreset     = -1;
int prevPomoStateInt   = -1;

// ====== Env values ======
float    curTemp = 0;
float    curHum  = 0;
uint16_t curTVOC = 0;
uint16_t curECO2 = 400;
unsigned long lastEnvRead = 0;

// ====== Clock vars ======
int    prevSecond  = -1;
String prevTimeStr = "";

// ====== Manual Time Adjustment ======
bool inTimeAdjustMode = false;      // 是否处于时间调节模式
int adjustSelectedField = 0;        // 0=时, 1=分, 2=秒, 3=退出
uint8_t adjustHour = 0;             // 调节时的小时
uint8_t adjustMin = 0;              // 调节时的分钟
uint8_t adjustSec = 0;              // 调节时的秒
unsigned long adjustBaseMillis = 0; // 调节基准时间
unsigned long lastBlinkMs = 0;      // 闪烁计时
bool blinkShow = true;              // 闪烁显示状态

// ====== Encoder & Buttons ======
int  lastEncA   = HIGH;
int  lastEncB   = HIGH;
bool lastEncBtn = HIGH;
bool lastKey0   = HIGH;
unsigned long lastBtnMs = 0;

// ====== Alarm ======
bool     alarmEnabled = false;
uint8_t  alarmHour    = 7;
uint8_t  alarmMinute  = 0;
bool     alarmRinging = false;
int      alarmSelectedField = 0; // 0 = hour, 1 = minute, 2 = enable
int      lastAlarmDayTriggered = -1;

// ====== Alert / LED Blink ======
enum AlertLevel {
  ALERT_NONE = 0,
  ALERT_CO2,
  ALERT_ALARM
};
AlertLevel currentAlertLevel = ALERT_NONE;
unsigned long lastLedToggleMs = 0;
bool ledState = false;

unsigned long lastCo2BlinkMs = 0;
bool co2BlinkOn = false;

// ========= Helper: encoder & button =========
int readEncoderStep() {
  int encA = digitalRead(ENC_A_PIN);
  int encB = digitalRead(ENC_B_PIN);
  int step = 0;
  if (encA != lastEncA) {
    if (encA == LOW) {
      if (encB == HIGH) step = +1;
      else              step = -1;
    }
  }
  lastEncA = encA;
  lastEncB = encB;
  return step;
}

bool checkButtonPressed(uint8_t pin, bool &lastState) {
  bool cur = digitalRead(pin);
  bool pressed = false;
  unsigned long now = millis();
  if (cur == LOW && lastState == HIGH && (now - lastBtnMs) > 150) {
    pressed = true;
    lastBtnMs = now;
  }
  lastState = cur;
  return pressed;
}

// ========= Alarm icon =========
void drawAlarmIcon() {
  int x = 148;
  tft.fillRect(x - 10, 0, 12, 12, CYBER_BG);
  if (!alarmEnabled) return;

  uint16_t c = CYBER_LIGHT; // cam
  tft.drawRoundRect(x - 9, 2, 10, 7, 2, c);
  tft.drawFastHLine(x - 8, 8, 8, c);
  tft.fillCircle(x - 4, 10, 1, c);
}

// ========= WiFi & Time =========
void connectWiFiAndSyncTime() {
  tft.fillScreen(CYBER_BG);
  tft.setTextColor(CYBER_LIGHT);
  tft.setTextSize(1);
  tft.setCursor(10, 55);
  tft.print("Connecting WiFi");
  WiFi.begin(ssid, password);

  uint8_t retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(300);
    tft.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    tft.fillScreen(CYBER_BG);
    tft.setCursor(10, 55);
    tft.print("Syncing time...");
    delay(800);
  } else {
    tft.fillScreen(CYBER_BG);
    tft.setCursor(10, 55);
    tft.setTextColor(ST77XX_RED);
    tft.print("WiFi FAILED!");
    delay(1000);
    
    // 没连上WiFi，设置默认时间 00:00:00
    struct tm tm;
    tm.tm_year = 2024 - 1900;  // 年份从1900开始计算
    tm.tm_mon = 0;              // 0-11，0表示1月
    tm.tm_mday = 1;             // 1-31
    tm.tm_hour = 0;             // 0-23
    tm.tm_min = 0;              // 0-59
    tm.tm_sec = 0;              // 0-59
    tm.tm_isdst = 0;            // 非夏令时
    time_t t = mktime(&tm);
    struct timeval tv = { t, 0 };
    settimeofday(&tv, NULL);
    
    // 启动手动调节模式
    inTimeAdjustMode = true;
    adjustSelectedField = 0;
    adjustHour = 0;
    adjustMin = 0;
    adjustSec = 0;
  }
}

String getTimeStr(char type) {
  // 调节模式下返回调节时间（时间不走动）
  if (inTimeAdjustMode) {
    char buf[8];
    if (type == 'H') sprintf(buf, "%02d", adjustHour);
    else if (type == 'M') sprintf(buf, "%02d", adjustMin);
    else if (type == 'S') sprintf(buf, "%02d", adjustSec);
    return String(buf);
  }
  
  // 正常模式下返回网络时间
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "--";
  char buf[8];
  if (type == 'H') strftime(buf, sizeof(buf), "%H", &timeinfo);
  else if (type == 'M') strftime(buf, sizeof(buf), "%M", &timeinfo);
  else if (type == 'S') strftime(buf, sizeof(buf), "%S", &timeinfo);
  return String(buf);
}

// ========= Env Sensors =========
void updateEnvSensors(bool force = false) {
  unsigned long now = millis();
  if (!force && (now - lastEnvRead) < 5000) return;
  lastEnvRead = now;

  sensors_event_t hum, temp;
  if (aht.getEvent(&hum, &temp)) {
    curTemp = temp.temperature;
    curHum  = hum.relative_humidity;
  }

  ens160.set_envdata(curTemp, curHum);
  ens160.measure();                      // blocking

  uint16_t newTVOC = ens160.getTVOC();
  uint16_t newCO2  = ens160.geteCO2();

  if (newTVOC != 0xFFFF) curTVOC = newTVOC;
  if (newCO2  != 0xFFFF) curECO2 = newCO2;

  Serial.print("ENS160: TVOC=");
  Serial.print(curTVOC);
  Serial.print(" eCO2=");
  Serial.println(curECO2);
}

// ========= Clock UI =========

// Grid layout
const int GRID_L   = 4;
const int GRID_R   = 156;
const int GRID_TOP = 56;
const int GRID_MID = 80;
const int GRID_BOT = 104;
const int GRID_MID_X = (GRID_L + GRID_R) / 2;

// Y cho label / value (text size 1, cao ~8px)
const int TOP_LABEL_Y    = GRID_TOP + 4;   // 60
const int TOP_VALUE_Y    = GRID_TOP + 15;  // 71
const int BOTTOM_LABEL_Y = GRID_MID + 4;   // 84
const int BOTTOM_VALUE_Y = GRID_MID + 15;  // 95

// bar layout
const int BAR_MARGIN_X = 2;
const int BAR_GAP      = 2;
const int BAR_Y        = 110;
const int BAR_H        = 6;
const int BAR_W        = (160 - 2 * BAR_MARGIN_X - 3 * BAR_GAP) / 4; // 37

// In text căn giữa trong khoảng [x0, x1]
void printCenteredText(const String &txt,
                       int x0, int x1,
                       int y,
                       uint16_t color,
                       uint16_t bg,
                       uint8_t size = 1) {
  int16_t bx, by;
  uint16_t w, h;
  tft.setTextSize(size);
  tft.getTextBounds(txt, 0, 0, &bx, &by, &w, &h);
  int x = x0 + ((x1 - x0) - (int)w) / 2;
  tft.setTextColor(color, bg);
  tft.setCursor(x, y);
  tft.print(txt);
}

uint16_t colorForCO2(uint16_t eco2) {
  if (eco2 <= 800)  return AQ_BAR_GREEN;
  if (eco2 <= 1200) return AQ_BAR_YELLOW;
  if (eco2 <= 1800) return AQ_BAR_ORANGE;
  return AQ_BAR_RED;
}

void initClockStaticUI() {
  tft.fillScreen(CYBER_BG);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);

  tft.setCursor(4, 4);
  tft.print("Monitor");

  tft.setCursor(4, 44);
  tft.print("Air Quality:");

  tft.drawFastHLine(GRID_L, GRID_TOP, GRID_R - GRID_L, ST77XX_WHITE);
  tft.drawFastHLine(GRID_L, GRID_MID, GRID_R - GRID_L, ST77XX_WHITE);
  tft.drawFastHLine(GRID_L, GRID_BOT, GRID_R - GRID_L, ST77XX_WHITE);
  tft.drawFastVLine(GRID_MID_X, GRID_TOP, GRID_BOT - GRID_TOP, ST77XX_WHITE);

  // Label căn giữa trong từng ô
  printCenteredText("HUMI", GRID_L,     GRID_MID_X, TOP_LABEL_Y,    ST77XX_WHITE, CYBER_BG, 1);
  printCenteredText("TEMP", GRID_MID_X, GRID_R,     TOP_LABEL_Y,    ST77XX_WHITE, CYBER_BG, 1);
  printCenteredText("TVOC", GRID_L,     GRID_MID_X, BOTTOM_LABEL_Y, ST77XX_WHITE, CYBER_BG, 1);
  printCenteredText("CO2",  GRID_MID_X, GRID_R,     BOTTOM_LABEL_Y, ST77XX_WHITE, CYBER_BG, 1);

  int x = BAR_MARGIN_X;
  tft.fillRect(x,                          BAR_Y, BAR_W, BAR_H, AQ_BAR_GREEN);
  tft.fillRect(x + (BAR_W + BAR_GAP),      BAR_Y, BAR_W, BAR_H, AQ_BAR_YELLOW);
  tft.fillRect(x + 2 * (BAR_W + BAR_GAP),  BAR_Y, BAR_W, BAR_H, AQ_BAR_ORANGE);
  tft.fillRect(x + 3 * (BAR_W + BAR_GAP),  BAR_Y, BAR_W, BAR_H, AQ_BAR_RED);

  drawAlarmIcon();
}

void drawClockTime(String hourStr, String minStr, String secStr) {
  String cur = hourStr + ":" + minStr + ":" + secStr;
  if (cur == prevTimeStr && !inTimeAdjustMode) return;
  prevTimeStr = cur;

  int16_t x1, y1;
  uint16_t w, h;
  tft.setTextSize(3);
  
  // 计算每个部分的位置
  String sep = ":";
  uint16_t wHour, wMin, wSec, wSep;
  tft.getTextBounds(hourStr, 0, 0, &x1, &y1, &wHour, &h);
  tft.getTextBounds(minStr, 0, 0, &x1, &y1, &wMin, &h);
  tft.getTextBounds(secStr, 0, 0, &x1, &y1, &wSec, &h);
  tft.getTextBounds(sep, 0, 0, &x1, &y1, &wSep, &h);
  
  int totalW = wHour + wSep + wMin + wSep + wSec;
  int startX = (160 - totalW) / 2;
  int y = 18;
  int x = startX;
  
  // 绘制小时
  tft.setTextColor((inTimeAdjustMode && adjustSelectedField == 0 && !blinkShow) ? CYBER_BG : CYBER_ACCENT, CYBER_BG);
  tft.setCursor(x, y);
  tft.print(hourStr);
  x += wHour;
  
  // 绘制第一个分隔符
  tft.setTextColor(CYBER_ACCENT, CYBER_BG);
  tft.setCursor(x, y);
  tft.print(":");
  x += wSep;
  
  // 绘制分钟
  tft.setTextColor((inTimeAdjustMode && adjustSelectedField == 1 && !blinkShow) ? CYBER_BG : CYBER_ACCENT, CYBER_BG);
  tft.setCursor(x, y);
  tft.print(minStr);
  x += wMin;
  
  // 绘制第二个分隔符
  tft.setTextColor(CYBER_ACCENT, CYBER_BG);
  tft.setCursor(x, y);
  tft.print(":");
  x += wSep;
  
  // 绘制秒
  tft.setTextColor((inTimeAdjustMode && adjustSelectedField == 2 && !blinkShow) ? CYBER_BG : CYBER_ACCENT, CYBER_BG);
  tft.setCursor(x, y);
  tft.print(secStr);
}

void drawCO2Value(uint16_t eco2, uint16_t col) {
  char co2Buf[12];
  sprintf(co2Buf, "%4uppm", eco2);
  printCenteredText(String(co2Buf),
                    GRID_MID_X, GRID_R,
                    BOTTOM_VALUE_Y,
                    col, CYBER_BG, 1);
}

void drawEnvDynamic(float temp, float hum, uint16_t tvoc, uint16_t eco2) {
  uint16_t colHUMI = CYBER_ACCENT;
  uint16_t colTEMP = CYBER_LIGHT;
  uint16_t colTVOC = CYBER_GREEN;
  uint16_t colCO2  = colorForCO2(eco2);

  // HUMI
  char humBuf[8];
  sprintf(humBuf, "%2.0f%%", hum);
  printCenteredText(String(humBuf),
                    GRID_L, GRID_MID_X,
                    TOP_VALUE_Y,
                    colHUMI, CYBER_BG, 1);

  // TEMP
  char tempBuf[10];
  sprintf(tempBuf, "%2.1fC", temp);
  printCenteredText(String(tempBuf),
                    GRID_MID_X, GRID_R,
                    TOP_VALUE_Y,
                    colTEMP, CYBER_BG, 1);

  // TVOC (mg/m3)
  float tvoc_mg = tvoc / 1000.0f;
  char tvocBuf[16];
  sprintf(tvocBuf, "%.3fmg/m3", tvoc_mg);
  printCenteredText(String(tvocBuf),
                    GRID_L, GRID_MID_X,
                    BOTTOM_VALUE_Y,
                    colTVOC, CYBER_BG, 1);

  // CO2 (ppm)
  drawCO2Value(eco2, colCO2);

  // Thanh màu + tam giác
  uint8_t level = 1;
  if (eco2 > 1800) level = 4;
  else if (eco2 > 1200) level = 3;
  else if (eco2 > 800)  level = 2;

  tft.fillRect(0, BAR_Y + BAR_H + 1, 160, 8, CYBER_BG);
  int centerX = BAR_MARGIN_X + (BAR_W / 2) + (level - 1) * (BAR_W + BAR_GAP);
  int tipY    = BAR_Y + BAR_H + 2;
  tft.fillTriangle(centerX,     tipY - 4,
                   centerX - 4, tipY + 2,
                   centerX + 4, tipY + 2,
                   ST77XX_WHITE);
}

// ========= Menu UI =========
void drawMenu() {
  tft.fillScreen(CYBER_BG);

  tft.setTextSize(1);
  tft.setTextColor(CYBER_LIGHT);
  tft.setCursor(10, 10);
  tft.print("MODE SELECT");

  const char* items[MENU_ITEMS] = {
    "Monitor",
    "Pomodoro",
    "Alarm",
    "DVD",
    "Tetris"
  };

  for (int i = 0; i < MENU_ITEMS; i++) {
    int y = 32 + i * 18;
    if (i == menuIndex) {
      tft.fillRect(6, y - 2, 148, 14, CYBER_ACCENT);
      tft.setTextColor(CYBER_BG);
    } else {
      tft.fillRect(6, y - 2, 148, 14, CYBER_BG);
      tft.setTextColor(ST77XX_WHITE);
    }
    tft.setCursor(12, y);
    tft.print(items[i]);
  }
  drawAlarmIcon();
}

// ========= Pomodoro =========
uint16_t pomoColorFromFrac(float f) {
  if (f < 0.33f) return AQ_BAR_GREEN;
  if (f < 0.66f) return AQ_BAR_YELLOW;
  if (f < 0.85f) return AQ_BAR_ORANGE;
  return AQ_BAR_RED;
}

// Vòng Pomodoro 270°
void drawPomodoroRing(float progress) {
  int cx = 80;
  int cy = 64;
  int rOuter = 55;
  int rInner = 44;

  // Vẽ cung 270°: từ -225° đến +45° (chừa hở đáy)
  const float startDeg = -225.0f;
  const float endDeg   =   45.0f;
  const float spanDeg  = endDeg - startDeg;   // 270°

  for (float deg = startDeg; deg <= endDeg; deg += 6.0f) {
    float frac = (deg - startDeg) / spanDeg;   // 0..1
    uint16_t col = (frac <= progress) ? pomoColorFromFrac(frac) : CYBER_DARK;

    float rad = deg * PI / 180.0f;
    int xOuter = cx + cos(rad) * rOuter;
    int yOuter = cy + sin(rad) * rOuter;
    int xInner = cx + cos(rad) * rInner;
    int yInner = cy + sin(rad) * rInner;
    tft.drawLine(xInner, yInner, xOuter, yOuter, col);
  }
}

void drawPomodoroScreen(bool forceStatic) {
  bool needStatic = forceStatic;
  if (prevPomoPreset != pomoPresetIndex || prevPomoStateInt != (int)pomoState) {
    needStatic = true;
    prevPomoPreset   = pomoPresetIndex;
    prevPomoStateInt = (int)pomoState;
  }

  if (needStatic) {
    tft.fillScreen(CYBER_BG);
    tft.fillRect(0, 0, 160, 16, CYBER_BG);
    tft.setTextSize(1);
    tft.setCursor(8, 4);
    tft.setTextColor(CYBER_LIGHT);
    tft.print("POMODORO");
    drawAlarmIcon();
  }

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE, CYBER_BG);
  tft.fillRect(100, 0, 60, 16, CYBER_BG);
  tft.setCursor(108, 4);
  tft.printf("%2d min", pomoDurationsMin[pomoPresetIndex]);

  unsigned long durationMs = pomoDurationsMin[pomoPresetIndex] * 60UL * 1000UL;
  unsigned long elapsed = 0;
  if (pomoState == POMO_RUNNING)      elapsed = millis() - pomoStartMillis;
  else if (pomoState == POMO_PAUSED)  elapsed = pomoPausedMillis - pomoStartMillis;
  else if (pomoState == POMO_DONE)    elapsed = durationMs;
  if (elapsed > durationMs) elapsed = durationMs;

  float progress = (durationMs > 0) ? (float)elapsed / durationMs : 0.0f;
  drawPomodoroRing(progress);

  int cx = 80;
  int cy = 64;
  tft.fillCircle(cx, cy, 38, CYBER_BG);

  unsigned long remain = durationMs - elapsed;
  uint16_t rmMin = remain / 60000UL;
  uint8_t  rmSec = (remain / 1000UL) % 60;

  char buf[8];
  sprintf(buf, "%02d:%02d", rmMin, rmSec);

  int16_t x1, y1;
  uint16_t w, h;
  tft.setTextSize(3);
  tft.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cx - w / 2, cy - 8);
  tft.setTextColor(ST77XX_WHITE, CYBER_BG);
  tft.print(buf);

  // Footer: chỉ hiện Paused / Completed
  tft.fillRect(0, 96, 160, 32, CYBER_BG);
  tft.setTextSize(1);
  tft.setCursor(8, 100);
  tft.setTextColor(CYBER_GREEN, CYBER_BG);
  if (pomoState == POMO_PAUSED)      tft.print("Paused");
  else if (pomoState == POMO_DONE)   tft.print("Completed");
}

// ========= Alarm UI =========
void drawAlarmScreen(bool full) {
  if (full) {
    tft.fillScreen(CYBER_BG);
    tft.setTextSize(1);
    tft.setCursor(8, 4);
    tft.setTextColor(CYBER_LIGHT);
    tft.print("ALARM");
    drawAlarmIcon();
  }

  char hBuf[3];
  char mBuf[3];
  sprintf(hBuf, "%02d", alarmHour);
  sprintf(mBuf, "%02d", alarmMinute);

  tft.setTextSize(3);

  String disp = String(hBuf) + ":" + String(mBuf);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(disp, 0, 0, &x1, &y1, &w, &h);
  int x = (160 - w) / 2;
  int y = 30;

  tft.setCursor(x, y);
  tft.setTextColor(alarmSelectedField == 0 ? CYBER_LIGHT : ST77XX_WHITE, CYBER_BG);
  tft.print(hBuf);

  tft.setTextColor(ST77XX_WHITE, CYBER_BG);
  tft.print(":");

  tft.setTextColor(alarmSelectedField == 1 ? CYBER_LIGHT : ST77XX_WHITE, CYBER_BG);
  tft.print(mBuf);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE, CYBER_BG);
  tft.fillRect(20, 80, 120, 24, CYBER_BG);
  tft.setCursor(30, 84);
  tft.print("Alarm:");
  tft.setCursor(80, 84);
  uint16_t enColor = alarmSelectedField == 2
                     ? CYBER_LIGHT
                     : (alarmEnabled ? CYBER_GREEN : ST77XX_RED);
  tft.setTextColor(enColor, CYBER_BG);
  tft.print(alarmEnabled ? "ON " : "OFF");
}

void drawAlarmRingingScreen() {
  tft.fillScreen(ST77XX_RED);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE, ST77XX_RED);
  tft.setCursor(30, 40);
  tft.print("ALARM!");
}

// ========= Alarm logic =========
void checkAlarmTrigger() {
  if (!alarmEnabled || alarmRinging) return;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  if (timeinfo.tm_hour == alarmHour &&
      timeinfo.tm_min  == alarmMinute &&
      timeinfo.tm_sec  == 0) {

    alarmRinging = true;
    lastAlarmDayTriggered = timeinfo.tm_mday;
    currentMode = MODE_ALARM;
    drawAlarmRingingScreen();
  }
}

// ========= Alert visual + audio =========
void updateAlertStateAndLED() {
  if (alarmRinging) currentAlertLevel = ALERT_ALARM;
  else if (curECO2 > 1800) currentAlertLevel = ALERT_CO2;
  else currentAlertLevel = ALERT_NONE;

  unsigned long now = millis();

  unsigned long interval;
  if (currentAlertLevel == ALERT_ALARM)      interval = 120;
  else if (currentAlertLevel == ALERT_CO2)   interval = 250;
  else                                       interval = 1000;

  if (now - lastLedToggleMs > interval) {
    lastLedToggleMs = now;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  }

  if (currentAlertLevel == ALERT_CO2) {
    if (now - lastCo2BlinkMs > 350) {
      lastCo2BlinkMs = now;
      co2BlinkOn = !co2BlinkOn;
      uint16_t baseCol = colorForCO2(curECO2);
      uint16_t col = co2BlinkOn ? baseCol : CYBER_DARK;
      drawCO2Value(curECO2, col);
      tone(BUZZ_PIN, 1800, 80);
    }
  }
}

// ========= GAME: Tetris (俄罗斯方块) =========

// 游戏区域设置
const int TETRIS_X = 20;      // 游戏区域左上角X
const int TETRIS_Y = 20;      // 游戏区域左上角Y
const int BLOCK_SIZE = 8;     // 每个方块的大小
const int GRID_W = 10;        // 游戏区域宽度（格子数）
const int GRID_H = 14;        // 游戏区域高度（格子数）

// 俄罗斯方块形状定义 (4x4矩阵)
// 每个形状有4种旋转状态
const uint16_t TETROMINO_I[4] = {0x0F00, 0x2222, 0x00F0, 0x4444};  // I形
const uint16_t TETROMINO_O[4] = {0x0660, 0x0660, 0x0660, 0x0660};  // O形
const uint16_t TETROMINO_T[4] = {0x0E40, 0x4C40, 0x4E00, 0x4640};  // T形
const uint16_t TETROMINO_S[4] = {0x06C0, 0x8C40, 0x06C0, 0x8C40};  // S形
const uint16_t TETROMINO_Z[4] = {0x0C60, 0x4C80, 0x0C60, 0x4C80};  // Z形
const uint16_t TETROMINO_J[4] = {0x0E20, 0x44C0, 0x8E00, 0x6440};  // J形
const uint16_t TETROMINO_L[4] = {0x0E80, 0xC440, 0x2E00, 0x4460};  // L形

const uint16_t* TETROMINOS[7] = {
  TETROMINO_I, TETROMINO_O, TETROMINO_T, TETROMINO_S,
  TETROMINO_Z, TETROMINO_J, TETROMINO_L
};

const uint16_t TETRIS_COLORS[7] = {
  CYBER_ACCENT,   // I - 青色
  AQ_BAR_YELLOW,   // O - 黄色
  CYBER_PINK,     // T - 紫色
  CYBER_GREEN,    // S - 绿色
  ST77XX_RED,     // Z - 红色
  CYBER_BLUE,     // J - 蓝色
  CYBER_LIGHT     // L - 橙色
};

// 游戏状态变量
uint8_t tetrisGrid[GRID_H][GRID_W];  // 游戏网格，0表示空，1-7表示不同颜色方块
int currentPiece = 0;                // 当前方块类型 (0-6)
int currentRotation = 0;             // 当前旋转状态 (0-3)
int currentX = 3;                    // 当前方块X位置
int currentY = 0;                    // 当前方块Y位置
int tetrisScore = 0;                 // 游戏分数
bool tetrisGameOver = false;         // 游戏结束标志
bool tetrisInited = false;           // 游戏初始化标志
unsigned long lastDropMs = 0;        // 上次下落时间
unsigned long dropInterval = 800;    // 下落间隔（毫秒）

// 初始化俄罗斯方块游戏
void initTetris() {
  // 清空网格
  for (int y = 0; y < GRID_H; y++) {
    for (int x = 0; x < GRID_W; x++) {
      tetrisGrid[y][x] = 0;
    }
  }
  
  tetrisScore = 0;
  tetrisGameOver = false;
  tetrisInited = true;
  
  // 生成第一个方块
  spawnNewPiece();
  
  // 绘制游戏界面
  tft.fillScreen(CYBER_BG);
  
  // 标题
  tft.setTextSize(1);
  tft.setTextColor(CYBER_LIGHT, CYBER_BG);
  tft.setCursor(10, 2);
  tft.print("TETRIS");
  
  // 分数
  tft.setCursor(100, 2);
  tft.print("0");
  
  // 绘制游戏区域边框
  int gameW = GRID_W * BLOCK_SIZE;
  int gameH = GRID_H * BLOCK_SIZE;
  tft.drawRect(TETRIS_X - 1, TETRIS_Y - 1, gameW + 2, gameH + 2, CYBER_ACCENT);
  
  drawAlarmIcon();
  drawTetris();
}

// 生成新方块
void spawnNewPiece() {
  currentPiece = random(7);
  currentRotation = 0;
  currentX = 3;
  currentY = 0;
  
  // 检查是否能放置
  if (!canPlacePiece(currentX, currentY, currentRotation)) {
    tetrisGameOver = true;
  }
}

// 检查方块是否能放置在指定位置
bool canPlacePiece(int px, int py, int rot) {
  uint16_t shape = TETROMINOS[currentPiece][rot];
  
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (shape & (1 << (15 - y * 4 - x))) {
        int gx = px + x;
        int gy = py + y;
        
        // 检查边界
        if (gx < 0 || gx >= GRID_W || gy >= GRID_H) return false;
        
        // 检查碰撞
        if (gy >= 0 && tetrisGrid[gy][gx] != 0) return false;
      }
    }
  }
  return true;
}

// 将当前方块固定到网格
void lockPiece() {
  uint16_t shape = TETROMINOS[currentPiece][currentRotation];
  
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (shape & (1 << (15 - y * 4 - x))) {
        int gx = currentX + x;
        int gy = currentY + y;
        if (gy >= 0 && gy < GRID_H && gx >= 0 && gx < GRID_W) {
          tetrisGrid[gy][gx] = currentPiece + 1;
        }
      }
    }
  }
  
  // 检查消行
  checkLines();
  
  // 生成新方块
  spawnNewPiece();
}

// 检查并消除满行
void checkLines() {
  int linesCleared = 0;
  
  for (int y = GRID_H - 1; y >= 0; y--) {
    bool full = true;
    for (int x = 0; x < GRID_W; x++) {
      if (tetrisGrid[y][x] == 0) {
        full = false;
        break;
      }
    }
    
    if (full) {
      // 消除该行
      for (int yy = y; yy > 0; yy--) {
        for (int x = 0; x < GRID_W; x++) {
          tetrisGrid[yy][x] = tetrisGrid[yy - 1][x];
        }
      }
      // 清空最上面一行
      for (int x = 0; x < GRID_W; x++) {
        tetrisGrid[0][x] = 0;
      }
      
      linesCleared++;
      y++; // 重新检查当前行
    }
  }
  
  // 计分
  if (linesCleared > 0) {
    tetrisScore += linesCleared * 100;
    drawTetrisScore();
    tone(BUZZ_PIN, 2000 + linesCleared * 200, 100);
  }
}

// 绘制俄罗斯方块
void drawTetris() {
  // 清空游戏区域
  int gameW = GRID_W * BLOCK_SIZE;
  int gameH = GRID_H * BLOCK_SIZE;
  tft.fillRect(TETRIS_X, TETRIS_Y, gameW, gameH, CYBER_BG);
  
  // 绘制已固定的方块
  for (int y = 0; y < GRID_H; y++) {
    for (int x = 0; x < GRID_W; x++) {
      if (tetrisGrid[y][x] != 0) {
        int px = TETRIS_X + x * BLOCK_SIZE;
        int py = TETRIS_Y + y * BLOCK_SIZE;
        uint16_t color = TETRIS_COLORS[tetrisGrid[y][x] - 1];
        tft.fillRect(px + 1, py + 1, BLOCK_SIZE - 2, BLOCK_SIZE - 2, color);
      }
    }
  }
  
  // 绘制当前下落的方块
  uint16_t shape = TETROMINOS[currentPiece][currentRotation];
  uint16_t color = TETRIS_COLORS[currentPiece];
  
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (shape & (1 << (15 - y * 4 - x))) {
        int gx = currentX + x;
        int gy = currentY + y;
        if (gy >= 0 && gy < GRID_H && gx >= 0 && gx < GRID_W) {
          int px = TETRIS_X + gx * BLOCK_SIZE;
          int py = TETRIS_Y + gy * BLOCK_SIZE;
          tft.fillRect(px + 1, py + 1, BLOCK_SIZE - 2, BLOCK_SIZE - 2, color);
        }
      }
    }
  }
}

// 绘制分数
void drawTetrisScore() {
  tft.fillRect(100, 0, 60, 10, CYBER_BG);
  tft.setCursor(100, 2);
  tft.setTextSize(1);
  tft.setTextColor(CYBER_GREEN, CYBER_BG);
  tft.print(tetrisScore);
}

// 俄罗斯方块游戏主逻辑
void updateTetrisLogic(int encStep, bool rotatePressed, bool backPressed) {
  if (backPressed) {
    tetrisInited = false;
    currentMode = MODE_MENU;
    drawMenu();
    return;
  }
  
  if (tetrisGameOver) {
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_WHITE, CYBER_BG);
    tft.setCursor(25, 60);
    tft.print("GAME OVER");
    tft.setTextSize(1);
    tft.setCursor(30, 85);
    tft.print("Score: ");
    tft.print(tetrisScore);
    
    if (rotatePressed || encStep != 0) {
      initTetris();
    }
    return;
  }
  
  // 旋转编码器调整位置（左右移动）
  if (encStep != 0) {
    int newX = currentX + encStep;
    if (canPlacePiece(newX, currentY, currentRotation)) {
      currentX = newX;
      drawTetris();
    }
  }
  
  // 单击编码器旋转（顺时针90度）
  if (rotatePressed) {
    int newRot = (currentRotation + 1) % 4;
    if (canPlacePiece(currentX, currentY, newRot)) {
      currentRotation = newRot;
      drawTetris();
      tone(BUZZ_PIN, 1500, 30);
    }
  }
  
  // 自动下落
  unsigned long now = millis();
  if (now - lastDropMs > dropInterval) {
    lastDropMs = now;
    
    // 尝试下落
    if (canPlacePiece(currentX, currentY + 1, currentRotation)) {
      currentY++;
      drawTetris();
    } else {
      // 无法下落，固定方块
      lockPiece();
      drawTetris();
      
      if (tetrisGameOver) {
        tone(BUZZ_PIN, 800, 500);
      }
    }
  }
}

// ========= DVD screensaver =========

bool dvdInited = false;
int  dvdX, dvdY;
int  dvdVX = 2;
int  dvdVY = 1;
int  dvdW  = 50;
int  dvdH  = 18;
unsigned long lastDvdMs = 0;
unsigned long dvdInterval = 35;

uint16_t dvdColors[] = {
  ST77XX_WHITE,
  CYBER_ACCENT,
  CYBER_LIGHT,
  CYBER_GREEN,
  CYBER_PINK,
  ST77XX_YELLOW
};
int dvdColorIndex = 0;

void drawDvdLogo(int x, int y, uint16_t c) {
  // x,y: top-left
  tft.fillRoundRect(x, y, dvdW, dvdH, 4, c);
  tft.drawRoundRect(x, y, dvdW, dvdH, 4, CYBER_BG);

  // chữ DVD ở giữa
  tft.setTextSize(1);
  tft.setTextColor(CYBER_BG, c);
  int16_t bx, by;
  uint16_t w, h;
  tft.getTextBounds("DVD", 0, 0, &bx, &by, &w, &h);
  int tx = x + (dvdW - (int)w) / 2;
  int ty = y + (dvdH - (int)h) / 2 + 1;
  tft.setCursor(tx, ty);
  tft.print("DVD");
}

void initDvd() {
  dvdInited = true;
  tft.fillScreen(CYBER_BG);

  // header
  tft.setTextSize(1);
  tft.setTextColor(CYBER_LIGHT, CYBER_BG);
  tft.setCursor(8, 4);
  tft.print("DVD");

  drawAlarmIcon();

  // vùng chạy logo: chừa header 16px
  dvdX = 40;
  dvdY = 40;
  dvdVX = 2;
  dvdVY = 1;
  lastDvdMs = millis();
  dvdColorIndex = 0;

  drawDvdLogo(dvdX, dvdY, dvdColors[dvdColorIndex]);
}

void updateDvd(int encStep, bool encPressed, bool backPressed) {
  (void)encPressed;

  if (backPressed) {
    dvdInited = false;
    currentMode = MODE_MENU;
    drawMenu();
    return;
  }

  // encoder chỉnh nhẹ tốc độ ngang
  if (encStep != 0) {
    int speed = abs(dvdVX) + encStep;
    if (speed < 1) speed = 1;
    if (speed > 4) speed = 4;
    dvdVX = (dvdVX >= 0 ? 1 : -1) * speed;
  }

  unsigned long now = millis();
  if (now - lastDvdMs < dvdInterval) return;
  lastDvdMs = now;

  // xóa logo cũ
  tft.fillRoundRect(dvdX, dvdY, dvdW, dvdH, 4, CYBER_BG);

  // update vị trí
  dvdX += dvdVX;
  dvdY += dvdVY;

  // biên trong vùng màn
  int left   = 0;
  int right  = 160 - dvdW;
  int top    = 16;
  int bottom = 128 - dvdH;

  bool hitX = false;
  bool hitY = false;

  if (dvdX <= left) {
    dvdX = left;
    dvdVX = -dvdVX;
    hitX = true;
  } else if (dvdX >= right) {
    dvdX = right;
    dvdVX = -dvdVX;
    hitX = true;
  }

  if (dvdY <= top) {
    dvdY = top;
    dvdVY = -dvdVY;
    hitY = true;
  } else if (dvdY >= bottom) {
    dvdY = bottom;
    dvdVY = -dvdVY;
    hitY = true;
  }

  // nếu đập vào góc (cả X & Y cùng va chạm) => đổi màu
  if (hitX && hitY) {
    dvdColorIndex++;
    if (dvdColorIndex >= (int)(sizeof(dvdColors) / sizeof(dvdColors[0]))) {
      dvdColorIndex = 0;
    }
    tone(BUZZ_PIN, 1500, 80);
  }

  drawDvdLogo(dvdX, dvdY, dvdColors[dvdColorIndex]);
}

// ========= SETUP =========
void setup() {
  Serial.begin(115200);
  delay(1500);

  pinMode(ENC_A_PIN,   INPUT_PULLUP);
  pinMode(ENC_B_PIN,   INPUT_PULLUP);
  pinMode(ENC_BTN_PIN, INPUT_PULLUP);
  pinMode(KEY0_PIN,    INPUT_PULLUP);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZ_PIN, OUTPUT);

  Wire.begin(SDA_PIN, SCL_PIN);
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(CYBER_BG);

  connectWiFiAndSyncTime();

  if (!aht.begin()) Serial.println("AHT21 not found");
  if (!ens160.begin()) Serial.println("ENS160 begin FAIL");
  else ens160.setMode(ENS160_OPMODE_STD);

  updateEnvSensors(true);

  // khởi động vào Monitor (clock)
  initClockStaticUI();
  prevTimeStr = "";
  drawClockTime(getTimeStr('H'), getTimeStr('M'), getTimeStr('S'));
  drawEnvDynamic(curTemp, curHum, curTVOC, curECO2);
}

// ========= LOOP =========
void loop() {
  static unsigned long lastWifiCheck = 0;
  if (millis() - lastWifiCheck > 10000) {
    lastWifiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, password);
    }
  }

  int encStep     = readEncoderStep();
  bool encPressed = checkButtonPressed(ENC_BTN_PIN, lastEncBtn);
  bool k0Pressed  = checkButtonPressed(KEY0_PIN,    lastKey0);

  checkAlarmTrigger();
  updateAlertStateAndLED();

  switch (currentMode) {
    case MODE_MENU: {
      if (encStep != 0) {
        menuIndex += encStep;
        if (menuIndex < 0) menuIndex = MENU_ITEMS - 1;
        if (menuIndex >= MENU_ITEMS) menuIndex = 0;
        drawMenu();
      }
      if (encPressed) {
        if (menuIndex == 0) {
          currentMode = MODE_CLOCK;
          initClockStaticUI();
          prevTimeStr = "";
          updateEnvSensors(true);
          drawClockTime(getTimeStr('H'), getTimeStr('M'), getTimeStr('S'));
          drawEnvDynamic(curTemp, curHum, curTVOC, curECO2);
        } else if (menuIndex == 1) {
          currentMode = MODE_POMODORO;
          pomoState = POMO_SELECT;
          prevPomoRemainSec = -1;
          prevPomoPreset = -1;
          prevPomoStateInt = -1;
          drawPomodoroScreen(true);
        } else if (menuIndex == 2) {
          currentMode = MODE_ALARM;
          alarmSelectedField = 0;
          drawAlarmScreen(true);
        } else if (menuIndex == 3) {
          currentMode = MODE_DVD;
          dvdInited = false;
        } else if (menuIndex == 4) {
          currentMode = MODE_GAME;
          tetrisInited = false;
        }
      }
      break;
    }

    case MODE_CLOCK: {
      // 时间调节模式
      if (inTimeAdjustMode) {
        // 处理闪烁效果
        if (millis() - lastBlinkMs > 300) {
          lastBlinkMs = millis();
          blinkShow = !blinkShow;
          drawClockTime(getTimeStr('H'), getTimeStr('M'), getTimeStr('S'));
        }
        
        // 编码器旋转调节选中值
        if (encStep != 0) {
          if (adjustSelectedField == 0) {
            // 调节小时
            adjustHour = (adjustHour + encStep + 24) % 24;
          } else if (adjustSelectedField == 1) {
            // 调节分钟
            adjustMin = (adjustMin + encStep + 60) % 60;
          } else if (adjustSelectedField == 2) {
            // 调节秒
            adjustSec = (adjustSec + encStep + 60) % 60;
          }
          blinkShow = true;
          lastBlinkMs = millis();
          drawClockTime(getTimeStr('H'), getTimeStr('M'), getTimeStr('S'));
        }
        
        // 编码器单击切换选中字段或退出
        if (encPressed) {
          adjustSelectedField++;
          if (adjustSelectedField >= 3) {
            // 第4下退出调节模式
            inTimeAdjustMode = false;
            adjustSelectedField = 0;
            // 应用调节后的时间
            configTime(0, 0, "pool.ntp.org");  // 重置NTP
            struct tm tm;
            tm.tm_year = 2024 - 1900;
            tm.tm_mon = 0;
            tm.tm_mday = 1;
            tm.tm_hour = adjustHour;
            tm.tm_min = adjustMin;
            tm.tm_sec = adjustSec;
            time_t t = mktime(&tm);
            struct timeval tv = { t, 0 };
            settimeofday(&tv, NULL);
            prevTimeStr = "";
            drawClockTime(getTimeStr('H'), getTimeStr('M'), getTimeStr('S'));
          } else {
            blinkShow = true;
            lastBlinkMs = millis();
            drawClockTime(getTimeStr('H'), getTimeStr('M'), getTimeStr('S'));
          }
        }
        
        // KEY0 也可以退出调节模式
        if (k0Pressed) {
          inTimeAdjustMode = false;
          adjustSelectedField = 0;
          prevTimeStr = "";
          drawClockTime(getTimeStr('H'), getTimeStr('M'), getTimeStr('S'));
        }
      } else {
        // 正常时钟显示模式
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
          int sec = timeinfo.tm_sec;
          if (sec != prevSecond) {
            prevSecond = sec;
            drawClockTime(getTimeStr('H'), getTimeStr('M'), getTimeStr('S'));
            if (sec % 5 == 0) {
              updateEnvSensors(true);
              drawEnvDynamic(curTemp, curHum, curTVOC, curECO2);
            }
          }
        }
        
        // 单击编码器进入调节模式
        if (encPressed) {
          inTimeAdjustMode = true;
          adjustSelectedField = 0;
          // 保存当前时间到调节变量
          struct tm timeinfo;
          if (getLocalTime(&timeinfo)) {
            adjustHour = timeinfo.tm_hour;
            adjustMin = timeinfo.tm_min;
            adjustSec = timeinfo.tm_sec;
          }
          blinkShow = true;
          lastBlinkMs = millis();
          drawClockTime(getTimeStr('H'), getTimeStr('M'), getTimeStr('S'));
        }
        
        if (k0Pressed) {
          currentMode = MODE_MENU;
          drawMenu();
        }
      }
      break;
    }

    case MODE_POMODORO: {
      if (pomoState == POMO_SELECT && encStep != 0) {
        pomoPresetIndex += encStep;
        if (pomoPresetIndex < 0) pomoPresetIndex = 2;
        if (pomoPresetIndex > 2) pomoPresetIndex = 0;
        prevPomoRemainSec = -1;
        drawPomodoroScreen(true);
      }

      if (encPressed) {
        if (pomoState == POMO_SELECT || pomoState == POMO_DONE) {
          pomoState = POMO_RUNNING;
          pomoStartMillis = millis();
          prevPomoRemainSec = -1;
          drawPomodoroScreen(true);
        } else if (pomoState == POMO_RUNNING) {
          pomoState = POMO_PAUSED;
          pomoPausedMillis = millis();
          drawPomodoroScreen(true);
        } else if (pomoState == POMO_PAUSED) {
          unsigned long pauseDur = millis() - pomoPausedMillis;
          pomoStartMillis += pauseDur;
          pomoState = POMO_RUNNING;
          prevPomoRemainSec = -1;
          drawPomodoroScreen(true);
        }
      }

      if (pomoState == POMO_RUNNING || pomoState == POMO_PAUSED || pomoState == POMO_DONE) {
        unsigned long durationMs = pomoDurationsMin[pomoPresetIndex] * 60UL * 1000UL;
        unsigned long elapsed = 0;
        if (pomoState == POMO_RUNNING) elapsed = millis() - pomoStartMillis;
        else if (pomoState == POMO_PAUSED) elapsed = pomoPausedMillis - pomoStartMillis;
        else if (pomoState == POMO_DONE) elapsed = durationMs;

        if (elapsed > durationMs) {
          elapsed = durationMs;
          if (pomoState != POMO_DONE) {
            pomoState = POMO_DONE;
            tone(BUZZ_PIN, 2000, 500);
            drawPomodoroScreen(true);
          }
        }

        int remainSec = (durationMs - elapsed) / 1000UL;
        if (remainSec != prevPomoRemainSec) {
          prevPomoRemainSec = remainSec;
          drawPomodoroScreen(false);
        }
      }

      if (k0Pressed) {
        currentMode = MODE_MENU;
        drawMenu();
      }
      break;
    }

    case MODE_ALARM: {
      if (alarmRinging) {
        static unsigned long lastBeep = 0;
        if (millis() - lastBeep > 1000) {
          lastBeep = millis();
          tone(BUZZ_PIN, 2000, 400);
        }
        if (encPressed || k0Pressed) {
          alarmRinging = false;
          lastAlarmDayTriggered = -1;
          noTone(BUZZ_PIN);
          drawAlarmScreen(true);
        }
        break;
      }

      bool changed = false;
      if (encStep != 0) {
        if (alarmSelectedField == 0) {
          if (encStep > 0) alarmHour = (alarmHour + 1) % 24;
          else             alarmHour = (alarmHour + 23) % 24;
          changed = true;
        } else if (alarmSelectedField == 1) {
          if (encStep > 0) alarmMinute = (alarmMinute + 1) % 60;
          else             alarmMinute = (alarmMinute + 59) % 60;
          changed = true;
        } else if (alarmSelectedField == 2) {
          alarmEnabled = !alarmEnabled;
          changed = true;
        }

        if (changed) {
          lastAlarmDayTriggered = -1;
        }
      }

      if (encPressed) {
        alarmSelectedField = (alarmSelectedField + 1) % 3;
        changed = true;
      }

      if (k0Pressed) {
        currentMode = MODE_MENU;
        drawMenu();
        break;
      }

      if (changed) {
        drawAlarmScreen(false);
        drawAlarmIcon();
      }

      break;
    }

    case MODE_DVD: {
      if (!dvdInited) {
        initDvd();
      }
      updateDvd(encStep, encPressed, k0Pressed);
      break;
    }

    case MODE_GAME: {
      if (!tetrisInited) {
        initTetris();
      }
      updateTetrisLogic(encStep, encPressed, k0Pressed);
      break;
    }
  }
}