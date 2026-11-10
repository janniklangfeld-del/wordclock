#pragma once
#include <Arduino.h>

class WebServer;

// Alexa-Steuerung ohne Cloud und ohne Skill: die Uhr meldet sich im Heimnetz als
// Philips-Hue-kompatible Farblampe (Espalexa). "Alexa, suche nach Geraeten" ->
// danach z. B. "Alexa, Wordclock auf 50 Prozent" / "... rot" / "... aus".
namespace alexa {
void begin(WebServer* server);  // nach WLAN-Verbindung, einmalig
void loop();
void sync();                    // Einstellungen -> Alexa-Zustand
bool running();
bool handleApi(const String& uri, const String& body);  // aus onNotFound
}  // namespace alexa
