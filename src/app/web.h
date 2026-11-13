#pragma once

#include <Arduino.h>
class WebServer;

// Weboberflaeche unter http://wordclock.local (bzw. IP), Port 80.
// Live-Bild der LEDs per WebSocket auf Port 81.
//
// REST (JSON):
//   GET  /v1/settings, POST /v1/settings (Teil-JSON)    – frei im Heimnetz
//   GET  /v1/info, GET /v1/layout, POST /v1/replay, POST /v1/test
//   POST /v1/check, /v1/reboot, /v1/wifi, /v1/password  – Benutzer "admin" + Admin-Passwort
//   GET/POST /update (Firmware-Datei)                    – Benutzer "admin" + Admin-Passwort
// /api/... ist fuer Alexa (Hue-Emulation) reserviert.
namespace web {
void begin();
void end();
bool running();
void loop();
void notifySettings();  // geaenderte Einstellungen an alle offenen Seiten
WebServer* server();
bool sendTo(uint8_t client, const String& text);  // WebSocket-Text an ein Handy
bool clientConnected(uint8_t client);
}  // namespace web
