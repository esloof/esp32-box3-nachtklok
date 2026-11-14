#pragma once

// ---------- Tijd / NTP ----------
// Tijdzone Nederland (CET/CEST, automatische zomertijd)
#define TZ_INFO      "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_SERVER_1 "nl.pool.ntp.org"
#define NTP_SERVER_2 "pool.ntp.org"
#define NTP_SERVER_3 "time.google.com"

// ---------- WiFi ----------
// Bij de eerste start (of als het netwerk niet bereikbaar is) start de klok een
// eigen access point. Verbind met je telefoon met dit netwerk en kies je WiFi.
#define SETUP_AP_NAME "Nachtklok-Setup"

// ---------- Buitentemperatuur (Open-Meteo, geen API-key nodig) ----------
// Locatie: Papendrecht
#define WEATHER_LAT  "51.83"
#define WEATHER_LON  "4.69"
#define WEATHER_INTERVAL_MS (15UL * 60UL * 1000UL)   // elke 15 minuten

// ---------- Scherm ----------
// Helderheid 0-255. Overdag / 's nachts (automatisch op basis van de tijd).
#define BRIGHT_DAY    120
#define BRIGHT_NIGHT  12
#define NIGHT_START_H 22   // vanaf 22:00 nachtstand
#define NIGHT_END_H   7    // tot 07:00

// Kleuren (RGB888). Rood/amber is het prettigst in het donker.
#define COLOR_TIME   0xFF3B1F
#define COLOR_DATE   0xA0603A
#define COLOR_TEMP   0xC08040
#define COLOR_DIM    0x402018
