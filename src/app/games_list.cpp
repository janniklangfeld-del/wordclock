// Die Spiele der MJ Arcade. Spielfeld: 11 x 10 Buchstabenfelder, y = 0 oben.
#include "games_internal.h"
#include "intro.h"

namespace games {

// ---------------------------------------------------------------------------
// Zeichenhilfen
// ---------------------------------------------------------------------------
namespace {
const uint8_t DIGIT3[10][5] = {
    {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
    {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 2, 2, 2}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},
};
int8_t sgn(int v) { return (v > 0) - (v < 0); }
int rnd(int lo, int hi) { return lo + (int)(esp_random() % (uint32_t)(hi - lo + 1)); }  // lo..hi
}  // namespace

void Canvas::digit(int x, int y, int d, CRGB c) {
  if (d < 0 || d > 9) return;
  for (int r = 0; r < 5; r++)
    for (int k = 0; k < 3; k++)
      if (DIGIT3[d][r] >> (2 - k) & 1) set(x + k, y + r, c);
}

void Canvas::number(int v, int y, CRGB c) {
  v = constrain(v, 0, 999);
  const int n = v >= 100 ? 3 : v >= 10 ? 2 : 1;
  int x = (GW - (n * 4 - 1)) / 2;
  char b[4];
  snprintf(b, sizeof(b), "%d", v);
  for (int i = 0; i < n; i++, x += 4) digit(x, y, b[i] - '0', c);
}

void Canvas::glyph(int x, int y, char ch, CRGB c) {
  uint8_t cols[8];
  const int n = intro::toColumns(String(ch), cols, sizeof(cols));
  for (int k = 0; k < n && k < 5; k++)
    for (int r = 0; r < 7; r++)
      if (cols[k] >> r & 1) set(x + k, y + r, c);
}

void Canvas::score(int a, int b) {
  digit(1, 2, constrain(a, 0, 9), P_COL[0]);
  digit(7, 2, constrain(b, 0, 9), P_COL[1]);
  set(5, 3, CRGB(90, 90, 90));
  set(5, 5, CRGB(90, 90, 90));
}

String mmss(uint32_t s) {
  char b[12];
  snprintf(b, sizeof(b), "%lu:%02lu", (unsigned long)(s / 60), (unsigned long)(s % 60));
  return b;
}

namespace {
const int8_t DX[4] = {0, 0, -1, 1};  // K_UP, K_DOWN, K_LEFT, K_RIGHT
const int8_t DY[4] = {-1, 1, 0, 0};

// ===========================================================================
// Snake (1 Spieler)
// ===========================================================================
class Snake : public Game {
  int8_t bx[GW * GH], by[GW * GH];
  int len = 3;
  int8_t dir = K_RIGHT;
  int8_t q[3];
  int qn = 0;
  int8_t ax = 0, ay = 0;
  bool dead = false, won = false;
  uint32_t lastStep = 0, interval = 300, deadT = 0;
  int apples = 0, steps = 0;

  bool onBody(int x, int y, int upto) const {
    for (int i = 0; i < upto; i++)
      if (bx[i] == x && by[i] == y) return true;
    return false;
  }
  void placeApple() {
    int freeCells = GW * GH - len;
    if (freeCells <= 0) { won = dead = true; deadT = t; return; }
    int k = rnd(0, freeCells - 1);
    for (int y = 0; y < GH; y++)
      for (int x = 0; x < GW; x++)
        if (!onBody(x, y, len) && k-- == 0) { ax = x; ay = y; return; }
  }

 public:
  void start() override {
    len = 3;
    for (int i = 0; i < len; i++) { bx[i] = 4 - i; by[i] = 5; }
    dir = K_RIGHT;
    qn = 0;
    lastStep = t;
    interval = 300;
    placeApple();
  }
  void key(uint8_t, uint8_t k, bool down) override {
    if (!down || k > K_RIGHT || qn >= 3) return;
    q[qn++] = k;
  }
  void tick() override {
    if (dead || t - lastStep < interval) return;
    lastStep = t;
    // naechste gueltige Richtung aus der Warteschlange (keine Umkehr, keine Wiederholung)
    while (qn) {
      int8_t k = q[0];
      memmove(q, q + 1, --qn);
      if (k != dir && !(DX[k] == -DX[dir] && DY[k] == -DY[dir])) { dir = k; break; }
    }
    const int nx = bx[0] + DX[dir], ny = by[0] + DY[dir];
    const bool eat = nx == ax && ny == ay;
    if (nx < 0 || nx >= GW || ny < 0 || ny >= GH || onBody(nx, ny, eat ? len : len - 1)) {
      dead = true;
      deadT = t;
      return;
    }
    if (eat) len++;
    memmove(bx + 1, bx, len - 1);
    memmove(by + 1, by, len - 1);
    bx[0] = nx;
    by[0] = ny;
    steps++;
    if (eat) {
      apples++;
      interval = max<uint32_t>(115, interval - 12);
      placeApple();
    }
  }
  void draw(Canvas& c) override {
    const bool flash = dead && !won && ((t - deadT) / 150) % 2 == 0;
    if (!dead || won) {
      CRGB a(255, 20, 10);
      a.nscale8_video(beatsin8(90, 150, 255, 0, 0));
      c.set(ax, ay, a);
    }
    for (int i = len - 1; i >= 0; i--) {
      CRGB col = i == 0 ? CRGB(200, 255, 120) : CHSV(96 - min(i, 40), 255, 255 - min(i * 7, 170));
      if (flash) col = CRGB(255, 0, 0);
      c.set(bx[i], by[i], col);
    }
    for (int k = 0; k < 4; k++) c.dots[k] = k < apples % 5 ? CRGB(255, 20, 10) : CRGB::Black;
  }
  bool over() const override { return dead && t - deadT > 1600; }
  int32_t score(uint8_t) const override { return apples; }
  int32_t hsValue(uint8_t p) const override { return p == 0 ? apples : -1; }
  void live(char* b, size_t n) const override { snprintf(b, n, "Länge %d", len); }
  void results(JsonArray r) const override {
    row(r, "Äpfel", String(apples));
    row(r, "Länge", String(len));
    row(r, "Spielzeit", mmss(seconds()));
    row(r, "Endtempo", String(1000.0f / interval, 1) + " Felder/s");
    if (won) row(r, "Feld voll", "Unglaublich!");
  }
};

// ===========================================================================
// Tetris (1 Spieler, 2 Spieler nebeneinander, gegen MJ-Bot)
// ===========================================================================
const int8_t SHAPE[7][4][2] = {
    {{0, 1}, {1, 1}, {2, 1}, {3, 1}},  // I
    {{0, 0}, {1, 0}, {0, 1}, {1, 1}},  // O
    {{1, 0}, {0, 1}, {1, 1}, {2, 1}},  // T
    {{1, 0}, {2, 0}, {0, 1}, {1, 1}},  // S
    {{0, 0}, {1, 0}, {1, 1}, {2, 1}},  // Z
    {{0, 0}, {0, 1}, {1, 1}, {2, 1}},  // J
    {{2, 0}, {0, 1}, {1, 1}, {2, 1}},  // L
};
const uint8_t BOXN[7] = {4, 2, 3, 3, 3, 3, 3};
const CRGB PIECE_COL[9] = {CRGB::Black,         CRGB(0, 220, 255), CRGB(255, 210, 0),  CRGB(190, 0, 255),
                           CRGB(0, 230, 40),    CRGB(255, 0, 30),  CRGB(0, 60, 255),   CRGB(255, 110, 0),
                           CRGB(70, 70, 80)};  // 8 = Muellzeile

void pieceCells(int p, int rot, int8_t out[4][2]) {
  const int n = BOXN[p];
  for (int i = 0; i < 4; i++) {
    int x = SHAPE[p][i][0], y = SHAPE[p][i][1];
    for (int r = 0; r < (rot & 3); r++) {
      int nx = n - 1 - y;
      y = x;
      x = nx;
    }
    out[i][0] = x;
    out[i][1] = y;
  }
}

struct TBoard {
  int w = 7, x0 = 0;
  uint8_t cell[GH][7];
  int piece = 0, rot = 0, px = 0, py = 0, next = 0;
  bool hasPiece = false, dead = false;
  uint32_t lastFall = 0, lockStart = 0, deadT = 0;
  int lockResets = 0;
  uint16_t clearMask = 0;
  uint32_t clearT = 0;
  int lines = 0, score = 0, level = 1, pieces = 0, tetrises = 0, garbageSent = 0, pendingGarbage = 0;
  bool hold[K_COUNT] = {};
  uint32_t rep[K_COUNT] = {};
  uint8_t bag[7];
  int bagN = 0;
  // Bot
  bool isBot = false;
  int tRot = 0, tX = 0;
  uint32_t botNext = 0;

