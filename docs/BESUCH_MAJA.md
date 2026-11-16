# Nächster Besuch bei Maja – Checkliste

Stand 16.11.2026. Laptop mitnehmen (für Tests und Updates per WLAN). Vor jedem Update prüfen, dass niemand spielt (`/v1/games` → `"ph":"idle"`).

## 1. Fernzugriff (VPN) reparieren – FRITZ!Box
Problem: Im WireGuard-Zugang steht als Endpunkt eine feste IPv6-Adresse (`[2003:c1:…]`) statt des MyFRITZ!-Namens. Die Telekom vergibt die Adresse neu → Tunnel tot. Außerdem steht `0.0.0.0/0` bei den zulässigen IPs → ohne Tunnel kein Internet am iPhone.

- [ ] **Internet → Online-Monitor:** notieren, ob öffentliche IPv4 oder „DS-Lite“/nur IPv6.
- [ ] **Internet → MyFRITZ!-Konto:** prüfen, dass MyFRITZ! aktiv ist und eine Adresse `….myfritz.net` angezeigt wird (ggf. einrichten, Bestätigungs-Mail anklicken). Adresse notieren.
- [ ] **Internet → Freigaben → VPN (WireGuard):** alten Zugang „Jannik Wordclock“ löschen, **neu anlegen** (Einzelnes Gerät, **nur Heimnetz**, nicht „gesamter Internetverkehr“).
- [ ] Im neuen QR-Code bzw. in der App prüfen: **Endpunkt = `….myfritz.net:<Port>`**, **zulässige IPs nur `192.168.178.0/24`** (+ fdc4-Eintrag), kein `0.0.0.0/0`.
- [ ] iPhone: alten Tunnel in der WireGuard-App löschen, neuen QR-Code scannen.
- [ ] Test **mit WLAN aus, nur LTE**: VPN an → „Letzter Handshake“ zeigt eine Zeit → `http://192.168.178.43` lädt → Google geht auch.

## 1b. Fernzugriff auf Majas FRITZ!Box (Einstellungen jederzeit von zu Hause)
Empfehlung: **über dasselbe WireGuard-VPN** (Abschnitt 1) – dann ist die FRITZ!Box nichts im Internet offen, du rufst mit VPN einfach `http://fritz.box` bzw. `http://192.168.178.1` auf.
- [ ] Mit Maja absprechen (Zugriff aufs ganze Heimnetz und ihre Box).
- [ ] **System → FRITZ!Box-Benutzer → Benutzer hinzufügen:** eigener Benutzer für dich (Name + starkes Passwort, nicht das Admin-Passwort der Uhr). Rechte: **„FRITZ!Box Einstellungen“** (und „VPN“). „Zugang aus dem Internet“ dafür **nicht** nötig.
- [ ] **System → FRITZ!Box-Benutzer → Anmeldung im Heimnetz:** auf „mit FRITZ!Box-Benutzernamen und Kennwort“ stellen (falls noch nur Kennwort).
- [ ] Test von unterwegs (LTE, VPN an): `http://192.168.178.1` → Anmeldung mit deinem Benutzer klappt.
- [ ] Bestätigungen „am Gerät per Taste“ (für neue VPN-Zugänge u. ä.) gehen aus der Ferne nicht → dafür ggf. Maja anrufen.
- [ ] Alternative ohne VPN (nur falls VPN nicht klappt): MyFRITZ!-Internetzugriff per HTTPS für deinen Benutzer freischalten („Zugang auch aus dem Internet erlaubt“) – dann ist die Anmeldeseite im Internet erreichbar, also nur mit sehr starkem Passwort + 2-Faktor (System → FRITZ!Box-Benutzer → Anmeldung → Zusätzliche Bestätigung).

## 2. Verbinden vor Ort (mehrere Handys klappen nicht auf Anhieb)
Symptom: Seite lädt am iPhone nicht, erst nach WLAN aus/an, warten, nochmal. Mit VPN oder vom Laptop ging es. Vermutung: Uhr merkt sich zu einer IP eine veraltete WLAN-Kennung (ARP); Minuten-Leerung (1.4.3) hat 8-s-Hänger erzeugt und wurde verworfen.

- [ ] **Majas iPhone:** Einstellungen → WLAN → (i) „MiCasaEsSuCasa“ → **Private WLAN-Adresse: Aus**, WLAN aus/an.
- [ ] **Dein iPhone:** gleiche Einstellung prüfen (war am 14.11. schon aus).
- [ ] Mit Laptop im WLAN gemeinsam mit Claude Fehlersuche: beide iPhones nacheinander/gleichzeitig verbinden lassen, dabei `/v1/info` (`ws`, `drops`) und ARP-Tabelle beobachten. Wenn es hängt: sofort Bescheid geben, nicht gleich WLAN aus/an (dann sehen wir den Fehler live).
- [ ] FRITZ!Box: **WLAN → Funkkanal / Mesh** – notieren, ob 2,4 und 5 GHz gleichen Namen haben; **WLAN → Sicherheit → „WLAN-Geräte dürfen untereinander kommunizieren“** muss an sein.

## 3. Spielen zu zweit (Firmware 1.4.5) bestätigen
- [ ] Beide iPhones: Seite neu laden (neue Version mit Lebenszeichen).
- [ ] Ein paar Runden Pong / Tetris / Vier gewinnt zu zweit – fliegt jemand raus? Wenn ja: Uhrzeit merken, Claude schaut in `drops`.
- [ ] Offen von früher: Wischen bei Snake/T.R.O.N, QR-Code „Mitspieler einladen“, Farbwähler („+“-Kreis) am iPhone.

## 4. Einstellungen an der Uhr
- [x] **Admin-Passwort geändert** (vom Nutzer erledigt, 16.11.). Prüfen, dass `WORDCLOCK_OTA_PASSWORD` (Windows-Benutzerumgebung) das neue Passwort enthält, sonst gehen Update-/Foto-Skripte nicht.
- [ ] **Majas Geburtstag eintragen** (Effekte → Geburtstag; Standard aus, 01.01.).
- [ ] Alexa: Uhr noch in der Alexa-App vorhanden und schaltbar?

## 5. Backlog (vorher am Ersatzboard umsetzen, dann vor Ort aufspielen)
- [ ] Pong: Endlos-Ballwechsel bei schräg gegenüberstehenden Schlägern.
- [ ] Verloren-Foto: schwarzen Hintergrund entfernen (Foto privat, nie ins Repo).
- [ ] Laufschrift: Tempo-Regler wirkt nicht.
- [ ] Danach GitHub-Release (Tag ≥ v1.4.6), damit Auto-Update wieder den aktuellen Stand kennt.
