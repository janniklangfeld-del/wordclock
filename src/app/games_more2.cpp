// Weitere Spiele der MJ Arcade (Teil 2): Herzregen, Mauerbrecher, Huerdenlauf.
// Spielfeld 11 x 10 Buchstabenfelder, y = 0 oben.
#include "games_internal.h"

namespace games {
namespace {

int rnd(int lo, int hi) { return lo + (int)(esp_random() % (uint32_t)(hi - lo + 1)); }  // lo..hi
float frnd() { return (esp_random() % 10000) / 10000.0f; }                               // 0..1
void fillAll(Canvas& c, CRGB col) {
  for (int y = 0; y < GH; y++)
    for (int x = 0; x < GW; x++) c.set(x, y, col);
}

// Gedrueckt halten = weiterlaufen: sofort 1 Schritt, nach 170 ms Wiederholung
struct Hold {
  bool l = false, r = false;
  uint32_t next = 0;
  int key(uint8_t k, bool down, uint32_t t) {
    if (k == K_LEFT) l = down;
    else if (k == K_RIGHT) r = down;
    else return 0;
    if (!down) return 0;
    next = t + 170;
    return k == K_LEFT ? -1 : 1;
  }
  int tick(uint32_t t, uint32_t every) {
    if (l == r || (int32_t)(t - next) < 0) return 0;
    next = t + every;
    return l ? -1 : 1;
  }
};

// ===========================================================================
// Herzregen (allein: 3 Leben / zu zweit: 60 s, wer mehr faengt): Herzen fangen,
// Goldherzen = 3 Punkte, Blitzen ausweichen.
// ===========================================================================
class Herz : public Game {
  enum : uint8_t { HEART, GOLD, ZAP };
  struct Item { float x, y, vy; uint8_t kind; bool on; };
  Item it[14];
  int bx[2] = {4, 7}, w = 3;
  Hold hold[2];
  int pts[2] = {0, 0}, hearts[2] = {0, 0}, golds[2] = {0, 0}, zaps[2] = {0, 0}, lives = 3, missed = 0;
  uint32_t nextSpawn = 0, lastPhys = 0, endT = 0, hitT[2] = {0, 0};
  float speed = 2.6f, spawnMs = 1050;
  static constexpr uint32_t DUR = 60000;

  int n() const { return humans == 2 ? 2 : 1; }
  void move(int p, int d) {
    if (d) bx[p] = constrain(bx[p] + d, 0, GW - w);
  }
  void spawn() {
    for (auto& i : it) {
      if (i.on) continue;
      const float r = frnd(), zapP = min(0.32f, 0.1f + (t - startT) / 1000.0f * 0.004f);
      i.kind = r < 0.08f ? GOLD : r < 0.08f + zapP ? ZAP : HEART;
      i.x = rnd(0, GW - 1);
      i.y = -0.5f;
      i.vy = speed * (0.8f + 0.4f * frnd());
      i.on = true;
      break;
    }
    speed = min(7.0f, speed + 0.035f);
    spawnMs = max(330.0f, spawnMs * 0.985f);
    nextSpawn = t + (uint32_t)(spawnMs * (0.6f + 0.8f * frnd()));
  }