  int draw7() {
    if (!bagN) {
      for (int i = 0; i < 7; i++) bag[i] = i;
      for (int i = 6; i > 0; i--) std::swap(bag[i], bag[rnd(0, i)]);
      bagN = 7;
    }
    return bag[--bagN];
  }
  bool fits(int p, int r, int x, int y) const {
    int8_t c[4][2];
    pieceCells(p, r, c);
    for (auto& q : c) {
      int cx = x + q[0], cy = y + q[1];
      if (cx < 0 || cx >= w || cy >= GH) return false;
      if (cy >= 0 && cell[cy][cx]) return false;
    }
    return true;
  }
  void init(int width, int xo, uint32_t now) {
    w = width;
    x0 = xo;
    memset(cell, 0, sizeof(cell));
    bagN = 0;
    next = draw7();
    lastFall = now;
    spawn(now);
  }
  uint32_t fallMs() const { return hold[K_DOWN] ? 45 : max(110, 760 - (level - 1) * 70); }
  void addGarbage(int n) {
    const int hole = rnd(0, w - 1);
    for (int g = 0; g < n; g++) {
      for (int x = 0; x < w; x++)
        if (cell[0][x]) dead = true;
      memmove(cell[0], cell[1], sizeof(cell[0]) * (GH - 1));
      for (int x = 0; x < w; x++) cell[GH - 1][x] = x == hole ? 0 : 8;
    }
  }
  void spawn(uint32_t now) {
    if (pendingGarbage) {
      addGarbage(pendingGarbage);
      pendingGarbage = 0;
    }
    piece = next;
    next = draw7();
    rot = 0;
    px = (w - BOXN[piece]) / 2;
    py = piece == 0 ? -1 : 0;
    hasPiece = true;
    lockStart = 0;
    lockResets = 0;
    lastFall = now;
    pieces++;
    if (dead || !fits(piece, rot, px, py)) {
      dead = true;
      deadT = now;
      hasPiece = false;
    }
    if (isBot && !dead) plan(now);
  }
  void moved() {
    if (lockStart && lockResets < 12) {
      lockStart = 0;
      lockResets++;
    }
  }
  bool move(int dx) {
    if (!hasPiece || !fits(piece, rot, px + dx, py)) return false;
    px += dx;
    moved();
    return true;
  }
  bool rotate() {
    if (!hasPiece || piece == 1) return false;
    static const int8_t KICK[][2] = {{0, 0}, {-1, 0}, {1, 0}, {-2, 0}, {2, 0}, {0, -1}};
    for (auto& k : KICK)
      if (fits(piece, rot + 1, px + k[0], py + k[1])) {
        rot = (rot + 1) & 3;
        px += k[0];
        py += k[1];
        moved();
        return true;
      }
    return false;
  }
  // Rueckgabe: gesendete Muellzeilen
  int lock(uint32_t now) {
    int8_t c[4][2];
    pieceCells(piece, rot, c);
    for (auto& q : c) {
      int cy = py + q[1];
      if (cy < 0) { dead = true; deadT = now; hasPiece = false; return 0; }
      cell[cy][px + q[0]] = piece + 1;
    }
    hasPiece = false;
    clearMask = 0;
    int n = 0;
    for (int y = 0; y < GH; y++) {
      bool full = true;
      for (int x = 0; x < w; x++) full &= cell[y][x] != 0;
      if (full) { clearMask |= 1 << y; n++; }
    }
    if (n) {
      static const int PTS[5] = {0, 100, 300, 500, 800};
      score += PTS[n] * level;
      lines += n;
      level = 1 + lines / 5;
      if (n == 4) tetrises++;
      clearT = now;
      const int send = n == 4 ? 4 : n - 1;
      garbageSent += send;
      return send;
    }
    spawn(now);
    return 0;
  }
  int hardDrop(uint32_t now) {
    if (!hasPiece) return 0;
    while (fits(piece, rot, px, py + 1)) { py++; score += 2; }
    return lock(now);
  }
  int tick(uint32_t now) {
    if (dead) return 0;
    if (clearMask) {  // Reihen blitzen kurz auf, dann verschwinden sie
      if (now - clearT < 220) return 0;
      for (int y = 0; y < GH; y++)
        if (clearMask >> y & 1) {
          memmove(cell[1], cell[0], sizeof(cell[0]) * y);
          memset(cell[0], 0, sizeof(cell[0]));
        }
      clearMask = 0;
      spawn(now);
      return 0;
    }
    if (!hasPiece) return 0;
    for (int k : {K_LEFT, K_RIGHT})  // gedrueckt halten: Uhr wiederholt selbst
      if (hold[k] && now >= rep[k]) {
        move(k == K_LEFT ? -1 : 1);
        rep[k] = now + 55;
      }
    int sent = 0;
    if (isBot && now >= botNext) sent = botStep(now);
    if (!hasPiece) return sent;
    if (now - lastFall >= fallMs()) {
      lastFall = now;
      if (fits(piece, rot, px, py + 1)) {
        py++;
        if (hold[K_DOWN]) score++;
      } else if (!lockStart) {
        lockStart = now;
      }
    }
    if (lockStart && now - lockStart >= 350 && !fits(piece, rot, px, py + 1)) sent += lock(now);
    return sent;
  }
  int key(uint8_t k, bool down, uint32_t now) {
    if (k < K_COUNT) hold[k] = down;
    if (!down || dead) return 0;
    switch (k) {
      case K_LEFT: move(-1); rep[k] = now + 170; break;
      case K_RIGHT: move(1); rep[k] = now + 170; break;
      case K_UP:
      case K_A: rotate(); break;
      case K_B: return hardDrop(now);
      case K_DOWN: lastFall = 0; break;
    }
    return 0;
  }

