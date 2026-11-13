// Weitere Spiele der MJ Arcade: Wortblitz, Tauziehen, Seeschlacht, Herzregen,
// Mauerbrecher, Huerdenlauf. Spielfeld 11 x 10 Buchstabenfelder, y = 0 oben.
#include "clockface.h"
#include "games_internal.h"

namespace games {
namespace {

int rnd(int lo, int hi) { return lo + (int)(esp_random() % (uint32_t)(hi - lo + 1)); }  // lo..hi
float frnd() { return (esp_random() % 10000) / 10000.0f; }                               // 0..1
void fillAll(Canvas& c, CRGB col) {
  for (int y = 0; y < GH; y++)
    for (int x = 0; x < GW; x++) c.set(x, y, col);
}

// ===========================================================================
// Wortblitz: Die Buchstaben eines Wortes blitzen nacheinander an ihren Plaetzen im
// Raster der Uhr auf. Auf dem Handy stehen 4 Woerter – wer es zuerst erkennt.
// ===========================================================================
const char* const WORDS[] = {
    "MAJA",    "HERZ",    "LIEBE",   "KUSS",     "SONNE",   "BLUME",   "KATZE",    "PIZZA",    "TORTE",   "KUCHEN",
    "FREUDE",  "SCHATZ",  "ENGEL",   "WOLKE",    "SOMMER",  "STERN",   "MOND",     "FISCH",    "VOGEL",   "NACHT",
    "ZAUBER",  "WUNDER",  "MUSIK",   "TANZEN",   "LACHEN",  "FREUND",  "HIMMEL",   "KERZE",    "GESCHENK", "BLUMEN",
    "HUND",    "MAUS",    "PFERD",   "FUCHS",    "BIENE",   "AFFE",    "TIGER",    "ZEBRA",    "PINGUIN", "MELONE",
    "BANANE",  "ZITRONE", "APFEL",   "BIRNE",    "KIRSCHE", "ERDBEERE", "MÖWE",    "FRÜHLING", "WINTER",  "HERBST",
    "REGEN",   "SCHNEE",  "STRAND",  "INSEL",    "WELLE",   "BERG",    "WALD",     "GARTEN",   "KAFFEE",  "KEKS",
    "TRAUM",   "KÜSSCHEN", "UMARMUNG", "LIEBLING", "HASE",   "EULE",    "IGEL",     "ELCH",     "WAL",     "SCHAF",
    "KLAVIER", "GITARRE", "TROMMEL", "FILM",     "BUCH",    "BRIEF",   "KRONE",    "PRINZ",    "ZWERG",   "HEXE",
    "DRACHE",  "RAKETE",  "PLANET",  "KOMET",    "SALAT",   "NUDELN",  "BREZEL",   "WAFFEL",   "MUFFIN",  "LIMO",
};
constexpr int N_WORDS = sizeof(WORDS) / sizeof(WORDS[0]);

int decode(const char* s, uint16_t* out, int max) {  // UTF-8 -> Zeichen (nur 1- und 2-Byte-Folgen)
  int n = 0;
  for (const uint8_t* p = (const uint8_t*)s; *p && n < max;) {
    if (*p < 0x80) out[n++] = *p++;
    else if ((*p & 0xE0) == 0xC0 && p[1]) { out[n++] = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F); p += 2; }
    else p++;
  }
  return n;
}
uint16_t GRID[GH][GW];
bool gridReady = false;
bool wordOk[N_WORDS];
void prepareWords() {
  if (gridReady) return;
  for (int y = 0; y < GH; y++) decode(clockface::LETTERS[y], GRID[y], GW);
  for (int w = 0; w < N_WORDS; w++) {  // passt das Wort ins Raster (genug passende Buchstaben)?
    uint16_t ch[16];
    const int n = decode(WORDS[w], ch, 16);
    bool ok = n >= 3 && n <= 10;
    for (int i = 0; i < n && ok; i++) {
      int need = 0, have = 0;
      for (int j = 0; j < n; j++) need += ch[j] == ch[i];
      for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) have += GRID[y][x] == ch[i];
      ok = have >= need;
    }
    wordOk[w] = ok;
  }
  gridReady = true;
}