 public:
  void start() override {
    w = n() == 1 ? 3 : 2;
    bx[0] = n() == 1 ? 4 : 2;
    bx[1] = 7;
    for (auto& i : it) i.on = false;
    lastPhys = t;
    nextSpawn = t + 600;
  }
  void key(uint8_t p, uint8_t k, bool down) override {
    if (p >= n() || endT) return;
    move(p, hold[p].key(k, down, t));
  }
  void tick() override {
    if (endT) return;
    const float s = min<uint32_t>(t - lastPhys, 60) / 1000.0f;
    lastPhys = t;
    for (int p = 0; p < n(); p++) move(p, hold[p].tick(t, 85));
    if ((int32_t)(t - nextSpawn) >= 0) spawn();
    for (auto& i : it) {
      if (!i.on) continue;
      const float before = i.y;
      i.y += i.vy * s;
      if (before < 8.5f && i.y >= 8.5f) {  // Hoehe der Koerbe erreicht
        const int x = (int)lroundf(i.x);
        int who = -1;
        float bestD = 99;
        for (int p = 0; p < n(); p++)
          if (x >= bx[p] && x < bx[p] + w) {
            const float d = fabsf(x - (bx[p] + (w - 1) / 2.0f));
            if (d < bestD) { bestD = d; who = p; }
          }
        if (who >= 0) {
          i.on = false;
          if (i.kind == HEART) { pts[who]++; hearts[who]++; }
          else if (i.kind == GOLD) { pts[who] += 3; golds[who]++; }
          else {
            zaps[who]++;
            hitT[who] = t;
            if (n() == 1) lives--;
            else pts[who] = max(0, pts[who] - 2);
          }
          continue;
        }
      }
      if (i.y > 9.6f) {
        i.on = false;
        if (n() == 1 && i.kind != ZAP) { lives--; missed++; hitT[0] = t; }
      }
    }
    if ((n() == 1 && lives <= 0) || (n() == 2 && t - startT >= DUR)) endT = t;
  }
  void draw(Canvas& c) override {
    for (const auto& i : it) {
      if (!i.on) continue;
      CRGB col = i.kind == HEART ? CRGB(255, 20, 70) : i.kind == GOLD ? CRGB(255, 200, 0)
                                                                      : ((t / 70) % 2 ? CRGB(170, 220, 255) : CRGB(40, 40, 255));
      c.set((int)lroundf(i.x), (int)lroundf(i.y), col);
    }
    for (int p = 0; p < n(); p++) {
      CRGB col = n() == 1 ? CRGB(255, 150, 60) : P_COL[p];
      if (t - hitT[p] < 400 && ((t - hitT[p]) / 80) % 2 == 0) col = CRGB(255, 0, 0);
      for (int x = 0; x < w; x++) c.set(bx[p] + x, GH - 1, col);
      if (n() == 1) { c.set(bx[p], GH - 2, col); c.set(bx[p] + w - 1, GH - 2, col); }  // Korbrand
    }
    if (n() == 1) {
      for (int k = 0; k < 4; k++) c.dots[k] = k < lives ? CRGB(255, 20, 70) : CRGB::Black;
    } else {
      const uint32_t left = DUR - min<uint32_t>(DUR, t - startT);  // 4 Punkte = verbleibende Zeit
      for (int k = 0; k < 4; k++) c.dots[k] = left > k * 15000 ? CRGB(255, 200, 0) : CRGB::Black;
    }
  }
  bool over() const override { return endT && t - endT > 800; }
  int8_t winner() const override { return n() == 1 ? -1 : pts[0] > pts[1] ? 0 : pts[1] > pts[0] ? 1 : 2; }
  int32_t score(uint8_t p) const override { return pts[p & 1]; }
  int32_t bigNumber() const override { return pts[0]; }
  int32_t hsValue(uint8_t p) const override { return p < n() ? pts[p] : -1; }
  void live(char* s, size_t l) const override {
    if (n() == 1) snprintf(s, l, "%d Punkte · %d Leben", pts[0], max(0, lives));
    else snprintf(s, l, "%d : %d · noch %lu s", pts[0], pts[1], (unsigned long)((DUR - min<uint32_t>(DUR, t - startT)) / 1000));
  }
  void results(JsonArray r) const override {
    if (n() == 1) {
      row(r, "Punkte", String(pts[0]));
      row(r, "Herzen", String(hearts[0]));
      row(r, "Goldherzen", String(golds[0]));
      row(r, "Blitze erwischt", String(zaps[0]));
      row(r, "Verpasst", String(missed));
    } else {
      row(r, "Punkte", String(pts[0]), String(pts[1]));
      row(r, "Herzen", String(hearts[0]), String(hearts[1]));
      row(r, "Goldherzen", String(golds[0]), String(golds[1]));
      row(r, "Blitze erwischt", String(zaps[0]), String(zaps[1]));
    }
    row(r, "Spielzeit", mmss(seconds()));
  }
};

// ===========================================================================
// Mauerbrecher (allein): Schlaeger unten, Ball, 4 Reihen Steine. 3 Leben, Level.
// ===========================================================================
class Mauer : public Game {
  static constexpr int ROWS = 4, Y0 = 1, PW = 3;
  uint8_t br[ROWS][GW];  // Haerte (0 = weg)
  float x = 5, y = 8, vx = 0, vy = 0, spd = 5.5f;
  int px = 4, lives = 3, pts = 0, level = 1, bricks = 0;
  bool stuck = true;
  Hold hold;
  uint32_t lastPhys = 0, stuckT = 0, lostT = 0, endT = 0;