  // --- MJ-Bot: bewertet alle Lagen (Hoehe, Loecher, Unebenheit, Reihen) ---
  float evalBoard(const uint8_t b[GH][7]) const {
    int agg = 0, holes = 0, bump = 0, full = 0, prevH = -1;
    for (int y = 0; y < GH; y++) {
      bool f = true;
      for (int x = 0; x < w; x++) f &= b[y][x] != 0;
      full += f;
    }
    for (int x = 0; x < w; x++) {
      int h = 0;
      bool seen = false;
      for (int y = 0; y < GH; y++) {
        if (b[y][x]) { if (!seen) { h = GH - y; seen = true; } }
        else if (seen) holes++;
      }
      agg += h;
      if (prevH >= 0) bump += abs(h - prevH);
      prevH = h;
    }
    return -0.51f * agg + 0.76f * full * 1.3f - 0.36f * holes * 2.0f - 0.18f * bump;
  }
  void plan(uint32_t now) {
    float best = -1e9f;
    tRot = 0;
    tX = px;
    uint8_t tmp[GH][7];
    for (int r = 0; r < 4; r++)
      for (int x = -3; x < w; x++) {
        if (!fits(piece, r, x, py)) continue;
        int y = py;
        while (fits(piece, r, x, y + 1)) y++;
        memcpy(tmp, cell, sizeof(tmp));
        int8_t c[4][2];
        pieceCells(piece, r, c);
        bool ok = true;
        for (auto& q : c) {
          if (y + q[1] < 0) ok = false;
          else tmp[y + q[1]][x + q[0]] = 1;
        }
        if (!ok) continue;
        float s = evalBoard(tmp) + (esp_random() % 100) / 400.0f;  // etwas Zufall
        if (s > best) { best = s; tRot = r; tX = x; }
      }
    if (esp_random() % 100 < 12) tX += (esp_random() & 1) ? 1 : -1;  // MJ-Bot ist nicht perfekt
    botNext = now + 420;
  }
  int botStep(uint32_t now) {
    botNext = now + max(70, 230 - level * 14);
    if (rot != tRot) { if (!rotate()) tRot = rot; return 0; }
    if (px < tX) { if (!move(1)) tX = px; return 0; }
    if (px > tX) { if (!move(-1)) tX = px; return 0; }
    botNext = now + 160;
    return hardDrop(now);
  }

  void draw(Canvas& c, uint32_t now) const {
    for (int y = 0; y < GH; y++)
      for (int x = 0; x < w; x++) {
        CRGB col = PIECE_COL[cell[y][x]];
        if (clearMask >> y & 1) col = ((now - clearT) / 55) % 2 ? CRGB::White : CRGB(255, 160, 220);
        else if (dead && cell[y][x]) col = CRGB(60, 0, 0) + CRGB(cell[y][x] == 8 ? 0 : 30, 0, 0);
        c.set(x0 + x, y, col);
      }
    if (hasPiece) {
      int8_t q[4][2];
      // Schatten: wo der Stein landen wuerde
      int gy = py;
      while (fits(piece, rot, px, gy + 1)) gy++;
      pieceCells(piece, rot, q);
      CRGB ghost = PIECE_COL[piece + 1];
      ghost.nscale8_video(38);
      for (auto& p : q) if (gy + p[1] >= 0) c.set(x0 + px + p[0], gy + p[1], ghost);
      for (auto& p : q) if (py + p[1] >= 0) c.set(x0 + px + p[0], py + p[1], PIECE_COL[piece + 1]);
    }
  }
  void drawNext(Canvas& c, int x, int y) const {
    int8_t q[4][2];
    pieceCells(next, next == 0 ? 1 : 0, q);
    int mx = 9, my = 9, Mx = 0;
    for (auto& p : q) { mx = min<int>(mx, p[0]); my = min<int>(my, p[1]); Mx = max<int>(Mx, p[0]); }
    const int wd = Mx - mx + 1, ox = x + (3 - wd) / 2;
    for (auto& p : q) c.set(ox + p[0] - mx, y + p[1] - my, PIECE_COL[next + 1]);
  }
};

class Tetris : public Game {
  TBoard b[2];
  int nb = 1;
  int8_t loser = -1;
  uint32_t endT = 0;

