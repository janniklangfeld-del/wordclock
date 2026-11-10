#pragma once
#include <Arduino.h>

// Dauerhafte Einstellungen (NVS / Preferences). WLAN-Zugangsdaten verwaltet
// der ESP32 selbst (WiFiManager), sie stehen NIE im Quellcode.
struct Settings {
  String adminPassword = "wordclock";  // schuetzt Web-Upload und lokales OTA
  bool autoUpdate = true;              // Internet-OTA von GitHub Releases
  uint8_t brightness = 80;

  void load();
  void save() const;
};

extern Settings settings;
