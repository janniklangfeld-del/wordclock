#pragma once
#include <Arduino.h>

// Drei Update-Wege:
//  1. Internet-OTA: die Uhr fragt regelmaessig GitHub Releases ab
//     (https://github.com/<GITHUB_REPO>/releases/latest) und installiert
//     firmware.bin, wenn die Version neuer ist. Funktioniert hinter jedem
//     Heim-Router, ohne Portfreigabe – also auch in Majas WLAN.
//  2. Lokales OTA (PlatformIO env:wordclock_ota / ArduinoOTA), gleiches Netz.
//  3. Datei-Upload ueber die Weboberflaeche (http://wordclock.local/update).
namespace ota {
void begin();
void loop();
void requestCheck();          // naechster loop() prueft sofort
String lastResult();          // Text fuer die Weboberflaeche
String latestKnownVersion();
}  // namespace ota

#ifndef FW_VERSION
#define FW_VERSION "0.0.0-dev"
#endif
#ifndef GITHUB_REPO
#define GITHUB_REPO ""
#endif
