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
- WLAN-Portal „Majas-Wordclock“: ohne Zugangsdaten, nach 3 min WLAN-Ausfall, BOOT 5 s, Web-Button.
- Libs (env wordclock): ArduinoJson 7.2.1, WebSockets 2.6.1 (Port 81, Live-Bild), Espalexa 2.7.0 (Alexa als Hue-Lampe, `/api` reserviert).
- Webseite: `web/index.html` (eine Datei, offline), `scripts/embed_web.py` → `src/app/web_index.h` (generiert, nicht im Git). Vorschau: `index.html?demo`. Überblick: docs/SOFTWARE.md.
- Fernzugriff: über das WireGuard-VPN von Majas FRITZ!Box (Glasfaser, ggf. DS-Lite → IPv6), Uhr dann unter http://wordclock.fritz.box. Keine Portfreigabe. Anleitung: docs/FERNZUGRIFF.md. Noch nicht eingerichtet (braucht Majas FRITZ!Box).
- Personalisiert: Name „Majas Wordclock“ (Webseite, Portal-Titel), in den Einstellungen änderbar.

## Stand (10.11.2026)
- Erledigt: `ledtest` und `wordclock` kompilieren fehlerfrei, ohne Warnungen im eigenen Code (Flash 22 % / 49 %).
- Erledigt: öffentliches Repo https://github.com/janniklangfeld-del/wordclock (gh-Login `janniklangfeld-del`), Branch `main`. Workflow liegt in `.github/workflows/firmware.yml`, erster CI-Lauf grün. Releases v1.0.0–v1.0.3 (11.11.). Jeder neue Tag geht automatisch an die Uhr → vorher auf Ersatzboard testen.
- Erledigt: LED-Test auf ESP32 geflasht (ESP32-D0WD-V3 rev 3.1, MAC 20:50:0d:d1:df:64). USB-Chip CP2102, Treiber installiert → **COM7**. COM5/COM6 sind Intel-Ports (VID 8087), NICHT der ESP32. Seriell läuft der Test sauber durch alle Phasen.
- Erledigt (Sichtprüfung, nur 1. von 3 Streifen angeschlossen): LEDs leuchten, Farbreihenfolge GRB stimmt (rot→grün→blau), Lauflicht bis zum Streifenende, Weiß-Test gleichmäßig (bei 2-A-Testlimit). Erster Fehlversuch: Netzteil war nicht an.
- Erledigt: alle 224 LEDs angeschlossen, Weiß-Test gleichmäßig über alle (2-A-Testlimit, nur Einspeisung am Anfang). Lauflicht bis LED 223 nicht ausdrücklich bestätigt.
- Offen: Einspeisung an 3 Punkten löten (Nutzerwunsch für höhere Helligkeit), danach ggf. `LED_MAX_MILLIAMPS` anheben.
- Erledigt: 1:1-Lötvorlagen (PDF, 2 × A4) für Start oben links/rechts in docs/hardware, erzeugt mit `scripts/loetvorlage.py`. Ansicht = LED-Seite = Vorderansicht (nicht gespiegelt). Reihe 1 oben, Reihe 10 unten → Minutenpunkte.
- Entschieden (10.11.): **Start oben links** (Vorderansicht). LED 0 = Reihe 1 links, Reihe 1 läuft nach rechts, Schlange; Reihe 10 endet links → Minutenpunkte LED 220–223 von links nach rechts. E2 rechts (Wende 5/6), E3 links. Vorlage: docs/hardware/loetvorlage_start_oben_links.pdf.
- Offen: Streifen zuschneiden und verlöten; Lichtgitter sind noch im 3D-Druck.
- Entschieden (12.11.): **Holzrahmen Buche** statt weißer L-Profile (Variante 1 verworfen, Dateien bleiben als Alternative). Buche-Leimholz 18 mm als Streifen 18 × 50 (BAUHAUS 14455864, Zuschnitt 5 × 50 × 300), 45°-Gehrung, außen 286,5, 18 mm sichtbar + 4 mm schwarzer gedruckter Anschlag, Front 3 mm zurück, Tiefe 53, Elektronikraum 27,4 mm (ESP32-Platine ist 22 mm hoch). Rückwand Pappel-Sperrholz 3 mm (24815310), 8 Schrauben. Plan + Einkaufsliste docs/hardware/rahmen_holz.svg, Modell cad/rahmen_holz.scad. Zuschnittblatt für Bauhaus-Mitarbeiter: docs/hardware/zuschnitt_leiste.pdf (12.11.). Übersicht Front + Querschnitt mit Höhen: docs/hardware/uebersicht_front_querschnitt.pdf (scripts/uebersicht_front.py). Offen: Front messen, kaufen, bauen.
- Erledigt (10.11.): Uhr-Firmware komplett (Uhrzeit-Mapping, Farben, 8 Effekte, 7 Übergänge, eigener Text, Sprachvarianten, Nachtmodus, BH1750-Autohelligkeit, Geburtstags-Herzen, Alexa, Webseite mit CSS-3D-Live-Ansicht). Kompiliert (Flash 63 %), Webseite im Demo-Modus geprüft (auch 375 px Handybreite).
- Erledigt (10.11.): Uhr-Firmware (Stand c35628a) auf ESP32 an COM7 geflasht – ersetzt den LED-Test (zurück: `pio run -e ledtest -t upload --upload-port COM7`). Bootet sauber, kein BH1750, startet Portal „Majas-Wordclock“ (noch kein WLAN gespeichert). Entwicklungs-Build → Auto-Update aus.
- Erledigt (11.11.): versteckter LED-Testmodus auf der Webseite (5× auf „Mit ♥ für Maja gebaut“ tippen oder `#test`), kompiliert, Demo geprüft, mit 59b9cc7 geflasht.
- Erledigt (11.11.): Begrüßung beim Einschalten: „MAJA“ als Laufschrift, Herz, Funken, dann Uhr (Einstellungen introOn/introText, Effekte-Tab). Raster kann MAJA nicht in Leserichtung → Pixelschrift. Kompiliert, Demo geprüft, auf COM7 geflasht (59b9cc7).
- Erledigt (11.11.): WLAN eingerichtet, Uhr verbindet sich, Webseite unter http://wordclock.local, Alexa-Gerät aktiv.
- Erledigt (11.11.): Update per WLAN über die Webseite (`/update`, Datei-Upload) getestet: 59b9cc7 → 57e21c8 in ~15 s, Neustart ok. Admin-Passwort noch Standard.
- ArduinoOTA (`wordclock_ota`): Einladung + Passwort ok, aber „No response from device“ – Rückverbindung Uhr → PC (TCP) blockiert vermutlich die Windows-Firewall (WLAN-Profil). Nutzer muss eine Eingangsregel für penv-python.exe selbst anlegen.
- Erledigt (11.11.): Release v1.0.0 (CI) per Web-Upload aufgespielt → Auto-Update aktiv, Uhr erreicht GitHub per TLS („aktuell (v1.0.0)“).
- Erledigt (11.11.): Internet-OTA getestet: v1.0.1 getaggt → CI-Release → „Jetzt nach Update suchen“ → Uhr lädt von GitHub, installiert, läuft mit 1.0.1 (43 s). Funktioniert auch aus fremdem WLAN, solange Internet da ist.
- Erledigt (11.11., Code auf main, noch nicht auf der Uhr): Begrüßung jetzt „HAPPY BIRTHDAY MAJA“ (auf der Uhr gesetzt), Eigener Text wahlweise als Laufschrift (customStyle/scrollText/scrollSpeed), Webseite ohne seitliches Verschieben (Leuchtkreis hinter der Uhr ragte raus), Schalter wieder 50 px (`.row label` gab flex:1).
- Release v1.0.2 (11.11.): Laufschrift, „HAPPY BIRTHDAY MAJA“, Webseiten-Fixes. Uhr hat sich selbst darauf aktualisiert (nach Neustart, Auto-Check).
- Erledigt (11.11., auf main, noch kein Release): Majas Foto statt „W“ links oben + Favicon. Quelle web/maja.jpg (128 px, 4 KB), als data-URI in web/index.html eingebettet. Release v1.0.3, Uhr per „Jetzt nach Update suchen“ aktualisiert (44 s), läuft mit 1.0.3. Begrüßungstext auf der Uhr auf „HAPPY BIRTHDAY MAJA“ gesetzt.
- Erledigt (13.11.): Einspeisung an 3 Punkten gelötet, Holzrahmen gebaut, Software auf der fertigen Uhr getestet – funktioniert.
- 13.11.: Übergabe bei Maja. WLAN wird dort über das Portal „Majas-Wordclock“ eingerichtet (Uhr findet das alte WLAN nicht → Portal öffnet nach ~20 s automatisch). Zugangsdaten NIE ins Repo (öffentlich). Danach Alexa dort neu suchen lassen.
- Offen: Fernzugriff an Majas FRITZ!Box einrichten (docs/FERNZUGRIFF.md). Admin-Passwort ändern. Majas Geburtstag eintragen (Default aus, 01.01.).
