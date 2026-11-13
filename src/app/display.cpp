#include "display.h"
#include <time.h>
#include "clockface.h"
#include "games.h"
#include "hardware.h"
#include "intro.h"
#include "light.h"
#include "settings.h"

namespace {
constexpr int W = LEDS_PER_ROW;  // 22 Pixel breit (2 LEDs pro Buchstabe)
constexpr int H = LED_ROWS;      // 10 Reihen
constexpr int CELLS = LETTER_COLS * LETTER_ROWS;
constexpr uint32_t FRAME_MS = 20;  // 50 Bilder/s

CRGB leds[LED_COUNT];                       // geht an FastLED
CRGB frameA[LED_COUNT], frameB[LED_COUNT];  // Uebergang: alt / neu
CRGB fx[LED_COUNT];                         // Effektpuffer (Effektmodus + Hintergrund)
uint8_t ledX[LED_COUNT], ledY[LED_COUNT];   // Pixelkoordinaten, Punkte: y = H
int16_t ledCell[LED_COUNT];                 // Buchstabe 0..109, Punkte 110..113
int16_t pixelLed[H][W];

SysState current = SysState::Booting;
uint32_t stateSince = 0;
uint8_t progress = 0;
bool testRequested = false;

struct TestState {
  uint8_t mode = 0;  // display::TestMode
  CRGB color = CRGB::White;
  uint16_t index = 0;
  uint8_t bri = 60;
  uint8_t speed = 50;
  uint32_t start = 0, lastCmd = 0;
} test;
constexpr uint32_t TEST_TIMEOUT_MS = 10UL * 60 * 1000;

// Begruessung beim Einschalten
constexpr int INTRO_MAX_COLS = 24 * 6 * 2;
constexpr uint32_t INTRO_COL_MS = 85;      // Laufschrift: ms pro Spalte
constexpr uint32_t INTRO_HEART_MS = 1600;  // Herz pulsiert
constexpr uint32_t INTRO_SPARK_MS = 1300;  // Funkenregen, danach baut sich die Uhr auf
struct IntroState {
  bool active = false;
  uint32_t start = 0;
  int cols = 0;
  uint8_t col[INTRO_MAX_COLS];
} introSt;
// Eigener Text als Laufschrift
constexpr int SCROLL_MAX_COLS = 60 * 6 * 2;
uint8_t scrollCols[SCROLL_MAX_COLS];
int scrollLen = 0;
uint32_t scrollStart = 0;

uint8_t forceTrans = 255;  // naechster Uhr-Aufbau mit diesem Uebergang (255 = Einstellung)
bool lastPower = true;

clockface::Face face, prevFace;
uint8_t wordOf[CELLS], prevWordOf[CELLS];
bool faceValid = false;
bool transActive = false;
uint8_t transType = TR_FADE;
uint16_t transDur = 1000;
uint32_t transStart = 0, transSeed = 1;
// Uebergangs-Uhr: zaehlt pro Bild hoechstens 40 ms weiter. Haengt die Hauptschleife kurz
// (z. B. Handy verbindet sich bei schwachem WLAN), pausiert der Uebergang statt zu springen.
uint32_t transElapsed = 0, transLastFrame = 0;
uint32_t introDoneAt = 0;  // Ende der Begruessung: danach bis die Uhrzeit da ist dunkel bleiben
uint8_t typeRank[CELLS];
uint8_t typeCount = 0;
LetterMask customMask;

float briNow = 0;
uint8_t briOut = 0;

uint8_t heat[W][H];
float dropPos[W], dropSpeed[W];
uint8_t confettiHue = 0;

const char* const HEART[H] = {
    "..XX...XX..", ".XXXX.XXXX.", "XXXXXXXXXXX", "XXXXXXXXXXX", ".XXXXXXXXX.",
    "..XXXXXXX..", "...XXXXX...", "....XXX....", ".....X.....", "...........",
};

uint8_t hash8(uint32_t a) {
  a ^= a >> 16;
  a *= 0x7feb352dUL;
  a ^= a >> 15;
  a *= 0x846ca68bUL;
  a ^= a >> 16;
  return a & 0xFF;
}

bool localNow(tm& t) {
  time_t now = time(nullptr);
  if (now < 1700000000) return false;  // noch keine NTP-Zeit
  localtime_r(&now, &t);
  return true;
}

// ---------------------------------------------------------------------------
// Effekte (Vollbild): schreiben in fx[]
// ---------------------------------------------------------------------------
void renderEffect(uint8_t id, uint32_t ms) {
  const float sp = settings.speed / 40.0f;
  const uint32_t t = (uint32_t)(ms * sp);
  switch (id) {
    case FX_RAINBOW:
      for (int i = 0; i < LED_COUNT; i++) fx[i] = CHSV(t / 12 - ledX[i] * 7 - ledY[i] * 5, 235, 255);
      break;
    case FX_PLASMA:
      for (int i = 0; i < LED_COUNT; i++) {
        uint8_t x = ledX[i] * 12, y = ledY[i] * 18;
        uint16_t v = sin8(x + t / 9) + sin8(y + t / 13) + sin8(x + y + t / 7);
        fx[i] = CHSV(v / 3 + t / 40, 255, 255);
      }
      break;
    case FX_SPARKLE:
      fadeToBlackBy(fx, LED_COUNT, 10 + settings.speed / 5);
      for (int n = 0; n < 1 + settings.speed / 35; n++)
        if (random8() < 110) fx[random16(LED_COUNT)] = CHSV(random8(), 110, 255);
      break;
    case FX_MATRIX:
      fadeToBlackBy(fx, LED_COUNT, 24);
      for (int x = 0; x < W; x++) {
        dropPos[x] += dropSpeed[x] * sp;
        if (dropPos[x] > H + 4) {
          dropPos[x] = -(float)random8(12);
          dropSpeed[x] = 0.10f + random8(100) / 350.0f;
        }
        int y = (int)dropPos[x];
        if (y - 1 >= 0 && y - 1 < H) fx[pixelLed[y - 1][x]] = CRGB(0, 210, 50);
        if (y >= 0 && y < H) fx[pixelLed[y][x]] = CRGB(170, 255, 170);
      }
      break;
    case FX_FIRE: {
      static uint32_t lastStep = 0;
      if (ms - lastStep > (uint32_t)(70 / max(sp, 0.2f))) {
        lastStep = ms;
        for (int x = 0; x < W; x++) {
          for (int y = 0; y < H; y++) heat[x][y] = qsub8(heat[x][y], random8(0, 70));
          for (int y = H - 1; y >= 2; y--) heat[x][y] = (heat[x][y - 1] + 2 * heat[x][y - 2]) / 3;
          if (random8() < 150) {
            int y = random8(2);
            heat[x][y] = qadd8(heat[x][y], random8(150, 255));
          }
        }
      }
      for (int x = 0; x < W; x++)
        for (int y = 0; y < H; y++) fx[pixelLed[H - 1 - y][x]] = HeatColor(heat[x][y]);
      for (int k = 0; k < LED_MINUTE_COUNT; k++) fx[clockface::dotLed(k)] = HeatColor(heat[9 + k][0]);
      break;
    }
    case FX_BREATHE: {
      CRGB c(settings.color);
      c.nscale8_video(beatsin8(4 + settings.speed / 8, 25, 255));
      fill_solid(fx, LED_COUNT, c);
      break;
    }
    case FX_HEARTS: {
      uint8_t pulse = beatsin8(40 + settings.speed / 2, 110, 255);
      for (int i = 0; i < LED_COUNT; i++) {
        bool in = ledY[i] < H && HEART[ledY[i]][ledX[i] / 2] == 'X';
        if (ledY[i] >= H) in = true;  // Minutenpunkte leuchten mit
        CRGB c = in ? CRGB(255, 20, 90) : CRGB(18, 0, 8);
        if (in) c.nscale8_video(pulse);
        fx[i] = c;
      }
      break;
    }
    case FX_CONFETTI:
    default:
      fadeToBlackBy(fx, LED_COUNT, 8);
      if (random8() < 80 + settings.speed) fx[random16(LED_COUNT)] = CHSV(confettiHue + random8(64), 200, 255);
      confettiHue++;
      break;
  }
}

// ---------------------------------------------------------------------------
// Uhrzeit-Darstellung
// ---------------------------------------------------------------------------
CRGB textColor(int i, uint8_t word, uint32_t ms) {
  const uint32_t t = (uint32_t)(ms * (settings.speed / 40.0f));
  const uint8_t y = min<uint8_t>(ledY[i], H - 1);
  switch (settings.colorMode) {
    case CM_RAINBOW: return CHSV(t / 14 + ledX[i] * 6 + y * 9, 230, 255);
    case CM_GRADIENT: {
      uint8_t f;
      if (settings.gradDir == 0) f = ledX[i] * 255 / (W - 1);
      else if (settings.gradDir == 1) f = y * 255 / (H - 1);
      else f = (ledX[i] + 2 * y) * 255 / (W - 1 + 2 * (H - 1));
      return blend(CRGB(settings.color), CRGB(settings.color2), f);
    }
    case CM_CYCLE: return CHSV(t / 40, 240, 255);
    case CM_WORDS: return CHSV(t / 60 + word * 71, 220, 255);
    default: return CRGB(settings.color);
  }
}

CRGB background(int i) {
  CRGB c;
  if (settings.bg == BG_COLOR) c = CRGB(settings.bgColor);
  else if (settings.bg == BG_EFFECT) c = fx[i];
  else return CRGB::Black;
  c.nscale8_video(settings.bgLevel * 255 / 100);
  return c;
}

void renderFace(const clockface::Face& f, const uint8_t* words, CRGB* out, uint32_t ms) {
  for (int i = 0; i < LED_COUNT; i++) {
    int cell = ledCell[i];
    bool on;
    uint8_t w = 0;
    if (cell < CELLS) {
      on = f.mask[cell];
      w = words[cell];
    } else {
      on = settings.dots && (cell - CELLS) < f.dots;
    }
    if (!on) out[i] = background(i);
    else if (cell >= CELLS && settings.dotsOwnColor) out[i] = CRGB(settings.dotColor);
    else out[i] = textColor(i, w, ms);
  }
}

bool currentFace(clockface::Face& f, uint8_t* words) {
  memset(words, 0, CELLS);
  if (settings.mode == MODE_CUSTOM) {
    f = clockface::Face();
    f.mask = customMask;
    return true;
  }
  tm t;
  if (!localNow(t)) return false;
  f = clockface::compute(t.tm_hour, t.tm_min, settings);
  for (int n = 0; n < f.wordCount; n++)
    for (int l = 0; l < f.words[n].len; l++) words[f.words[n].row * LETTER_COLS + f.words[n].col + l] = n;
  return true;
}

void beginTransition(bool dotsOnly) {
  transType = dotsOnly ? TR_FADE : settings.transition;
  transDur = dotsOnly ? 500 : settings.transMs;
  if (!dotsOnly && forceTrans != 255) {
    transType = forceTrans;
    transDur = max<uint16_t>(settings.transMs, 1800);
    forceTrans = 255;
  }
  transStart = millis();
  transElapsed = 0;
  transLastFrame = transStart;
  transSeed = esp_random();
  transActive = transType != TR_NONE;
  typeCount = 0;
  for (int c = 0; c < CELLS; c++)
    typeRank[c] = (face.mask[c] && !prevFace.mask[c]) ? typeCount++ : 255;
}

// 0 = noch altes Bild, 1 = schon neues Bild (pro Buchstabe / Punkt)
float cellWeight(int cell, float p) {
  int col, row;
  if (cell < CELLS) { col = cell % LETTER_COLS; row = cell / LETTER_COLS; }
  else { col = 4 + (cell - CELLS); row = H; }
  float d;
  switch (transType) {
    case TR_FADE: return p;
    case TR_WIPE: d = col / 10.0f; break;
    case TR_DIAG: d = (col + row) / 20.0f; break;
    case TR_RANDOM: d = hash8(transSeed + cell * 2654435761UL) / 255.0f; break;
    case TR_MATRIX: d = row / 10.0f * 0.7f + hash8(transSeed + col * 7919) / 255.0f * 0.3f; break;
    case TR_TYPE:
      d = (cell < CELLS && typeRank[cell] != 255 && typeCount > 1) ? (float)typeRank[cell] / (typeCount - 1) : 0;
      break;
    default: return 1;
  }
  return constrain((p - d * 0.7f) / 0.3f, 0.0f, 1.0f);
}

bool inNight(const tm& t) {
  uint16_t m = t.tm_hour * 60 + t.tm_min;
  uint16_t a = settings.nightFrom, b = settings.nightTo;
  return a <= b ? (m >= a && m < b) : (m >= a || m < b);
}

uint8_t targetBrightness(bool haveTime, const tm& t) {
  if (!settings.power) return 0;
  uint8_t b = settings.brightness;
  if (settings.autoOn && light::found() && light::lux() >= 0) {
    float f = log1pf(light::lux()) / log1pf((float)settings.autoLux);
    f = constrain(f, 0.0f, 1.0f);
    b = settings.autoMin + (settings.autoMax - settings.autoMin) * f;
  }
  if (haveTime && settings.nightOn && inNight(t)) b = settings.nightOff ? 0 : settings.nightBri;
  return b;
}

// Statusanzeige auf den Minutenpunkten, solange keine Uhrzeit bekannt ist
void renderStatus(uint32_t now) {
  uint32_t t = now - stateSince;
  CRGB c = CRGB::Black;
  // Direkt nach der Begruessung keine Statuspunkte: dunkel bleiben, bis die Uhrzeit da ist,
  // dann blendet sich die Uhr weich ein. Portal (blau) und Fehler (rot) bleiben sichtbar.
  const bool quiet = introDoneAt && now - introDoneAt < 60000 && current != SysState::Portal && current != SysState::Error;
  switch (quiet ? SysState::Updating : current) {  // Updating = hier "nichts zeigen"
    case SysState::Booting: c = CRGB(40, 40, 40); break;
    case SysState::Connecting: c = (t / 300) % 2 ? CRGB(80, 60, 0) : CRGB::Black; break;
    case SysState::Portal: c = CRGB(0, 0, 40 + beatsin8(15, 0, 180)); break;
    case SysState::Online: c = CRGB(0, 60 + beatsin8(30, 0, 60), 0); break;  // wartet auf NTP
    case SysState::Error: c = CRGB(120, 0, 0); break;
    default: break;
  }
  fill_solid(leds, LED_COUNT, CRGB::Black);
  for (int k = 0; k < LED_MINUTE_COUNT; k++) leds[clockface::dotLed(k)] = c;
}

// Eigener Text als Endlos-Laufschrift (Farben wie die Uhrzeit, Hintergrund wie eingestellt)
void renderScroll(uint32_t now) {
  const uint32_t step = 40 + (uint32_t)(100 - constrain(settings.scrollSpeed, 1, 100)) * 3;  // 40..337 ms
  const int period = scrollLen + LETTER_COLS;
  const int offset = period > 0 ? ((now - scrollStart) / step) % period : 0;
  for (int i = 0; i < LED_COUNT; i++) {
    const int y = ledY[i];
    const int tc = ledX[i] / 2 + offset - LETTER_COLS;
    const bool on = y >= 1 && y <= 7 && tc >= 0 && tc < scrollLen && (scrollCols[tc] >> (y - 1) & 1);
    leds[i] = on ? textColor(i, tc / 6, now) : background(i);
  }
}

// Begruessung: 1) Text laeuft von rechts nach links durch, 2) Herz pulsiert,
// 3) Funkenregen. Danach baut sich die Uhr mit "Zufall" aus dem Dunkeln auf.
// Rueckgabe false = fertig.
bool renderIntro(uint32_t now) {
  const uint32_t t = now - introSt.start;
  const uint32_t scrollMs = (uint32_t)(introSt.cols + LETTER_COLS) * INTRO_COL_MS;
  fill_solid(leds, LED_COUNT, CRGB::Black);
  if (t < scrollMs) {
    const int offset = t / INTRO_COL_MS;  // Spalte 0 der Uhr zeigt Textspalte offset - 11
    for (int i = 0; i < LED_COUNT; i++) {
      const int y = ledY[i];
      if (y < 1 || y > 7) continue;  // Schrift 7 Zeilen hoch, Zeilen 2..8
      const int tc = ledX[i] / 2 + offset - LETTER_COLS;
      if (tc < 0 || tc >= introSt.cols || !(introSt.col[tc] >> (y - 1) & 1)) continue;
      leds[i] = CHSV(tc * 6 - t / 12, 200, 255);  // sanfter Regenbogen ueber den Text
    }
    return true;
  }
  if (t < scrollMs + INTRO_HEART_MS) {
    const uint32_t h = t - scrollMs;
    // zweimal pulsieren (Herzschlag), weich ein- und ausblenden
    uint8_t v = sin8(h * 256 / 400 + 192);
    uint8_t env = h < 250 ? h * 255 / 250 : (h > INTRO_HEART_MS - 300 ? (INTRO_HEART_MS - h) * 255 / 300 : 255);
    CRGB c(255, 20, 90);
    c.nscale8_video(scale8(qadd8(v / 2, 128), env));
    for (int i = 0; i < LED_COUNT; i++)
      if (ledY[i] < H && HEART[ledY[i]][ledX[i] / 2] == 'X') leds[i] = c;
    return true;
  }
  if (t < scrollMs + INTRO_HEART_MS + INTRO_SPARK_MS) {
    const uint32_t s = t - scrollMs - INTRO_HEART_MS;
    fadeToBlackBy(fx, LED_COUNT, 40);
    const uint8_t n = 6 - s * 5 / INTRO_SPARK_MS;  // wird zum Ende weniger
    for (uint8_t k = 0; k < n; k++) fx[random16(LED_COUNT)] = CHSV(random8(), 150, 255);
    memcpy(leds, fx, sizeof(leds));
    return true;
  }
  return false;
}

// Testbilder: Schrittdauer aus Geschwindigkeit 1..100 -> ca. 400..8 ms
void renderTest(uint32_t now) {
  fill_solid(leds, LED_COUNT, CRGB::Black);
  const uint32_t t = now - test.start;
  const uint32_t step = 8 + (uint32_t)(100 - constrain(test.speed, 1, 100)) * 4;
  switch (test.mode) {
    case display::TEST_CHASE: {  // eine LED mit kurzem Schweif, Index 0 -> 223
      int pos = (t / step) % LED_COUNT;
      for (int k = 0; k < 6; k++) {
        int i = pos - k;
        if (i < 0) break;
        leds[i] = test.color;
        leds[i].nscale8(255 >> k);
      }
      break;
    }
    case display::TEST_ROWS: {  // Reihe 1..10, dann Minutenpunkte
      int row = (t / (step * 12)) % (H + 1);
      for (int i = 0; i < LED_COUNT; i++)
        if (ledY[i] == row) leds[i] = test.color;
      break;
    }
    case display::TEST_LETTERS: {  // Buchstabe fuer Buchstabe (beide LEDs), dann Punkte
      int cell = (t / (step * 3)) % (CELLS + LED_MINUTE_COUNT);
      for (int i = 0; i < LED_COUNT; i++)
        if (ledCell[i] == cell) leds[i] = test.color;
      break;
    }
    case display::TEST_RGBW: {
      static const CRGB C[4] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::White};
      fill_solid(leds, LED_COUNT, C[(t / 1500) % 4]);
      break;
    }
    case display::TEST_COLOR:
      fill_solid(leds, LED_COUNT, test.color);
      break;
    case display::TEST_LED:
      if (test.index < LED_COUNT) leds[test.index] = test.color;
      break;
    case display::TEST_DOTS: {  // Punkte nacheinander, dann alle
      int k = (t / (step * 6)) % (LED_MINUTE_COUNT + 1);
      for (int d = 0; d < LED_MINUTE_COUNT; d++)
        if (k == LED_MINUTE_COUNT || d == k) leds[clockface::dotLed(d)] = test.color;
      break;
    }
  }
}

}  // namespace

