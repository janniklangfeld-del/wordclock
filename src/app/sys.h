#pragma once
#include <Arduino.h>

// Systemwaechter: Neustart-Grund merken und anzeigen, Wachhund gegen Haenger.
//   Haengt die Hauptschleife laenger als WDT_SECONDS, startet die Uhr neu, statt
//   eingefroren stehen zu bleiben (z. B. mitten in der Begruessung).
//   Bei Updates (dauern laenger) wird der Wachhund pausiert.
namespace sys {
constexpr int WDT_SECONDS = 60;
void begin();        // ganz am Anfang von setup()
void feed();         // regelmaessig aus Schleifen aufrufen
void pauseWatchdog();   // vor langen, absichtlich blockierenden Vorgaengen (Update)
void resumeWatchdog();
const char* resetReason();  // Grund des letzten Neustarts (deutsch)
uint32_t boots();           // Anzahl Starts seit Einbau
uint32_t crashes();         // davon unerwartet (Absturz, Wachhund, Spannungseinbruch)
}  // namespace sys