 public:
  void start() override {
    nb = (humans == 2 || bot) ? 2 : 1;
    if (nb == 1) b[0].init(7, 0, t);
    else {
      b[0].init(5, 0, t);
      b[1].init(5, 6, t);
      b[1].isBot = bot;
    }
  }
  void key(uint8_t p, uint8_t k, bool down) override {
    if (p >= nb || (bot && p == 1) || loser >= 0 || b[p].dead) return;
    const int sent = b[p].key(k, down, t);
    if (sent && nb == 2) b[1 - p].pendingGarbage += sent;
  }
  void tick() override {
    if (loser >= 0) return;
    for (int i = 0; i < nb; i++) {
      const int sent = b[i].tick(t);
      if (sent && nb == 2) b[1 - i].pendingGarbage += sent;
    }
    for (int i = 0; i < nb; i++)
      if (b[i].dead && loser < 0) { loser = i; endT = t; }
  }
  void draw(Canvas& c) override {
    for (int i = 0; i < nb; i++) b[i].draw(c, t);
    if (nb == 1) {
      for (int y = 0; y < GH; y++) c.set(7, y, CRGB(22, 22, 34));
      b[0].drawNext(c, 8, 1);
      for (int k = 0; k < b[0].lines % 5; k++) c.set(9, GH - 1 - k, CRGB(255, 25, 130));
      for (int k = 0; k < 4; k++) c.dots[k] = k < min(b[0].level - 1, 4) ? CRGB(255, 180, 0) : CRGB::Black;
    } else {
      for (int y = 0; y < GH; y++) {
        // Trennlinie zeigt anstehende Muellzeilen (oben links fuer P1, unten fuer P2)
        CRGB col(22, 22, 34);
        if (y < b[0].pendingGarbage) col = CRGB(255, 0, 0);
        if (GH - 1 - y < b[1].pendingGarbage) col = CRGB(255, 0, 0);
        c.set(5, y, col);
      }
      c.dots[0] = c.dots[1] = P_COL[0];
      c.dots[2] = c.dots[3] = P_COL[1];
    }
  }
  bool over() const override { return loser >= 0 && t - endT > 1800; }
  int8_t winner() const override { return nb == 2 ? 1 - loser : -1; }
  int32_t score(uint8_t p) const override { return p < nb ? b[p].score : 0; }
  int32_t bigNumber() const override { return b[0].lines; }
  int32_t hsValue(uint8_t p) const override { return p < nb && !(bot && p == 1) ? b[p].lines : -1; }
  void live(char* s, size_t n) const override {
    if (nb == 1) snprintf(s, n, "Level %d · %d Reihen", b[0].level, b[0].lines);
    else snprintf(s, n, "%d : %d Reihen", b[0].lines, b[1].lines);
  }
  void results(JsonArray r) const override {
    if (nb == 1) {
      row(r, "Punkte", String(b[0].score));
      row(r, "Reihen", String(b[0].lines));
      row(r, "Level", String(b[0].level));
      row(r, "Steine", String(b[0].pieces));
      row(r, "Vierer (Tetris)", String(b[0].tetrises));
    } else {
      row(r, "Punkte", String(b[0].score), String(b[1].score));
      row(r, "Reihen", String(b[0].lines), String(b[1].lines));
      row(r, "Müll geschickt", String(b[0].garbageSent), String(b[1].garbageSent));
      row(r, "Steine", String(b[0].pieces), String(b[1].pieces));
      row(r, "Vierer", String(b[0].tetrises), String(b[1].tetrises));
    }
    row(r, "Spielzeit", mmss(seconds()));
  }
};

// ===========================================================================
// Pong (2 Spieler oder gegen MJ-Bot) – bis 5 Punkte
// ===========================================================================
class Pong : public Game {
  static constexpr int PH = 3, WIN = 5;
  int pad[2] = {3, 3};
  int bxp = 5, byp = 5, vx = 1, vy = 1;
  uint32_t lastBall = 0, ballMs = 170, pauseUntil = 0;
  int pts[2] = {0, 0}, hits[2] = {0, 0}, rally = 0, bestRally = 0;
  bool hold[2][2] = {};
  uint32_t rep[2] = {0, 0}, botNext = 0;
  int botTarget = 4;
  int8_t lastScorer = -1;
  uint32_t endT = 0;

  void serve(int toward) {
    bxp = 5;
    byp = rnd(2, 7);
    vx = toward == 0 ? -1 : 1;
    vy = (esp_random() & 1) ? 1 : -1;
    ballMs = 170;
    rally = 0;
    pauseUntil = t + 1300;
    lastBall = pauseUntil;
  }
  void movePad(int p, int d) { pad[p] = constrain(pad[p] + d, 0, GH - PH); }
  int predictY() const {  // wo kommt der Ball bei x = 9 an?
    int x = bxp, y = byp, dy = vy;
    for (int i = 0; i < 40 && x < 9; i++) {
      x++;
      int ny = y + dy;
      if (ny < 0 || ny >= GH) { dy = -dy; ny = y + dy; }
      y = ny;
    }
    return y;
  }

 public:
  void start() override { serve(esp_random() & 1); }
  void key(uint8_t p, uint8_t k, bool down) override {
    if (p > 1 || (bot && p == 1)) return;
    int i = k == K_UP || k == K_LEFT ? 0 : k == K_DOWN || k == K_RIGHT ? 1 : -1;
    if (i < 0) return;
    hold[p][i] = down;
    if (down) {
      movePad(p, i ? 1 : -1);
      rep[p] = t + 120;
    }
  }
  void tick() override {
    if (endT) return;
    for (int p = 0; p < 2; p++)
      if ((hold[p][0] || hold[p][1]) && t >= rep[p]) {
        movePad(p, hold[p][1] ? 1 : -1);
        rep[p] = t + 60;
      }
    if (bot && t >= botNext) {
      botNext = t + 95;
      int center = pad[1] + 1;
      if (vx > 0 && bxp >= 3) {
        if (bxp == 3 || botTarget < 0) botTarget = predictY() + ((esp_random() % 100) < 30 ? rnd(-2, 2) : 0);
      } else {
        botTarget = -1;
      }
      int target = botTarget < 0 ? 4 : botTarget;
      if (target < center) movePad(1, -1);
      else if (target > center) movePad(1, 1);
    }
    if (t < pauseUntil || t - lastBall < ballMs) return;
    lastBall = t;
    int nx = bxp + vx, ny = byp + vy;
    if (ny < 0 || ny >= GH) { vy = -vy; ny = byp + vy; }
    const int side = nx <= 0 ? 0 : nx >= GW - 1 ? 1 : -1;
    if (side >= 0) {
      const int rel = ny - pad[side];
      const int relNow = byp - pad[side];
      if ((rel >= 0 && rel < PH) || (relNow >= 0 && relNow < PH)) {  // getroffen
        const int r = (rel >= 0 && rel < PH) ? rel : relNow;
        vx = -vx;
        vy = r == 0 ? -1 : r == PH - 1 ? 1 : (vy == 0 ? ((esp_random() & 1) ? 1 : -1) * (esp_random() % 3 == 0) : vy);
        hits[side]++;
        rally++;
        bestRally = max(bestRally, rally);
        ballMs = max<uint32_t>(55, ballMs - 8);
        nx = bxp + vx;
        ny = byp + vy;
        if (ny < 0 || ny >= GH) { vy = -vy; ny = byp + vy; }
      } else {  // vorbei -> Punkt fuer den anderen
        bxp = nx;
        byp = ny;
        lastScorer = 1 - side;
        pts[lastScorer]++;
        if (pts[lastScorer] >= WIN) endT = t;
        else serve(side);
        return;
      }
    }
    bxp = nx;
    byp = ny;
  }
  void draw(Canvas& c) override {
    if (t < pauseUntil && lastScorer >= 0) {  // Spielstand zwischen den Punkten
      c.score(pts[0], pts[1]);
      return;
    }
    for (int y = 0; y < GH; y += 2) c.set(5, y, CRGB(14, 14, 20));
    for (int p = 0; p < 2; p++)
      for (int k = 0; k < PH; k++) c.set(p ? GW - 1 : 0, pad[p] + k, P_COL[p]);
    if (endT) {
      const int w = pts[0] >= WIN ? 0 : 1;
      if (((t - endT) / 140) % 2) for (int y = 0; y < GH; y++) c.set(w ? GW - 1 : 0, y, P_COL[w]);
    }
    if (t >= pauseUntil || ((t / 120) % 2)) c.set(bxp, byp, CRGB::White);
    for (int k = 0; k < 4; k++) c.dots[k] = CRGB::Black;
  }
  bool over() const override { return endT && t - endT > 1500; }
  int8_t winner() const override { return pts[0] > pts[1] ? 0 : 1; }
  int32_t score(uint8_t p) const override { return pts[p & 1]; }
  void live(char* s, size_t n) const override { snprintf(s, n, "Ballwechsel %d", rally); }
  void results(JsonArray r) const override {
    row(r, "Punkte", String(pts[0]), String(pts[1]));
    row(r, "Treffer", String(hits[0]), String(hits[1]));
    row(r, "Längster Ballwechsel", String(bestRally));
    row(r, "Spielzeit", mmss(seconds()));
  }
};

// ===========================================================================
// T.R.O.N – Lichtrenner (2 Spieler oder gegen MJ-Bot) – wer zuerst 3 Runden holt
// ===========================================================================
class Tron : public Game {
  static constexpr int WIN = 3;
  uint8_t trail[GH][GW];
  int hx[2], hy[2];
  int8_t dir[2];
  int8_t q[2][2];
  int qn[2] = {0, 0};
  int wins[2] = {0, 0}, steps = 0, totalSteps = 0, bestSteps = 0;
  bool crashed[2] = {false, false};
  uint32_t lastStep = 0, stepMs = 230, roundEnd = 0, showScoreUntil = 0, endT = 0;
  int rounds = 0;

