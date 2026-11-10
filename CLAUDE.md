# Wordclock für Maja – Kontext für Claude Code

Geburtstagsgeschenk-Wordclock. Sprache im Projekt: Deutsch. Bei fehlendem Kontext nachfragen statt raten.

## Hardware
- ELEGOO ESP32-WROOM-32 USB-C Devboard (3 Stück vorhanden → Ersatzboards zum Testen von Releases).
- 224 WS2812B (100 LEDs/m): 10 Reihen × 22 LEDs (2 pro Buchstabe, 11×10 Buchstaben), schlangenförmig, danach 4 Minutenpunkte.
- Daten: GPIO18 → 74AHCT125 Pin 2 (1A) → Pin 3 (1Y) → 330 Ω → DIN. Pin 1 + 7 an GND, Pin 14 an +5 V, 100 nF. Pin 8 nicht verbunden.
- Netzteil 5 V / 20 A. Software-Stromlimit 7 A (Test: 2 A). ESP32 aktuell per USB versorgt, VIN NICHT an +5 V.
- Gelötet (10.11.): unbenutzte OE (4,10,13) → +5 V, Eingänge (5,9,12) → GND, Pin 6 versehentlich auch GND (unkritisch).
- Geplant: Einspeisung an 3 Punkten (E1 Anfang, E2 Wende Reihe 5/6, E3 Ende Reihe 10) über Verteiler mit 10-A-Sicherung und 1000-µF-Elko → docs/hardware/einspeisung_3_punkte.svg.
- Offen: Rückspeiseschutz für VIN (Schottky Netzteil-+ → VIN, nie an 3V3).
- Details: docs/hardware/README.md, docs/PROJEKTSTAND_2026-11-09.md

## Software
- PlatformIO: espressif32@6.9.0, FastLED 3.9.4, WiFiManager 2.0.17, Partition min_spiffs.
- Envs: `ledtest` (src/ledtest), `wordclock` (src/app), `wordclock_ota` (lokales ArduinoOTA).
- pio unter Windows: `$env:USERPROFILE\.platformio\penv\Scripts\pio.exe`
- Internet-OTA: Uhr fragt alle 6 h `github.com/<repo>/releases/latest` ab (302 → Tag) und lädt firmware.bin. Öffentliches Repo, kein Token. TLS über certs/github_roots.pem. Release = Tag `vX.Y.Z` → .github/workflows/firmware.yml.
- WLAN-Portal „Wordclock-Setup“: ohne Zugangsdaten, nach 5 min WLAN-Ausfall, BOOT 5 s, Web-Button.

## Stand (10.11.2026)
- Erledigt: `ledtest` und `wordclock` kompilieren fehlerfrei, ohne Warnungen im eigenen Code (Flash 22 % / 49 %).
- Erledigt: öffentliches Repo https://github.com/janniklangfeld-del/wordclock (gh-Login `janniklangfeld-del`), Branch `main`. Workflow liegt in `.github/workflows/firmware.yml`, erster CI-Lauf grün. Noch kein Release-Tag.
- Erledigt: LED-Test auf ESP32 geflasht (ESP32-D0WD-V3 rev 3.1, MAC 20:50:0d:d1:df:64). USB-Chip CP2102, Treiber installiert → **COM7**. COM5/COM6 sind Intel-Ports (VID 8087), NICHT der ESP32. Seriell läuft der Test sauber durch alle Phasen.
- Erledigt (Sichtprüfung, nur 1. von 3 Streifen angeschlossen): LEDs leuchten, Farbreihenfolge GRB stimmt (rot→grün→blau), Lauflicht bis zum Streifenende, Weiß-Test gleichmäßig (bei 2-A-Testlimit). Erster Fehlversuch: Netzteil war nicht an.
- Erledigt: alle 224 LEDs angeschlossen, Weiß-Test gleichmäßig über alle (2-A-Testlimit, nur Einspeisung am Anfang). Lauflicht bis LED 223 nicht ausdrücklich bestätigt.
- Offen: Einspeisung an 3 Punkten löten (Nutzerwunsch für höhere Helligkeit), danach ggf. `LED_MAX_MILLIAMPS` anheben.
- Offen: Lichtgitter sind noch im 3D-Druck → Startecke und Schlangenrichtung erst nach Einbau vermessen → Uhrzeit-Mapping (Buchstabenmatrix in cad/Wordclock_30x30_v3.scad).
