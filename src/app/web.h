#pragma once

// Weboberflaeche unter http://wordclock.local (bzw. IP).
// Aenderungen sind mit Benutzer "admin" + Admin-Passwort geschuetzt.
namespace web {
void begin();
void end();
bool running();
void loop();
}  // namespace web
