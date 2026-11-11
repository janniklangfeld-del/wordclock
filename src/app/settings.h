#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

// Alle Einstellungen der Uhr. Gespeichert als JSON im NVS (Preferences), ueber
// die Weboberflaeche als JSON lesbar/aenderbar (Schluessel siehe toJson()).
// WLAN-Zugangsdaten verwaltet der ESP32 selbst, sie stehen nie hier.

enum : uint8_t { MODE_CLOCK, MODE_EFFECT, MODE_CUSTOM };
enum : uint8_t { CM_SOLID, CM_RAINBOW, CM_GRADIENT, CM_CYCLE, CM_WORDS };
enum : uint8_t { BG_OFF, BG_COLOR, BG_EFFECT };
enum : uint8_t { FX_RAINBOW, FX_PLASMA, FX_SPARKLE, FX_MATRIX, FX_FIRE, FX_BREATHE, FX_HEARTS, FX_CONFETTI };
enum : uint8_t { TR_NONE, TR_FADE, TR_WIPE, TR_DIAG, TR_RANDOM, TR_MATRIX, TR_TYPE };

// Welche Bereiche sich bei applyJson() geaendert haben
enum : uint32_t { CH_DISPLAY = 1, CH_TIME = 2, CH_ALEXA = 4, CH_SYSTEM = 8 };

struct Settings {
  // --- Anzeige ---------------------------------------------------------------
  String name = "Majas Wordclock";  // Titel der Weboberflaeche
  bool power = true;
  uint8_t brightness = 90;          // 1..255
  uint8_t mode = MODE_CLOCK;
  uint32_t color = 0xFFB46E;        // warmweiss
  uint32_t color2 = 0xFF4FA0;
  uint8_t colorMode = CM_SOLID;
  uint8_t gradDir = 0;              // 0 horizontal, 1 vertikal, 2 diagonal
  uint8_t speed = 40;               // 1..100
  uint8_t bg = BG_OFF;
  uint32_t bgColor = 0x281400;
  uint8_t bgLevel = 15;             // 0..100 %
  uint8_t effect = FX_RAINBOW;
  uint8_t transition = TR_FADE;
  uint16_t transMs = 1200;
  bool dots = true;
  bool dotsOwnColor = false;
  uint32_t dotColor = 0xFF4FA0;
  String custom;                    // 110 x '0'/'1' (Modus "Eigener Text")
  bool customScroll = false;        // Eigener Text als Laufschrift statt Buchstaben
  String scrollText = "MAJA <3";
  uint8_t scrollSpeed = 50;         // 1..100

  // --- Sprache ---------------------------------------------------------------
  uint8_t prefix = 0;               // "Es ist": 0 immer, 1 nur volle/halbe, 2 nie
  uint8_t quarter = 0;              // 0 "Viertel nach drei", 1 "Viertel vier"
  uint8_t threeQuarter = 0;         // 0 "Viertel vor vier", 1 "Dreiviertel vier"
  uint8_t twenty = 0;               // 0 "zwanzig nach", 1 "zehn vor halb"

  // --- Zeitplan / Licht -------------------------------------------------------
  bool nightOn = false;
  uint16_t nightFrom = 22 * 60 + 30;  // Minuten seit Mitternacht
  uint16_t nightTo = 6 * 60 + 30;
  uint8_t nightBri = 12;
  bool nightOff = false;
  bool autoOn = false;               // Helligkeitssensor (BH1750)
  uint8_t autoMin = 8, autoMax = 200;
  uint16_t autoLux = 250;            // ab dieser Helligkeit volle autoMax
  bool bdayOn = false;               // Herzchen an Majas Geburtstag (Datum noch offen)
  uint8_t bdayDay = 1, bdayMonth = 1;
  bool introOn = true;               // Begruessung beim Einschalten (Laufschrift)
  String introText = "HAPPY BIRTHDAY MAJA";

  // --- System ----------------------------------------------------------------
  String tz = "CET-1CEST,M3.5.0,M10.5.0/3";
  String ntp = "de.pool.ntp.org";
  bool alexa = true;
  String alexaName = "Wordclock";
  bool autoUpdate = true;
  String adminPassword = "wordclock";  // nie per JSON ausgegeben

  void load();
  void save() const;
  void requestSave();  // speichert verzoegert (schont den Flash bei Schiebereglern)
  void loop();
  void toJson(JsonObject o) const;
  uint32_t applyJson(JsonObjectConst o);
};

extern Settings settings;
