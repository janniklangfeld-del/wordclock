#include "web.h"
#include <ArduinoJson.h>
#include <Update.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <WiFi.h>
#include "alexa.h"
#include "app.h"
#include "clockface.h"
#include "display.h"
#include "hardware.h"
#include "light.h"
#include "net.h"
#include "ota.h"
#include "settings.h"
#include "web_index.h"

namespace {
WebServer http(80);
WebSocketsServer ws(81);
bool routesReady = false;
bool listening = false;
uint32_t lastFrameSent = 0;
uint8_t frameBuf[2 + LED_COUNT * 3];

void sendJson(int code, const JsonDocument& doc) {
  String s;
  serializeJson(doc, s);
  http.sendHeader("Cache-Control", "no-store");
  http.send(code, "application/json", s);
}

void sendOk() { http.send(200, "application/json", "{\"ok\":true}"); }

// Ohne WWW-Authenticate: die Webseite fragt das Passwort selbst ab.
bool authOk() {
  if (http.authenticate("admin", settings.adminPassword.c_str())) return true;
  http.send(401, "application/json", "{\"error\":\"auth\"}");
  return false;
}

String settingsJson(bool wrapped) {
  JsonDocument d;
  if (wrapped) {
    d["t"] = "settings";
    settings.toJson(d["s"].to<JsonObject>());
  } else {
    settings.toJson(d.to<JsonObject>());
  }
  String s;
  serializeJson(d, s);
  return s;
}

void handleIndex() {
  http.sendHeader("Content-Encoding", "gzip");
  http.sendHeader("Cache-Control", "no-cache");
  http.send_P(200, "text/html; charset=utf-8", (const char*)WEB_INDEX_GZ, WEB_INDEX_GZ_LEN);
}

void handleInfo() {
  JsonDocument d;
  d["fw"] = FW_VERSION;
  d["repo"] = GITHUB_REPO;
  d["ip"] = WiFi.localIP().toString();
  d["ssid"] = WiFi.SSID();
  d["rssi"] = WiFi.RSSI();
  d["mac"] = WiFi.macAddress();
  d["uptime"] = millis() / 1000;
  d["heap"] = ESP.getFreeHeap();
  d["timeValid"] = display::timeValid();
  time_t now = time(nullptr);
  tm t;
  localtime_r(&now, &t);
  char buf[24];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &t);
  d["localTime"] = buf;
  d["sensor"]["found"] = light::found();
  d["sensor"]["lux"] = light::lux();
  d["update"]["result"] = ota::lastResult();
  d["update"]["latest"] = ota::latestKnownVersion();
  d["alexa"] = alexa::running();
  d["brightness"] = display::brightness();
  sendJson(200, d);
}

void handleLayout() {
  JsonDocument d;
  JsonArray rows = d["letters"].to<JsonArray>();
  for (int r = 0; r < LETTER_ROWS; r++) rows.add(clockface::LETTERS[r]);
  JsonArray leds = d["leds"].to<JsonArray>();
  for (int r = 0; r < LETTER_ROWS; r++)
    for (int c = 0; c < LETTER_COLS; c++) {
      JsonArray p = leds.add<JsonArray>();
      p.add(clockface::letterLed(r, c, 0));
      p.add(clockface::letterLed(r, c, 1));
    }
  JsonArray dots = d["dots"].to<JsonArray>();
  for (int k = 0; k < LED_MINUTE_COUNT; k++) dots.add(clockface::dotLed(k));
  d["count"] = LED_COUNT;
  sendJson(200, d);
}

void handleSettingsPost() {
  JsonDocument d;
  if (deserializeJson(d, http.arg("plain")) || !d.is<JsonObject>()) {
    http.send(400, "application/json", "{\"error\":\"json\"}");
    return;
  }
  uint32_t ch = settings.applyJson(d.as<JsonObjectConst>());
  app::settingsChanged(ch, app::SRC_WEB);
  http.sendHeader("Cache-Control", "no-store");
  http.send(200, "application/json", settingsJson(false));
}

void handlePassword() {
  if (!authOk()) return;
  JsonDocument d;
  deserializeJson(d, http.arg("plain"));
  String pw = d["password"] | "";
  pw.trim();
  if (pw.length() < 4 || pw.length() > 32) {
    http.send(400, "application/json", "{\"error\":\"Passwort: 4 bis 32 Zeichen\"}");
    return;
  }
  settings.adminPassword = pw;
  settings.save();
  sendOk();
}