namespace display {

void begin() {
  for (int r = 0; r < H; r++)
    for (int c = 0; c < LETTER_COLS; c++)
      for (int h = 0; h < 2; h++) {
        int i = clockface::letterLed(r, c, h);
        ledX[i] = c * 2 + h;
        ledY[i] = r;
        ledCell[i] = r * LETTER_COLS + c;
        pixelLed[r][c * 2 + h] = i;
      }
  for (int k = 0; k < LED_MINUTE_COUNT; k++) {
    int i = clockface::dotLed(k);
    ledX[i] = 9 + k;  // liegen unter den Spalten 9..12 (Pixel)
    ledY[i] = H;
    ledCell[i] = CELLS + k;
  }
  for (int x = 0; x < W; x++) {
    dropPos[x] = -(float)random8(12);
    dropSpeed[x] = 0.10f + random8(100) / 350.0f;
  }
  settingsChanged();
  lastPower = settings.power;
  if (settings.power && settings.introOn) playIntro();
  FastLED.addLeds<WS2812B, PIN_LED_DATA, GRB>(leds, LED_COUNT);
  FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTAGE_V, LED_MAX_MILLIAMPS);
  FastLED.setBrightness(0);
  FastLED.clear(true);
}

void setState(SysState s) {
  if (s == current) return;
  current = s;
  stateSince = millis();
  progress = 0;
}

