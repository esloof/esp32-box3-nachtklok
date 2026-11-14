// Night clock for the ESP32-S3-BOX-3
// - Time via WiFi + NTP (automatic daylight saving time)
// - Date in English
// - Outdoor temperature via Open-Meteo
// - Indoor temperature/humidity when the Sensor dock (AHT30) is present
// - Dark background, automatically dimmed at night
// - Boot button (GPIO0): cycle brightness (auto / bright / dim / off)

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <time.h>
#include <LovyanGFX.hpp>
#include "config.h"

// ---------------- Hardware ESP32-S3-BOX-3 ----------------
#define PIN_LCD_SCLK 7
#define PIN_LCD_MOSI 6
#define PIN_LCD_DC   4
#define PIN_LCD_CS   5
#define PIN_LCD_RST  48   // active HIGH on the BOX-3 (shared with touch)
#define PIN_LCD_BL   47
#define PIN_BUTTON   0    // Boot button

// Change these if the image is upside down or the colours are inverted
#ifndef LCD_ROTATION
#define LCD_ROTATION 2
#endif
#ifndef LCD_INVERT
#define LCD_INVERT false
#endif

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9342 _panel;
  lgfx::Bus_SPI _bus;
  lgfx::Light_PWM _light;

 public:
  LGFX() {
    {
      auto cfg = _bus.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read = 16000000;
      cfg.pin_sclk = PIN_LCD_SCLK;
      cfg.pin_mosi = PIN_LCD_MOSI;
      cfg.pin_miso = -1;
      cfg.pin_dc = PIN_LCD_DC;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs = PIN_LCD_CS;
      cfg.pin_rst = -1;  // manual (active high)
      cfg.pin_busy = -1;
      cfg.panel_width = 320;
      cfg.panel_height = 240;
      cfg.offset_rotation = 0;
      cfg.readable = false;
      cfg.invert = LCD_INVERT;
      cfg.rgb_order = false;
      cfg.bus_shared = false;
      _panel.config(cfg);
    }
    {
      auto cfg = _light.config();
      cfg.pin_bl = PIN_LCD_BL;
      cfg.invert = false;
      cfg.freq = 12000;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    setPanel(&_panel);
  }
};

static LGFX lcd;
static LGFX_Sprite timeSpr(&lcd);

// ---------------- Status ----------------
static float outTemp = NAN;
static int outCode = -1;
static float inTemp = NAN, inHum = NAN;
static bool haveAHT = false;
static TwoWire *ahtBus = nullptr;
static unsigned long lastWeather = 0, lastAHT = 0;
static bool timeValid = false;

enum BrightMode { B_AUTO, B_DAY, B_NIGHT, B_OFF };
static BrightMode brightMode = B_AUTO;

static const char *DAYS[] = {"Sunday", "Monday", "Tuesday", "Wednesday",
                             "Thursday", "Friday", "Saturday"};
static const char *MONTHS[] = {"January", "February", "March",     "April",
                               "May",     "June",     "July",      "August",
                               "September", "October", "November", "December"};

// ---------------- AHT30 (Sensor dock) ----------------
static bool ahtProbe(TwoWire &w) {
  w.beginTransmission(0x38);
  return w.endTransmission() == 0;
}

static bool ahtRead(float &t, float &h) {
  if (!ahtBus) return false;
  ahtBus->beginTransmission(0x38);
  ahtBus->write(0xAC);
  ahtBus->write(0x33);
  ahtBus->write(0x00);
  if (ahtBus->endTransmission() != 0) return false;
  delay(85);
  if (ahtBus->requestFrom(0x38, 7) != 7) return false;
  uint8_t d[7];
  for (int i = 0; i < 7; i++) d[i] = ahtBus->read();
  if (d[0] & 0x80) return false;  // still busy
  uint32_t rh = ((uint32_t)d[1] << 12) | ((uint32_t)d[2] << 4) | (d[3] >> 4);
  uint32_t rt = (((uint32_t)d[3] & 0x0F) << 16) | ((uint32_t)d[4] << 8) | d[5];
  h = rh * 100.0f / 1048576.0f;
  t = rt * 200.0f / 1048576.0f - 50.0f;
  return true;
}

