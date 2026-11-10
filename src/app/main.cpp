// Majas Wordclock – Hauptfirmware
//
// Deutsche Wortuhr (11 x 10 Buchstaben, 4 Minutenpunkte) mit Weboberflaeche,
// Live-3D-Ansicht, Effekten, Alexa (Hue-Emulation), optionalem Helligkeitssensor
// und Updates ueber GitHub Releases. Ueberblick: docs/SOFTWARE.md

#include <Arduino.h>
#include "alexa.h"
#include "app.h"
#include "display.h"
#include "light.h"
#include "net.h"
#include "ota.h"
#include "settings.h"
#include "web.h"

namespace app {
void settingsChanged(uint32_t flags, Source src) {
  if (!flags) return;
  settings.requestSave();
  if (flags & CH_TIME) net::applyTimeConfig();
  if (flags & CH_DISPLAY) {
    display::settingsChanged();
    if (src != SRC_ALEXA) alexa::sync();
  }
  web::notifySettings();
}
}  // namespace app

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("\n=== Majas Wordclock %s (Repo: %s) ===\n", FW_VERSION, GITHUB_REPO);

  settings.load();
  display::begin();
  display::setState(SysState::Booting);
  display::loop();
  light::begin();

  net::begin();
  ota::begin();
}

void loop() {
  net::loop();
  web::loop();
  alexa::loop();
  ota::loop();
  light::loop();
  settings.loop();
  display::loop();
  delay(1);
}