SysState state() { return current; }

void setProgress(uint8_t percent) {
  progress = percent;
  loop();  // waehrend des Updates blockiert die Hauptschleife -> direkt zeichnen
}

void testPattern() { testRequested = true; }

void playIntro() {
  introSt.cols = intro::toColumns(settings.introText, introSt.col, INTRO_MAX_COLS);
  introSt.start = millis();
  introSt.active = true;
  fill_solid(fx, LED_COUNT, CRGB::Black);
}

bool introRunning() { return introSt.active; }

void setTest(TestMode mode, CRGB color, uint16_t index, uint8_t bri, uint8_t speed) {
  const uint32_t now = millis();
  if (mode != test.mode) test.start = now;
  test.mode = mode;
  test.color = color;
  test.index = index < LED_COUNT ? index : LED_COUNT - 1;
  test.bri = bri;
  test.speed = speed;
  test.lastCmd = now;
}

bool testActive() { return test.mode != TEST_OFF; }

void replayTransition() {
  if (!faceValid) return;
  prevFace = clockface::Face();  // aus dem Dunkeln neu aufbauen
  memset(prevWordOf, 0, CELLS);
  beginTransition(false);
}

void settingsChanged() {
  if (settings.power && !lastPower && settings.introOn) playIntro();
  if (!settings.power) introSt.active = false;
  lastPower = settings.power;
  {
    static String lastText;
    if (settings.scrollText != lastText) {
      lastText = settings.scrollText;
      scrollLen = intro::toColumns(settings.scrollText, scrollCols, SCROLL_MAX_COLS);
      scrollStart = millis();
    }
  }
  customMask.reset();
  for (size_t i = 0; i < settings.custom.length() && i < (size_t)CELLS; i++)
    if (settings.custom[i] == '1') customMask.set(i);
}