static void initSensor() {
  // Sensor dock: I2C on GPIO41 (SDA) / GPIO40 (SCL)
  Wire1.begin(41, 40, 100000);
  if (ahtProbe(Wire1)) {
    ahtBus = &Wire1;
  } else {
    // Internal I2C bus (GPIO8/18) as a fallback
    Wire.begin(8, 18, 100000);
    if (ahtProbe(Wire)) ahtBus = &Wire;
  }
  haveAHT = ahtBus != nullptr;
  Serial.printf("AHT30 sensor: %s\n", haveAHT ? "found" : "not found");
}

// ---------------- Weather ----------------
static const char *weatherText(int c) {
  if (c < 0) return "";
  if (c == 0) return "clear";
  if (c <= 2) return "partly cloudy";
  if (c == 3) return "cloudy";
  if (c == 45 || c == 48) return "fog";
  if (c >= 51 && c <= 57) return "drizzle";
  if (c >= 61 && c <= 67) return "rain";
  if (c >= 71 && c <= 77) return "snow";
  if (c >= 80 && c <= 82) return "showers";
  if (c >= 85 && c <= 86) return "snow showers";
  if (c >= 95) return "thunderstorm";
  return "";
}

static void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  String url = String("http://api.open-meteo.com/v1/forecast?latitude=") + WEATHER_LAT +
               "&longitude=" + WEATHER_LON + "&current=temperature_2m,weather_code";
  http.setTimeout(8000);
  http.useHTTP10(true);  // no chunked encoding, so getStream() yields clean JSON
  Serial.println("Fetching weather: " + url);
  if (http.begin(url)) {
    int code = http.GET();
    if (code == 200) {
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, http.getStream());
      if (err) Serial.printf("JSON error: %s\n", err.c_str());
      if (!err) {
        outTemp = doc["current"]["temperature_2m"] | NAN;
        outCode = doc["current"]["weather_code"] | -1;
        Serial.printf("Outdoor: %.1f C, code %d\n", outTemp, outCode);
      }
    } else {
      Serial.printf("Weather HTTP error: %d\n", code);
    }
    http.end();
  }
}

// ---------------- Brightness ----------------
static void applyBrightness(const struct tm &t) {
  uint8_t b;
  switch (brightMode) {
    case B_DAY: b = BRIGHT_DAY; break;
    case B_NIGHT: b = BRIGHT_NIGHT; break;
    case B_OFF: b = 0; break;
    default: {
      bool night = (NIGHT_START_H > NIGHT_END_H)
                       ? (t.tm_hour >= NIGHT_START_H || t.tm_hour < NIGHT_END_H)
                       : (t.tm_hour >= NIGHT_START_H && t.tm_hour < NIGHT_END_H);
      b = (timeValid && night) ? BRIGHT_NIGHT : BRIGHT_DAY;
    }
  }
  static int last = -1;
  if (b != last) {
    lcd.setBrightness(b);
    last = b;
  }
}

