#include "alexa.h"
#include <Espalexa.h>
#include "app.h"
#include "settings.h"

namespace {
Espalexa espalexa;
EspalexaDevice* device = nullptr;
bool started = false;

void onAlexa(EspalexaDevice* d) {
  if (!d) return;
  switch (d->getLastChangedProperty()) {
    case EspalexaDeviceProperty::off:
      settings.power = false;
      break;
    case EspalexaDeviceProperty::on:
      settings.power = true;
      break;
    case EspalexaDeviceProperty::bri:
      settings.power = true;
      settings.brightness = max<uint8_t>(1, d->getValue());
      break;
    case EspalexaDeviceProperty::hs:
    case EspalexaDeviceProperty::xy:
    case EspalexaDeviceProperty::ct:
      settings.power = true;
      settings.color = d->getRGB() & 0xFFFFFF;
      settings.colorMode = CM_SOLID;
      if (settings.mode == MODE_EFFECT) settings.mode = MODE_CLOCK;
      break;
    default:
      return;
  }
  Serial.printf("[alexa] an=%d hell=%u farbe=%06lX\n", settings.power, settings.brightness,
                (unsigned long)settings.color);
  app::settingsChanged(CH_DISPLAY, app::SRC_ALEXA);
}
}  // namespace

namespace alexa {

void begin(WebServer* server) {
  if (started || !settings.alexa) return;
  device = new EspalexaDevice(settings.alexaName, onAlexa, EspalexaDeviceType::color, settings.brightness);
  espalexa.addDevice(device);
  started = espalexa.begin(server);
  sync();
  Serial.printf("[alexa] Geraet '%s' %s\n", settings.alexaName.c_str(), started ? "aktiv" : "Start fehlgeschlagen");
}

void loop() {
  if (started) espalexa.loop();
}

void sync() {
  if (!device) return;
  device->setValue(settings.brightness);
  device->setColor((settings.color >> 16) & 0xFF, (settings.color >> 8) & 0xFF, settings.color & 0xFF);
  if (!settings.power) device->setState(false);
}

bool running() { return started; }

bool handleApi(const String& uri, const String& body) {
  return started && espalexa.handleAlexaApiCall(uri, body);
}

}  // namespace alexa
