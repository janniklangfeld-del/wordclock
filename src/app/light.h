#pragma once

// Optionaler Helligkeitssensor BH1750 (I2C, siehe hardware.h).
// Wird beim Start erkannt; ohne Sensor liefert found() false.
namespace light {
void begin();
void loop();
bool found();
float lux();  // geglaettet, < 0 = noch kein Messwert
}  // namespace light
