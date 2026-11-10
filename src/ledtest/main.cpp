// LED-Streifen-Test fuer die Wordclock-Hardware.
//
// Ablauf (Endlosschleife, Fortschritt im Seriellen Monitor, 115200 Baud):
//   1. Farbreihenfolge: erste LED 1 s ROT, 1 s GRUEN, 1 s BLAU
//      -> zeigt sie andere Farben, stimmt COLOR_ORDER nicht.
//   2. Lauflicht: ein weisser Punkt laeuft ueber alle LEDs
//      -> bleibt er irgendwo haengen, ist dort eine Loetstelle/Trennung defekt.
//   3. Alle LEDs gedimmt weiss (Strom-/Spannungsabfall-Test)
//      -> Ende des Streifens rot-/gelbstichig = Einspeisung zu schwach.
//   4. Regenbogen fuer 10 s.
//
// Serielle Befehle: '+' / '-' Helligkeit, 'n' naechste Phase, 'p' Pause.

#include <Arduino.h>
#include <FastLED.h>
#include "hardware.h"

// Genug Puffer fuer drei 1-m-Streifen; mehr LEDs ansteuern als vorhanden schadet nicht.
constexpr int TEST_LEDS = 300;
constexpr int TEST_MAX_MILLIAMPS = 2000;  // fuer den Test bewusst niedrig
#define COLOR_ORDER GRB

CRGB leds[TEST_LEDS];
uint8_t brightness = 40;
bool paused = false;
volatile bool skip = false;

void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '+') brightness = min(255, brightness + 15);
    if (c == '-') brightness = max(5, brightness - 15);
    if (c == 'n') skip = true;
    if (c == 'p') { paused = !paused; Serial.println(paused ? "Pause" : "Weiter"); }
    if (c == '+' || c == '-') {
      FastLED.setBrightness(brightness);
      Serial.printf("Helligkeit %u/255\n", brightness);
    }
  }
}

// Wartet ms Millisekunden, bleibt aber reaktiv. Liefert false bei Phasen-Sprung.
bool waitMs(uint32_t ms) {
  uint32_t t0 = millis();
  while (millis() - t0 < ms || paused) {
    handleSerial();
    if (skip) { skip = false; return false; }
    FastLED.show();
    delay(5);
  }
  return true;
}

void phaseColorOrder() {
  Serial.println("[1] Farbreihenfolge: LED 0 -> ROT, GRUEN, BLAU");
  const CRGB colors[] = {CRGB::Red, CRGB::Green, CRGB::Blue};
  for (auto c : colors) {
    FastLED.clear();
    leds[0] = c;
    if (!waitMs(1000)) return;
  }
}

void phaseChase() {
  Serial.println("[2] Lauflicht ueber alle LEDs (Index wird alle 10 LEDs ausgegeben)");
  for (int i = 0; i < TEST_LEDS; i++) {
    FastLED.clear();
    leds[i] = CRGB::White;
    if (i % 10 == 0) Serial.printf("  LED %d\n", i);
    if (!waitMs(40)) return;
  }
}

void phaseWhite() {
  Serial.println("[3] Alle LEDs weiss (gedimmt, Strom begrenzt)");
  fill_solid(leds, TEST_LEDS, CRGB::White);
  waitMs(4000);
}

void phaseRainbow() {
  Serial.println("[4] Regenbogen");
  uint32_t t0 = millis();
  uint8_t hue = 0;
  while (millis() - t0 < 10000) {
    fill_rainbow(leds, TEST_LEDS, hue++, 3);
    if (!waitMs(20)) return;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== Wordclock LED-Test ===");
  Serial.printf("Daten-Pin GPIO%d, Puffer %d LEDs, Limit %d mA\n", PIN_LED_DATA, TEST_LEDS, TEST_MAX_MILLIAMPS);
  Serial.println("Befehle: + / - Helligkeit, n naechste Phase, p Pause");

  FastLED.addLeds<WS2812B, PIN_LED_DATA, COLOR_ORDER>(leds, TEST_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTAGE_V, TEST_MAX_MILLIAMPS);
  FastLED.setBrightness(brightness);
  FastLED.clear(true);
}

void loop() {
  phaseColorOrder();
  phaseChase();
  phaseWhite();
  phaseRainbow();
}