// LED-Testmodus: {"mode":"chase|rows|letters|rgbw|color|led|dots|off","color":"#RRGGBB","index":0..223,"bri":0..255,"speed":1..100}
// Ohne Inhalt: einmaliges Lauflicht wie frueher.
void handleTest() {
  const String body = http.arg("plain");
  if (body.isEmpty()) {
    display::testPattern();
    sendOk();
    return;
  }
  JsonDocument d;
  if (deserializeJson(d, body)) {
    http.send(400, "application/json", "{\"error\":\"json\"}");
    return;
  }
  static const char* const NAMES[] = {"off", "chase", "rows", "letters", "rgbw", "color", "led", "dots"};
  const char* m = d["mode"] | "off";
  uint8_t mode = display::TEST_OFF;
  for (uint8_t i = 0; i < sizeof(NAMES) / sizeof(NAMES[0]); i++)
    if (!strcmp(m, NAMES[i])) mode = i;
  const char* c = d["color"] | "#FFFFFF";
  const uint32_t rgb = c[0] == '#' ? strtoul(c + 1, nullptr, 16) : 0xFFFFFF;
  display::setTest((display::TestMode)mode, CRGB(rgb), d["index"] | 0, constrain(d["bri"] | 60, 0, 255),
                   constrain(d["speed"] | 50, 1, 100));
  sendOk();
}

void setupRoutes() {
  http.on("/", HTTP_GET, handleIndex);
  http.on("/index.html", HTTP_GET, handleIndex);
  http.on("/v1/settings", HTTP_GET, [] {
    http.sendHeader("Cache-Control", "no-store");
    http.send(200, "application/json", settingsJson(false));
  });
  http.on("/v1/settings", HTTP_POST, handleSettingsPost);
  http.on("/v1/info", HTTP_GET, handleInfo);
  http.on("/v1/layout", HTTP_GET, handleLayout);
  http.on("/v1/replay", HTTP_POST, [] { display::replayTransition(); sendOk(); });
  http.on("/v1/test", HTTP_POST, handleTest);
  http.on("/v1/auth", HTTP_GET, [] { if (authOk()) sendOk(); });
  http.on("/v1/check", HTTP_POST, [] { if (!authOk()) return; ota::requestCheck(); sendOk(); });
  http.on("/v1/wifi", HTTP_POST, [] { if (!authOk()) return; sendOk(); net::requestPortal(); });
  http.on("/v1/reboot", HTTP_POST, [] {
    if (!authOk()) return;
    sendOk();
    settings.save();
    delay(300);
    ESP.restart();
  });
  http.on("/v1/password", HTTP_POST, handlePassword);

  // Firmware-Datei hochladen (aus GitHub Release oder .pio/build/wordclock/firmware.bin)
  http.on(
      "/update", HTTP_POST,
      [] {
        if (!authOk()) return;
        bool ok = !Update.hasError();
        http.send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"update\"}");
        if (ok) {
          delay(500);
          ESP.restart();
        }
      },
      [] {
        if (!http.authenticate("admin", settings.adminPassword.c_str())) return;
        HTTPUpload& up = http.upload();
        if (up.status == UPLOAD_FILE_START) {
          display::setState(SysState::Updating);
          Update.begin(UPDATE_SIZE_UNKNOWN);
        } else if (up.status == UPLOAD_FILE_WRITE) {
          Update.write(up.buf, up.currentSize);
          if (up.totalSize) display::setProgress(min<size_t>(99, up.totalSize * 100 / 1400000));
        } else if (up.status == UPLOAD_FILE_END) {
          Update.end(true);
          if (Update.hasError()) display::setState(SysState::Error);
        }
      });

  http.onNotFound([] {
    if (alexa::handleApi(http.uri(), http.arg(0))) return;  // Hue-API fuer Alexa
    http.send(404, "text/plain", "nicht gefunden");
  });

  ws.onEvent([](uint8_t num, WStype_t type, uint8_t*, size_t) {
    if (type == WStype_CONNECTED) {
      String s = settingsJson(true);
      ws.sendTXT(num, s);
    }
  });
}
}  // namespace

namespace web {

void begin() {
  if (!routesReady) {
    setupRoutes();
    ws.begin();
    routesReady = true;
  }
  if (!listening) {
    http.begin();
    listening = true;
    Serial.println("[web] http://wordclock.local");
  }
}

void end() {
  if (!listening) return;
  http.stop();
  listening = false;
}

bool running() { return listening; }

void loop() {
  if (!routesReady) return;
  if (listening) http.handleClient();
  ws.loop();
  // Live-Bild fuer die 3D-Ansicht: ~15 Bilder/s, nur wenn jemand zuschaut
  if (ws.connectedClients() > 0 && millis() - lastFrameSent >= 66) {
    lastFrameSent = millis();
    frameBuf[0] = 1;
    frameBuf[1] = display::brightness();
    memcpy(frameBuf + 2, display::frame(), LED_COUNT * 3);
    ws.broadcastBIN(frameBuf, sizeof(frameBuf));
  }
}

void notifySettings() {
  if (!routesReady || ws.connectedClients() == 0) return;
  String s = settingsJson(true);
  ws.broadcastTXT(s);
}

WebServer* server() { return &http; }

}  // namespace web