  void newLevel() {
    for (int r = 0; r < ROWS; r++)
      for (int c = 0; c < GW; c++) br[r][c] = (level >= 2 && r == 0) || (level >= 3 && r == 1) ? 2 : 1;
    spd = 5.5f + (level - 1) * 0.7f;
    reset();
  }
  void reset() {
    stuck = true;
    stuckT = t;
  }
  void launch() {
    if (!stuck) return;
    stuck = false;
    const float a = (frnd() - 0.5f) * 1.0f;  // leicht schraeg nach oben
    vx = spd * sinf(a);
    vy = -spd * cosf(a);
    x = px + 1;
    y = 8;
  }
  bool any() const {
    for (int r = 0; r < ROWS; r++)
      for (int c = 0; c < GW; c++)
        if (br[r][c]) return true;
    return false;
  }

 public:
  void start() override {
    lastPhys = t;
    newLevel();
  }
  void key(uint8_t, uint8_t k, bool down) override {
    if (endT) return;
    if (down && (k == K_A || k == K_UP || k == K_B)) launch();
    px = constrain(px + hold.key(k, down, t), 0, GW - PW);
  }
  void tick() override {
    if (endT) return;
    const float dt = min<uint32_t>(t - lastPhys, 60) / 1000.0f;
    lastPhys = t;
    px = constrain(px + hold.tick(t, 70), 0, GW - PW);
    if (stuck) {
      x = px + 1;
      y = 8;
      if (t - stuckT > 1200) launch();
      return;
    }
    const int steps = max(1, (int)ceilf(spd * dt / 0.2f));
    const float h = dt / steps;
    for (int k = 0; k < steps; k++) {
      float nx = x + vx * h, ny = y + vy * h;
      if (nx < 0) { nx = -nx; vx = fabsf(vx); }
      if (nx > GW - 1) { nx = 2 * (GW - 1) - nx; vx = -fabsf(vx); }
      if (ny < 0) { ny = -ny; vy = fabsf(vy); }
      const int cx = constrain((int)lroundf(nx), 0, GW - 1), cy = (int)lroundf(ny);
      if (cy >= Y0 && cy < Y0 + ROWS && br[cy - Y0][cx]) {  // Stein getroffen
        if (--br[cy - Y0][cx] == 0) { pts++; bricks++; }
        if ((int)lroundf(y) != cy) vy = -vy;
        else vx = -vx;
        continue;  // an der alten Stelle bleiben, neue Richtung
      }
      if (vy > 0 && y < 8.5f && ny >= 8.5f && cx >= px && cx < px + PW) {  // Schlaeger: Winkel je nach Trefferpunkt
        const float off = constrain((nx - (px + 1)) / 1.5f, -1.0f, 1.0f);
        const float a = off * 1.05f;  // bis ~60 Grad
        vx = spd * sinf(a);
        vy = -spd * cosf(a);
        ny = 8.5f - (ny - 8.5f);
      }
      x = nx;
      y = ny;
      if (y > 9.6f) {  // verloren
        lives--;
        lostT = t;
        if (lives <= 0) { endT = t; return; }
        reset();
        return;
      }
    }
    if (!any()) {
      level++;
      pts += 5;
      newLevel();
    }
  }
  void draw(Canvas& c) override {
    for (int r = 0; r < ROWS; r++)
      for (int col = 0; col < GW; col++)
        if (br[r][col]) {
          CRGB k = CHSV(r * 45 + level * 20, 255, 255);
          if (br[r][col] == 2) k += CRGB(70, 70, 70);  // harter Stein: heller
          else k.nscale8_video(200);
          c.set(col, Y0 + r, k);
        }
    for (int i = 0; i < PW; i++) c.set(px + i, GH - 1, CRGB(180, 180, 255));
    c.set((int)lroundf(x), (int)lroundf(y), CRGB(255, 255, 200));
    if (t - lostT < 500 && ((t - lostT) / 100) % 2 == 0) for (int col = 0; col < GW; col++) c.set(col, GH - 1, CRGB(255, 0, 0));
    for (int k = 0; k < 4; k++) c.dots[k] = k < lives ? CRGB(255, 120, 0) : CRGB::Black;
  }
  bool over() const override { return endT && t - endT > 800; }
  int32_t score(uint8_t) const override { return pts; }
  int32_t bigNumber() const override { return pts; }
  int32_t hsValue(uint8_t p) const override { return p == 0 ? pts : -1; }
  void live(char* s, size_t l) const override { snprintf(s, l, "Level %d · %d Leben", level, max(0, lives)); }
  void results(JsonArray r) const override {
    row(r, "Punkte", String(pts));
    row(r, "Steine", String(bricks));
    row(r, "Level", String(level));
    row(r, "Spielzeit", mmss(seconds()));
  }
};

// ===========================================================================
// Huerdenlauf (allein / zu zweit auf zwei Bahnen): Springen ueber Huerden, Ducken
// unter Voegeln. Zu zweit gewinnt, wer laenger laeuft.
// ===========================================================================
class Lauf : public Game {
  enum : uint8_t { H_LOW, H_HIGH, BIRD };
  struct Obs { float x; uint8_t kind; bool on; };
  static constexpr int RX = 2;
  static constexpr float GRAV = 37.0f, V0 = 13.9f;  // ~2,6 Felder hoch, ~0,75 s in der Luft
  Obs ob[8];
  float speed = 6.0f, gap = 8;
  float hgt[2] = {0, 0}, vh[2] = {0, 0};
  bool duck[2] = {false, false}, alive[2] = {true, true};
  int pts[2] = {0, 0}, jumps[2] = {0, 0}, ducks[2] = {0, 0};
  uint32_t deadT[2] = {0, 0}, lastPhys = 0, endT = 0;