  bool freeCell(int x, int y) const { return x >= 0 && x < GW && y >= 0 && y < GH && !trail[y][x]; }
  void newRound() {
    memset(trail, 0, sizeof(trail));
    hx[0] = 1; hy[0] = 4; dir[0] = K_RIGHT;
    hx[1] = GW - 2; hy[1] = 5; dir[1] = K_LEFT;
    trail[hy[0]][hx[0]] = 1;
    trail[hy[1]][hx[1]] = 2;
    qn[0] = qn[1] = 0;
    crashed[0] = crashed[1] = false;
    steps = 0;
    stepMs = 230;
    lastStep = t + 600;  // kurz durchatmen
    roundEnd = 0;
  }
  static bool reverse(int8_t a, int8_t b) { return DX[a] == -DX[b] && DY[a] == -DY[b]; }
  int area(int sx, int sy) const {  // Flutfuellung: freie Felder ab (sx, sy)
    if (!freeCell(sx, sy)) return 0;
    uint8_t seen[GH][GW] = {};
    int8_t st[GW * GH][2];
    int n = 0, cnt = 0;
    st[n][0] = sx; st[n][1] = sy; n++;
    seen[sy][sx] = 1;
    while (n) {
      n--;
      const int x = st[n][0], y = st[n][1];
      cnt++;
      for (int d = 0; d < 4; d++) {
        const int nx = x + DX[d], ny = y + DY[d];
        if (freeCell(nx, ny) && !seen[ny][nx]) { seen[ny][nx] = 1; st[n][0] = nx; st[n][1] = ny; n++; }
      }
    }
    return cnt;
  }
  void botThink() {
    int best = -1, bestD = dir[1];
    for (int d = 0; d < 4; d++) {
      if (reverse(d, dir[1])) continue;
      const int nx = hx[1] + DX[d], ny = hy[1] + DY[d];
      if (!freeCell(nx, ny)) continue;
      int s = area(nx, ny) * 4 + (d == dir[1] ? 2 : 0) + (int)(esp_random() % 3);
      if (abs(nx - hx[0]) + abs(ny - hy[0]) <= 1) s -= 6;  // nicht frontal auf den Gegner
      if (s > best) { best = s; bestD = d; }
    }
    if (esp_random() % 100 < 4) return;  // ab und zu zu spaet reagiert
    dir[1] = bestD;
  }

 public:
  void start() override { newRound(); }
  void key(uint8_t p, uint8_t k, bool down) override {
    if (!down || p > 1 || (bot && p == 1) || k > K_RIGHT || qn[p] >= 2) return;
    q[p][qn[p]++] = k;
  }
  void tick() override {
    if (endT) return;
    if (roundEnd) {
      if (t - roundEnd > 1300 && !showScoreUntil) showScoreUntil = t + 1400;
      if (showScoreUntil && t > showScoreUntil) {
        showScoreUntil = 0;
        if (wins[0] >= WIN || wins[1] >= WIN) endT = t;
        else newRound();
      }
      return;
    }
    if (t < lastStep || t - lastStep < stepMs) return;
    lastStep = t;
    for (int p = 0; p < 2; p++)
      while (qn[p]) {
        const int8_t k = q[p][0];
        q[p][0] = q[p][1];
        qn[p]--;
        if (k != dir[p] && !reverse(k, dir[p])) { dir[p] = k; break; }
      }
    if (bot) botThink();
    int nx[2], ny[2];
    for (int p = 0; p < 2; p++) {
      nx[p] = hx[p] + DX[dir[p]];
      ny[p] = hy[p] + DY[dir[p]];
      crashed[p] = !freeCell(nx[p], ny[p]);
    }
    if (nx[0] == nx[1] && ny[0] == ny[1]) crashed[0] = crashed[1] = true;
    for (int p = 0; p < 2; p++)
      if (!crashed[p]) { hx[p] = nx[p]; hy[p] = ny[p]; trail[hy[p]][hx[p]] = p + 1; }
    steps++;
    totalSteps++;
    stepMs = max<uint32_t>(135, stepMs - 2);
    if (crashed[0] || crashed[1]) {
      rounds++;
      bestSteps = max(bestSteps, steps);
      if (!crashed[0]) wins[0]++;
      if (!crashed[1]) wins[1]++;
      roundEnd = t;
    }
  }
  void draw(Canvas& c) override {
    if (showScoreUntil) { c.score(wins[0], wins[1]); return; }
    for (int y = 0; y < GH; y++)
      for (int x = 0; x < GW; x++)
        if (trail[y][x]) { CRGB col = P_COL[trail[y][x] - 1]; col.nscale8_video(90); c.set(x, y, col); }
    for (int p = 0; p < 2; p++) {
      CRGB h = P_COL[p];
      h += CRGB(60, 60, 60);
      if (roundEnd && crashed[p]) h = ((t - roundEnd) / 110) % 2 ? CRGB(255, 0, 0) : CRGB(255, 200, 0);
      c.set(hx[p], hy[p], h);
    }
    for (int k = 0; k < 4; k++) c.dots[k] = k < 2 ? (k < wins[0] ? P_COL[0] : CRGB::Black) : (3 - k < wins[1] ? P_COL[1] : CRGB::Black);
  }
  bool over() const override { return endT && t - endT > 300; }
  int8_t winner() const override { return wins[0] > wins[1] ? 0 : wins[1] > wins[0] ? 1 : 2; }
  int32_t score(uint8_t p) const override { return wins[p & 1]; }
  void live(char* s, size_t n) const override { snprintf(s, n, "Runde %d · erster bis %d", rounds + 1, WIN); }
  void results(JsonArray r) const override {
    row(r, "Runden gewonnen", String(wins[0]), String(wins[1]));
    row(r, "Runden gespielt", String(rounds));
    row(r, "Längste Runde", String(bestSteps) + " Felder");
    row(r, "Spielzeit", mmss(seconds()));
  }
};

// ===========================================================================
// Vier gewinnt (2 Spieler oder gegen MJ-Bot)
// ===========================================================================
class Connect4 : public Game {
  static constexpr int C = 7, R = 6, X0 = 2, Y0 = 4;
  uint8_t b[R][C];
  int cur = 0, cursor = 3, moves = 0;
  int dropCol = -1, dropY = 0, dropRow = 0;
  uint32_t dropT = 0, botT = 0, endT = 0;
  int8_t win = -1;
  uint8_t winMask[R][C];
  int botCol = -1;

