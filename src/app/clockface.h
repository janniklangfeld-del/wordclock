#pragma once
#include <Arduino.h>
#include <bitset>
#include "settings.h"

// Deutsche Uhrzeit -> Buchstaben. Die Matrix entspricht cad/Wordclock_30x30_v3.scad.
struct Word {
  uint8_t row, col, len;
};
using LetterMask = std::bitset<110>;  // Index = Reihe * 11 + Spalte

namespace clockface {
extern const char* const LETTERS[10];  // UTF-8, je 11 Zeichen

struct Face {
  LetterMask mask;
  uint8_t dots = 0;  // 0..4 Minutenpunkte
  Word words[8];
  uint8_t wordCount = 0;
};

Face compute(int hour24, int minute, const Settings& s);
int letterLed(int row, int col, int half);  // half: 0 = linke, 1 = rechte LED
int dotLed(int k);                          // k: 0 = linker Punkt .. 3 = rechter
}  // namespace clockface
