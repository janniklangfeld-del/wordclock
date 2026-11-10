#pragma once
#include <Arduino.h>

// Zentrale Stelle, an der Einstellungsaenderungen (Web, Alexa) wirksam werden.
namespace app {
enum Source : uint8_t { SRC_WEB, SRC_ALEXA };
void settingsChanged(uint32_t flags, Source src);
}  // namespace app
