#pragma once
#include <Arduino.h>

// WLAN-Verwaltung – die Uhr ist IMMER neu einrichtbar:
//   * Keine/ungueltige Zugangsdaten  -> Portal "Wordclock-Setup" startet sofort
//   * WLAN laenger als 5 min weg      -> Portal oeffnet zusaetzlich (Uhr versucht
//                                       parallel weiter, das alte WLAN zu erreichen)
//   * BOOT-Taste 5 s halten           -> Portal
//   * Weboberflaeche "WLAN aendern"   -> Portal
// Neue Zugangsdaten werden erst nach erfolgreicher Verbindung uebernommen.
namespace net {
constexpr const char* HOSTNAME = "wordclock";   // -> http://wordclock.local
constexpr const char* AP_NAME  = "Wordclock-Setup";

void begin();
void loop();
bool online();
bool portalActive();
void openPortal();
void requestPortal();  // sicher aus Web-Handlern aufrufbar
}  // namespace net
