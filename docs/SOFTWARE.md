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
| Zeitplan | Nachtmodus (Zeitfenster, gedimmt oder aus). Während des Nachtmodus stellt der Helligkeitsregler oben die Nachthelligkeit (mit Hinweis „🌙 Nachtmodus bis …“), bei „Ganz ausschalten“ schaltet er die Uhr wieder ein (Zeit vom Handy). |
| Helligkeit | Regler; automatisch mit BH1750-Lichtsensor (wird beim Start erkannt) |
| Begrüßung | Beim Einschalten (Strom an, Knopf auf der Webseite, „Alexa, … an“) läuft „HAPPY BIRTHDAY MAJA“ als Laufschrift in 5×7-Pixelschrift über die Buchstabenfelder, dann pulsiert ein Herz, ein Funkenregen folgt, und die Uhrzeit baut sich mit „Zufall“ auf (ca. 13 s). Text und An/Aus unter Effekte → Begrüßung. Hinweis: „MAJA“ lässt sich auf dem Buchstabenraster nicht in Leserichtung schreiben (zwischen M und dem einzigen J steht kein A), deshalb Laufschrift. |
| Geburtstag | An Majas Geburtstag zu jeder Viertelstunde 20 s Herzchen (Datum in der Webseite eintragen) |
| Alexa | Meldet sich als Hue-kompatible Farblampe im Heimnetz (Espalexa, ohne Cloud/Skill): an/aus, Helligkeit, Farbe |
| Webseite | `http://wordclock.local` – live mitlaufende 3D-Uhr (zeigt exakt die LED-Farben), alle Einstellungen, Updates, WLAN, Neustart |
| Updates | GitHub Releases (automatisch), lokales ArduinoOTA, Datei-Upload über die Webseite |
| **MJ Arcade** (Spiele) | Runder Knopf mit Majas Gesicht unten rechts (beim Scrollen durchlässig, verdeckt nie etwas dauerhaft) → Spielhalle. Spiele auf den 11 × 10 Buchstabenfeldern: Snake (allein), Tetris (allein / zu zweit nebeneinander mit Müllzeilen / gegen MJ-Bot), Pong, T.R.O.N (Lichtrenner), Vier gewinnt (zu zweit / gegen MJ-Bot), Reaktion (Test allein / Duell), Flatterherz (allein / zu zweit gleichzeitig), Wortblitz (allein / zu zweit), Tauziehen (zu zweit / gegen MJ-Bot), Seeschlacht (zu zweit / gegen MJ-Bot), Herzregen (allein / zu zweit), Mauerbrecher (allein), Hürdenlauf (allein / zu zweit). 13 Spiele. Handys sind Controller (Lobby, Bereit, 3-2-1, Pause, Statistik, Rekorde/Ruhmeshalle). Siehe unten. |
| Fotos | 5 Fotos (Knopf, Spielhalle, Gewonnen, Game Over, Verloren) liegen **nur im Flash der Uhr** (LittleFS, 128 KB), nie im Repo. Hochladen: System → Fotos (wird im Browser verkleinert) oder `scripts/fotos_hochladen.py` mit den Dateien aus `private/fotos/` (gitignored). |
| LED-Testmodus | Versteckt: unten auf der Webseite 5× schnell auf „Mit ♥ für Maja gebaut“ tippen (oder `http://wordclock.local/#test`). Lauflicht, Reihen, Buchstaben, R·G·B·W, alle an, Einzel-LED (mit Anzeige Reihe/Buchstabe; Buchstabe in der 3D-Uhr antippbar), Minutenpunkte. Farbe, Helligkeit, Tempo einstellbar. Endet 10 min nach der letzten Eingabe oder beim Schließen der Seite. |

Von unterwegs: über das WireGuard-VPN der FRITZ!Box, siehe [FERNZUGRIFF.md](FERNZUGRIFF.md).

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
| `games.*` | MJ Arcade: Plätze, Lobby, Countdown, Pause, Ergebnis, Rekorde (NVS „games“) |
| `games_list.cpp` | Die Spiele (Snake, Tetris + Bot, Pong + Bot, Tron + Bot, Vier gewinnt + Minimax-Bot, Reaktion) |
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
| GET | `/v1/games` | Spieleliste, Rekorde, aktueller Stand | nein |
| GET | `/v1/photos`, `/photo/<name>` | Vorhandene Fotos (Größe = Version), Foto ausliefern | nein |
| POST | `/v1/photo?n=<name>`, `/v1/photo/del?n=<name>` | Foto hochladen (multipart, max. 48 KB) / löschen | ja |
| WS | `:81` | Binär `[1, Helligkeit, 224×RGB]` ~15/s, Text `{"t":"settings"}` bei Änderungen | – |

