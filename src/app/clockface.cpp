#include "clockface.h"
#include "hardware.h"

namespace {
//                Reihe, Spalte, Laenge          Reihe:  0 ESKISTLFÜNF
constexpr Word ES{0, 0, 2}, IST{0, 3, 3};         //      1 ZEHNZWANZIG
constexpr Word M_FUENF{0, 7, 4};                  //      2 DREIVIERTEL
constexpr Word M_ZEHN{1, 0, 4}, ZWANZIG{1, 4, 7}; //      3 VORFUNKNACH
constexpr Word DREIVIERTEL{2, 0, 11};             //      4 HALBAELFÜNF
constexpr Word VIERTEL{2, 4, 7};                  //      5 EINSXAMZWEI
constexpr Word VOR{3, 0, 3}, NACH{3, 7, 4};       //      6 DREIPMJVIER
constexpr Word HALB{4, 0, 4};                     //      7 SECHSNLACHT
constexpr Word EIN{5, 0, 3};                      //      8 SIEBENZWÖLF
constexpr Word UHR{9, 8, 3};                      //      9 ZEHNEUNKUHR

// Index = Stunde % 12 (0 = zwoelf)
constexpr Word HOURS[12] = {
    {8, 6, 5},  // ZWÖLF
    {5, 0, 4},  // EINS
    {5, 7, 4},  // ZWEI
    {6, 0, 4},  // DREI
    {6, 7, 4},  // VIER
    {4, 7, 4},  // FÜNF
    {7, 0, 5},  // SECHS
    {8, 0, 6},  // SIEBEN
    {7, 7, 4},  // ACHT
    {9, 3, 4},  // NEUN
    {9, 0, 4},  // ZEHN
    {4, 5, 3},  // ELF
};
}  // namespace

namespace clockface {

const char* const LETTERS[10] = {
    "ESKISTLFÜNF", "ZEHNZWANZIG", "DREIVIERTEL", "VORFUNKNACH", "HALBAELFÜNF",
    "EINSXAMZWEI", "DREIPMJVIER", "SECHSNLACHT", "SIEBENZWÖLF", "ZEHNEUNKUHR",
};

Face compute(int hour24, int minute, const Settings& s) {
  Face f;
  auto add = [&](const Word& w) {
    if (f.wordCount < 8) f.words[f.wordCount++] = w;
    for (int i = 0; i < w.len; i++) f.mask.set(w.row * LETTER_COLS + w.col + i);
  };
  const int five = minute / 5;
  const int h = hour24 % 12;
  const int next = (h + 1) % 12;
  auto hour = [&](int x) { add(HOURS[x % 12]); };
  f.dots = minute % 5;

  if (s.prefix == 0 || (s.prefix == 1 && (five == 0 || five == 6))) {
    add(ES);
    add(IST);
  }
  switch (five) {
    case 0:  // drei Uhr / ein Uhr
      if (h == 1) add(EIN); else hour(h);
      add(UHR);
      break;
    case 1: add(M_FUENF); add(NACH); hour(h); break;
    case 2: add(M_ZEHN); add(NACH); hour(h); break;
    case 3:
      if (s.quarter == 1) { add(VIERTEL); hour(next); }
      else { add(VIERTEL); add(NACH); hour(h); }
      break;
    case 4:
      if (s.twenty == 1) { add(M_ZEHN); add(VOR); add(HALB); hour(next); }
      else { add(ZWANZIG); add(NACH); hour(h); }
      break;
    case 5: add(M_FUENF); add(VOR); add(HALB); hour(next); break;
    case 6: add(HALB); hour(next); break;
    case 7: add(M_FUENF); add(NACH); add(HALB); hour(next); break;
    case 8:
      if (s.twenty == 1) { add(M_ZEHN); add(NACH); add(HALB); hour(next); }
      else { add(ZWANZIG); add(VOR); hour(next); }
      break;
    case 9:
      if (s.threeQuarter == 1) { add(DREIVIERTEL); hour(next); }
      else { add(VIERTEL); add(VOR); hour(next); }
      break;
    case 10: add(M_ZEHN); add(VOR); hour(next); break;
    default: add(M_FUENF); add(VOR); hour(next); break;
  }
  return f;
}

int letterLed(int row, int col, int half) {
  int pos = col * 2 + half;
  bool leftToRight = (row % 2 == 0) == LED_START_TOP_LEFT;
  return row * LEDS_PER_ROW + (leftToRight ? pos : LEDS_PER_ROW - 1 - pos);
}

int dotLed(int k) { return LED_MATRIX_COUNT + (LED_START_TOP_LEFT ? k : LED_MINUTE_COUNT - 1 - k); }

}  // namespace clockface
