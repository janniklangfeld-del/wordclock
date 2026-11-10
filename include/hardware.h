#pragma once
// Hardware-Konfiguration – gemeinsam fuer ledtest und wordclock.
// Passt zu docs/hardware/verdrahtungsplan_v1.jpg

// --- LED-Daten ---------------------------------------------------------------
// ESP32 GPIO18 -> 74AHCT125 Pin 2 (1A) -> Pin 3 (1Y) -> 330 Ohm -> DIN Streifen
constexpr int PIN_LED_DATA = 18;

// --- LED-Layout ----------------------------------------------------------------
// 10 Reihen a 22 LEDs (2 LEDs pro Buchstabe, 11 Spalten), schlangenfoermig,
// danach 4 Minutenpunkte. Siehe docs/PROJEKTSTAND_2026-11-09.md
constexpr int LED_ROWS          = 10;
constexpr int LEDS_PER_ROW      = 22;
constexpr int LED_MINUTE_COUNT  = 4;
constexpr int LED_MATRIX_COUNT  = LED_ROWS * LEDS_PER_ROW;          // 220
constexpr int LED_COUNT         = LED_MATRIX_COUNT + LED_MINUTE_COUNT; // 224

// --- Strombegrenzung -------------------------------------------------------------
// Netzteil 5 V / 20 A. Leitungen/Einspeisung sind noch nicht final ausgelegt,
// daher konservativ: Software begrenzt den LED-Strom (FastLED schaetzt ihn).
constexpr int LED_VOLTAGE_V     = 5;
constexpr int LED_MAX_MILLIAMPS = 7000;

// --- Bedienung --------------------------------------------------------------------
// BOOT-Taste des ELEGOO-Devboards (GPIO0, gegen GND, interner Pull-up).
// 5 s halten -> WLAN-Einrichtungsportal oeffnen.
constexpr int PIN_BUTTON = 0;
