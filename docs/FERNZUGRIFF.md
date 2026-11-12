# Fernzugriff auf die Uhr – WireGuard-VPN der FRITZ!Box

Ziel: Von unterwegs mit dem Handy die Webseite der Uhr in Majas Heimnetz öffnen (Texte, Laufschrift, Einstellungen, Live-Ansicht), **ohne Portfreigabe** und ohne Extra-Gerät. Die Uhr selbst muss dafür nicht geändert werden.

Voraussetzungen: FRITZ!Box mit **FRITZ!OS 7.50 oder neuer** (System → Update), Admin-Zugang zur FRITZ!Box, einmal vor Ort sein (Bestätigung am Gerät).

## 1. Anschluss prüfen (Glasfaser!)

FRITZ!Box → **Internet → Online-Monitor**:

- Steht dort eine **öffentliche IPv4-Adresse** → alles gut, VPN geht von überall.
- Steht dort **„DS-Lite“** (z. B. bei Deutsche Glasfaser üblich) → VPN geht nur über **IPv6**. Mobilfunk (Telekom, Vodafone, o2) kann IPv6 meist → klappt unterwegs. In fremden WLANs (Hotel, Café) oft nur IPv4 → dann im Zweifel Mobilfunk nehmen.

## 2. MyFRITZ! einrichten (feste Adresse)

**Internet → MyFRITZ!-Konto** → mit E-Mail registrieren, Bestätigungs-Mail anklicken. Danach hat die Box eine Adresse wie `xxxxxxxx.myfritz.net` – die nutzt die VPN-Konfiguration automatisch.

## 3. Uhr immer gleiche Adresse geben

**Heimnetz → Netzwerk → Netzwerkverbindungen** → Gerät **„wordclock“** → Bearbeiten → **„Diesem Netzwerkgerät immer die gleiche IPv4-Adresse zuweisen“** → OK. IP notieren (z. B. `192.168.178.50`).

## 4. VPN für dein Handy anlegen

1. **Internet → Freigaben → VPN (WireGuard)** → WireGuard aktivieren (falls gefragt).
2. **„Verbindung hinzufügen“** → **„Vereinfachte Einrichtung“** → **„Einzelnes Gerät (Smartphone, Tablet, Notebook) verbinden“**.
3. Name vergeben, z. B. „Handy Wordclock“. Frage nach Internetverkehr: **nur Heimnetz** reicht (sonst läuft dein ganzer Handy-Verkehr über Majas Anschluss).
4. Bestätigen – je nach Box am **Gerät per Taste** oder per DECT-Telefon.
5. Es erscheint ein **QR-Code** (und eine Datei zum Herunterladen – gut aufheben, wie ein Schlüssel).

## 5. Handy einrichten

1. App **„WireGuard“** installieren (iOS/Android, kostenlos).
2. **+ → QR-Code scannen** → Namen geben → Erlauben.
3. Schalter in der App an → verbunden.

## 6. Uhr aufrufen

Mit eingeschaltetem VPN im Browser:

- **`http://wordclock.fritz.box`** (die FRITZ!Box kennt den Namen der Uhr), oder
- **`http://192.168.178.50`** (die IP aus Schritt 3).

`wordclock.local` funktioniert über VPN **nicht** (der Name wird nur direkt im WLAN aufgelöst). Lesezeichen bzw. „Zum Home-Bildschirm“ auf die fritz.box-Adresse setzen.

Alles geht wie zu Hause: Einstellungen, Laufschrift (Anzeige → Eigener Text → Laufschrift), Live-3D-Ansicht. Danach VPN wieder ausschalten.

## Hinweise

- Über das VPN ist **nichts im Internet offen** – nur Geräte mit dem Schlüssel kommen ins Heimnetz. Deshalb ist keine Portfreigabe und kein zusätzlicher Login an der Uhr nötig.
- Der VPN-Zugang erlaubt dir Zugriff auf **Majas ganzes Heimnetz** – mit ihr absprechen. Zugang jederzeit in der FRITZ!Box unter VPN (WireGuard) löschbar.
- Admin-Passwort der Uhr (Standard `wordclock`) trotzdem ändern: System → Wartung → Admin-Passwort.
- Firmware-Updates kommen ohnehin über GitHub (siehe docs/OTA.md), dafür braucht es das VPN nicht.