class Wort : public Game {
  struct Pos { int8_t x, y; };
  Pos pos[10];
  int len = 0, word = 0, opt[4] = {0, 0, 0, 0}, ok = 0;
  enum { SHOW, RES } ph = SHOW;
  uint32_t phT = 0, endT = 0, step = 480;
  int8_t ans[2] = {-1, -1}, roundWin = -1;
  int pts[2] = {0, 0}, rounds = 0, right[2] = {0, 0}, wrong[2] = {0, 0};
  uint8_t recent[8];
  int nRecent = 0;

  int n() const { return humans == 2 ? 2 : 1; }
  int shown() const { return min<int>(len, (t - phT) / step + 1); }  // Buchstaben im ersten Durchlauf
  int pick(int notA, int lenHint) {
    for (int tries = 0; tries < 60; tries++) {
      const int w = rnd(0, N_WORDS - 1);
      if (!wordOk[w] || w == notA) continue;
      bool dup = false;
      for (int i = 0; i < 4; i++) dup |= opt[i] == w;
      for (int i = 0; i < nRecent; i++) dup |= recent[i] == w;
      if (dup) continue;
      uint16_t ch[16];
      if (lenHint && tries < 40 && abs(decode(WORDS[w], ch, 16) - lenHint) > 1) continue;
      return w;
    }
    for (int w = 0; w < N_WORDS; w++)
      if (wordOk[w] && w != notA) return w;
    return 0;
  }
  void next() {
    for (int i = 0; i < 4; i++) opt[i] = -1;
    word = pick(-1, 0);
    if (nRecent == 8) memmove(recent, recent + 1, --nRecent);
    recent[nRecent++] = word;
    uint16_t ch[16];
    len = decode(WORDS[word], ch, 10);
    bool taken[GH][GW] = {};
    for (int i = 0; i < len; i++) {  // fuer jeden Buchstaben ein zufaelliges passendes Feld
      int cand = 0;
      for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) cand += GRID[y][x] == ch[i] && !taken[y][x];
      int k = rnd(0, max(0, cand - 1));
      for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++)
          if (GRID[y][x] == ch[i] && !taken[y][x] && k-- == 0) { pos[i] = {(int8_t)x, (int8_t)y}; taken[y][x] = true; }
    }
    ok = rnd(0, 3);
    opt[ok] = word;
    for (int i = 0; i < 4; i++)
      if (i != ok) opt[i] = pick(word, len);
    ans[0] = ans[1] = -1;
    roundWin = -1;
    step = max<uint32_t>(260, 480 - rounds * 18);
    ph = SHOW;
    phT = t;
  }
  void endRound(int8_t w) {
    roundWin = w;
    ph = RES;
    phT = t;
    rounds++;
  }

 public:
  void start() override {
    prepareWords();
    next();
  }
  void key(uint8_t p, uint8_t k, bool down) override {
    if (!down || ph != SHOW || p >= n() || k > K_RIGHT || ans[p] >= 0 || endT) return;
    ans[p] = k;
    if (k == ok) {
      right[p]++;
      if (n() == 1) pts[0] += 1 + max(0, len - shown());  // frueh erkannt = mehr Punkte
      else pts[p]++;
      endRound(p);
    } else {
      wrong[p]++;
      bool all = true;
      for (int q = 0; q < n(); q++) all &= ans[q] >= 0;
      if (all) endRound(-1);
    }
  }
  void tick() override {
    if (endT) return;
    if (ph == SHOW && t - phT > 2 * len * step + 3000) endRound(-1);  // zweimal gezeigt, keiner wusste es
    if (ph == RES && t - phT > 1900) {
      if ((n() == 1 && rounds >= 10) || (n() == 2 && (pts[0] >= 5 || pts[1] >= 5 || rounds >= 15))) endT = t;
      else next();
    }
  }
  void draw(Canvas& c) override {
    if (ph == SHOW) {
      const uint32_t e = t - phT, cyc = len * step;
      const uint32_t inCyc = e % (cyc + 500);
      const int cur = inCyc < cyc ? inCyc / step : -1;
      const int seen = e >= cyc ? len : cur + 1;
      for (int i = 0; i < seen; i++) c.set(pos[i].x, pos[i].y, CRGB(40, 28, 6));  // schon gezeigt: schwach
      if (cur >= 0) {
        CRGB col = CHSV(word * 37, 160, 255);
        col.nscale8_video(255 - (inCyc % step) * 120 / step);
        c.set(pos[cur].x, pos[cur].y, col);
      }
      if (n() == 2)
        for (int p = 0; p < 2; p++) c.dots[p * 2] = c.dots[p * 2 + 1] = ans[p] >= 0 ? CRGB(120, 0, 0) : P_COL[p];
    } else {
      const CRGB col = roundWin >= 0 ? (n() == 2 ? P_COL[roundWin] : CRGB(40, 255, 60)) : CRGB(255, 90, 0);
      for (int i = 0; i < len; i++) c.set(pos[i].x, pos[i].y, col);
    }
  }
  bool over() const override { return endT && t - endT > 300; }
  int8_t winner() const override { return n() == 1 ? -1 : pts[0] > pts[1] ? 0 : pts[1] > pts[0] ? 1 : 2; }
  int32_t score(uint8_t p) const override { return pts[p & 1]; }
  int32_t bigNumber() const override { return pts[0]; }
  int32_t hsValue(uint8_t p) const override { return n() == 1 && p == 0 ? pts[0] : -1; }
  void live(char* s, size_t l) const override {
    if (n() == 1) snprintf(s, l, "Wort %d von 10", min(rounds + 1, 10));
    else snprintf(s, l, "Runde %d · erster bis 5", rounds + 1);
  }
  void extra(JsonObject o) const override {
    o["st"] = ph == SHOW ? "q" : "r";
    for (int i = 0; i < 4; i++) o["opt"].add(opt[i] >= 0 ? WORDS[opt[i]] : "");
    o["a"].add(ans[0]);
    o["a"].add(ans[1]);
    o["n"] = rounds;
    if (ph == RES) { o["ok"] = ok; o["w"] = roundWin; }
  }
  void results(JsonArray r) const override {
    if (n() == 1) {
      row(r, "Punkte", String(pts[0]));
      row(r, "Richtig", String(right[0]) + " von " + String(rounds));
      row(r, "Daneben", String(wrong[0]));
    } else {
      row(r, "Punkte", String(pts[0]), String(pts[1]));
      row(r, "Richtig", String(right[0]), String(right[1]));
      row(r, "Daneben", String(wrong[0]), String(wrong[1]));
    }
    row(r, "Spielzeit", mmss(seconds()));
  }
};