  int n() const { return humans == 2 ? 2 : 1; }
  int ground(int p) const { return n() == 1 ? GH - 1 : (p ? GH - 1 : 4); }
  // Felder des Laeufers (unterstes zuerst); Ducken = nur 1 Feld hoch
  int body(int p, int* rows) const {
    const int g = ground(p), lift = (int)lroundf(hgt[p]);
    if (duck[p] && lift == 0) { rows[0] = g - 1; return 1; }
    rows[0] = g - 1 - lift;
    rows[1] = g - 2 - lift;
    return 2;
  }
  int obsRows(const Obs& o, int g, int* rows) const {
    if (o.kind == H_LOW) { rows[0] = g - 1; return 1; }
    if (o.kind == H_HIGH) { rows[0] = g - 1; rows[1] = g - 2; return 2; }
    rows[0] = g - 2;  // Vogel auf Kopfhoehe
    return 1;
  }
  bool hits(int p) const {
    int br[2], orow[2];
    const int nb = body(p, br);
    for (const auto& o : ob) {
      if (!o.on || (int)lroundf(o.x) != RX) continue;
      const int no = obsRows(o, ground(p), orow);
      for (int i = 0; i < nb; i++)
        for (int j = 0; j < no; j++)
          if (br[i] == orow[j]) return true;
    }
    return false;
  }
  float lastX() const {
    float m = -99;
    for (const auto& o : ob)
      if (o.on) m = max(m, o.x);
    return m;
  }

