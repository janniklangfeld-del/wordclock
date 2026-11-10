#pragma once
#include <Arduino.h>
#include <FastLED.h>

// Systemzustand: steuert, was die Uhr zeigt, solange (noch) keine Uhrzeit da ist,
// und Overlays (Einrichtungsportal, Update-Fortschritt).
enum class SysState : uint8_t {
  Booting,     // weiss
  Connecting,  // gelb, blinkend
  Portal,      // blau, atmend  -> mit "Majas-Wordclock" verbinden
  Online,      // gruen, kurz
  Updating,    // lila, Matrix zeigt Fortschritt
  Error,       // rot (10 s)
};

namespace display {
void begin();
void setState(SysState s);
SysState state();
void setProgress(uint8_t percent);  // nur im Zustand Updating
void testPattern();                 // Lauflicht (Weboberflaeche)
void replayTransition();            // aktuellen Uebergang vorfuehren
void settingsChanged();
void loop();
const CRGB* frame();                // aktuelle Farben (vor Helligkeit), LED_COUNT Eintraege
uint8_t brightness();               // aktuelle Gesamthelligkeit 0..255
bool timeValid();
}  // namespace display
