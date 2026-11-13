#include "web.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Update.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <WiFi.h>
#include "alexa.h"
#include "app.h"
#include "clockface.h"
#include "display.h"
#include "games.h"
#include "hardware.h"
#include "light.h"
#include "net.h"
#include "ota.h"
#include "settings.h"
#include "sys.h"
#include "web_index.h"

namespace {
WebServer http(80);
WebSocketsServer ws(81);
bool routesReady = false;
bool listening = false;
uint32_t lastFrameSent = 0;
uint8_t frameBuf[2 + LED_COUNT * 3];
bool wantFrames[WEBSOCKETS_SERVER_CLIENT_MAX];  // Controller schalten das Live-Bild ab
uint32_t slowUntil[WEBSOCKETS_SERVER_CLIENT_MAX];  // langsames Handy (schwaches WLAN): nur 1 Bild/s
uint32_t lastSentTo[WEBSOCKETS_SERVER_CLIENT_MAX];
uint32_t connectedAt[WEBSOCKETS_SERVER_CLIENT_MAX];
uint32_t lastRx[WEBSOCKETS_SERVER_CLIENT_MAX];  // letzte Nachricht vom Handy (sichtbare Seiten senden alle 5 s "hb")
uint8_t lastFrame[2 + LED_COUNT * 3];
uint32_t lastChange = 0;

// Senden blockiert, bis das Handy die Daten abnimmt (max. WEBSOCKETS_TCP_TIMEOUT).
// Langsame Handys werden gebremst, bei Fehler getrennt (Datenstrom waere sonst kaputt,
// die Seite verbindet sich von selbst neu).
bool sendChecked(uint8_t c, bool bin, uint8_t* data, size_t len) {
  const uint32_t t0 = millis();
  const bool ok = bin ? ws.sendBIN(c, data, len) : ws.sendTXT(c, data, len);
  const uint32_t dt = millis() - t0;
  if (!ok) {
    Serial.printf("[web] Handy %u haengt -> trennen\n", c);
    ws.disconnect(c);
    return false;
  }
  if (dt > 60 && c < WEBSOCKETS_SERVER_CLIENT_MAX) slowUntil[c] = millis() + 5000;
  return true;
}

// Fotos fuer die Webseite liegen im Flash der Uhr (LittleFS), nicht im (oeffentlichen) Repo.
const char* const PHOTOS[] = {"gesicht", "bereit", "kuss", "muetze", "frech"};
constexpr size_t PHOTO_MAX = 48 * 1024;
bool fsOk = false;
File upFile;
bool upTooBig = false;
String photoPath(const String& n) { return "/" + n + ".img"; }
bool photoName(const String& n) {
  for (auto p : PHOTOS)
    if (n == p) return true;
  return false;
}

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
  d["reset"] = sys::resetReason();
  d["boots"] = sys::boots();
  d["crashes"] = sys::crashes();
  // Offene Live-Verbindungen (zur Fehlersuche: wer belegt die Plaetze?)
  d["wsMax"] = WEBSOCKETS_SERVER_CLIENT_MAX;
  JsonArray wl = d["ws"].to<JsonArray>();
  for (uint8_t c = 0; c < WEBSOCKETS_SERVER_CLIENT_MAX; c++) {
    if (!ws.clientIsConnected(c)) continue;
    JsonObject o = wl.add<JsonObject>();
    o["n"] = c;
    o["ip"] = ws.remoteIP(c).toString();
    o["s"] = (millis() - connectedAt[c]) / 1000;
    o["idle"] = (millis() - lastRx[c]) / 1000;
  }
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

void handlePhotoList() {
  JsonDocument d;
  d.to<JsonObject>();
  for (auto p : PHOTOS) {
    String f = photoPath(p);
    if (fsOk && LittleFS.exists(f)) {
      File h = LittleFS.open(f, "r");
      d[p] = h.size();  // Groesse dient als Version
      h.close();
    }
  }
  if (fsOk) d["_free"] = LittleFS.totalBytes() - LittleFS.usedBytes();
  sendJson(200, d);
}

void servePhoto(const char* name) {
  String f = photoPath(name);
  if (!fsOk || !LittleFS.exists(f)) {
    http.send(404, "text/plain", "kein Foto");
    return;
  }
  File h = LittleFS.open(f, "r");
  uint8_t m[4] = {0};
  h.read(m, 4);
  h.seek(0);
  const char* type = m[0] == 0xFF ? "image/jpeg" : m[0] == 0x89 ? "image/png" : "image/webp";
  http.sendHeader("Cache-Control", "max-age=31536000");  // Webseite haengt ?v=Groesse an
  http.streamFile(h, type);
  h.close();
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
  http.on("/v1/intro", HTTP_POST, [] { display::playIntro(); sendOk(); });
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
  http.on("/v1/games", HTTP_GET, [] {
    JsonDocument d;
    games::recordsJson(d.to<JsonObject>());
    sendJson(200, d);
  });
  http.on("/v1/photos", HTTP_GET, handlePhotoList);
  for (auto p : PHOTOS) http.on(String("/photo/") + p, HTTP_GET, [p] { servePhoto(p); });
  http.on("/v1/photo/del", HTTP_POST, [] {
    if (!authOk()) return;
    String n = http.arg("n");
    if (!photoName(n)) {
      http.send(400, "application/json", "{\"error\":\"name\"}");
      return;
    }
    LittleFS.remove(photoPath(n));
    sendOk();
  });
  // Foto hochladen: POST /v1/photo?n=gesicht (multipart; die Webseite verkleinert vorher)
  http.on(
      "/v1/photo", HTTP_POST,
      [] {
        if (!authOk()) return;
        const String n = http.arg("n");
        if (!fsOk || !photoName(n)) {
          http.send(400, "application/json", "{\"error\":\"Foto-Speicher\"}");
          return;
        }
        if (upTooBig) {
          LittleFS.remove("/up.tmp");
          http.send(413, "application/json", "{\"error\":\"Foto zu gross oder Speicher voll\"}");
          return;
        }
        LittleFS.remove(photoPath(n));
        LittleFS.rename("/up.tmp", photoPath(n));
        sendOk();
      },
      [] {
        if (!fsOk || !http.authenticate("admin", settings.adminPassword.c_str())) return;
        HTTPUpload& up = http.upload();
        if (up.status == UPLOAD_FILE_START) {
          upTooBig = false;
          upFile = LittleFS.open("/up.tmp", "w");
        } else if (up.status == UPLOAD_FILE_WRITE) {
          if (up.totalSize > PHOTO_MAX) upTooBig = true;
          if (upFile && !upTooBig && upFile.write(up.buf, up.currentSize) != up.currentSize) upTooBig = true;
        } else if (up.status == UPLOAD_FILE_END || up.status == UPLOAD_FILE_ABORTED) {
          if (upFile) upFile.close();
        }
      });

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
          sys::pauseWatchdog();
          display::setState(SysState::Updating);
          Update.begin(UPDATE_SIZE_UNKNOWN);
        } else if (up.status == UPLOAD_FILE_WRITE) {
          Update.write(up.buf, up.currentSize);
          if (up.totalSize) display::setProgress(min<size_t>(99, up.totalSize * 100 / 1400000));
        } else if (up.status == UPLOAD_FILE_END) {
          Update.end(true);
          if (Update.hasError()) {
            display::setState(SysState::Error);
            sys::resumeWatchdog();
          }
        }
      });

  http.onNotFound([] {
    if (alexa::handleApi(http.uri(), http.arg(0))) return;  // Hue-API fuer Alexa
    http.send(404, "text/plain", "nicht gefunden");
  });

  ws.onEvent([](uint8_t num, WStype_t type, uint8_t* data, size_t len) {
    switch (type) {
      case WStype_CONNECTED: {
        if (num < WEBSOCKETS_SERVER_CLIENT_MAX) {
          wantFrames[num] = true;
          slowUntil[num] = 0;
          lastSentTo[num] = 0;
          connectedAt[num] = lastRx[num] = millis();
        }
        String s = settingsJson(true);
        ws.sendTXT(num, s);
        games::onConnect(num);
        break;
      }
      case WStype_DISCONNECTED:
        if (num < WEBSOCKETS_SERVER_CLIENT_MAX) wantFrames[num] = true;
        games::onDisconnect(num);
        break;
      case WStype_BIN:  // Spieltaste: [2, Taste, gedrueckt]
        if (num < WEBSOCKETS_SERVER_CLIENT_MAX) lastRx[num] = millis();
        if (len >= 3 && data[0] == 2) games::onKey(num, data[1], data[2]);
        break;
      case WStype_TEXT: {
        if (num < WEBSOCKETS_SERVER_CLIENT_MAX) lastRx[num] = millis();
        JsonDocument d;
        if (deserializeJson(d, data, len) || !d.is<JsonObject>()) break;
        const char* t = d["t"] | "";
        if (!strcmp(t, "frames")) {
          if (num < WEBSOCKETS_SERVER_CLIENT_MAX) wantFrames[num] = d["on"] | true;
        } else if (!strncmp(t, "g.", 2)) {
          games::onText(num, d.as<JsonObjectConst>());
        }
        break;
      }
      default:
        break;
    }
  });
}

}  // namespace