 public:
  void start() override {
    for (auto& o : ob) o.on = false;
    for (int p = 0; p < 2; p++) alive[p] = p < n();
    lastPhys = t;
  }
  void key(uint8_t p, uint8_t k, bool down) override {
    if (p >= n() || !alive[p] || endT) return;
    if ((k == K_UP || k == K_A) && down && hgt[p] == 0 && vh[p] == 0) { vh[p] = V0; jumps[p]++; duck[p] = false; }
    if (k == K_DOWN || k == K_B) {
      if (down && !duck[p]) ducks[p]++;
      duck[p] = down;
      if (down && hgt[p] > 0) vh[p] = min(vh[p], -8.0f);  // in der Luft: schnell runter
    }
  }
  void tick() override {
    if (endT) return;
    const float s = min<uint32_t>(t - lastPhys, 60) / 1000.0f;
    lastPhys = t;
    if (t - startT < 800) return;  // kurz Anlauf nehmen
    for (int p = 0; p < n(); p++) {
      if (!alive[p] || (hgt[p] == 0 && vh[p] == 0)) continue;
      vh[p] -= GRAV * s;
      hgt[p] += vh[p] * s;
      if (hgt[p] <= 0) { hgt[p] = 0; vh[p] = 0; }
    }
    const int best = max(pts[0], pts[1]);
    speed = min(12.0f, 6.0f + best * 0.15f);
    for (auto& o : ob) {
      if (!o.on) continue;
      const float before = o.x;
      o.x -= speed * s;
      if (before >= RX - 0.5f && o.x < RX - 0.5f)
        for (int p = 0; p < n(); p++)
          if (alive[p]) pts[p]++;
      if (o.x < -1) o.on = false;
    }
    if (lastX() < GW - 1 - gap) {  // naechstes Hindernis (beide Bahnen gleich)
      for (auto& o : ob)
        if (!o.on) {
          const int r = rnd(0, 99);
          o.kind = best >= 4 && r < 30 ? BIRD : r < 70 ? H_LOW : H_HIGH;
          o.x = GW + 0.4f;
          o.on = true;
          break;
        }
      gap = rnd(4, 9) + speed * 0.25f;
    }
    for (int p = 0; p < n(); p++)
      if (alive[p] && hits(p)) { alive[p] = false; deadT[p] = t; }
    bool any = false;
    uint32_t last = 0;
    for (int p = 0; p < n(); p++) { any |= alive[p]; last = max(last, deadT[p]); }
    if (!any && t - last > 1300) endT = t;
  }
  void draw(Canvas& c) override {
    for (int p = 0; p < n(); p++) {
      const int g = ground(p);
      for (int x = 0; x < GW; x++) c.set(x, g, (x + (int)(t * speed / 1000)) % 3 ? CRGB(0, 35, 0) : CRGB(0, 70, 10));
      int rows[2];
      for (const auto& o : ob) {
        if (!o.on) continue;
        const int no = obsRows(o, g, rows);
        const CRGB col = o.kind == BIRD ? ((t / 120) % 2 ? CRGB(200, 200, 255) : CRGB(120, 120, 255)) : CRGB(255, 110, 0);
        for (int j = 0; j < no; j++) c.set((int)lroundf(o.x), rows[j], col);
      }
      CRGB rc = P_COL[p];
      if (!alive[p]) {
        if (t - deadT[p] > 1300) continue;
        rc = ((t - deadT[p]) / 110) % 2 ? CRGB(255, 0, 0) : CRGB(255, 200, 0);
      }
      const int nb = body(p, rows);
      for (int i = 0; i < nb; i++) c.set(RX, rows[i], rc);
    }
    if (n() == 1) {
      for (int k = 0; k < 4; k++) c.dots[k] = k < pts[0] % 5 ? P_COL[0] : CRGB::Black;
    } else {
      c.dots[0] = c.dots[1] = alive[0] ? P_COL[0] : CRGB::Black;
      c.dots[2] = c.dots[3] = alive[1] ? P_COL[1] : CRGB::Black;
    }
  }
  bool over() const override { return endT && t - endT > 100; }
  int8_t winner() const override {
    if (n() == 1) return -1;
    if (deadT[0] != deadT[1]) return deadT[0] > deadT[1] ? 0 : 1;
    return pts[0] > pts[1] ? 0 : pts[1] > pts[0] ? 1 : 2;
  }
  int32_t score(uint8_t p) const override { return pts[p & 1]; }
  int32_t bigNumber() const override { return pts[0]; }
  int32_t hsValue(uint8_t p) const override { return p < n() ? pts[p] : -1; }
  void live(char* s, size_t l) const override {
    if (n() == 1) snprintf(s, l, "%d Hürden", pts[0]);
    else snprintf(s, l, "%d : %d Hürden", pts[0], pts[1]);
  }
  void results(JsonArray r) const override {
    if (n() == 1) {
      row(r, "Hürden", String(pts[0]));
      row(r, "Gesprungen", String(jumps[0]) + "×");
      row(r, "Geduckt", String(ducks[0]) + "×");
    } else {
      row(r, "Hürden", String(pts[0]), String(pts[1]));
      row(r, "Gesprungen", String(jumps[0]) + "×", String(jumps[1]) + "×");
      row(r, "Geduckt", String(ducks[0]) + "×", String(ducks[1]) + "×");
    }
    row(r, "Spielzeit", mmss(seconds()));
    row(r, "Endtempo", String(speed, 1) + " Felder/s");
  }
};

}  // namespace

Game* newHerz() { return new Herz(); }
Game* newMauer() { return new Mauer(); }
Game* newLauf() { return new Lauf(); }

}  // namespace games
