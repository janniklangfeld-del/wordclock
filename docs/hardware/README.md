# Hardware

![Verdrahtungsplan v1](verdrahtungsplan_v1.jpg)

## Verdrahtung v1 (gelötet)

| Von | Nach |
|---|---|
| ESP32 GPIO18 (D18) | 74AHCT125 Pin 2 (1A) |
| 74AHCT125 Pin 3 (1Y) | 330 Ω → LED DIN (grün) |
| 74AHCT125 Pin 1 (1OE) | GND (Gatter dauerhaft aktiv) |
| 74AHCT125 Pin 7 | GND |
| 74AHCT125 Pin 14 | +5 V, 100 nF zwischen 14 und 7 |
| ESP32 GND | Netzteil − (gemeinsame Masse) |
| LED +5 V (rot) / GND (weiß) | Netzteil 5 V / 20 A |
| Pin 8 | **nicht verbunden** (gestrichelte Linie im Plan ist nicht verlötet) |

## Offene Punkte / Empfehlungen

- [ ] Unbenutzte Gatter: Pins 4, 10, 13 (OE) an +5 V, Pins 5, 9, 12 (Eingänge) an GND. Offene CMOS-Eingänge können schwingen und Strom ziehen. Für den Test unkritisch, vor dem Einbau nachrüsten.
- [ ] Elko 1000 µF / ≥ 10 V parallel zur LED-Einspeisung (fehlt im Plan).
- [ ] ESP32 VIN an +5 V erst nach Prüfung des Boards und mit Rückspeiseschutz (Diode/Schottky), solange parallel per USB programmiert wird.
- [ ] Netzteil 20 A: Sicherung je Stromgruppe, Leitungsquerschnitte (siehe Projektstand: 5 abgesicherte Gruppen).
- [ ] LED-Reihenfolge am realen Aufbau dokumentieren (Startecke, Schlangenrichtung) → Grundlage für Buchstaben-Mapping.

## LED-Test (`env:ledtest`)

Aufbau: ESP32 per USB am PC, Streifen am Netzteil, GND gemeinsam, ESP32-VIN **nicht** an +5 V.
Reihenfolge beim Einschalten: erst Netzteil, dann USB.

1. LED 0: rot → grün → blau. Andere Farben = `COLOR_ORDER` falsch.
2. Lauflicht über alle LEDs. Stoppt es, liegt dort ein Fehler.
3. Alle weiß gedimmt (Strom auf 2 A begrenzt).
4. Regenbogen.

Serieller Monitor (115200): `+`/`-` Helligkeit, `n` nächste Phase, `p` Pause.
Leuchtet gar nichts: GND-Verbindung, Pin 1 des 74AHCT125 auf GND, Datenrichtung des Streifens (Pfeil auf dem Band, DIN-Ende) prüfen.
