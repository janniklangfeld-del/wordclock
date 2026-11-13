// MJ Arcade – Spiel-Engine: Plaetze, Lobby, Countdown, Pause, Ergebnis, Rekorde.
#include "games.h"
#include <Preferences.h>
#include <WiFi.h>
#include "clockface.h"
#include "games_internal.h"
#include "settings.h"
#include "web.h"

namespace games {
namespace {

enum Phase : uint8_t { IDLE, LOBBY, COUNT, PLAY, PAUSE, OVER };
const char* const PHASE_NAMES[] = {"idle", "lobby", "count", "play", "pause", "over"};
enum : uint8_t { M_1P = 1, M_2P = 2, M_BOT = 4 };
const char* const MODE_NAMES[] = {"1p", "2p", "bot"};

struct Info {
  const char* key;
  const char* title;
  uint8_t modes;
  uint8_t rec;  // 0 nur Siege, 1 hoeher ist besser, 2 niedriger ist besser (Einzelspiel)
  const char* icon[GH];
};
const Info GAMES[] = {
    {"snake", "Snake", M_1P, 1,
     {"...........", ".GGGGG.....", ".G...G.....", ".G...G..R..", ".G...GGGG..", ".G.........", ".GGGGGGGGW.",
      "...........", "...........", "..........."}},
    {"tetris", "Tetris", M_1P | M_2P | M_BOT, 1,
     {"....YY.....", "....YY.....", "...........", "........C..", "........C..", "........C..", "P.......C..",
      "PP....RR...", "PBBB.GRRGGO", "BBGGGGOOOOO"}},
    {"pong", "Pong", M_2P | M_BOT, 0,
     {"...........", "...........", "P..........", "P.........T", "P....W....T", "..........T", "...........",
      "...........", "...........", "..........."}},
    {"tron", "Lichtrenner", M_2P | M_BOT, 0,
     {"...........", ".PPPPP.....", ".....P.....", ".....P.....", ".....PW....", "...........", "....TTTTTT.",
      "....T......", "....T......", "..........."}},
    {"vier", "Vier gewinnt", M_2P | M_BOT, 0,
     {"...........", "...........", "...........", "....T......", "..bbbbbbb..", "..bbbbbbb..", "..bbbTbbb..",
      "..bbbPTbb..", "..bTPPTbb..", "..PPTPTPb.."}},
    {"reaktion", "Reaktion", M_1P | M_2P, 2,
     {"...........", "....YYY....", "...Y...Y...", "..Y..Y..Y..", "..Y..YY.Y..", "..Y.....Y..", "...Y...Y...",
      "....YYY....", "...........", "..........."}},
};
constexpr int N_GAMES = sizeof(GAMES) / sizeof(GAMES[0]);
#ifndef WEBSOCKETS_SERVER_CLIENT_MAX
#define WEBSOCKETS_SERVER_CLIENT_MAX 5
#endif
constexpr int MAX_CLIENTS = WEBSOCKETS_SERVER_CLIENT_MAX;

struct Slot {
  bool used = false, ready = false, isBot = false;
  int8_t client = -1;
  String name;
  String dev;  // zufaellige Geraete-Kennung der Webseite (localStorage)
};

Phase phase = IDLE;
int8_t gameId = -1;
uint8_t mode = M_1P;
Slot slots[2];
Game* game = nullptr;
bool resumeAfterCount = false;
uint32_t phaseT = 0, lastActivity = 0;
uint32_t gclock = 0, lastReal = 0;  // Spielzeit (steht in der Pause still)
String pauseReason;
uint32_t emptySince = 0;  // Lobby ohne verbundene Spieler seit

struct Ev { uint8_t p, k, down; };
Ev evq[32];
uint8_t evHead = 0, evTail = 0;

bool dirty = true;
uint32_t lastSent = 0;
String lastLive;

JsonDocument rec;        // Rekorde (NVS)
JsonDocument resultDoc;  // letztes Ergebnis
Canvas canvas;

uint8_t needed() { return mode == M_2P ? 2 : 1; }
bool slotOk(int i) { return slots[i].used && (slots[i].isBot || slots[i].client >= 0); }
bool allPresent() {
  for (int i = 0; i < needed(); i++)
    if (!slotOk(i)) return false;
  return mode != M_BOT || slotOk(1);
}
int slotOf(uint8_t client) {
  for (int i = 0; i < 2; i++)
    if (slots[i].used && !slots[i].isBot && slots[i].client == client) return i;
  return -1;
}

String cleanName(String n) {
  n.trim();
  return n.substring(0, 14);
}
// Zwei Spieler mit gleichem Namen (z. B. beide "MJ") -> der zweite heisst "MJ 2"
String uniqueName(int slot, const String& n) {
  const Slot& o = slots[1 - slot];
  if (o.used && o.name.equalsIgnoreCase(n)) return n.substring(0, 12) + " 2";
  return n;
}

void setPhase(Phase p) {
  if (p == phase) return;
  emptySince = 0;
  phase = p;
  phaseT = millis();
  lastActivity = phaseT;
  dirty = true;
}

void freeGame() {
  delete game;
  game = nullptr;
}

void loadRecords() {
  Preferences p;
  p.begin("games", true);
  String s = p.getString("rec", "{}");
  p.end();
  if (deserializeJson(rec, s) || !rec.is<JsonObject>()) rec.to<JsonObject>();
}
void saveRecords() {
  String s;
  serializeJson(rec, s);
  Preferences p;
  p.begin("games", false);
  p.putString("rec", s);
  p.end();
}

// Ergebnis festhalten und Rekorde fortschreiben
void finish() {
  const Info& gi = GAMES[gameId];
  resultDoc.clear();
  JsonObject r = resultDoc.to<JsonObject>();
  const int8_t w = game->winner();
  r["winner"] = w;
  r["secs"] = game->seconds();
  JsonArray sc = r["score"].to<JsonArray>();
  sc.add(game->score(0));
  sc.add(game->score(1));
  game->results(r["rows"].to<JsonArray>());

  JsonObject g = rec[gi.key].is<JsonObject>() ? rec[gi.key].as<JsonObject>() : rec[gi.key].to<JsonObject>();
  g["n"] = (g["n"] | 0) + 1;
  g["s"] = (g["s"] | 0) + game->seconds();
  if (mode == M_1P && gi.rec) {
    const int32_t v = game->bigNumber();
    r["big"] = v;
    const bool has = g["hs"].is<int>();
    const bool better = !has || (gi.rec == 1 ? v > (int32_t)g["hs"] : v < (int32_t)g["hs"]);
    if (better && (gi.rec == 1 ? v > 0 : v < 999)) {
      r["record"] = true;
      r["prev"] = has ? g["hs"].as<int32_t>() : -1;
      g["hs"] = v;
      g["hsn"] = slots[0].name;
    }
  }
  if (w == 0 || w == 1) {
    JsonObject wins = g["w"].is<JsonObject>() ? g["w"].as<JsonObject>() : g["w"].to<JsonObject>();
    const String& n = slots[w].name;
    if (wins[n].is<int>() || wins.size() < 8) wins[n] = (wins[n] | 0) + 1;
  }
  rec["total"] = (rec["total"] | 0) + 1;
  saveRecords();
  setPhase(OVER);
}

void startGame() {
  freeGame();
  game = create(gameId);
  if (!game) { setPhase(IDLE); return; }
  game->humans = mode == M_2P ? 2 : 1;
  game->bot = mode == M_BOT;
  game->t = gclock;
  game->startT = gclock;
  game->start();
  evHead = evTail = 0;
  setPhase(PLAY);
}

void openLobby(int8_t id, uint8_t m) {
  freeGame();
  gameId = id;
  mode = m;
  for (auto& s : slots) s.ready = false;
  if (slots[1].isBot) slots[1] = Slot();
  if (mode == M_1P || mode == M_BOT) {  // nur ein Mensch: Platz 2 frei machen
    if (!slots[0].used && slots[1].used && !slots[1].isBot) { slots[0] = slots[1]; }
    slots[1] = Slot();
  }
  if (mode == M_BOT) {
    slots[1].used = slots[1].ready = slots[1].isBot = true;
    slots[1].name = "MJ-Bot";
  }
  resultDoc.clear();
  setPhase(LOBBY);
}

void goIdle() {
  freeGame();
  gameId = -1;
  for (auto& s : slots) s = Slot();
  setPhase(IDLE);
}

void pause(const char* why) {
  if (phase != PLAY && phase != COUNT) return;
  if (phase == COUNT && !resumeAfterCount) { setPhase(LOBBY); return; }
  pauseReason = why;
  setPhase(PAUSE);
}

void beginCount(bool resume) {
  resumeAfterCount = resume;
  setPhase(COUNT);
}

// ---------- Zustand an alle Handys ----------
void broadcast() {
  JsonDocument d;
  d["t"] = "game";
  d["ph"] = PHASE_NAMES[phase];
  if (gameId >= 0) {
    d["g"] = GAMES[gameId].key;
    d["title"] = GAMES[gameId].title;
    d["m"] = MODE_NAMES[mode == M_1P ? 0 : mode == M_2P ? 1 : 2];
  }
  d["need"] = needed() + (mode == M_BOT ? 1 : 0);
  JsonArray ps = d["p"].to<JsonArray>();
  for (int i = 0; i < 2; i++) {
    JsonObject o = ps.add<JsonObject>();
    o["u"] = slots[i].used;
    o["n"] = slots[i].name;
    o["r"] = slots[i].ready;
    o["c"] = slots[i].isBot || slots[i].client >= 0;
    o["bot"] = slots[i].isBot;
    if (game) o["s"] = game->score(i);
  }
  if (phase == COUNT) d["cd"] = 3 - (int)min<uint32_t>(2, (millis() - phaseT) / 1000);
  if (phase == PAUSE) d["why"] = pauseReason;
  if (game && (phase == PLAY || phase == PAUSE)) {
    char b[48];
    game->live(b, sizeof(b));
    d["live"] = b;
    d["secs"] = game->seconds();
  }
  if (phase == OVER) d["res"] = resultDoc.as<JsonObjectConst>();
  if (gameId >= 0) d["rec"] = rec[GAMES[gameId].key];
  String s;
  serializeJson(d, s);
  // pro Handy "you" (eigener Platz) voranstellen
  for (uint8_t c = 0; c < MAX_CLIENTS; c++) {
    if (!web::clientConnected(c)) continue;
    String m = String("{\"you\":") + slotOf(c) + "," + s.substring(1);
    web::sendTo(c, m);
  }
  lastSent = millis();
  dirty = false;
}

void drawIcon(Canvas& c, const Info& gi, uint32_t now) {
  const uint8_t pulse = beatsin8(24, 120, 255);
  for (int y = 0; y < GH; y++)
    for (int x = 0; x < GW; x++) {
      CRGB col;
      switch (gi.icon[y][x]) {
        case 'G': col = CRGB(0, 230, 40); break;
        case 'W': col = CRGB(220, 255, 200); break;
        case 'R': col = CRGB(255, 20, 10); break;
        case 'Y': col = CRGB(255, 200, 0); break;
        case 'C': col = CRGB(0, 220, 255); break;
        case 'B': col = CRGB(0, 60, 255); break;
        case 'O': col = CRGB(255, 110, 0); break;
        case 'P': col = P_COL[0]; break;
        case 'T': col = P_COL[1]; break;
        case 'b': col = CRGB(0, 0, 26); break;
        default: continue;
      }
      col.nscale8_video(pulse);
      c.set(x, y, col);
    }
  // Minutenpunkte: belegte Plaetze (hell = bereit)
  for (int i = 0; i < 2; i++) {
    CRGB col = P_COL[i];
    if (!slots[i].used) col = CRGB::Black;
    else if (!slots[i].ready) col.nscale8_video(((now / 400) % 2) ? 120 : 30);
    c.dots[i * 2] = c.dots[i * 2 + 1] = col;
  }
}

const char* const TROPHY[GH] = {"...........", "..XXXXXXX..", ".XXXXXXXXX.", ".X.XXXXX.X.", "..XXXXXXX..",
                                "...XXXXX...", "....XXX....", ".....X.....", "...XXXXX...", "..........."};

void drawOver(Canvas& c, uint32_t now) {
  const uint32_t t = now - phaseT;
  if (game && t < 600) {  // letztes Spielbild kurz stehen lassen
    game->t = gclock;
    game->draw(c);
    return;
  }
  const int8_t w = resultDoc["winner"] | -1;
  if (w == 0 || w == 1) {
    for (int y = 0; y < GH; y++)
      for (int x = 0; x < GW; x++)
        if (TROPHY[y][x] == 'X') {
          CRGB col = P_COL[w];
          col.nscale8_video(qadd8(beatsin8(40, 100, 255), sin8(x * 30 + y * 20 + now / 4) / 4));
          c.set(x, y, col);
        }
    if (random8() < 90) c.set(random8(GW), random8(GH), CRGB(255, 255, 200));
    for (int k = 0; k < 4; k++) c.dots[k] = P_COL[w];
  } else if (w == 2) {
    for (int y = 0; y < GH; y++)
      for (int x = 0; x < GW; x++) {
        CRGB col = P_COL[x < 5 ? 0 : 1];
        col.nscale8_video(beatsin8(30, 40, 200, 0, x < 5 ? 0 : 128));
        if (x != 5) c.set(x, y, col);
      }
  } else {
    const int32_t big = resultDoc["big"] | (game ? game->bigNumber() : 0);
    const bool isRec = resultDoc["record"] | false;
    c.number(big, 2, CHSV(now / 10, 200, 255));
    if (isRec) {  // neuer Rekord: Funkeln oben und unten
      for (int k = 0; k < 3; k++) c.set(random8(GW), random8(2) ? 0 : GH - 1, CHSV(random8(), 150, 255));
      for (int k = 0; k < 4; k++) c.dots[k] = CHSV(now / 4 + k * 60, 255, 255);
    }
  }
}

}  // namespace

// ---------------------------------------------------------------------------
void begin() { loadRecords(); }

bool active() { return phase != IDLE; }
bool busy() { return phase == COUNT || phase == PLAY; }
uint8_t brightness() { return settings.power ? max<uint8_t>(settings.brightness, 60) : 90; }

void loop() {
  const uint32_t now = millis();
  if (phase == PLAY) gclock += now - lastReal;
  lastReal = now;
  if (phase == IDLE) {  // auch im Ruhezustand melden (z. B. "Spiel beendet" an alle Seiten)
    if (dirty && now - lastSent > 60) broadcast();
    return;
  }

  switch (phase) {
    case LOBBY:
      if (allPresent()) {
        bool ready = true;
        for (int i = 0; i < 2; i++)
          if (slots[i].used && !slots[i].ready) ready = false;
        if (ready) beginCount(false);
      }
      {
        bool anyone = false;
        for (auto& s : slots) anyone |= s.used && !s.isBot && s.client >= 0;
        if (anyone) emptySince = 0;
        else if (!emptySince) emptySince = now;
        if ((emptySince && now - emptySince > 30000) || now - lastActivity > 10UL * 60 * 1000) goIdle();
      }
      break;
    case COUNT:
      if (!allPresent()) { pause("Spieler fehlt"); break; }
      if (now - phaseT >= 3000) {
        if (resumeAfterCount && game) setPhase(PLAY);
        else startGame();
      } else if ((now - phaseT) / 1000 != (now - phaseT - 20) / 1000) {
        dirty = true;  // Countdown-Zahl aktualisieren
      }
      break;
    case PLAY: {
      while (evTail != evHead) {
        const Ev& e = evq[evTail];
        evTail = (evTail + 1) % 32;
        game->t = gclock;
        game->key(e.p, e.k, e.down);
      }
      game->t = gclock;
      game->tick();
      if (game->over()) { finish(); break; }
      if (now - lastSent > 250) {  // Live-Zeile nur bei Aenderung schicken
        char b[48];
        game->live(b, sizeof(b));
        String l = String(b) + game->score(0) + ":" + game->score(1);
        if (l != lastLive) { lastLive = l; dirty = true; }
      }
      break;
    }
    case PAUSE:
      if (allPresent() && pauseReason == "Spieler fehlt") beginCount(true);
      if (now - lastActivity > 15UL * 60 * 1000) goIdle();
      break;
    case OVER:
      if (now - lastActivity > 5UL * 60 * 1000) goIdle();
      break;
    default:
      break;
  }
  if (dirty && now - lastSent > 60) broadcast();
}

void render(CRGB* leds, uint32_t now) {
  canvas.clear();
  switch (phase) {
    case LOBBY:
      if (gameId >= 0) drawIcon(canvas, GAMES[gameId], now);
      break;
    case COUNT: {
      const int n = 3 - (int)min<uint32_t>(2, (now - phaseT) / 1000);
      if (resumeAfterCount && game) {
        game->t = gclock;
        game->draw(canvas);
        canvas.dim(50);
      }
      const uint32_t f = (now - phaseT) % 1000;
      CRGB col = n == 3 ? CRGB(255, 40, 40) : n == 2 ? CRGB(255, 200, 0) : CRGB(40, 255, 60);
      col.nscale8_video(255 - f / 5);
      canvas.glyph(3, 1, '0' + n, col);
      break;
    }
    case PLAY:
      game->t = gclock;
      game->draw(canvas);
      break;
    case PAUSE:
      if (game) {
        game->t = gclock;
        game->draw(canvas);
        canvas.dim(45);
      }
      if ((now / 600) % 2)
        for (int y = 3; y < 7; y++) { canvas.set(4, y, CRGB(200, 200, 200)); canvas.set(6, y, CRGB(200, 200, 200)); }
      break;
    case OVER:
      drawOver(canvas, now);
      break;
    default:
      break;
  }
  for (int y = 0; y < GH; y++)
    for (int x = 0; x < GW; x++) {
      leds[clockface::letterLed(y, x, 0)] = canvas.px[y][x];
      leds[clockface::letterLed(y, x, 1)] = canvas.px[y][x];
    }
  for (int k = 0; k < 4; k++) leds[clockface::dotLed(k)] = canvas.dots[k];
}

void onConnect(uint8_t) { dirty = true; }

void onDisconnect(uint8_t client) {
  const int s = slotOf(client);
  if (s < 0) return;
  slots[s].client = -1;
  if (phase == LOBBY) slots[s] = Slot();
  else if (phase == PLAY || phase == COUNT) pause("Spieler fehlt");
  dirty = true;
}

void onKey(uint8_t client, uint8_t key, bool down) {
  if (phase != PLAY || key >= K_COUNT) return;
  const int s = slotOf(client);
  if (s < 0) return;
  const uint8_t nh = (evHead + 1) % 32;
  if (nh == evTail) return;  // Puffer voll
  evq[evHead] = {(uint8_t)s, key, (uint8_t)down};
  evHead = nh;
  lastActivity = millis();
}

void onText(uint8_t client, JsonObjectConst m) {
  const char* t = m["t"] | "";
  lastActivity = millis();
  dirty = true;
  int me = slotOf(client);

  if (!strcmp(t, "g.hello")) return;  // nur aktuellen Stand schicken

  if (!strcmp(t, "g.open")) {
    if (phase == PLAY || phase == COUNT || phase == PAUSE) {
      if (me < 0) return;  // laufendes Spiel: nur Mitspieler duerfen wechseln
    }
    int8_t id = -1;
    for (int i = 0; i < N_GAMES; i++)
      if (!strcmp(m["game"] | "", GAMES[i].key)) id = i;
    if (id < 0) return;
    const char* ms = m["mode"] | "1p";
    uint8_t md = !strcmp(ms, "2p") ? M_2P : !strcmp(ms, "bot") ? M_BOT : M_1P;
    if (!(GAMES[id].modes & md)) md = GAMES[id].modes & M_1P ? M_1P : GAMES[id].modes & M_BOT ? M_BOT : M_2P;
    openLobby(id, md);
    me = slotOf(client);
    const String n = cleanName(m["name"] | "");
    const String dev = m["id"] | "";
    // wer oeffnet, spielt mit (auch wenn der Platz noch vom selben Geraet aus einem anderen Tab belegt ist)
    if (me < 0 && (!slots[0].used || slots[0].client < 0 || (dev.length() && slots[0].dev == dev))) {
      slots[0] = Slot();
      slots[0].used = true;
      slots[0].client = client;
      slots[0].dev = dev;
      slots[0].name = uniqueName(0, n.length() ? n : String("Spieler 1"));
    }
    return;
  }
  if (phase == IDLE) return;

  if (!strcmp(t, "g.join")) {
    int want = m["slot"] | -1;
    const String n = cleanName(m["name"] | "");
    const String dev = m["id"] | "";
    // frei, verwaist oder vom selben Geraet belegt (zweiter Tab / Seite neu geladen)
    auto freeFor = [&](int i) {
      return i < needed() && !slots[i].isBot &&
             (!slots[i].used || slots[i].client < 0 || (dev.length() && slots[i].dev == dev));
    };
    if (want < 0 || want > 1 || !freeFor(want)) {
      want = -1;
      for (int i = 0; i < needed(); i++)  // eigener alter Platz zuerst (Wiederkommen nach Abbruch)
        if (freeFor(i) && slots[i].used && ((dev.length() && slots[i].dev == dev) || slots[i].name == n)) want = i;
      for (int i = 0; i < needed() && want < 0; i++)
        if (freeFor(i)) want = i;
    }
    if (want < 0) return;
    if (me >= 0 && me != want) {
      if (phase == LOBBY) slots[me] = Slot();
      else slots[me].client = -1;
    }
    Slot& s = slots[want];
    const bool rejoin = s.used && phase != LOBBY;
    s.used = true;
    s.client = client;
    s.dev = dev;
    if (!rejoin) s.name = uniqueName(want, n.length() ? n : String("Spieler ") + (want + 1));
    if (phase == LOBBY) s.ready = false;
    return;
  }
  if (!strcmp(t, "g.leave")) {
    if (me < 0) return;
    if (phase == LOBBY || phase == OVER) slots[me] = Slot();
    else { slots[me].client = -1; pause("Spieler fehlt"); }
    return;
  }
  if (!strcmp(t, "g.ready")) {
    if (me < 0 || phase != LOBBY) return;
    slots[me].ready = m["on"] | true;
    return;
  }
  if (!strcmp(t, "g.pause")) {
    if (me < 0) return;
    if (phase == PLAY) pause("Pause");
    else if (phase == PAUSE && allPresent()) beginCount(true);
    return;
  }
  if (!strcmp(t, "g.again")) {
    if (gameId >= 0 && (phase == OVER || phase == PAUSE)) openLobby(gameId, mode);
    return;
  }
  if (!strcmp(t, "g.quit")) {
    if (phase == PLAY || phase == COUNT || phase == PAUSE) {
      if (me < 0) return;
    }
    goIdle();
    return;
  }
}

void recordsJson(JsonObject out) {
  out["rec"] = rec.as<JsonObjectConst>();
  out["ph"] = PHASE_NAMES[phase];
  if (gameId >= 0) out["g"] = GAMES[gameId].key;
  JsonArray gl = out["games"].to<JsonArray>();
  for (const auto& g : GAMES) {
    JsonObject o = gl.add<JsonObject>();
    o["key"] = g.key;
    o["title"] = g.title;
    JsonArray ms = o["modes"].to<JsonArray>();
    for (int i = 0; i < 3; i++)
      if (g.modes & (1 << i)) ms.add(MODE_NAMES[i]);
  }
}

}  // namespace games
