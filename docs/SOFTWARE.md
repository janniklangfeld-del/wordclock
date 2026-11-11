# Software – Majas Wordclock

Stand 10.11.2026: kompiliert fehlerfrei, Webseite im Demo-Modus geprüft. **Auf der echten Uhr noch nicht getestet** (WLAN, Alexa, Live-Ansicht, LED-Zuordnung).

## Funktionen

| Bereich | Was geht |
|---|---|
| Uhrzeit | Deutsche Wortuhr in 5-Minuten-Schritten + 4 Minutenpunkte, NTP-Zeit, Zeitzone wählbar |
| Sprache | „Es ist“ immer / nur volle & halbe / nie · „Viertel nach drei“ oder „Viertel vier“ · „Viertel vor vier“ oder „Dreiviertel vier“ · „zwanzig nach“ oder „zehn vor halb“ |
| Farben | Einfarbig, Regenbogen, Verlauf (2 Farben, 3 Richtungen), Farbwechsel, Wortfarben; eigene Farbe für Minutenpunkte |
| Hintergrund | Unbenutzte Buchstaben aus / leise in einer Farbe / leise mit Effekt |
| Effekte | Regenbogen, Plasma, Funkeln, Matrix, Feuer, Atmen, Herzen, Konfetti (Vollbild oder als Hintergrund) |
| Übergänge | Sofort, Überblenden, Wischen, Diagonal, Zufall, Matrix, Schreibmaschine – Dauer einstellbar |
| Eigener Text | Buchstaben in der 3D-Ansicht antippen → eigene Wörter/Botschaften; oder **Laufschrift**: beliebiger Text (bis 60 Zeichen) läuft endlos durch, Farbe/Hintergrund wie die Uhr, Tempo einstellbar |
| Zeitplan | Nachtmodus (Zeitfenster, gedimmt oder aus) |
| Helligkeit | Regler; automatisch mit BH1750-Lichtsensor (wird beim Start erkannt) |
| Begrüßung | Beim Einschalten (Strom an, Knopf auf der Webseite, „Alexa, … an“) läuft „HAPPY BIRTHDAY MAJA“ als Laufschrift in 5×7-Pixelschrift über die Buchstabenfelder, dann pulsiert ein Herz, ein Funkenregen folgt, und die Uhrzeit baut sich mit „Zufall“ auf (ca. 13 s). Text und An/Aus unter Effekte → Begrüßung. Hinweis: „MAJA“ lässt sich auf dem Buchstabenraster nicht in Leserichtung schreiben (zwischen M und dem einzigen J steht kein A), deshalb Laufschrift. |
| Geburtstag | An Majas Geburtstag zu jeder Viertelstunde 20 s Herzchen (Datum in der Webseite eintragen) |
| Alexa | Meldet sich als Hue-kompatible Farblampe im Heimnetz (Espalexa, ohne Cloud/Skill): an/aus, Helligkeit, Farbe |
| Webseite | `http://wordclock.local` – live mitlaufende 3D-Uhr (zeigt exakt die LED-Farben), alle Einstellungen, Updates, WLAN, Neustart |
| Updates | GitHub Releases (automatisch), lokales ArduinoOTA, Datei-Upload über die Webseite |
| LED-Testmodus | Versteckt: unten auf der Webseite 5× schnell auf „Mit ♥ für Maja gebaut“ tippen (oder `http://wordclock.local/#test`). Lauflicht, Reihen, Buchstaben, R·G·B·W, alle an, Einzel-LED (mit Anzeige Reihe/Buchstabe; Buchstabe in der 3D-Uhr antippbar), Minutenpunkte. Farbe, Helligkeit, Tempo einstellbar. Endet 10 min nach der letzten Eingabe oder beim Schließen der Seite. |

## Aufbau (`src/app/`)

| Datei | Aufgabe |
|---|---|
| `main.cpp` | Start, Hauptschleife, `app::settingsChanged()` (wendet Änderungen an) |
| `settings.*` | Alle Einstellungen, JSON im NVS, verzögertes Speichern |
| `clockface.*` | Buchstabenmatrix, Uhrzeit → Wörter, Buchstabe → LED-Index (Schlange, Start oben links) |
| `display.*` | Renderer mit 50 Bildern/s: Uhrzeit, Farben, Effekte, Übergänge, Nachtmodus, Helligkeit, Statusanzeige |
| `web.*` | HTTP-Server (Port 80, REST unter `/v1/…`), WebSocket (Port 81) für das Live-Bild |
| `alexa.*` | Espalexa-Gerät, synchron mit den Einstellungen |
| `intro.*` | Pixelschrift 5×7 für die Begrüßung (A–Z, ÄÖÜ, 0–9, ! - ., Herz mit `<3`) |
| `light.*` | BH1750-Lichtsensor (I2C, SDA 21 / SCL 22) |
| `net.*`, `ota.*` | WLAN-Portal „Majas-Wordclock“, Internet-OTA |

Die Webseite liegt in `web/index.html` (eine Datei, keine externen Quellen, funktioniert offline). `scripts/embed_web.py` packt sie beim Build per gzip in die Firmware.

## Webseite ohne Uhr ansehen

`web/index.html?demo` (oder per Doppelklick als Datei öffnen) startet eine Demo, die die Uhr im Browser simuliert. Zum Ausprobieren von Design und Bedienung, ohne zu flashen.

## Schnittstelle

| Methode | Pfad | Zweck | Passwort |
|---|---|---|---|
| GET/POST | `/v1/settings` | Einstellungen lesen / teilweise ändern (JSON) | nein |
| GET | `/v1/info` | Firmware, WLAN, Zeit, Sensor, Update-Status | nein |
| GET | `/v1/layout` | Buchstaben + LED-Indizes (für die 3D-Ansicht) | nein |
| POST | `/v1/replay` | Übergang vorführen | nein |
| POST | `/v1/intro` | Begrüßung abspielen | nein |
| POST | `/v1/test` | LED-Testmodus: `{"mode":"chase\|rows\|letters\|rgbw\|color\|led\|dots\|off","color":"#RRGGBB","index":0–223,"bri":0–255,"speed":1–100}`; ohne Inhalt einmaliges Lauflicht | nein |
| POST | `/v1/check`, `/v1/wifi`, `/v1/reboot`, `/v1/password` | Update suchen, WLAN-Portal, Neustart, Passwort | ja |
| POST | `/update` | Firmware-Datei | ja |
| WS | `:81` | Binär `[1, Helligkeit, 224×RGB]` ~15/s, Text `{"t":"settings"}` bei Änderungen | – |

`/api/…` ist für Alexa reserviert. Passwort: Benutzer `admin`, Standard `wordclock`.

## Erweiterungen später

- **Lichtsensor**: BH1750 an 3,3 V, GND, SDA 21, SCL 22 anschließen → wird automatisch erkannt, „Automatische Helligkeit“ einschalten.
- **Echte 3D-Modell-Ansicht**: Die Live-Daten kommen bereits über den WebSocket. Die aktuelle Ansicht ist reines CSS-3D (läuft offline). Ein detailliertes Modell (z. B. Three.js) könnte dieselben Daten nutzen.
