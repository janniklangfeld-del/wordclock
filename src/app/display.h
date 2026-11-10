#pragma once
#include <Arduino.h>

// LED-Ausgabe. Solange die Uhrzeit-Logik noch nicht existiert, zeigen die
// vier Minutenpunkte den Systemzustand an.
enum class SysState : uint8_t {
  Booting,     // weiss
  Connecting,  // gelb, blinkend
  Portal,      // blau, atmend  -> mit "Wordclock-Setup" verbinden
  Online,      // gruen, kurz – danach dunkel mit Herzschlag
  Updating,    // lila, Matrix zeigt Fortschritt
  Error,       // rot
};

namespace display {
void begin();
void setState(SysState s);
SysState state();
void setProgress(uint8_t percent);  // nur im Zustand Updating
void setBrightness(uint8_t b);
void testPattern();                 // kurzer Lauflicht-Test (Weboberflaeche)
void loop();
}  // namespace display
