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

## Unbenutzte Gatter des 74AHCT125 (gelötet am 10.11.2026)

Draufsicht (Bauteilseite):

![74AHCT125 unbenutzte Pins, Draufsicht](74AHCT125_unbenutzte_pins.svg)

Ansicht von unten (Lötseite, gespiegelt – Pin 1 oben rechts), zum Löten bei umgedrehter Platine:

![74AHCT125 unbenutzte Pins, Lötseite](74AHCT125_unbenutzte_pins_unten.svg)

| Pins | Funktion | Anschluss |
|---|---|---|
| 4, 10, 13 | OE der Gatter 2–4 | +5 V (Gatter aus, Ausgang hochohmig) |
| 5, 9, 12 | Eingänge A der Gatter 2–4 | GND |
| 6, 8, 11 | Ausgänge Y der Gatter 2–4 | frei lassen |

**Tatsächlicher Aufbau:** Pin 6 (2Y) liegt zusätzlich auf GND (versehentlich, aber unkritisch: Gatter 2 ist über Pin 4 = +5 V abgeschaltet, und mit Pin 5 = GND könnte der Ausgang ohnehin nur Low treiben). Voraussetzung: Pin 4 und Pin 5 müssen sicher verlötet sein.

Pin 13 darf direkt mit Pin 14 gebrückt werden. Die Nachbarpaare 4/5, 9/10 und 12/13 führen verschiedene Potenziale und dürfen **nicht** überbrückt werden. Nach dem Löten Pin 14 gegen Pin 7 auf Kurzschluss messen.

## LED-Streifen: Anschlüsse laut Herstellerkarte

![Herstellerkarte: Reihenschaltung und Einspeisung](led_streifen_anleitung.jpg)

- Jedes Streifenende hat einen 3-poligen Stecker: **rot = +5 V, grün = Daten, weiß = GND** (passt zur Verdrahtung oben).
- Daten fließen nur in **Pfeilrichtung**. Der Datenausgang des 74AHCT125 gehört an das Ende, an dem der Pfeil vom Stecker weg zeigt (DIN).
- An beiden Enden hängen **zusätzliche lose Adern rot (+V) und weiß (−V)**. Sie dienen nur der Stromeinspeisung („Voltage Compensation“), nicht den Daten. Wird das Streifenende im Test (Phase 3, alles weiß) rot-/gelbstichig, hier zusätzlich +5 V/GND vom Netzteil einspeisen.
- Den „LED Controller“ aus der Karte brauchen wir nicht, ESP32 + 74AHCT125 ersetzen ihn. Das Netzteil versorgt den Streifen direkt.

## Stromeinspeisung an 3 Punkten (geplant)

Test am 10.11.2026 mit allen 224 LEDs (2-A-Limit): Weiß überall gleich, Einspeisung nur am Anfang reicht dafür. Für höhere Helligkeit zusätzlich Mitte und Ende einspeisen:

![Einspeisung an 3 Punkten](einspeisung_3_punkte.svg)

| Punkt | Wo | Anschluss |
|---|---|---|
| E1 | erste LED (DIN-Ende von Reihe 1) | +5 V / GND vom Verteiler |
| E2 | Wende zwischen Reihe 5 und 6 | +5 V / GND vom Verteiler |
| E3 | Ende Reihe 10, vor den Minutenpunkten | +5 V / GND vom Verteiler |

- Netzteil → Verteiler (WAGO o. ä.): 1,5 mm², 10-A-Sicherung im +5-V-Zweig, Elko 1000 µF ≥ 10 V am Verteiler.
- Verteiler → E1/E2/E3: je ein Leitungspaar 0,75 mm² (mind. 0,5 mm²). Nur +5 V und GND an die Kupferpads, Daten bleiben unverändert in der Kette.
- 74AHCT125 Pin 14 und ESP32-GND ebenfalls am Verteiler.
- Mehr Helligkeit braucht zusätzlich ein höheres Software-Limit (`LED_MAX_MILLIAMPS`, aktuell 7000).

## Offene Punkte / Empfehlungen

- [x] Unbenutzte Gatter: Pins 4, 10, 13 (OE) an +5 V, Pins 5, 9, 12 (Eingänge) an GND – gelötet am 10.11.2026 (Pin 6 zusätzlich auf GND, unkritisch).
- [ ] Einspeisung an 3 Punkten inkl. Verteiler, 10-A-Sicherung und Elko 1000 µF / ≥ 10 V (Plan oben).
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

**Ergebnis 10.11.2026** (nur 1. von 3 Streifen): Farbreihenfolge GRB korrekt, Lauflicht bis zum Streifenende, Weiß gleichmäßig. Streifen 2 und 3 sowie Minutenpunkte noch nicht getestet.

Serieller Monitor (115200): `+`/`-` Helligkeit, `n` nächste Phase, `p` Pause, `d` Pin-Messmodus (GPIO18 wechselt alle 3 s HIGH/LOW; Pin 2 ≈ 3,3 V, Pin 3 und DIN ≈ 5 V; Ende nur per Reset).
Leuchtet gar nichts: **zuerst prüfen, ob das Netzteil an ist** – ohne Netzteil haben weder Streifen noch 74AHCT125 (Pin 14) Strom, USB versorgt nur den ESP32 (so passiert am 10.11.2026). Danach: GND-Verbindung, Pin 1 des 74AHCT125 auf GND, Datenrichtung des Streifens (Pfeil auf dem Band, DIN-Ende) prüfen.