// ===========================================================================
// Tauziehen (zu zweit / gegen MJ-Bot): so schnell tippen wie moeglich. Der Knoten
// wandert zum Staerkeren; wer ihn bis an seinen Rand zieht, holt die Runde. Bis 2.
// ===========================================================================
class Tau : public Game {
  static constexpr float GOAL = 5.0f, PULL = 0.2f;
  float pos = 0;  // < 0 Richtung Spieler 1 (links), > 0 Richtung Spieler 2
  enum { PULLING, SHOW } ph = PULLING;
  uint32_t phT = 0, roundT = 0, endT = 0, botNext = 0, pullMs = 0;
  int wins[2] = {0, 0}, taps[2] = {0, 0}, rounds = 0;
  int8_t lastWin = -1;

  void tug(int p) {
    pos += p ? PULL : -PULL;
    taps[p]++;
  }
  void newRound() {
    pos = 0;
    ph = PULLING;
    roundT = phT = t;
    botNext = t + 400;
  }

 public:
  void start() override { newRound(); }
  void key(uint8_t p, uint8_t k, bool down) override {
    if (!down || ph != PULLING || p > 1 || (bot && p == 1) || (k != K_A && k != K_B) || endT) return;
    if (t - roundT < 300) return;  // kurzes "Los!"-Aufleuchten abwarten
    tug(p);
  }
  void tick() override {
    if (endT) return;
    if (ph == PULLING) {
      if (bot && t >= botNext && t - roundT >= 300) {  // MJ-Bot: ~7 Zuege/s, wird mit jeder verlorenen Runde schneller
        tug(1);
        const float rate = 6.8f + wins[0] * 0.9f;
        botNext = t + (uint32_t)(1000.0f / rate * (0.7f + 0.6f * frnd()));
      }
      if (fabsf(pos) >= GOAL || t - roundT > 20000) {
        const int8_t w = pos < -0.05f ? 0 : pos > 0.05f ? 1 : -1;
        if (w >= 0) wins[w]++;
        lastWin = w;
        pullMs += t - roundT;
        rounds++;
        ph = SHOW;
        phT = t;
      }
    } else if (t - phT > 2300) {
      if (wins[0] >= 2 || wins[1] >= 2) endT = t;
      else newRound();
    }
  }
  void draw(Canvas& c) override {
    if (ph == SHOW) {
      if (lastWin >= 0 && t - phT < 900) {
        if (((t - phT) / 150) % 2 == 0) fillAll(c, P_COL[lastWin]);
      } else c.score(wins[0], wins[1]);
    } else {
      const int mx = constrain((int)lroundf(5 + pos), 0, GW - 1);
      for (int x = 0; x < GW; x++) {
        CRGB col = x < mx ? P_COL[0] : P_COL[1];
        col.nscale8_video(x == 0 || x == GW - 1 ? 255 : 70);
        c.set(x, 4, col);
        c.set(x, 5, col);
      }
      c.set(mx, 4, CRGB(255, 255, 255));
      c.set(mx, 5, CRGB(255, 255, 255));
      const uint8_t z = beatsin8(80, 20, 120);  // Ziele blinken
      for (int y = 3; y <= 6; y++) {
        CRGB a = P_COL[0], b = P_COL[1];
        a.nscale8_video(z);
        b.nscale8_video(z);
        c.set(0, y, a);
        c.set(GW - 1, y, b);
      }
      if (t - roundT < 300) fillAll(c, CRGB(40, 255, 60));  // Los!
    }
    for (int p = 0; p < 2; p++)
      for (int k = 0; k < 2; k++) c.dots[p * 2 + k] = k < wins[p] ? P_COL[p] : CRGB::Black;
  }
  bool over() const override { return endT && t - endT > 300; }
  int8_t winner() const override { return wins[0] > wins[1] ? 0 : wins[1] > wins[0] ? 1 : 2; }
  int32_t score(uint8_t p) const override { return wins[p & 1]; }
  void live(char* s, size_t l) const override { snprintf(s, l, "Runde %d · wer zuerst 2 hat", rounds + 1); }
  void extra(JsonObject o) const override {
    o["st"] = ph == PULLING ? "pull" : "show";
    o["pos"] = (int)lroundf(pos * 10);  // -50 .. 50
  }
  void results(JsonArray r) const override {
    const float s = max<uint32_t>(pullMs, 1) / 1000.0f;
    row(r, "Runden", String(wins[0]), String(wins[1]));
    row(r, "Tipper", String(taps[0]), String(taps[1]));
    row(r, "Tipper pro Sekunde", String(taps[0] / s, 1), String(taps[1] / s, 1));
    row(r, "Spielzeit", mmss(seconds()));
  }
};

