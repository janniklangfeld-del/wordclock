# Wordclock für Maja – Kontext für Claude Code

Geburtstagsgeschenk-Wordclock. Sprache im Projekt: Deutsch. Bei fehlendem Kontext nachfragen statt raten.

## Hardware
- ELEGOO ESP32-WROOM-32 USB-C Devboard (3 Stück vorhanden → Ersatzboards zum Testen von Releases).
- 224 WS2812B (100 LEDs/m): 10 Reihen × 22 LEDs (2 pro Buchstabe, 11×10 Buchstaben), schlangenförmig, danach 4 Minutenpunkte.
- Daten: GPIO18 → 74AHCT125 Pin 2 (1A) → Pin 3 (1Y) → 330 Ω → DIN. Pin 1 + 7 an GND, Pin 14 an +5 V, 100 nF. Pin 8 nicht verbunden.
- Netzteil 5 V / 20 A. Software-Stromlimit 7 A (Test: 2 A). ESP32 aktuell per USB versorgt, VIN NICHT an +5 V.
- Offen: unbenutzte OE (4,10,13) → +5 V, Eingänge (5,9,12) → GND; 1000-µF-Elko; Rückspeiseschutz für VIN.
- Details: docs/hardware/README.md, docs/PROJEKTSTAND_2026-11-09.md

## Software
- PlatformIO: espressif32@6.9.0, FastLED 3.9.4, WiFiManager 2.0.17, Partition min_spiffs.
- Envs: `ledtest` (src/ledtest), `wordclock` (src/app), `wordclock_ota` (lokales ArduinoOTA).
- pio unter Windows: `$env:USERPROFILE\.platformio\penv\Scripts\pio.exe`
- Internet-OTA: Uhr fragt alle 6 h `github.com/<repo>/releases/latest` ab (302 → Tag) und lädt firmware.bin. Öffentliches Repo, kein Token. TLS über certs/github_roots.pem. Release = Tag `vX.Y.Z` → .github/workflows/firmware.yml.
- WLAN-Portal „Wordclock-Setup“: ohne Zugangsdaten, nach 5 min WLAN-Ausfall, BOOT 5 s, Web-Button.

## Stand (10.11.2026)
- Code geschrieben, aber NOCH NIE KOMPILIERT. Erster Schritt: `pio run -e ledtest -e wordclock`, Fehler beheben.
- Danach: `scripts/setup-github.ps1` (Git + öffentliches Repo `wordclock` auf GitHub-Konto Jannik Langfeld). Die Workflow-Datei liegt noch in `_move_to_github_workflows/` und muss nach `.github/workflows/` (das Skript erledigt das).
- Dann LED-Test flashen (`pio run -e ledtest -t upload -t monitor`), Farbreihenfolge/Lauflicht prüfen.
- Danach: LED-Reihenfolge am Aufbau vermessen (Startecke, Schlangenrichtung) → Uhrzeit-Mapping (Buchstabenmatrix in cad/Wordclock_30x30_v3.scad).
