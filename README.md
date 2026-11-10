# Majas Wordclock

Wordclock 30 × 30 cm, 11 × 10 Buchstaben, 224 WS2812B-LEDs, ESP32.
Updates jederzeit **über das Internet** (GitHub Releases) – auch wenn die Uhr schon in Majas WLAN hängt.

## Struktur

| Ordner | Inhalt |
|---|---|
| `src/app/` | Uhr-Firmware: Uhrzeit, Effekte, Webseite mit 3D-Live-Ansicht, Alexa, OTA – siehe [docs/SOFTWARE.md](docs/SOFTWARE.md) |
| `web/` | Webseite (eine Datei, wird beim Build in die Firmware gepackt; `index.html?demo` = Vorschau ohne Uhr) |
| `src/ledtest/` | Eigenständiger LED-Streifen-Test |
| `include/hardware.h` | Pins, LED-Anzahl, Stromlimit (gilt für beide) |
| `certs/` | Root-Zertifikate für sicheres OTA von GitHub |
| `cad/` | OpenSCAD-Modell v3, STLs, Referenzbilder |
| `docs/` | Projektstand, Hardware/Verdrahtung, OTA-Ablauf |
| `.github/workflows/` | Build bei jedem Push, Release bei Tag `vX.Y.Z` |

## Schnellstart (Windows, PowerShell im Projektordner)

```powershell
powershell -ExecutionPolicy Bypass -File scripts\setup-github.ps1   # einmalig: Git + GitHub-Repo
powershell -ExecutionPolicy Bypass -File scripts\flash.ps1           # LED-Test flashen + Monitor
powershell -ExecutionPolicy Bypass -File scripts\flash.ps1 wordclock # Uhr-Firmware per USB
```

## Neue Version an die Uhr ausliefern

```powershell
git tag v0.2.0
git push origin v0.2.0
```

GitHub baut `firmware.bin` und hängt sie an ein Release. Die Uhr prüft alle 6 h (und 1 min nach dem Start) und installiert neuere Versionen selbst. Sofort: Webseite → System → „Jetzt nach Update suchen“. Details: [docs/OTA.md](docs/OTA.md).

**Vor jedem Tag**: Firmware zuerst auf einem der Ersatz-ESP32 testen. Ein Release mit kaputtem WLAN-Code kann die Uhr nur noch per USB retten.

## WLAN einrichten / ändern

1. Uhr sucht ihr gespeichertes WLAN. Gelingt das nicht, blinken die Minutenpunkte **blau**.
2. Mit dem WLAN **`Majas-Wordclock`** verbinden → Einrichtungsseite öffnet sich (sonst `http://192.168.4.1`).
3. WLAN wählen, Passwort eingeben, speichern.

Portal öffnet außerdem: BOOT-Taste 5 s halten · Webseite → System → „WLAN ändern“ · WLAN länger als 5 min weg.

## Statusanzeige (Minutenpunkte)

Solange noch keine Uhrzeit da ist: weiß = Start · gelb blinkend = verbinde · blau atmend = Einrichtungsportal · grün pulsierend = online, warte auf Internetzeit · lila = Update · rot = Fehler.
Läuft die Uhr schon, zeigen die Minutenpunkte nur noch das offene Portal (blau) und Fehler (rot, 10 s).

## Weboberfläche

`http://wordclock.local` (oder IP): live mitlaufende 3D-Uhr, Farben, Effekte, Übergänge, Sprache, Nachtmodus, Lichtsensor, Geburtstag, Alexa, Updates.
Anzeige-Einstellungen sind im Heimnetz frei; Update, WLAN, Passwort und Neustart brauchen Benutzer `admin`, Passwort Standard `wordclock` (in der Webseite änderbar).

Alexa: „Alexa, suche nach Geräten“ – danach „Alexa, Wordclock auf 40 Prozent / rot / aus“.
