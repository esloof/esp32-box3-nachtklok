# Nachtklok voor de ESP32-S3-BOX-3

Een rustige nachtklok voor de [Espressif ESP32-S3-BOX-3](https://github.com/espressif/esp-box). Rode cijfers op een zwarte achtergrond. De klok synchroniseert via WiFi met een NTP-server en toont de datum, de binnentemperatuur en de buitentemperatuur.

## Functies

- **Grote 7-segment-klok** met knipperende dubbele punt en zwak zichtbare "spooksegmenten".
- **Tijd via NTP** (`nl.pool.ntp.org`), met automatische zomer- en wintertijd.
- **Datum in het Nederlands**, bijvoorbeeld *zaterdag 14 november 2026*.
- **Buitentemperatuur en weertype** via [Open-Meteo](https://open-meteo.com). Daarvoor is geen API-key nodig.
- **Binnentemperatuur en luchtvochtigheid** als de Sensor-dock (AHT30) is aangesloten. De klok vindt de sensor zelf.
- **Nachtstand:** tussen 22:00 en 07:00 wordt het scherm automatisch gedimd.
- **Boot-knop:** wisselt de helderheid tussen auto, fel, gedimd en uit.
- **WiFi instellen zonder code aan te passen**, via een captive portal met WiFiManager.

## Hardware

- ESP32-S3-BOX-3
- Optioneel: de ESP32-S3-BOX-3-SENSOR dock, voor binnentemperatuur en luchtvochtigheid
- USB-C-kabel die data kan overbrengen (een kabel die alleen oplaadt werkt niet)

## Installatie

Je hebt [PlatformIO](https://platformio.org) nodig, als CLI of als extensie in VS Code.

```bash
git clone https://github.com/esloof/esp32-box3-nachtklok.git
cd esp32-box3-nachtklok
pio run -t upload
pio device monitor      # optioneel: logberichten bekijken
```

Wordt de BOX-3 niet gevonden? Houd dan **Boot** ingedrukt, druk kort op **Reset** en laat Boot weer los.

## WiFi instellen

1. Bij de eerste start toont de klok **"WiFi instellen"**.
2. Verbind je telefoon met het WiFi-netwerk **Nachtklok-Setup**.
3. Er opent een pagina. Gebeurt dat niet, ga dan naar `192.168.4.1`.
4. Kies **Configure WiFi**, selecteer je netwerk, vul je wachtwoord in en druk op **Save**.

De klok onthoudt je netwerk. Je WiFi-gegevens worden alleen op het apparaat zelf bewaard en staan niet in de broncode.

## Aanpassen

Alle instellingen staan in [`include/config.h`](include/config.h):

| Instelling | Betekenis |
|---|---|
| `WEATHER_LAT` / `WEATHER_LON` | Locatie voor de buitentemperatuur |
| `TZ_INFO` | Tijdzone (POSIX-formaat) |
| `NTP_SERVER_1..3` | NTP-servers |
| `BRIGHT_DAY` / `BRIGHT_NIGHT` | Helderheid overdag en 's nachts (0–255) |
| `NIGHT_START_H` / `NIGHT_END_H` | Tijden van de nachtstand |
| `COLOR_*` | Kleuren (RGB888) |

Staat het beeld op z'n kop of zijn de kleuren omgekeerd? Pas dan `LCD_ROTATION` of `LCD_INVERT` bovenaan [`src/main.cpp`](src/main.cpp) aan.

## Gebruikte bibliotheken

- [LovyanGFX](https://github.com/lovyan03/LovyanGFX): aansturing van het scherm
- [WiFiManager](https://github.com/tzapu/WiFiManager): WiFi instellen via een captive portal
- [ArduinoJson](https://arduinojson.org): verwerken van de weerdata

## Licentie

MIT. Zie [LICENSE](LICENSE).