  int landing(const uint8_t bb[R][C], int col) const {
    for (int r = R - 1; r >= 0; r--)
      if (!bb[r][col]) return r;
    return -1;
  }
  static bool fourAt(const uint8_t bb[R][C], int p, uint8_t mark[R][C]) {
    static const int D[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    bool any = false;
    for (int r = 0; r < R; r++)
      for (int c = 0; c < C; c++)
        for (auto& d : D) {
          int k = 0;
          for (; k < 4; k++) {
            int rr = r + d[1] * k, cc = c + d[0] * k;
            if (rr < 0 || rr >= R || cc >= C || bb[rr][cc] != p) break;
          }
          if (k == 4) {
            any = true;
            if (mark) for (k = 0; k < 4; k++) mark[r + d[1] * k][c + d[0] * k] = 1;
          }
        }
    return any;
  }
  static int evalWindow(int mine, int theirs) {
    if (mine && theirs) return 0;
    static const int S[5] = {0, 1, 8, 60, 10000};
    return mine ? S[mine] : -S[theirs] * 11 / 10;
  }
  static int evalBoard(const uint8_t bb[R][C], int p) {
    int s = 0;
    for (int r = 0; r < R; r++) s += (bb[r][3] == p) * 4 - (bb[r][3] == 3 - p) * 4;
    static const int D[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    for (int r = 0; r < R; r++)
      for (int c = 0; c < C; c++)
        for (auto& d : D) {
          int m = 0, o = 0, k = 0;
          for (; k < 4; k++) {
            int rr = r + d[1] * k, cc = c + d[0] * k;
            if (rr < 0 || rr >= R || cc >= C) break;
            m += bb[rr][cc] == p;
            o += bb[rr][cc] == 3 - p;
          }
          if (k == 4) s += evalWindow(m, o);
        }
    return s;
  }
  int negamax(uint8_t bb[R][C], int depth, int alpha, int beta, int p) {
    if (fourAt(bb, 3 - p, nullptr)) return -100000 - depth;
    if (depth == 0) return evalBoard(bb, p);
    static const int ORDER[C] = {3, 2, 4, 1, 5, 0, 6};
    bool moved = false;
    for (int c : ORDER) {
      int r = landing(bb, c);
      if (r < 0) continue;
      moved = true;
      bb[r][c] = p;
      int v = -negamax(bb, depth - 1, -beta, -alpha, 3 - p);
      bb[r][c] = 0;
      if (v > alpha) alpha = v;
      if (alpha >= beta) break;
    }
    return moved ? alpha : 0;
  }
  int botChoose() {
    uint8_t bb[R][C];
    memcpy(bb, b, sizeof(bb));
    int best = INT32_MIN, bestC = 3;
    static const int ORDER[C] = {3, 2, 4, 1, 5, 0, 6};
    for (int c : ORDER) {
      int r = landing(bb, c);
      if (r < 0) continue;
      bb[r][c] = 2;
      int v = -negamax(bb, 4, -1000000, 1000000, 1) + (int)(esp_random() % 6);
      bb[r][c] = 0;
      if (v > best) { best = v; bestC = c; }
    }
    return bestC;
  }
  void drop() {
    if (dropCol >= 0 || win >= 0) return;
    const int r = landing(b, cursor);
    if (r < 0) return;
    dropCol = cursor;
    dropRow = r;
    dropY = -1;
    dropT = t;
  }

 public:
  void start() override {
    memset(b, 0, sizeof(b));
    memset(winMask, 0, sizeof(winMask));
    cur = esp_random() & 1;
    cursor = 3;
    botT = t + 700;
  }
  void key(uint8_t p, uint8_t k, bool down) override {
    if (!down || p != cur || (bot && p == 1) || dropCol >= 0 || win >= 0) return;
    if (k == K_LEFT) cursor = max(0, cursor - 1);
    else if (k == K_RIGHT) cursor = min(C - 1, cursor + 1);
    else if (k == K_A || k == K_B || k == K_DOWN) drop();
  }
  void tick() override {
    if (win >= 0) return;
    if (dropCol >= 0) {
      if (t - dropT >= 55) {
        dropT = t;
        if (++dropY >= dropRow) {
          b[dropRow][dropCol] = cur + 1;
          dropCol = -1;
          moves++;
          if (fourAt(b, cur + 1, winMask)) { win = cur; endT = t; return; }
          bool full = true;
          for (int c = 0; c < C; c++) full &= b[0][c] != 0;
          if (full) { win = 2; endT = t; return; }
          cur = 1 - cur;
          botT = t + 650;
          botCol = -1;
        }
      }
      return;
    }
    if (bot && cur == 1 && t >= botT) {  // MJ-Bot: kurz "nachdenken", dann hinfahren
      if (botCol < 0) botCol = botChoose();
      botT = t + 130;
      if (cursor < botCol) cursor++;
      else if (cursor > botCol) cursor--;
      else drop();
    }
  }
  void draw(Canvas& c) override {
    const CRGB hole(0, 0, 26);
    for (int r = 0; r < R; r++)
      for (int k = 0; k < C; k++) {
        CRGB col = b[r][k] ? P_COL[b[r][k] - 1] : hole;
        if (win >= 0 && win < 2 && !winMask[r][k] && b[r][k]) col.nscale8_video(70);
        if (winMask[r][k] && ((t - endT) / 160) % 2) col = CRGB::White;
        c.set(X0 + k, Y0 + r, col);
      }
    for (int y = Y0; y < GH; y++) {  // Seiten zeigen, wer dran ist
      CRGB l = P_COL[0], rr = P_COL[1];
      l.nscale8_video(cur == 0 && win < 0 ? 200 : 25);
      rr.nscale8_video(cur == 1 && win < 0 ? 200 : 25);
      c.set(0, y, l);
      c.set(GW - 1, y, rr);
    }
    if (win < 0) {
      if (dropCol >= 0) c.set(X0 + dropCol, Y0 + dropY, P_COL[cur]);
      else {
        CRGB p = P_COL[cur];
        p.nscale8_video(beatsin8(70, 120, 255));
        c.set(X0 + cursor, Y0 - 1, p);
      }
    }
  }
  bool over() const override { return win >= 0 && t - endT > 2400; }
  int8_t winner() const override { return win; }
  int32_t score(uint8_t p) const override { return win == p ? 1 : 0; }
  void live(char* s, size_t n) const override { snprintf(s, n, "Zug %d", moves + 1); }
  void results(JsonArray r) const override {
    row(r, "Züge", String(moves));
    row(r, "Spielzeit", mmss(seconds()));
  }
};

// ===========================================================================
// Reaktion: Einzeln (5 Messungen) oder Duell (bis 5 Punkte)
// ===========================================================================
class Reaction : public Game {
  enum { WAIT, GO, SHOW } ph = WAIT;
  uint32_t goAt = 0, phT = 0;
  int pts[2] = {0, 0}, rounds = 0, fouls[2] = {0, 0};
  uint32_t best[2] = {9999, 9999}, sum[2] = {0, 0}, worst = 0;
  int cnt[2] = {0, 0};
  int8_t lastWin = -1;
  bool lastFoul = false;
  uint32_t lastMs = 0, endT = 0;
  int target() const { return humans == 2 ? 5 : 5; }

  void next() {
    ph = WAIT;
    phT = t;
    goAt = t + rnd(1600, 4800);
  }
  void finishRound(int8_t w, bool foul, uint32_t ms) {
    lastWin = w;
    lastFoul = foul;
    lastMs = ms;
    ph = SHOW;
    phT = t;
    rounds++;
  }

 public:
  void start() override { next(); }
  void key(uint8_t p, uint8_t k, bool down) override {
    if (!down || p >= humans || endT || (k != K_A && k != K_B)) return;
    if (ph == WAIT) {  // zu frueh!
      fouls[p]++;
      if (humans == 2) pts[1 - p]++;
      finishRound(humans == 2 ? 1 - p : -1, true, 0);
    } else if (ph == GO) {
      const uint32_t ms = t - goAt;
      best[p] = min(best[p], ms);
      sum[p] += ms;
      cnt[p]++;
      worst = max(worst, ms);
      pts[p]++;
      finishRound(p, false, ms);
    }
  }
  void tick() override {
    if (endT) return;
    if (ph == WAIT && t >= goAt) { ph = GO; phT = t; }
    if (ph == GO && t - goAt > 3000) finishRound(-1, true, 0);  // keiner hat gedrueckt
    if (ph == SHOW && t - phT > 1700) {
      if ((humans == 2 && (pts[0] >= 5 || pts[1] >= 5)) || (humans == 1 && cnt[0] >= target())) endT = t;
      else next();
    }
  }
  void draw(Canvas& c) override {
    if (ph == WAIT) {
      const uint8_t v = beatsin8(30, 10, 45);
      for (int x = 0; x < GW; x++) { c.set(x, 0, CRGB(v, v, v)); c.set(x, GH - 1, CRGB(v, v, v)); }
      for (int y = 0; y < GH; y++) { c.set(0, y, CRGB(v, v, v)); c.set(GW - 1, y, CRGB(v, v, v)); }
      if (humans == 2) for (int k = 0; k < 4; k++) c.dots[k] = k < 2 ? P_COL[0] : P_COL[1];
    } else if (ph == GO) {
      for (int y = 0; y < GH; y++) for (int x = 0; x < GW; x++) c.set(x, y, CRGB(40, 255, 60));
    } else {
      if (lastFoul && lastWin < 0 && humans == 1) {
        for (int y = 0; y < GH; y++) for (int x = 0; x < GW; x++) c.set(x, y, ((t - phT) / 120) % 2 ? CRGB(120, 0, 0) : CRGB::Black);
      } else if (humans == 1) {
        c.number(min<uint32_t>(lastMs, 999), 2, lastMs < 250 ? CRGB(40, 255, 60) : lastMs < 350 ? CRGB(255, 200, 0) : CRGB(255, 60, 0));
      } else if (lastWin >= 0) {
        if (t - phT < 800) {
          for (int y = 0; y < GH; y++) for (int x = 0; x < GW; x++) c.set(x, y, P_COL[lastWin]);
          if (lastFoul) for (int x = 0; x < GW; x++) c.set(x, (t / 80) % GH, CRGB::Red);
        } else c.score(pts[0], pts[1]);
      }
    }
  }
  bool over() const override { return endT && t - endT > 200; }
  int8_t winner() const override { return humans == 2 ? (pts[0] > pts[1] ? 0 : 1) : -1; }
  int32_t score(uint8_t p) const override { return pts[p & 1]; }
  int32_t bigNumber() const override { return cnt[0] ? sum[0] / cnt[0] : 999; }
  // Durchschnitt zaehlt ab 3 gueltigen Messungen (allein sind es immer 5)
  int32_t hsValue(uint8_t p) const override { return p < humans && cnt[p] >= 3 ? (int32_t)(sum[p] / cnt[p]) : -1; }
  void live(char* s, size_t n) const override {
    if (humans == 1) snprintf(s, n, "Messung %d von 5", min(cnt[0] + 1, 5));
    else snprintf(s, n, "Runde %d · erster bis 5", rounds + 1);
  }
  void results(JsonArray r) const override {
    auto avg = [&](int p) { return cnt[p] ? String(sum[p] / cnt[p]) + " ms" : String("–"); };
    auto bst = [&](int p) { return cnt[p] ? String(best[p]) + " ms" : String("–"); };
    if (humans == 1) {
      row(r, "Durchschnitt", avg(0));
      row(r, "Schnellste", bst(0));
      row(r, "Langsamste", String(worst) + " ms");
      row(r, "Zu früh gedrückt", String(fouls[0]));
    } else {
      row(r, "Punkte", String(pts[0]), String(pts[1]));
      row(r, "Schnellste", bst(0), bst(1));
      row(r, "Durchschnitt", avg(0), avg(1));
      row(r, "Zu früh", String(fouls[0]), String(fouls[1]));
    }
    row(r, "Hinweis", "inkl. WLAN-Weg (~10–30 ms)");
  }
};

// ===========================================================================
// Flatterherz (allein oder zu zweit gleichzeitig): Tippen = flattern, durch die
// Luecken der Roehren fliegen. Zu zweit gewinnt, wer laenger in der Luft bleibt.
// ===========================================================================
class Flap : public Game {
  static constexpr int BX = 2;          // Spalte der Herzen
  static constexpr float GRAV = 15.0f;  // Felder/s^2
  static constexpr float LIFT = -6.2f;  // Felder/s direkt nach dem Flattern (~1,3 Felder hoch)
  struct Pipe { int8_t x, gap, h; };    // Luecke: Zeilen gap .. gap+h-1
  Pipe pipes[4];
  int np = 0;
  float y[2] = {4, 5}, vy[2] = {0, 0};
  bool alive[2] = {true, true}, flying[2] = {false, false};
  int pts[2] = {0, 0}, flaps[2] = {0, 0};
  uint32_t deadT[2] = {0, 0}, lastPhys = 0, lastScroll = 0, scrollMs = 300, endT = 0;
  int lastGap = 3, sinceSpawn = 0;

  int players() const { return humans == 2 ? 2 : 1; }
  float base(int p) const { return p ? 5.0f : 4.0f; }
  int cell(int p) const { return constrain((int)(y[p] + 0.5f), 0, GH - 1); }
  void spawn() {
    const int h = max(pts[0], pts[1]) < 10 ? 4 : 3;  // ab 10 Punkten engere Luecken
    lastGap = constrain(lastGap + rnd(-3, 3), 1, GH - 1 - h);
    if (np < 4) pipes[np++] = {(int8_t)(GW - 1), (int8_t)lastGap, (int8_t)h};
  }
  bool hitsPipe(int r) const {
    for (int i = 0; i < np; i++)
      if (pipes[i].x == BX && (r < pipes[i].gap || r >= pipes[i].gap + pipes[i].h)) return true;
    return false;
  }
  void kill(int p) {
    if (!alive[p]) return;
    alive[p] = false;
    deadT[p] = t;
  }

 public:
  void start() override {
    for (int p = 0; p < 2; p++) {
      y[p] = base(p);
      vy[p] = 0;
      alive[p] = p < players();
      flying[p] = false;
    }
    np = 0;
    lastPhys = t;
    lastScroll = t + 900;  // erste Roehre kommt nach ~1 s ins Bild
    scrollMs = 300;
    sinceSpawn = 99;
  }
  void key(uint8_t p, uint8_t k, bool down) override {
    if (!down || p >= players() || !alive[p] || (k != K_A && k != K_B && k != K_UP)) return;
    vy[p] = LIFT;
    flying[p] = true;
    flaps[p]++;
  }
  void tick() override {
    if (endT) return;
    const uint32_t dt = min<uint32_t>(t - lastPhys, 60);
    lastPhys = t;
    const float s = dt / 1000.0f;
    const bool hover = t - startT < 1300;  // am Anfang schweben, bis man flattert
    for (int p = 0; p < players(); p++) {
      if (!alive[p]) continue;
      if (hover && !flying[p]) { y[p] = base(p) + 0.35f * sinf((t - startT) / 230.0f); continue; }
      vy[p] += GRAV * s;
      y[p] += vy[p] * s;
      if (y[p] < 0) { y[p] = 0; vy[p] = 0; }  // Decke: anstossen erlaubt
      if (y[p] > GH - 0.5f) { y[p] = GH - 1; kill(p); continue; }  // Boden
      if (hitsPipe(cell(p))) kill(p);
    }
    if ((int32_t)(t - lastScroll) >= (int32_t)scrollMs) {
      lastScroll = t;
      int n = 0;
      for (int i = 0; i < np; i++) {
        pipes[i].x--;
        if (pipes[i].x == BX - 1)  // Luecke geschafft
          for (int p = 0; p < players(); p++)
            if (alive[p]) pts[p]++;
        if (pipes[i].x == BX - 1) scrollMs = max<uint32_t>(150, scrollMs - 6);
        if (pipes[i].x >= 0) pipes[n++] = pipes[i];
      }
      np = n;
      if (++sinceSpawn >= 5) { spawn(); sinceSpawn = 0; }
      for (int p = 0; p < players(); p++)
        if (alive[p] && hitsPipe(cell(p))) kill(p);
    }
    bool anyAlive = false;
    uint32_t lastDeath = 0;
    for (int p = 0; p < players(); p++) {
      anyAlive |= alive[p];
      lastDeath = max(lastDeath, deadT[p]);
    }
    if (!anyAlive && t - lastDeath > 1300) endT = t;
  }
  void draw(Canvas& c) override {
    for (int i = 0; i < np; i++)
      for (int r = 0; r < GH; r++) {
        const Pipe& pp = pipes[i];
        if (r >= pp.gap && r < pp.gap + pp.h) continue;
        const bool lip = r == pp.gap - 1 || r == pp.gap + pp.h;  // Rand an der Luecke heller
        c.set(pp.x, r, lip ? CRGB(90, 255, 120) : CRGB(0, 150, 45));
      }
    for (int p = 0; p < players(); p++) {
      CRGB col = P_COL[p];
      if (!alive[p]) {
        if (t - deadT[p] > 1300) continue;
        col = ((t - deadT[p]) / 120) % 2 ? CRGB(255, 0, 0) : CRGB(255, 200, 0);
      } else if (vy[p] < -2.5f) {
        col += CRGB(70, 70, 70);  // kurz heller beim Flattern
      }
      if (p == 1 && alive[0] && cell(0) == cell(1)) c.add(BX, cell(p), col);
      else c.set(BX, cell(p), col);
    }
    if (players() == 1) {
      for (int k = 0; k < 4; k++) c.dots[k] = k < pts[0] % 5 ? P_COL[0] : CRGB::Black;
    } else {
      c.dots[0] = c.dots[1] = alive[0] ? P_COL[0] : CRGB::Black;
      c.dots[2] = c.dots[3] = alive[1] ? P_COL[1] : CRGB::Black;
    }
  }
  bool over() const override { return endT && t - endT > 100; }
  int8_t winner() const override {
    if (players() == 1) return -1;
    if (deadT[0] != deadT[1]) return deadT[0] > deadT[1] ? 0 : 1;  // wer laenger fliegt
    return pts[0] > pts[1] ? 0 : pts[1] > pts[0] ? 1 : 2;
  }
  int32_t score(uint8_t p) const override { return pts[p & 1]; }
  int32_t bigNumber() const override { return pts[0]; }
  int32_t hsValue(uint8_t p) const override { return p < players() ? pts[p] : -1; }
  void live(char* b, size_t n) const override {
    if (players() == 1) snprintf(b, n, "%d Lücken", pts[0]);
    else snprintf(b, n, "%d : %d Lücken", pts[0], pts[1]);
  }
  void results(JsonArray r) const override {
    if (players() == 1) {
      row(r, "Lücken", String(pts[0]));
      row(r, "Geflattert", String(flaps[0]) + "×");
    } else {
      row(r, "Lücken", String(pts[0]), String(pts[1]));
      row(r, "Geflattert", String(flaps[0]) + "×", String(flaps[1]) + "×");
    }
    row(r, "Spielzeit", mmss(seconds()));
    row(r, "Endtempo", String(1000.0f / scrollMs, 1) + " Felder/s");
  }
};

}  // namespace

Game* create(uint8_t id) {
  switch (id) {
    case 0: return new Snake();
    case 1: return new Tetris();
    case 2: return new Pong();
    case 3: return new Tron();
    case 4: return new Connect4();
    case 5: return new Reaction();
    case 6: return new Flap();
    default: return nullptr;
  }
}

}  // namespace games
