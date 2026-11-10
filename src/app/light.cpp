#include "light.h"
#include <Arduino.h>
#include <Wire.h>
#include "hardware.h"

namespace {
constexpr uint8_t BH1750_ADDR = 0x23;
bool present = false;
float luxValue = -1;
uint32_t lastRead = 0;

bool command(uint8_t c) {
  Wire.beginTransmission(BH1750_ADDR);
  Wire.write(c);
  return Wire.endTransmission() == 0;
}
}  // namespace

namespace light {

void begin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setTimeOut(20);
  present = command(0x01) && command(0x10);  // Power on, dauerhaft 1 lx Aufloesung
  Serial.printf("[light] BH1750 %s\n", present ? "gefunden" : "nicht vorhanden");
}

void loop() {
  if (!present || millis() - lastRead < 500) return;
  lastRead = millis();
  if (Wire.requestFrom((int)BH1750_ADDR, 2) != 2) return;
  uint16_t raw = (Wire.read() << 8) | Wire.read();
  float l = raw / 1.2f;
  luxValue = luxValue < 0 ? l : luxValue * 0.8f + l * 0.2f;
}

bool found() { return present; }
float lux() { return luxValue; }

}  // namespace light