const CRGB* frame() { return leds; }
uint8_t brightness() { return briOut; }

bool timeValid() {
  tm t;
  return localNow(t);
}

void loop() {
  static uint32_t lastFrame = 0;
  const uint32_t now = millis();
  if (now - lastFrame < FRAME_MS && current != SysState::Updating) return;
  lastFrame = now;

  if (testRequested) {
    testRequested = false;
    FastLED.setBrightness(max<uint8_t>(briOut, 40));
    for (int i = 0; i < LED_COUNT; i++) {
      FastLED.clear();
      leds[i] = CRGB::White;
      FastLED.show();
      delay(12);
    }
  }

  if (current == SysState::Updating) {  // Fortschrittsbalken ueber die ganze Matrix
    fill_solid(leds, LED_COUNT, CRGB::Black);
    int n = (int)LED_MATRIX_COUNT * progress / 100;
    for (int i = 0; i < n; i++) leds[i] = CRGB(30, 0, 40);
    for (int k = 0; k < LED_MINUTE_COUNT; k++) leds[clockface::dotLed(k)] = CRGB(80, 0, 100);
    FastLED.setBrightness(80);
    FastLED.show();
    return;
  }

  if (test.mode != TEST_OFF) {
    if (now - test.lastCmd > TEST_TIMEOUT_MS) {
      test.mode = TEST_OFF;
    } else {
      renderTest(now);
      briOut = test.bri;
      FastLED.setBrightness(briOut);
      FastLED.show();
      return;
    }
  }

  // MJ Arcade: das Spiel hat die Anzeige
  if (games::active()) {
    introSt.active = false;
    games::render(leds, now);
    faceValid = false;  // danach baut sich die Uhr neu auf
    briNow = briOut = games::brightness();
    FastLED.setBrightness(briOut);
    FastLED.show();
    return;
  }

  tm t;
  const bool haveTime = localNow(t);
  clockface::Face nf;
  uint8_t nw[CELLS];
  const bool scrolling = settings.mode == MODE_CUSTOM && settings.customScroll;
  const bool showFace = settings.mode != MODE_EFFECT && !scrolling && currentFace(nf, nw);

  if (settings.mode == MODE_EFFECT || settings.bg == BG_EFFECT) renderEffect(settings.effect, now);

  if (showFace) {
    if (!faceValid) {  // erster Aufbau: aus dem Dunkeln einblenden
      prevFace = clockface::Face();
      memset(prevWordOf, 0, CELLS);
      face = nf;
      memcpy(wordOf, nw, CELLS);
      faceValid = true;
      beginTransition(false);
    } else if (nf.mask != face.mask || nf.dots != face.dots) {
      bool dotsOnly = nf.mask == face.mask;
      prevFace = face;
      memcpy(prevWordOf, wordOf, CELLS);
      face = nf;
      memcpy(wordOf, nw, CELLS);
      beginTransition(dotsOnly);
    } else {
      memcpy(wordOf, nw, CELLS);
    }

    if (transActive) {
      transElapsed += min<uint32_t>(now - transLastFrame, 40);
      transLastFrame = now;
    }
    float p = transActive ? transElapsed / (float)transDur : 1.0f;
    if (p >= 1.0f) transActive = false;
    if (!transActive) {
      renderFace(face, wordOf, leds, now);
    } else {
      renderFace(prevFace, prevWordOf, frameA, now);
      renderFace(face, wordOf, frameB, now);
      for (int i = 0; i < LED_COUNT; i++) {
        float w = cellWeight(ledCell[i], p);
        leds[i] = blend(frameA[i], frameB[i], (uint8_t)(w * 255));
        if (transType == TR_MATRIX && w > 0.02f && w < 0.98f)
          nblend(leds[i], CRGB(120, 255, 140), (uint8_t)((1 - fabsf(w - 0.5f) * 2) * 220));
      }
    }
  } else if (scrolling) {
    renderScroll(now);
    faceValid = false;  // beim Zurueckwechseln Uhr neu einblenden
  } else if (settings.mode == MODE_EFFECT) {
    memcpy(leds, fx, sizeof(leds));
  } else {
    renderStatus(now);
  }

  // Begruessung ueberlagert alles; danach baut sich die Uhr neu auf
  if (introSt.active) {
    if (renderIntro(now)) {
      if (current == SysState::Updating) introSt.active = false;
    } else {
      introSt.active = false;
      faceValid = false;
      forceTrans = TR_RANDOM;
      introDoneAt = now;
      fill_solid(leds, LED_COUNT, CRGB::Black);
    }
  }

  // Majas Geburtstag: zu jeder Viertelstunde 20 s Herzchen
  if (haveTime && settings.bdayOn && t.tm_mday == settings.bdayDay && t.tm_mon + 1 == settings.bdayMonth &&
      t.tm_min % 15 == 0 && t.tm_sec < 20) {
    renderEffect(FX_HEARTS, now);
    memcpy(leds, fx, sizeof(leds));
  }

  // Overlays: Einrichtungsportal offen / Fehler (10 s)
  if ((showFace || scrolling || settings.mode == MODE_EFFECT) && !introSt.active) {
    if (current == SysState::Portal)
      for (int k = 0; k < LED_MINUTE_COUNT; k++) leds[clockface::dotLed(k)] = CRGB(0, 0, 40 + beatsin8(15, 0, 180));
    if (current == SysState::Error && now - stateSince < 10000)
      for (int k = 0; k < LED_MINUTE_COUNT; k++) leds[clockface::dotLed(k)] = CRGB(150, 0, 0);
  }

  uint8_t target = showFace || scrolling || settings.mode == MODE_EFFECT || introSt.active ? targetBrightness(haveTime, t) : 80;
  briNow += (target - briNow) * 0.08f;
  if (fabsf(target - briNow) < 0.5f) briNow = target;
  briOut = (uint8_t)lroundf(briNow);
  FastLED.setBrightness(briOut);
  FastLED.show();
}

}  // namespace display
