# Night Clock for the ESP32-S3-BOX-3

A calm bedside clock for the [Espressif ESP32-S3-BOX-3](https://github.com/espressif/esp-box). It shows red digits on a black background, syncs its time over WiFi with an NTP server, and displays the date plus the indoor and outdoor temperature.

## Features

- **Large 7-segment clock** with a blinking colon and faint "ghost" segments.
- **NTP time** (`nl.pool.ntp.org`) with automatic daylight saving time.
- **Date**, for example *Saturday 14 November 2026*.
- **Outdoor temperature and weather** from [Open-Meteo](https://open-meteo.com). No API key needed.
- **Indoor temperature and humidity** when the Sensor dock (AHT30) is attached. The clock detects the sensor automatically.
- **Night mode:** the screen dims automatically between 22:00 and 07:00.
- **Boot button:** cycles the brightness between auto, bright, dim and off.
- **WiFi setup without editing code**, through a captive portal (WiFiManager).

## Hardware

- ESP32-S3-BOX-3
- Optional: the ESP32-S3-BOX-3-SENSOR dock, for indoor temperature and humidity
- A USB-C cable that carries data (a charge-only cable won't work)

## Installation

You need [PlatformIO](https://platformio.org), either the CLI or the VS Code extension.

```bash
git clone https://github.com/esloof/esp32-box3-nachtklok.git
cd esp32-box3-nachtklok
pio run -t upload
pio device monitor      # optional: view log messages
```

If the BOX-3 isn't detected, hold **Boot**, briefly press **Reset**, then release Boot.

## WiFi setup

1. On first boot the clock shows **"WiFi setup"**.
2. Connect your phone to the WiFi network **NightClock-Setup**.
3. A page opens. If it doesn't, browse to `192.168.4.1`.
4. Choose **Configure WiFi**, select your network, enter the password and press **Save**.

The clock remembers your network. Your WiFi credentials are stored only on the device itself and are not part of the source code.

## Configuration

All settings live in [`include/config.h`](include/config.h):

| Setting | Meaning |
|---|---|
| `WEATHER_LAT` / `WEATHER_LON` | Location for the outdoor temperature |
| `TZ_INFO` | Time zone (POSIX format) |
| `NTP_SERVER_1..3` | NTP servers |
| `BRIGHT_DAY` / `BRIGHT_NIGHT` | Brightness during the day and at night (0–255) |
| `NIGHT_START_H` / `NIGHT_END_H` | Night mode hours |
| `COLOR_*` | Colours (RGB888) |

Is the image upside down or are the colours inverted? Change `LCD_ROTATION` or `LCD_INVERT` at the top of [`src/main.cpp`](src/main.cpp).

## Libraries

- [LovyanGFX](https://github.com/lovyan03/LovyanGFX): display driver
- [WiFiManager](https://github.com/tzapu/WiFiManager): WiFi setup through a captive portal
- [ArduinoJson](https://arduinojson.org): parsing the weather data

## License

MIT. See [LICENSE](LICENSE).