// ---------------- Drawing ----------------
static void drawCentered(const char *s, int y, uint32_t color, const lgfx::IFont *f,
                         float size = 1.0f) {
  lcd.setFont(f);
  lcd.setTextSize(size);
  lcd.setTextColor(lcd.color888((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF),
                   TFT_BLACK);
  lcd.setTextDatum(textdatum_t::top_center);
  // shrink so a 20px margin remains on the left and right
  const int maxW = lcd.width() - 40;
  int w = lcd.textWidth(s);
  if (w > maxW) lcd.setTextSize(size * maxW / w);
  int h = lcd.fontHeight() * size;
  lcd.fillRect(0, y, lcd.width(), h, TFT_BLACK);
  lcd.drawString(s, lcd.width() / 2, y);
}

static void drawTime(const struct tm &t, bool colon) {
  char buf[8];
  snprintf(buf, sizeof(buf), "%02d%c%02d", t.tm_hour, colon ? ':' : ' ', t.tm_min);
  timeSpr.fillScreen(TFT_BLACK);
  timeSpr.setFont(&fonts::Font7);
  // scale so the time is ~300px wide
  timeSpr.setTextSize(1.0f);
  float scale = 300.0f / timeSpr.textWidth("88:88");
  if (scale * timeSpr.fontHeight() > timeSpr.height()) scale = (float)timeSpr.height() / timeSpr.fontHeight();
  timeSpr.setTextSize(scale);
  timeSpr.setTextDatum(textdatum_t::middle_center);
  // very faint "ghost" segments in the background
  timeSpr.setTextColor(lcd.color888(0x1A, 0x06, 0x03));
  timeSpr.drawString("88:88", timeSpr.width() / 2, timeSpr.height() / 2);
  timeSpr.setTextColor(lcd.color888((COLOR_TIME >> 16) & 0xFF, (COLOR_TIME >> 8) & 0xFF, COLOR_TIME & 0xFF));
  timeSpr.drawString(buf, timeSpr.width() / 2, timeSpr.height() / 2);
  timeSpr.pushSprite(0, 34);
}

static void drawDate(const struct tm &t) {
  char buf[48];
  snprintf(buf, sizeof(buf), "%s %d %s %d", DAYS[t.tm_wday], t.tm_mday, MONTHS[t.tm_mon],
           t.tm_year + 1900);
  drawCentered(buf, 150, COLOR_DATE, &fonts::lgfxJapanGothicP_24);
}

static void drawTemps() {
  char buf[64];
  lcd.fillRect(0, 185, lcd.width(), 55, TFT_BLACK);
  if (haveAHT && !isnan(inTemp)) {
    // two columns: indoor | outdoor
    lcd.setFont(&fonts::lgfxJapanGothicP_20);
    lcd.setTextSize(1);
    lcd.setTextDatum(textdatum_t::top_center);
    uint32_t lbl = lcd.color888((COLOR_DIM >> 16) & 0xFF, (COLOR_DIM >> 8) & 0xFF, COLOR_DIM & 0xFF);
    uint32_t val = lcd.color888((COLOR_TEMP >> 16) & 0xFF, (COLOR_TEMP >> 8) & 0xFF, COLOR_TEMP & 0xFF);
    lcd.setTextColor(lbl, TFT_BLACK);
    lcd.drawString("indoor", 80, 188);
    lcd.drawString("outdoor", 240, 188);
    lcd.setFont(&fonts::lgfxJapanGothicP_24);
    lcd.setTextColor(val, TFT_BLACK);
    snprintf(buf, sizeof(buf), "%.1f°C  %d%%", inTemp, (int)roundf(inHum));
    lcd.drawString(buf, 80, 208);
    if (!isnan(outTemp)) snprintf(buf, sizeof(buf), "%.1f°C", outTemp);
    else snprintf(buf, sizeof(buf), "--");
    lcd.drawString(buf, 240, 208);
  } else {
    if (!isnan(outTemp))
      snprintf(buf, sizeof(buf), "outdoor %.1f°C  %s", outTemp, weatherText(outCode));
    else
      snprintf(buf, sizeof(buf), "fetching temperature...");
    drawCentered(buf, 195, COLOR_TEMP, &fonts::lgfxJapanGothicP_24);
  }
}

static void drawStatus() {
  lcd.fillRect(0, 0, lcd.width(), 22, TFT_BLACK);
  lcd.setFont(&fonts::Font0);
  lcd.setTextSize(1);
  lcd.setTextDatum(textdatum_t::top_right);
  lcd.setTextColor(lcd.color888(0x40, 0x20, 0x18), TFT_BLACK);
  const char *m = brightMode == B_AUTO ? "auto" : brightMode == B_DAY ? "bright" : brightMode == B_NIGHT ? "dim" : "off";
  char buf[48];
  snprintf(buf, sizeof(buf), "%s  %s", WiFi.status() == WL_CONNECTED ? "wifi" : "no wifi", m);
  lcd.drawString(buf, lcd.width() - 6, 6);
}

static void message(const char *l1, const char *l2 = "") {
  lcd.fillScreen(TFT_BLACK);
  drawCentered(l1, 90, COLOR_TIME, &fonts::lgfxJapanGothicP_24);
  drawCentered(l2, 130, COLOR_DATE, &fonts::lgfxJapanGothicP_20);
}

// ---------------- Setup ----------------
static void configModeCallback(WiFiManager *wm) {
  message("WiFi setup", "Connect to network '" SETUP_AP_NAME "'");
  drawCentered("and open 192.168.4.1", 155, COLOR_DATE, &fonts::lgfxJapanGothicP_20);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  // LCD reset is active high on the BOX-3
  pinMode(PIN_LCD_RST, OUTPUT);
  digitalWrite(PIN_LCD_RST, HIGH);
  delay(20);
  digitalWrite(PIN_LCD_RST, LOW);
  delay(120);

  lcd.init();
  lcd.setRotation(LCD_ROTATION);
  lcd.setBrightness(BRIGHT_DAY);
  lcd.fillScreen(TFT_BLACK);

  timeSpr.setColorDepth(16);
  if (!timeSpr.createSprite(320, 110)) Serial.println("Failed to create sprite");

  message("Night Clock", "connecting to WiFi...");
  initSensor();

  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.setAPCallback(configModeCallback);
  wm.setConfigPortalTimeout(300);
  wm.setConnectTimeout(20);
  if (!wm.autoConnect(SETUP_AP_NAME)) {
    message("No WiFi", "retrying...");
    delay(3000);
    ESP.restart();
  }

  message("Night Clock", "syncing time...");
  configTzTime(TZ_INFO, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
  struct tm t;
  for (int i = 0; i < 30 && !getLocalTime(&t, 500); i++) {}
  timeValid = getLocalTime(&t, 0) && t.tm_year > 120;

  fetchWeather();
  lastWeather = millis();
  if (haveAHT) {
    ahtRead(inTemp, inHum);
    lastAHT = millis();
  }

  lcd.fillScreen(TFT_BLACK);
  drawStatus();
  drawTemps();
}

// ---------------- Loop ----------------
void loop() {
  static int lastSec = -1, lastMin = -1, lastDay = -1;
  static bool lastBtn = HIGH;
  static unsigned long lastWifiTry = 0;

  // Button: cycle brightness mode
  bool btn = digitalRead(PIN_BUTTON);
  if (btn == LOW && lastBtn == HIGH) {
    brightMode = (BrightMode)((brightMode + 1) % 4);
    drawStatus();
    delay(30);
  }
  lastBtn = btn;

  // Reconnect WiFi
  if (WiFi.status() != WL_CONNECTED && millis() - lastWifiTry > 30000) {
    lastWifiTry = millis();
    WiFi.reconnect();
  }

  struct tm t;
  if (!getLocalTime(&t, 0) || t.tm_year < 120) {
    delay(50);
    return;
  }
  timeValid = true;

  if (t.tm_sec != lastSec) {
    lastSec = t.tm_sec;
    applyBrightness(t);
    drawTime(t, (t.tm_sec % 2) == 0);
    if (t.tm_mday != lastDay) {
      lastDay = t.tm_mday;
      drawDate(t);
    }
    if (t.tm_min != lastMin) {
      lastMin = t.tm_min;
      drawStatus();
    }
  }

  if (millis() - lastWeather > WEATHER_INTERVAL_MS) {
    lastWeather = millis();
    fetchWeather();
    drawTemps();
  }
  if (haveAHT && millis() - lastAHT > 30000) {
    lastAHT = millis();
    if (ahtRead(inTemp, inHum)) drawTemps();
  }

  delay(20);
}
