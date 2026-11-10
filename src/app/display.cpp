#include "display.h"
#include <FastLED.h>
#include "hardware.h"

namespace {
CRGB leds[LED_COUNT];
SysState current = SysState::Booting;
uint32_t stateSince = 0;
uint8_t progress = 0;
bool testRequested = false;

void setMinuteDots(const CRGB& c) {
  for (int i = 0; i < LED_MINUTE_COUNT; i++) leds[LED_MATRIX_COUNT + i] = c;
}
}  // namespace

namespace display {

void begin() {
  FastLED.addLeds<WS2812B, PIN_LED_DATA, GRB>(leds, LED_COUNT);
  FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTAGE_V, LED_MAX_MILLIAMPS);
  FastLED.setBrightness(80);
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
  loop();  // waehrend des Updates blockiert loop(), daher direkt zeichnen
}

void setBrightness(uint8_t b) { FastLED.setBrightness(b); }

void testPattern() { testRequested = true; }

void loop() {
  static uint32_t lastFrame = 0;
  uint32_t now = millis();
  if (now - lastFrame < 20 && current != SysState::Updating) return;
  lastFrame = now;

  if (testRequested) {
    testRequested = false;
    for (int i = 0; i < LED_COUNT; i++) {
      FastLED.clear();
      leds[i] = CRGB::White;
      FastLED.show();
      delay(15);
    }
  }

  FastLED.clear();
  uint32_t t = now - stateSince;
  switch (current) {
    case SysState::Booting:
      setMinuteDots(CRGB(40, 40, 40));
      break;
    case SysState::Connecting:
      setMinuteDots((t / 300) % 2 ? CRGB(80, 60, 0) : CRGB::Black);
      break;
    case SysState::Portal:
      setMinuteDots(CRGB(0, 0, 40 + beatsin8(15, 0, 180)));
      break;
    case SysState::Online:
      if (t < 2000) setMinuteDots(CRGB(0, 120, 0));
      else if ((now / 100) % 50 == 0) leds[LED_MATRIX_COUNT] = CRGB(0, 8, 0);  // Herzschlag
      break;
    case SysState::Updating: {
      int n = (int)LED_MATRIX_COUNT * progress / 100;
      for (int i = 0; i < n; i++) leds[i] = CRGB(30, 0, 40);
      setMinuteDots(CRGB(80, 0, 100));
      break;
    }
    case SysState::Error:
      setMinuteDots(CRGB(120, 0, 0));
      break;
  }
  FastLED.show();
}

}  // namespace display
