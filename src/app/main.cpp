// Wordclock fuer Maja – Hauptfirmware
//
// Stand: Grundgeruest (WLAN-Einrichtung, Weboberflaeche, OTA ueber GitHub).
// Die eigentliche Uhrzeit-Anzeige folgt, sobald die LED-Reihenfolge am realen
// Aufbau vermessen ist.

#include <Arduino.h>
#include "display.h"
#include "net.h"
#include "ota.h"
#include "settings.h"
#include "web.h"

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("\n=== Wordclock %s (Repo: %s) ===\n", FW_VERSION, GITHUB_REPO);

  settings.load();
  display::begin();
  display::setBrightness(settings.brightness);
  display::setState(SysState::Booting);
  display::loop();

  net::begin();
  ota::begin();
}

void loop() {
  net::loop();
  web::loop();
  ota::loop();
  display::loop();
  delay(2);
}
