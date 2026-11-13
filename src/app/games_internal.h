#pragma once
// Gemeinsame Bausteine fuer die Spiele (nur games.cpp / games_list.cpp).
#include <Arduino.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include "games.h"

namespace games {

constexpr int GW = 11;  // Spielfeld = Buchstabenfelder
constexpr int GH = 10;

// Spielerfarben: Pink und Tuerkis (dieselben wie auf dem Handy-Controller)
const CRGB P_COL[2] = {CRGB(255, 25, 130), CRGB(0, 215, 185)};

struct Canvas {
  CRGB px[GH][GW];
  CRGB dots[4];
  void clear() {
    for (auto& r : px)
      for (auto& c : r) c = CRGB::Black;
    for (auto& d : dots) d = CRGB::Black;
  }
  void set(int x, int y, CRGB c) {
    if (x >= 0 && x < GW && y >= 0 && y < GH) px[y][x] = c;
  }
  void add(int x, int y, CRGB c) {
    if (x >= 0 && x < GW && y >= 0 && y < GH) px[y][x] += c;
  }
  void dim(uint8_t keep) {
    for (auto& r : px)
      for (auto& c : r) c.nscale8_video(keep);
    for (auto& d : dots) d.nscale8_video(keep);
  }
  void digit(int x, int y, int d, CRGB c);           // 3 x 5
  void number(int value, int y, CRGB c);             // zentriert, bis 3 Stellen
  void glyph(int x, int y, char ch, CRGB c);         // 5 x 7 (Schrift der Begruessung)
  void score(int a, int b);                          // "a:b" in Spielerfarben
};

// Basisklasse. Zeit "t" ist die Spielzeit (steht in der Pause still) und wird von
// der Engine vor jedem Aufruf gesetzt.
class Game {
 public:
  uint32_t t = 0;
  uint8_t humans = 1;  // Spieler am Handy
  bool bot = false;    // Platz 2 spielt "MJ-Bot"
  uint32_t startT = 0;

  virtual ~Game() {}
  virtual void start() = 0;
  virtual void key(uint8_t p, uint8_t k, bool down) = 0;
  virtual void tick() = 0;
  virtual void draw(Canvas& c) = 0;
  virtual bool over() const = 0;
  virtual int8_t winner() const { return -1; }      // 0 / 1, 2 = unentschieden, -1 = Einzelspiel
  virtual int32_t score(uint8_t) const { return 0; }
  virtual int32_t bigNumber() const { return score(0); }  // Endstand auf der Uhr (Einzelspiel)
  virtual void live(char* buf, size_t n) const { buf[0] = 0; }  // Zeile fuer den Controller
  virtual void results(JsonArray rows) const = 0;   // Statistik: [Bezeichnung, Wert1, Wert2?]
  uint32_t seconds() const { return (t - startT) / 1000; }
};

Game* create(uint8_t id);

// Hilfsfunktion fuer Statistik-Zeilen
inline void row(JsonArray rows, const char* label, const String& a, const String& b = String()) {
  JsonArray r = rows.add<JsonArray>();
  r.add(label);
  r.add(a);
  if (b.length()) r.add(b);
}
String mmss(uint32_t s);

}  // namespace games
