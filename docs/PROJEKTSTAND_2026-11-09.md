# WORDCLOCK – Projektakte und Übergabe
**Stand:** 09.11.2026  | **Status:** CAD v3 in Arbeit, Elektronik noch nicht verdrahtet

## Ziel
Geschenk-Wordclock mit minimalistischer **mattschwarzer Front**, **hellem Holzrahmen** (alternativ weißes L-Winkelprofil), **30 × 30 cm Außenmaß**, ESP32, lokaler Weboberfläche, WLAN-/QR-Einrichtung, später Alexa-Steuerung und Alexa-Wecker-Alarmmodus.

## Festgelegt / ausgelegt
- Sichtbare Front: 250 × 250 mm. Rahmen anfangs 25 mm je Seite; zuletzt auch 18–20 mm als elegantere Alternative diskutiert. **Rahmenkonstruktion noch nicht festgelegt.**
- Buchstabenraster: **11 Spalten × 10 Reihen = 110 Buchstaben**. 20 × 20 mm Kammer-Raster; Textfeld 220 × 200 mm, 13 mm Lichtkammerhöhe, Trennwände 1,5 mm, schwarze Front 1,8 mm, weißer PLA-Diffusor 0,8 mm.
- Vier **Minutenpunkte mittig unten**, kleine runde Sichtfenster, **gleicher WS2812B-Streifen** in EINEM Stück mit vier aufeinander folgenden LEDs, 10 mm Mittelpunktabstand. Das Minutenmodul ist zwischen BL (1 Punkt) und BR (3 Punkte) geteilt.
- LED: **WS2812B, 5 V, 100 LEDs/m, RGB, IP30, nach jeder LED trennbar**; 2 LEDs pro Buchstabe = 220, plus 4 für die Minutenpunkte = **224 LEDs**. Drei 1-m-Streifen liefern 300 LEDs, 76 Reserve. 10 horizontale Reihen à 22 LEDs; Daten schlangenförmig, Minutenband zuletzt. Physische 5V/GND-Stromversorgung muss mehrfach unabhängig eingespeist werden.
- Drucker: **Bambu Lab P1S**, 256 × 256 × 256 mm Bauraum, 0,4-mm-Düse, schwarzes/weißes PLA. Hauptfront/Gitter in vier Segmente geteilt (TL, TR, BL, BR).
- ESP32: ELEGOO ESP32-WROOM-32 USB-C Devboard (3er-Set wurde ausgewählt). Pegelwandler **74AHCT125** empfohlen (3,3V → 5V Datensignal), 100 nF am IC, 330–470 Ohm in Datenleitung, 1000 µF / >=10 V an LED-Versorgung.
- Netzteil: geplantes externes 5 V / 10 A, **keine 100%-Vollweiß-Ausgabe aller LEDs**. Worst-case bis ~13,4 A LEDs bei 60 mA/LED. Softwareseitiges LED-Stromlimit ca. **7 A**, ESP32/Leitungen/Verluste als Reserve. Für Normaluhrzeit (nur ausgewählte Wörter) 100% Helligkeit möglich, nicht für jeden Effekt mit allen LEDs.
- Elektrik: **Alle GNDs gemeinsam** (ESP32, LED-Verteiler, 74AHCT125, Netzteil), +5V NICHT auf ESP32-3V3-Pin. Bei paralleler USB-Programmierung Rückspeisung verhindern. Sicherungen, Leitungsauslegung, Einspeisung und Anschlüsse festlegen.

## Aktuelle CAD-Dateien
- `CAD_aktuell_v3/Wordclock_30x30_v3.scad`: parametrisches Gesamtmodell, Part/Quadrant-Auswahl in OpenSCAD. **Nicht als komplett produktionsfreigegeben betrachten.**
- `CAD_aktuell_v3/lichtgitter_*.stl`: Korrigiertes **v3-Lichtgitter**, volle 1,5-mm-Wand an Segmentgrenzen statt zwei 0,75-mm-Hälften. Nicht mit v2 mischen.
- `CAD_aktuell_v3/front_BR_v3.stl`, `diffusor_BR_v3.stl`, `lichtgitter_BR_v3.stl`: gemeinsames Druckpaket **unten rechts**. Front/Diffusor 134,1 × 135,9 mm; Gitter ca. 119,85 × 126,5 mm (kleiner, weil Gitter nur Lichtkammern bedeckt). Nicht anhand Außenecken bündig ausrichten; in gemeinsamem Koordinatensystem passen Kammerpositionen zu Buchstaben.
- Der BR-Diffusor hat die gleiche ebene Form wie die Front, ohne Buchstabenlöcher. Alternativ mit Backpapier ausprobieren, zwischen Front und Gitter ohne Kleber im Sichtbereich einklemmen.
- Die alten v2-Dateien sind **nur Archiv, nicht drucken**.