// ===========================================================================
// Seeschlacht (zu zweit / gegen MJ-Bot): je ein 5 x 5 Meer mit 3 Schiffen (3, 2, 2).
// Eigene Schiffe sieht nur das eigene Handy. Treffer = nochmal schiessen.
// Uhr: links das Meer von Spieler 1 (pink), rechts das von Spieler 2 (tuerkis).
// ===========================================================================
class See : public Game {
  static constexpr int N = 5, CELLS = N * N, SHIPS = 3;
  const int LEN[SHIPS] = {3, 2, 2};
  uint8_t ship[2][CELLS];  // Schiff-Nr. + 1 im Meer von Spieler i, 0 = Wasser
  uint8_t shot[2][CELLS];  // Schuesse auf das Meer von Spieler i: 0 nichts, 1 Wasser, 2 Treffer
  bool ready[2] = {false, false};
  enum { PLACE, AIM, FLY, RES } ph = PLACE;
  int turn = 0, shots[2] = {0, 0}, hits[2] = {0, 0}, ver = 0;
  int8_t lastSea = -1, lastCell = -1, lastRes = 0, win = -1;
  uint32_t phT = 0, endT = 0;

  bool sunk(int sea, int s) const {
    for (int c = 0; c < CELLS; c++)
      if (ship[sea][c] == s + 1 && shot[sea][c] != 2) return false;
    return true;
  }
  int left(int sea) const {
    int n = 0;
    for (int s = 0; s < SHIPS; s++) n += !sunk(sea, s);
    return n;
  }
  void place(int sea) {
    for (int attempt = 0; attempt < 400; attempt++) {
      memset(ship[sea], 0, CELLS);
      bool okAll = true;
      for (int s = 0; s < SHIPS && okAll; s++) {
        bool placed = false;
        for (int tries = 0; tries < 60 && !placed; tries++) {
          const bool hz = esp_random() & 1;
          const int x = rnd(0, hz ? N - LEN[s] : N - 1), y = rnd(0, hz ? N - 1 : N - LEN[s]);
          bool free = true;
          for (int i = 0; i < LEN[s] && free; i++) {
            const int cx = x + (hz ? i : 0), cy = y + (hz ? 0 : i);
            for (int dy = -1; dy <= 1; dy++)  // Schiffe beruehren sich nicht (bei vielen Versuchen)
              for (int dx = -1; dx <= 1; dx++) {
                const int nx = cx + dx, ny = cy + dy;
                if (nx < 0 || ny < 0 || nx >= N || ny >= N) continue;
                if (ship[sea][ny * N + nx] && (attempt < 300 || (dx == 0 && dy == 0))) free = false;
              }
          }
          if (!free) continue;
          for (int i = 0; i < LEN[s]; i++) ship[sea][(y + (hz ? 0 : i)) * N + x + (hz ? i : 0)] = s + 1;
          placed = true;
        }
        okAll = placed;
      }
      if (okAll) return;
    }
  }
  void fire(int c) {
    lastSea = 1 - turn;
    lastCell = c;
    shots[turn]++;
    ph = FLY;
    phT = t;
  }
  int botPick() const {
    const int sea = 0;  // Bot schiesst auf das Meer von Spieler 1
    int cand[CELLS], n = 0;
    for (int c = 0; c < CELLS; c++) {  // neben Treffern weitersuchen
      if (shot[sea][c] != 2 || sunk(sea, ship[sea][c] - 1)) continue;
      const int x = c % N, y = c / N;
      const int nb[4][2] = {{x - 1, y}, {x + 1, y}, {x, y - 1}, {x, y + 1}};
      for (auto& q : nb)
        if (q[0] >= 0 && q[1] >= 0 && q[0] < N && q[1] < N && !shot[sea][q[1] * N + q[0]]) cand[n++] = q[1] * N + q[0];
    }
    if (n) return cand[rnd(0, n - 1)];
    for (int c = 0; c < CELLS; c++)
      if (!shot[sea][c] && ((c % N) + (c / N)) % 2 == 0) cand[n++] = c;  // Schachbrett reicht fuer Schiffe >= 2
    if (!n)
      for (int c = 0; c < CELLS; c++)
        if (!shot[sea][c]) cand[n++] = c;
    return n ? cand[rnd(0, n - 1)] : 0;
  }