WebSocket für Spiele: Text `{"t":"g.open|g.join|g.leave|g.ready|g.pause|g.again|g.quit|g.hello", …}`, `{"t":"frames","on":false}` (Live-Bild für diesen Client aus), binär `[2, Taste, gedrückt]` mit Taste 0 hoch, 1 runter, 2 links, 3 rechts, 4 A, 5 B. Antwort `{"t":"game","you":Platz,…}` bei jeder Änderung.

`/api/…` ist für Alexa reserviert. Passwort: Benutzer `admin`, Standard `wordclock`.

## Erweiterungen später

- **Lichtsensor**: BH1750 an 3,3 V, GND, SDA 21, SCL 22 anschließen → wird automatisch erkannt, „Automatische Helligkeit“ einschalten.
- **Echte 3D-Modell-Ansicht**: Die Live-Daten kommen bereits über den WebSocket. Die aktuelle Ansicht ist reines CSS-3D (läuft offline). Ein detailliertes Modell (z. B. Three.js) könnte dieselben Daten nutzen.

## MJ Arcade – wie die Spiele flüssig bleiben

- Die Uhr ist Schiedsrichter: Sie nimmt die Tasten aller Handys an, rechnet das Spiel und zeigt es. Die Handys sind nur Controller, deshalb gibt es kein „wer war zuerst“-Problem.
- Tasten gehen **binär über den offenen WebSocket** (3 Bytes), gesendet bei `pointerdown`/`pointerup`, nicht bei `click`. Gedrückt halten (z. B. Tetris seitwärts) rechnet die Uhr selbst (170 ms Verzögerung, dann alle 55 ms), so stört WLAN-Jitter das Tempo nicht.
- Eingaben landen in einer Warteschlange (32 Einträge) und werden im nächsten Spieltakt verarbeitet. Snake/Tron haben eine eigene Richtungs-Warteschlange, schnelle Doppel-Lenkungen gehen nicht verloren.
- **Tastenfeld = untere 3/4 des Bildschirms, lückenlos**: Abstände zwischen Tasten sind durchsichtiger Rand der Taste, Treffer zählt das Rechteck jeder Taste (auch runde Ecken). Snake/T.R.O.N: Steuerkreuz aus vier Dreiecken über die ganze Fläche, Richtung nach Winkel zur Mitte.
- **Snake/T.R.O.N-Steuerung** (Lobby, pro Handy gespeichert `wc_ctl`): *Beides* (Standard: Tippen zählt nach 90 ms oder beim Loslassen, ab 22 px Bewegung ist es ein Wisch), *Drücken* (sofort, Gleiten wechselt die Richtung), *Wischen* (überall unter der Leiste, Weiterwischen = nächste Kurve).
- **Flatterherz**: Tippen = flattern (Schwerkraft 15 Felder/s², Flattern −6,2 Felder/s ≈ 1,3 Felder hoch). Röhren alle 5 Spalten, Lücke 4 Zeilen (ab 10 Punkten 3), Tempo steigt pro Lücke. Am Anfang schwebt das Herz 1,3 s. Zu zweit fliegen beide gleichzeitig, wer länger fliegt, gewinnt.
- **Reaktion = Loslassen**: Alle halten den Knopf, dann 1,5–4,5 s Wartezeit, dann Grün. Gemessen wird die Zeit bis zum Loslassen (jeder Spieler, max. 1,5 s). Zu früh loslassen = Runde verloren (zu zweit Punkt für den anderen). Das Handy zeigt auf dem Knopf, was zu tun ist, und nach jeder Runde die eigene Zeit (zu zweit auch die des Gegners). Zustand kommt als `x` im Spielstatus (`Game::extra`). Tasten, die man schon während 3-2-1 hält, schickt das Handy beim Start nochmal.
- **Pong**: Aufschlag gerade (nicht schräg ins Eck) und langsamer (280 ms pro Feld), ab dem ersten Rückschlag normales Tempo (170 ms, danach je Treffer schneller).
- **Wortblitz**: Wortliste in `games_more.cpp`; beim Start wird geprüft, ob jedes Wort mit den Buchstaben des Rasters legbar ist (kein Q/Y). Buchstaben blitzen nacheinander an zufälligen passenden Feldern auf (2 Durchläufe), Handy zeigt 4 Wörter (Tasten 0–3). Allein: 10 Wörter, Punkte = 1 + noch nicht gezeigte Buchstaben. Zu zweit: erster Richtige holt den Punkt, wer falsch tippt, ist für die Runde raus; bis 5.
- **Tauziehen**: jeder Tipp zieht den Knoten 0,2 Felder; ±5 = Runde. Bis 2 Runden, max. 20 s pro Runde. Bot ~6,8 Züge/s, pro verlorener Runde schneller.
- **Seeschlacht**: 5 × 5 Meer je Spieler, Schiffe 3/2/2 (berühren sich nicht). Eigene Schiffe gehen nur an das eigene Handy (`Game::priv` → Feld `me` im Spielstatus). Eingaben mit Daten: `{"t":"g.in","a":"mix"|"ok"|"shot","c":Feld}` → `Game::input`. Treffer = nochmal schießen. Bot sucht neben Treffern weiter, sonst Schachbrettmuster.
- **Herzregen**: Korb (allein 3 breit, zu zweit je 2), Herz 1 Punkt, Goldherz 3, Blitz = Leben weg bzw. −2. Allein 3 Leben (verpasste Herzen kosten auch), zu zweit 60 s.
- **Mauerbrecher**: 4 Reihen Steine, Schläger 3 breit, Abprallwinkel je nach Trefferpunkt, ab Level 2 harte Steine. 3 Leben.
- **Hürdenlauf**: Springen (Taste ▲) über Hürden, Ducken (▼ halten) unter Vögeln. Zu zweit zwei Bahnen mit denselben Hindernissen, wer länger läuft, gewinnt.
- **Kein Doppeltipp-Zoom**: in der Spielhalle `touch-action: manipulation`, im Controller `none`, dazu `touchend`/`dblclick`/`gesturestart` abgefangen (echte Knöpfe bleiben klickbar).
- **Nach dem Spiel**: 15 s ohne Aktion → die Uhr zeigt wieder die Uhrzeit, Handys auf dem Ergebnis-Bildschirm schließen die Spielhalle.
- **Knopf „Zur Uhr“** (Uhr-Symbol über dem Spiele-Knopf): beendet jedes Spiel (`g.home`, auch für Zuschauer, mit Rückfrage bei laufendem Spiel), schaltet Effekt/Laufschrift auf Uhrzeit und die Uhr an.
- **Highscore nach jedem Spiel**: Snake (Äpfel), Tetris (Reihen; zu zweit eigene Liste `hs2`), Reaktion (ms, ab 3 gültigen Messungen) – jeder Mensch zählt, auch zu zweit. Ergebnis zeigt Rekord, Halter und wie viel gefehlt hat. Spiele zu zweit zusätzlich „Meiste Siege“ mit Abstand zum Spitzenreiter.
- **WLAN-Stromsparen ist aus, solange die Spielhalle offen ist** (`WiFi.setSleep(false)`). Sonst hört der ESP32 nur alle ~100 ms ins WLAN.
- Während Countdown und Spiel ruhen Internet-Update-Check (HTTPS blockiert 1–3 s), Flash-Speichern und Lichtsensor. Controller-Handys bekommen kein Live-Bild.
- Fällt ein Handy weg (Heartbeat 4 s), pausiert das Spiel, bis jemand den Platz wieder einnimmt (gleicher Name → gleicher Platz). Danach 3-2-1 und es geht weiter. Ein drittes Handy kann zuschauen.
- Zu zweit: zweites Handy öffnet wordclock.local → MJ-Knopf → „Mitspielen“. Spielerfarben Pink (1) und Türkis (2), auch auf den Minutenpunkten der Lobby.
- Erwartete Verzögerung im selben WLAN: ~10–30 ms vom Tippen bis zur LED (Bild alle 20 ms).
- Noch nicht auf echter Hardware getestet (Stand 13.11.): Spielbarkeit, Tempo, Bot-Stärke.
