# Updates (OTA)

## Warum GitHub Releases

Die Uhr steht später in Majas WLAN hinter ihrem Router. Von außen ist sie nicht erreichbar (kein Port-Forwarding, keine feste IP). Deshalb **holt sich die Uhr Updates selbst**:

```
git tag vX.Y.Z  ->  GitHub Actions baut firmware.bin  ->  Release
                                                           |
Uhr (alle 6 h):  HEAD github.com/<repo>/releases/latest  --+--> 302 .../tag/vX.Y.Z
                 neuer als eigene Version?  -> firmware.bin laden -> flashen -> Neustart
```

* Keine GitHub-API, kein Token, kein Rate-Limit (öffentliches Repo).
* TLS wird gegen `certs/github_roots.pem` geprüft. CI kontrolliert wöchentlich, ob GitHubs Zertifikatskette noch abgedeckt ist. Schlägt der Check fehl: neue Root in die PEM aufnehmen und ein Release bauen, **bevor** die alte Kette ausläuft.
* Der ESP32 schreibt in den jeweils anderen App-Slot (`min_spiffs.csv`, 2 × 1,9 MB). Bricht der Download ab, läuft die alte Firmware weiter.

## Versionen

* Release-Builds: `FW_VERSION` = Tag ohne `v` (z. B. `0.2.0`). Nur echte `X.Y.Z` lösen Auto-Updates aus.
* Lokale Builds: `git describe` (z. B. `v0.1.0-3-gabc123-dirty`) → **Auto-Update aus**, damit ein Test-Stand nicht überschrieben wird. Manuell „Jetzt prüfen“ installiert trotzdem das neueste Release.

## Andere Wege

| Weg | Wann |
|---|---|
| USB (`pio run -e wordclock -t upload`) | Entwicklung, Notfall |
| Lokal per WLAN (`$env:WORDCLOCK_OTA_PASSWORD="…"; pio run -e wordclock_ota -t upload`) | Uhr im selben Netz |
| Weboberfläche → Firmware-Datei hochladen | Ohne PlatformIO, z. B. bei Maja vor Ort |

## Risiken

* **Kaputtes Release** (z. B. WLAN-Code defekt) → Uhr kann keine weiteren Updates holen. Darum: jedes Release vorher auf einem Ersatz-ESP32 testen.
* Kein automatisches Rollback bei Absturz der neuen Firmware (Arduino-Bootloader). Mögliche Ausbaustufe später.
