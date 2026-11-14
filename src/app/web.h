#pragma once

#include <Arduino.h>
class AsyncWebServer;

// Weboberflaeche unter http://wordclock.local (bzw. IP), alles auf Port 80, asynchron.
// Live-Bild der LEDs und Spiel-Controller per WebSocket unter ws://<uhr>/ws.
//
// REST (JSON):
//   GET  /v1/settings, POST /v1/settings (Teil-JSON)    – frei im Heimnetz
//   GET  /v1/info, GET /v1/layout, POST /v1/replay, POST /v1/test
//   POST /v1/check, /v1/reboot, /v1/wifi, /v1/password  – Benutzer "admin" + Admin-Passwort
//   POST /update (Firmware-Datei, multipart)             – Benutzer "admin" + Admin-Passwort
// /api/... ist fuer Alexa (Hue-Emulation) reserviert.
namespace web {
void begin();
void end();
bool running();
void loop();
void notifySettings();  // geaenderte Einstellungen an alle offenen Seiten
AsyncWebServer* server();
bool sendTo(uint8_t client, const String& text);  // WebSocket-Text an ein Handy
bool clientConnected(uint8_t client);

// Sperre fuer gemeinsame Daten: die Hauptschleife haelt sie waehrend jedes Durchlaufs,
// Web-Anfragen holen sie sich dazwischen (siehe web.cpp).
void lock();
void unlock();
}  // namespace web