## Validierungsstand / wichtige Vorsicht
- Frühere digitale Prüfung meldete, dass alle **1.440 Uhrzeiten eines Tages** mit dem Letterraster und vier Minutenpunkten darstellbar sind. Das ist noch keine reale Firmware-Abnahme.
- Die STL-Dateien wurden früher auf geschlossene Geometrie geprüft, P1S-Bauraum passt. **Noch kein realer Zusammenbau / Maßtoleranztest / Lichttest**. Stützstruktur-/Überhangvorschau in Bambu Studio prüfen.
- An v3-Schnittflächen wurden geringe ~0,15-mm-Geometrieüberstände erwähnt; kann nachgedruckt/nachbearbeitet werden müssen. Keine Ausrichtungsstifte, Schraubhalter, endgültige Rahmenbefestigung, Leiterplattenhalter oder Kabelzugentlastung fertig konstruiert.
- **LED-Band real nachmessen**, insbesondere 10 mm Pitch, 10 mm Bandbreite, Lötpads und Durchlass (im CAD ~11,4 mm Breite, 3,2 mm Höhe); zuerst Probe trockenlegen. Das Modul der Minutenpunkte muss über Segmentfuge hinweg durchgehendes LED-Band aufnehmen.
- Weitere Gittersegmente fertig, Front/Diffusor als STL hier nur unten rechts vorgepackt; restliche ggf. in OpenSCAD aus `part=front_quarter` / `diffuser_quarter` exportieren.
- Die gesamte 30cm-Uhr ist von der Rahmenwahl abhängig. Bitte Grundplatte/Front nicht endgültig zuschneiden oder verkleben, bevor Profilquerschnitt und Befestigung entschieden sind.

## Nächster Chat: Leiterplatte / Stromverteiler
Ziel: Verdrahtungsplan und möglichst einfach herstellbare Trägerplatine/Lochrasterplanung für ESP32, 74AHCT125, Sicherungen, externe 5-V-10-A-Versorgung, LED-Datenkette, 5 abgesicherte Stromgruppen (je 2 Reihen) und eigenes Minutenmodul, BH1750 (optional), Taster, RTC DS3231 (optional), USB/5-V-Rückspeiseschutz, Schraubklemmen und Kabelanschlüsse.

**Noch zu klären bzw. zu prüfen:** exaktes Modell/Stecker des bestellten Netzteils, exakte ESP32-Pinbelegung, tatsächliches 74AHCT125-Modul oder nackter DIP-IC, ob BH1750/RTC/Taster bereits gekauft, Leitungslängen, Platz unter der Uhr, Lötkenntnisse. Nicht voraussetzen, dass jede im Chat diskutierte Option auch bestellt wurde.

**Empfehlung:** Start im nächsten Chat mit konkreter Pin-/Versorgungsmatrix und Schaltplan, bevor PCB-Layout oder Löten beginnt. Bei 5V/10A stromtragfähige DC-Steckverbindung, abgesicherte Zweige, Netzteilreserve und geeignete Leitungsquerschnitte berechnen. Kein offenes 230V-AC-Netzteil im Holzrahmen.

## Software-Ideen für später
ESP32-Webserver für Farben, Helligkeit, Farbverläufe, Effekte, Profile, Nachtmodus, Zeit/NTP, Wecker, Temperatur-/Lichtsensor optional, OTA. WLAN-AP mit Captive Portal, QR-Code WLAN-Zugang und zweiter QR-Link `http://wordclock.local` (automatisches Öffnen nach WLAN-Verbindung nicht auf allen Smartphones garantiert). Alexa on/off/Farbe/Helligkeit z.B. über passende Integration, Alarmroutine später separat testen. Software noch **nicht implementiert**.

## Druck und Montage – nächster praktischer Schritt
1. Testsegment BR, Front und Diffusor aus v3 drucken (schwarz/weiß, 0,4-mm-Düse, 0,2-mm-Schicht, 100% Skalierung, Support-Vorschau prüfen).
2. LED-Band trocken darunterlegen, Blick auf LED-Ausrichtung, Durchlass und Segmentfuge.
3. Dünnen PLA-Diffusor mit Backpapier vergleichen; keine Kleberreste unter Buchstaben.
4. Gehäusematerial und tatsächlichen Profilquerschnitt entscheiden; Rückwand und Befestigungen planen.
