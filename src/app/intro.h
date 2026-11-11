#pragma once
#include <Arduino.h>

// Pixelschrift 5x7 fuer die Begruessung beim Einschalten ("MAJA" als Laufschrift).
// Die 11 x 10 Buchstabenfelder dienen als Pixel. Jedes Zeichen ist 5 Spalten breit,
// Bit 0 = oberste Zeile.
namespace intro {

// Text (UTF-8) in Pixelspalten wandeln. Rueckgabe: Anzahl Spalten (max. maxCols).
// Zwischen Zeichen liegt eine leere Spalte. "<3" oder "♥" wird ein Herz.
int toColumns(const String& text, uint8_t* cols, int maxCols);

}  // namespace intro