namespace web {

void begin() {
  if (!routesReady) {
    fsOk = LittleFS.begin(true);
    if (!fsOk) Serial.println("[web] LittleFS fehlt - keine Fotos");
    setupRoutes();
    ws.begin();
    ws.enableHeartbeat(3000, 2500, 2);  // weggelegte Handys schnell erkennen -> Spiel pausiert
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
  // Live-Bild fuer die 3D-Ansicht: bis 10 Bilder/s, nur an Seiten, die zuschauen
  // (Controller im Spiel schalten es ab, damit nichts die Tasten aufhaelt)
  // Immer einen Platz frei halten: sind (fast) alle belegt, fliegt die Verbindung raus, die am
  // laengsten nichts gesendet hat (Tab im Hintergrund, Handy gesperrt). Sichtbare Seiten senden
  // alle 5 s ein Lebenszeichen, Spieler in einer Partie bleiben also drin.
  const uint32_t now = millis();
  static uint32_t lastEvict = 0;
  if (ws.connectedClients() >= WEBSOCKETS_SERVER_CLIENT_MAX - 1 && now - lastEvict > 1000) {
    lastEvict = now;
    int oldest = -1;
    for (uint8_t c = 0; c < WEBSOCKETS_SERVER_CLIENT_MAX; c++)
      if (ws.clientIsConnected(c) && now - lastRx[c] > 15000 && (oldest < 0 || lastRx[c] < lastRx[oldest])) oldest = c;
    if (oldest >= 0) {
      Serial.printf("[web] Platz frei machen: Handy %d (still seit %lus)\n", oldest, (now - lastRx[oldest]) / 1000);
      ws.disconnect(oldest);
    }
  }

  // Nur geaenderte Bilder schicken (die Uhrzeit steht meist still), sonst 1x pro Sekunde.
  if (ws.connectedClients() > 0 && now - lastFrameSent >= 100) {  // max. 10 Bilder/s
    lastFrameSent = now;
    frameBuf[0] = 1;
    frameBuf[1] = display::brightness();
    memcpy(frameBuf + 2, display::frame(), LED_COUNT * 3);
    const bool changed = memcmp(frameBuf, lastFrame, sizeof(frameBuf)) != 0;
    if (changed) { memcpy(lastFrame, frameBuf, sizeof(frameBuf)); lastChange = now; }
    for (uint8_t c = 0; c < WEBSOCKETS_SERVER_CLIENT_MAX; c++) {
      if (!wantFrames[c] || !ws.clientIsConnected(c)) continue;
      const uint32_t minGap = now < slowUntil[c] ? 1000 : (changed ? 0 : 1000);
      if (now - lastSentTo[c] < minGap) continue;
      if (!changed && lastSentTo[c] > lastChange && now - lastSentTo[c] < 1000) continue;
      lastSentTo[c] = now;
      sendChecked(c, true, frameBuf, sizeof(frameBuf));
    }
  }
}

void notifySettings() {
  if (!routesReady || ws.connectedClients() == 0) return;
  String s = settingsJson(true);
  ws.broadcastTXT(s);
}

WebServer* server() { return &http; }

bool sendTo(uint8_t client, const String& text) {
  if (!routesReady) return false;
  return sendChecked(client, false, (uint8_t*)text.c_str(), text.length());
}

bool clientConnected(uint8_t client) { return routesReady && ws.clientIsConnected(client); }

}  // namespace web
