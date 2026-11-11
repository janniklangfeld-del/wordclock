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
void testPattern();                 // Lauflicht (alte Schnittstelle)

// LED-Testmodus (versteckt in der Weboberflaeche). Ueberlagert alles andere,
// endet automatisch 10 min nach dem letzten Befehl.
enum TestMode : uint8_t { TEST_OFF, TEST_CHASE, TEST_ROWS, TEST_LETTERS, TEST_RGBW, TEST_COLOR, TEST_LED, TEST_DOTS };
void setTest(TestMode mode, CRGB color, uint16_t index, uint8_t bri, uint8_t speed);
bool testActive();
void replayTransition();            // aktuellen Uebergang vorfuehren
void playIntro();                   // Begruessung: Laufschrift, Herz, Funken, dann Uhr
bool introRunning();
void settingsChanged();
void loop();
const CRGB* frame();                // aktuelle Farben (vor Helligkeit), LED_COUNT Eintraege
uint8_t brightness();               // aktuelle Gesamthelligkeit 0..255
bool timeValid();
}  // namespace display
