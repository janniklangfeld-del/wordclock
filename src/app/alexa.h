#pragma once
#include <Arduino.h>

class AsyncWebServer;
class AsyncWebServerRequest;

// Alexa-Steuerung ohne Cloud und ohne Skill: die Uhr meldet sich im Heimnetz als
// Philips-Hue-kompatible Farblampe (Espalexa). "Alexa, suche nach Geraeten" ->
// danach z. B. "Alexa, Wordclock auf 50 Prozent" / "... rot" / "... aus".
namespace alexa {
void begin(AsyncWebServer* server);  // nach WLAN-Verbindung, einmalig
void loop();
void sync();                    // Einstellungen -> Alexa-Zustand
bool running();
bool handleApi(AsyncWebServerRequest* request);  // aus onNotFound (unter web::lock)
}  // namespace alexa
