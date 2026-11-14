#pragma once

// ---------- Time / NTP ----------
// Time zone: Netherlands (CET/CEST, automatic daylight saving time)
#define TZ_INFO      "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_SERVER_1 "nl.pool.ntp.org"
#define NTP_SERVER_2 "pool.ntp.org"
#define NTP_SERVER_3 "time.google.com"

// ---------- WiFi ----------
// On first boot (or when the saved network is unreachable) the clock starts its
// own access point. Connect to it with your phone and choose your WiFi network.
#define SETUP_AP_NAME "NightClock-Setup"

// ---------- Outdoor temperature (Open-Meteo, no API key needed) ----------
// Location: Papendrecht, the Netherlands
#define WEATHER_LAT  "51.83"
#define WEATHER_LON  "4.69"
#define WEATHER_INTERVAL_MS (15UL * 60UL * 1000UL)   // every 15 minutes

// ---------- Display ----------
// Brightness 0-255, for day and night (switched automatically by time of day).
#define BRIGHT_DAY    120
#define BRIGHT_NIGHT  12
#define NIGHT_START_H 22   // night mode from 22:00
#define NIGHT_END_H   7    // until 07:00

// Colours (RGB888). Red/amber is easiest on the eyes in the dark.
#define COLOR_TIME   0xFF3B1F
#define COLOR_DATE   0xA0603A
#define COLOR_TEMP   0xC08040
#define COLOR_DIM    0x402018