 public:
  void start() override {
    memset(shot, 0, sizeof(shot));
    place(0);
    place(1);
    ready[0] = false;
    ready[1] = bot;
    ph = PLACE;
    phT = t;
    turn = rnd(0, 1);
  }
  void key(uint8_t, uint8_t, bool) override {}
  void input(uint8_t p, JsonObjectConst m) override {
    if (p > 1 || endT) return;
    const char* a = m["a"] | "";
    if (!strcmp(a, "mix") && ph == PLACE && !ready[p]) { place(p); ver++; }
    else if (!strcmp(a, "ok") && ph == PLACE) ready[p] = true;
    else if (!strcmp(a, "shot") && ph == AIM && p == turn && !(bot && p == 1)) {
      const int c = m["c"] | -1;
      if (c >= 0 && c < CELLS && !shot[1 - turn][c]) fire(c);
    }
  }
  void tick() override {
    if (endT) return;
    switch (ph) {
      case PLACE:
        if (ready[0] && ready[1]) { ph = AIM; phT = t; }
        break;
      case AIM:
        if (bot && turn == 1 && t - phT > 1100) fire(botPick());
        break;
      case FLY:
        if (t - phT > 700) {
          const int sea = lastSea, c = lastCell;
          if (ship[sea][c]) {
            shot[sea][c] = 2;
            hits[turn]++;
            lastRes = sunk(sea, ship[sea][c] - 1) ? 3 : 2;
            if (!left(sea)) win = turn;
          } else {
            shot[sea][c] = 1;
            lastRes = 1;
          }
          ph = RES;
          phT = t;
        }
        break;
      case RES:
        if (t - phT > 1000) {
          if (win >= 0) endT = t;
          else {
            if (lastRes == 1) turn = 1 - turn;  // Wasser: der andere ist dran
            ph = AIM;
            phT = t;
          }
        }
        break;
    }
  }
  void draw(Canvas& c) override {
    for (int sea = 0; sea < 2; sea++) {
      const int ox = sea ? 6 : 0, oy = 2;
      CRGB bar = P_COL[sea];  // Farbleiste: wessen Meer (blinkt, wenn darauf gezielt wird)
      const bool target = (ph == AIM || ph == FLY) && sea == 1 - turn;
      const bool rdy = ph == PLACE && ready[sea];
      bar.nscale8_video(target ? beatsin8(60, 60, 255) : rdy ? 255 : 50);
      for (int x = 0; x < N; x++) c.set(ox + x, 0, bar);
      for (int cell = 0; cell < CELLS; cell++) {
        CRGB col(0, 0, 22);
        if (shot[sea][cell] == 1) col = CRGB(40, 70, 130);
        if (shot[sea][cell] == 2) col = sunk(sea, ship[sea][cell] - 1) ? CRGB(255, 0, 0) : CRGB(255, 120, 0);
        if (sea == lastSea && cell == lastCell) {
          if (ph == FLY) col = ((t - phT) / 90) % 2 ? CRGB(255, 255, 255) : CRGB(0, 0, 0);
          if (ph == RES && lastRes >= 2) col = ((t - phT) / 120) % 2 ? CRGB(255, 220, 0) : CRGB(255, 0, 0);
        }
        c.set(ox + cell % N, oy + cell / N, col);
      }
      for (int s = 0; s < SHIPS; s++) c.set(ox + s * 2, 8, sunk(sea, s) ? CRGB(60, 0, 0) : P_COL[sea]);
    }
    for (int y = 2; y < 2 + N; y++) c.set(5, y, CRGB(15, 15, 20));
    const CRGB tc = ph == PLACE ? CRGB::Black : P_COL[turn];
    for (int k = 0; k < 4; k++) c.dots[k] = tc;
  }
  bool over() const override { return endT && t - endT > 1400; }
  int8_t winner() const override { return win; }
  int32_t score(uint8_t p) const override { return hits[p & 1]; }
  void live(char* s, size_t l) const override { snprintf(s, l, "Treffer %d : %d", hits[0], hits[1]); }
  void extra(JsonObject o) const override {
    static const char* const ST[] = {"place", "aim", "fly", "res"};
    o["st"] = ST[ph];
    o["turn"] = turn;
    o["v"] = ver;
    for (int sea = 0; sea < 2; sea++) {
      o["rdy"].add(ready[sea]);
      o["left"].add(left(sea));
      JsonArray a = o["s"].add<JsonArray>();
      for (int c = 0; c < CELLS; c++) a.add(shot[sea][c] == 2 && sunk(sea, ship[sea][c] - 1) ? 3 : shot[sea][c]);
    }
    if (lastCell >= 0) { o["ls"] = lastSea; o["lc"] = lastCell; o["lr"] = ph == FLY ? 0 : lastRes; }
  }
  void priv(uint8_t slot, JsonObject o) const override {
    if (slot > 1) return;
    JsonArray a = o["ships"].to<JsonArray>();
    for (int c = 0; c < CELLS; c++) a.add(ship[slot][c]);
  }
  void results(JsonArray r) const override {
    auto q = [&](int p) { return shots[p] ? String(hits[p] * 100 / shots[p]) + " %" : String("–"); };
    row(r, "Schüsse", String(shots[0]), String(shots[1]));
    row(r, "Treffer", String(hits[0]), String(hits[1]));
    row(r, "Trefferquote", q(0), q(1));
    row(r, "Spielzeit", mmss(seconds()));
  }
};

}  // namespace

Game* newWort() { return new Wort(); }
Game* newTau() { return new Tau(); }
Game* newSee() { return new See(); }

}  // namespace games
