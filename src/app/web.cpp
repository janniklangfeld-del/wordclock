#include "web.h"
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Update.h>
#include <WiFi.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
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

// Asynchroner Webserver (ESPAsyncWebServer): Netzwerk laeuft in einem eigenen Task, die
// Hauptschleife (LEDs, Spiele) wartet nie auf ein langsames Handy und ein beschaeftigtes
// Handy blockiert nie die anderen. Seite, REST und WebSocket (/ws) teilen sich Port 80.
//
// Gemeinsame Daten (Einstellungen, Anzeige, Spiele) gehoeren der Hauptschleife. Sie haelt
// dafuer eine Sperre (web::lock), die sie nur zwischen zwei Durchlaeufen kurz freigibt:
//   - REST-Anfragen, die diese Daten anfassen, holen sich die Sperre (max. 3 s, sonst 503).
//   - WebSocket-Nachrichten werden nur in eine Warteschlange gelegt und in web::loop()
//     (Hauptschleife) verarbeitet. Der WebSocket-Teil nimmt die Sperre nie -> kein Verklemmen.
//   - Seite und Fotos brauchen keine Sperre und kommen daher immer sofort.

namespace {
constexpr uint8_t MAXC = WEBSOCKETS_SERVER_CLIENT_MAX;  // Plaetze (Handys/Seiten) fuer Spiele
constexpr uint32_t LOCK_WAIT_MS = 3000;   // < Task-Wachhund (5 s) des Webserver-Tasks
constexpr uint32_t PING_MS = 3000;        // Lebenszeichen an jedes Handy ...
constexpr uint32_t DEAD_MS = 12000;       // ... nichts vom Handy so lange -> gilt als weg (Spiel pausiert)
constexpr uint32_t STILL_MS = 7000;       // sichtbare Seiten senden alle 3 s "hb"; stiller = Hintergrund-Tab, darf Platz raeumen
constexpr size_t WS_MSG_MAX = 1024;

AsyncWebServer http(80);
AsyncWebSocket ws("/ws");
SemaphoreHandle_t mtx = nullptr;
QueueHandle_t evq = nullptr;
bool routesReady = false;
bool listening = false;
volatile uint32_t restartAt = 0;

SemaphoreHandle_t mutex() {
  if (!mtx) mtx = xSemaphoreCreateRecursiveMutex();
  return mtx;
}

// ---------- WebSocket: Ereignisse aus dem Webserver-Task -> Hauptschleife ----------
enum EvType : uint8_t { EV_CONNECT, EV_DISCONNECT, EV_TEXT, EV_BIN, EV_PONG };
struct Ev {
  EvType type;
  uint32_t id;
  uint32_t ip;
  uint16_t len;
  uint8_t* data;  // malloc, gibt die Hauptschleife frei
};

struct Client {
  uint32_t id = 0;  // 0 = frei
  uint32_t ip = 0;
  uint32_t connectedAt = 0, lastRx = 0, lastAlive = 0, lastSentTo = 0, slowUntil = 0;
  bool wantFrames = true;
};
Client cl[MAXC];  // gehoert der Hauptschleife

uint32_t lastFrameSent = 0, lastPing = 0, lastEvict = 0, lastChange = 0;
uint8_t frameBuf[2 + LED_COUNT * 3];
uint8_t lastFrame[2 + LED_COUNT * 3];

void pushEv(EvType t, uint32_t id, uint32_t ip, const uint8_t* data, size_t len) {
  Ev e{t, id, ip, 0, nullptr};
  if (data && len) {
    if (len > WS_MSG_MAX) return;
    e.data = (uint8_t*)malloc(len + 1);
    if (!e.data) return;
    memcpy(e.data, data, len);
    e.data[len] = 0;
    e.len = len;
  }
  // Verbinden/Trennen darf nicht verloren gehen, Tasten/Text notfalls schon
  const TickType_t wait = (t == EV_CONNECT || t == EV_DISCONNECT) ? pdMS_TO_TICKS(200) : 0;
  if (xQueueSend(evq, &e, wait) != pdTRUE) free(e.data);
}

void onWsEvent(AsyncWebSocket*, AsyncWebSocketClient* c, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      // Volle Warteschlange: Nachricht verwerfen statt Handy trennen (Live-Bild fuellte sie
      // sonst bis zum Anschlag, die naechste Spielnachricht warf das Handy raus).
      c->setCloseClientOnQueueFull(false);
      if (c->client()) {
        c->client()->setAckTimeout(20000);  // kurze WLAN-Aussetzer (iPhone-Stromsparen) nicht gleich trennen
        c->client()->setNoDelay(true);
      }
      pushEv(EV_CONNECT, c->id(), (uint32_t)c->remoteIP(), nullptr, 0);
      break;
    case WS_EVT_DISCONNECT:
      pushEv(EV_DISCONNECT, c->id(), 0, nullptr, 0);
      break;
    case WS_EVT_PONG:
      pushEv(EV_PONG, c->id(), 0, nullptr, 0);
      break;
    case WS_EVT_DATA: {
      const AwsFrameInfo* info = (const AwsFrameInfo*)arg;
      // Nachrichten sind klein und kommen in einem Stueck; zerstueckelte werden ignoriert
      if (!info->final || info->index != 0 || info->len != len) break;
      pushEv(info->opcode == WS_BINARY ? EV_BIN : EV_TEXT, c->id(), 0, data, len);
      break;
    }
    default:
      break;
  }
}

int slotOf(uint32_t id) {
  for (int i = 0; i < MAXC; i++)
    if (cl[i].id == id) return i;
  return -1;
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

// Letzte Trennungen mit Grund (fuer /v1/info, Fehlersuche "Spieler fliegt raus")
struct Drop { uint32_t at, ip, dur; uint8_t q; const char* why; };
Drop drops[8];
uint8_t dropN = 0;

void dropSlot(int s, bool closeSocket, const char* why) {
  const uint32_t id = cl[s].id;
  AsyncWebSocketClient* wc = ws.client(id);
  drops[dropN % 8] = {millis(), cl[s].ip, millis() - cl[s].connectedAt, (uint8_t)(wc ? wc->queueLen() : 255), why};
  dropN++;
  cl[s] = Client();
  if (closeSocket) ws.close(id);
  games::onDisconnect(s);
}

void handleEv(Ev& e) {
  const uint32_t now = millis();
  int s = slotOf(e.id);
  switch (e.type) {
    case EV_CONNECT: {
      if (s >= 0) break;
      for (int i = 0; i < MAXC && s < 0; i++)
        if (!cl[i].id) s = i;
      if (s < 0) {  // voll: Platz der Seite nehmen, die am laengsten still ist (Hintergrund-Tab)
        int oldest = -1;
        for (int i = 0; i < MAXC; i++)
          if (now - cl[i].lastRx > STILL_MS && (oldest < 0 || cl[i].lastRx < cl[oldest].lastRx)) oldest = i;
        if (oldest < 0) {
          Serial.println("[web] alle Plaetze belegt -> neue Verbindung abgewiesen");
          ws.close(e.id, 1013, "voll");
          break;
        }
        dropSlot(oldest, true, "voll");
        s = oldest;
      }
      cl[s].id = e.id;
      cl[s].ip = e.ip;
      cl[s].connectedAt = cl[s].lastRx = cl[s].lastAlive = now;
      cl[s].wantFrames = true;
      ws.text(e.id, settingsJson(true));
      games::onConnect(s);
      break;
    }
    case EV_DISCONNECT:
      if (s >= 0) dropSlot(s, false, "zu");  // vom Handy/Netz/Timeout geschlossen
      break;
    case EV_PONG:
      if (s >= 0) cl[s].lastAlive = now;
      break;
    case EV_BIN:  // Spieltaste: [2, Taste, gedrueckt]
      if (s < 0) break;
      cl[s].lastRx = cl[s].lastAlive = now;
      if (e.len >= 3 && e.data[0] == 2) games::onKey(s, e.data[1], e.data[2]);
      break;
    case EV_TEXT: {
      if (s < 0) break;
      cl[s].lastRx = cl[s].lastAlive = now;
      JsonDocument d;
      if (deserializeJson(d, (const char*)e.data, e.len) || !d.is<JsonObject>()) break;
      const char* t = d["t"] | "";
      if (!strcmp(t, "frames")) {
        cl[s].wantFrames = d["on"] | true;
      } else if (!strncmp(t, "g.", 2)) {
        games::onText(s, d.as<JsonObjectConst>());
      }
      break;
    }
  }
}

// ---------- REST ----------
// Laeuft im Webserver-Task. Fasst die Funktion gemeinsame Daten an -> unter der Sperre.
template <class F>
void locked(AsyncWebServerRequest* r, F f) {
  if (xSemaphoreTakeRecursive(mutex(), pdMS_TO_TICKS(LOCK_WAIT_MS)) != pdTRUE) {
    r->send(503, "application/json", "{\"error\":\"beschaeftigt\"}");
    return;
  }
  f();
  xSemaphoreGiveRecursive(mutex());
}

// Rumpf (JSON) einer Anfrage sammeln -> r->_tempObject (wird vom Server freigegeben)
void collectBody(AsyncWebServerRequest* r, uint8_t* data, size_t len, size_t index, size_t total) {
  if (total > 4096) return;
  if (index == 0 && !r->_tempObject) r->_tempObject = calloc(total + 1, 1);
  if (r->_tempObject && index + len <= total) memcpy((uint8_t*)r->_tempObject + index, data, len);
}
const char* body(AsyncWebServerRequest* r) { return r->_tempObject ? (const char*)r->_tempObject : ""; }

void sendJson(AsyncWebServerRequest* r, int code, const String& s) {
  AsyncWebServerResponse* res = r->beginResponse(code, "application/json", s);
  res->addHeader("Cache-Control", "no-store");
  r->send(res);
}
void sendJson(AsyncWebServerRequest* r, int code, const JsonDocument& doc) {
  String s;
  serializeJson(doc, s);
  sendJson(r, code, s);
}
void sendOk(AsyncWebServerRequest* r) { r->send(200, "application/json", "{\"ok\":true}"); }

bool authValid(AsyncWebServerRequest* r) { return r->authenticate("admin", settings.adminPassword.c_str()); }
// Ohne WWW-Authenticate: die Webseite fragt das Passwort selbst ab.
bool authOk(AsyncWebServerRequest* r) {
  if (authValid(r)) return true;
  r->send(401, "application/json", "{\"error\":\"auth\"}");
  return false;
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
String arg(AsyncWebServerRequest* r, const char* name) {
  if (r->hasParam(name)) return r->getParam(name)->value();
  if (r->hasParam(name, true)) return r->getParam(name, true)->value();
  return "";
}

void handleIndex(AsyncWebServerRequest* r) {
  AsyncWebServerResponse* res = r->beginResponse(200, "text/html; charset=utf-8", WEB_INDEX_GZ, WEB_INDEX_GZ_LEN);
  res->addHeader("Content-Encoding", "gzip");
  res->addHeader("Cache-Control", "no-cache");
  r->send(res);
}

void handleInfo(AsyncWebServerRequest* r) {
  JsonDocument d;
  d["fw"] = FW_VERSION;
  d["repo"] = GITHUB_REPO;
  d["ip"] = WiFi.localIP().toString();
  d["ssid"] = WiFi.SSID();
  d["rssi"] = WiFi.RSSI();
  d["mac"] = WiFi.macAddress();
  d["uptime"] = millis() / 1000;
  d["heap"] = ESP.getFreeHeap();
  d["heapMin"] = ESP.getMinFreeHeap();
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
  d["server"] = "async";
  // Offene Live-Verbindungen (zur Fehlersuche: wer belegt die Plaetze?)
  d["wsMax"] = MAXC;
  JsonArray wl = d["ws"].to<JsonArray>();
  const uint32_t ms = millis();
  for (uint8_t c = 0; c < MAXC; c++) {
    if (!cl[c].id) continue;
    JsonObject o = wl.add<JsonObject>();
    o["n"] = c;
    o["ip"] = IPAddress(cl[c].ip).toString();
    o["s"] = (ms - cl[c].connectedAt) / 1000;
    o["idle"] = (ms - cl[c].lastRx) / 1000;
    AsyncWebSocketClient* wc = ws.client(cl[c].id);
    o["q"] = wc ? (int)wc->queueLen() : -1;
    o["tx"] = wc && wc->client() ? (int)wc->client()->space() : -1;
  }
  JsonArray dl = d["drops"].to<JsonArray>();
  for (uint8_t k = 0; k < 8 && k < dropN; k++) {
    const Drop& x = drops[(dropN - 1 - k) % 8];
    JsonObject o = dl.add<JsonObject>();
    o["ago"] = (ms - x.at) / 1000;
    o["ip"] = IPAddress(x.ip).toString();
    o["s"] = x.dur / 1000;
    o["q"] = x.q;
    o["why"] = x.why;
  }
  sendJson(r, 200, d);
}

void handleLayout(AsyncWebServerRequest* r) {
  JsonDocument d;
  JsonArray rows = d["letters"].to<JsonArray>();
  for (int i = 0; i < LETTER_ROWS; i++) rows.add(clockface::LETTERS[i]);
  JsonArray leds = d["leds"].to<JsonArray>();
  for (int i = 0; i < LETTER_ROWS; i++)
    for (int c = 0; c < LETTER_COLS; c++) {
      JsonArray p = leds.add<JsonArray>();
      p.add(clockface::letterLed(i, c, 0));
      p.add(clockface::letterLed(i, c, 1));
    }
  JsonArray dots = d["dots"].to<JsonArray>();
  for (int k = 0; k < LED_MINUTE_COUNT; k++) dots.add(clockface::dotLed(k));
  d["count"] = LED_COUNT;
  sendJson(r, 200, d);
}

void handleSettingsPost(AsyncWebServerRequest* r) {
  JsonDocument d;
  if (deserializeJson(d, body(r)) || !d.is<JsonObject>()) {
    r->send(400, "application/json", "{\"error\":\"json\"}");
    return;
  }
  uint32_t ch = settings.applyJson(d.as<JsonObjectConst>());
  app::settingsChanged(ch, app::SRC_WEB);
  sendJson(r, 200, settingsJson(false));
}

void handlePassword(AsyncWebServerRequest* r) {
  if (!authOk(r)) return;
  JsonDocument d;
  deserializeJson(d, body(r));
  String pw = d["password"] | "";
  pw.trim();
  if (pw.length() < 4 || pw.length() > 32) {
    r->send(400, "application/json", "{\"error\":\"Passwort: 4 bis 32 Zeichen\"}");
    return;
  }
  settings.adminPassword = pw;
  settings.save();
  sendOk(r);
}

// LED-Testmodus: {"mode":"chase|rows|letters|rgbw|color|led|dots|off","color":"#RRGGBB","index":0..223,"bri":0..255,"speed":1..100}
// Ohne Inhalt: einmaliges Lauflicht wie frueher.
void handleTest(AsyncWebServerRequest* r) {
  const char* b = body(r);
  if (!*b) {
    display::testPattern();
    sendOk(r);
    return;
  }
  JsonDocument d;
  if (deserializeJson(d, b)) {
    r->send(400, "application/json", "{\"error\":\"json\"}");
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
  sendOk(r);
}

void handlePhotoList(AsyncWebServerRequest* r) {
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
  sendJson(r, 200, d);
}

void servePhoto(AsyncWebServerRequest* r, const char* name) {
  String f = photoPath(name);
  if (!fsOk || !LittleFS.exists(f)) {
    r->send(404, "text/plain", "kein Foto");
    return;
  }
  uint8_t m[4] = {0};
  File h = LittleFS.open(f, "r");
  h.read(m, 4);
  h.close();
  const char* type = m[0] == 0xFF ? "image/jpeg" : m[0] == 0x89 ? "image/png" : "image/webp";
  AsyncWebServerResponse* res = r->beginResponse(LittleFS, f, type);
  res->addHeader("Cache-Control", "max-age=31536000");  // Webseite haengt ?v=Groesse an
  r->send(res);
}

// Firmware-Upload: Zustand pro Upload (nur einer gleichzeitig)
bool fwAuthOk = false;
bool fwStarted = false;

void setupRoutes() {
  ws.onEvent(onWsEvent);
  http.addHandler(&ws);

  http.on("/", HTTP_GET, handleIndex);
  http.on("/index.html", HTTP_GET, handleIndex);
  http.on("/v1/settings", HTTP_GET, [](AsyncWebServerRequest* r) { locked(r, [r] { sendJson(r, 200, settingsJson(false)); }); });
  http.on("/v1/settings", HTTP_POST, [](AsyncWebServerRequest* r) { locked(r, [r] { handleSettingsPost(r); }); }, nullptr,
          collectBody);
  http.on("/v1/info", HTTP_GET, [](AsyncWebServerRequest* r) { locked(r, [r] { handleInfo(r); }); });
  http.on("/v1/layout", HTTP_GET, handleLayout);
  http.on("/v1/replay", HTTP_POST, [](AsyncWebServerRequest* r) { locked(r, [r] { display::replayTransition(); sendOk(r); }); });
  http.on("/v1/test", HTTP_POST, [](AsyncWebServerRequest* r) { locked(r, [r] { handleTest(r); }); }, nullptr, collectBody);
  http.on("/v1/intro", HTTP_POST, [](AsyncWebServerRequest* r) { locked(r, [r] { display::playIntro(); sendOk(r); }); });
  http.on("/v1/auth", HTTP_GET, [](AsyncWebServerRequest* r) { if (authOk(r)) sendOk(r); });
  http.on("/v1/check", HTTP_POST, [](AsyncWebServerRequest* r) { locked(r, [r] { if (authOk(r)) { ota::requestCheck(); sendOk(r); } }); });
  http.on("/v1/wifi", HTTP_POST, [](AsyncWebServerRequest* r) { locked(r, [r] { if (authOk(r)) { sendOk(r); net::requestPortal(); } }); });
  http.on("/v1/reboot", HTTP_POST, [](AsyncWebServerRequest* r) {
    locked(r, [r] {
      if (!authOk(r)) return;
      sendOk(r);
      settings.save();
      restartAt = millis() + 500;
    });
  });
  http.on("/v1/password", HTTP_POST, [](AsyncWebServerRequest* r) { locked(r, [r] { handlePassword(r); }); }, nullptr,
          collectBody);
  http.on("/v1/games", HTTP_GET, [](AsyncWebServerRequest* r) {
    locked(r, [r] {
      JsonDocument d;
      games::recordsJson(d.to<JsonObject>());
      sendJson(r, 200, d);
    });
  });
  http.on("/v1/photos", HTTP_GET, handlePhotoList);
  for (auto p : PHOTOS)
    http.on((String("/photo/") + p).c_str(), HTTP_GET, [p](AsyncWebServerRequest* r) { servePhoto(r, p); });
  http.on("/v1/photo/del", HTTP_POST, [](AsyncWebServerRequest* r) {
    if (!authOk(r)) return;
    String n = arg(r, "n");
    if (!photoName(n)) {
      r->send(400, "application/json", "{\"error\":\"name\"}");
      return;
    }
    LittleFS.remove(photoPath(n));
    sendOk(r);
  });
  // Foto hochladen: POST /v1/photo?n=gesicht (multipart; die Webseite verkleinert vorher)
  http.on(
      "/v1/photo", HTTP_POST,
      [](AsyncWebServerRequest* r) {
        if (!authOk(r)) return;
        const String n = arg(r, "n");
        if (!fsOk || !photoName(n)) {
          r->send(400, "application/json", "{\"error\":\"Foto-Speicher\"}");
          return;
        }
        if (upTooBig) {
          LittleFS.remove("/up.tmp");
          r->send(413, "application/json", "{\"error\":\"Foto zu gross oder Speicher voll\"}");
          return;
        }
        LittleFS.remove(photoPath(n));
        LittleFS.rename("/up.tmp", photoPath(n));
        sendOk(r);
      },
      [](AsyncWebServerRequest* r, const String&, size_t index, uint8_t* data, size_t len, bool final) {
        if (!fsOk || !authValid(r)) return;
        if (index == 0) {
          upTooBig = false;
          if (upFile) upFile.close();
          upFile = LittleFS.open("/up.tmp", "w");
        }
        if (index + len > PHOTO_MAX) upTooBig = true;
        if (upFile && !upTooBig && upFile.write(data, len) != len) upTooBig = true;
        if (final && upFile) upFile.close();
      });

  // Firmware-Datei hochladen (aus GitHub Release oder .pio/build/wordclock/firmware.bin).
  // Gleiche Schnittstelle wie bisher: multipart, Benutzer "admin" + Admin-Passwort, Antwort {"ok":true}.
  http.on(
      "/update", HTTP_POST,
      [](AsyncWebServerRequest* r) {
        if (!authOk(r)) return;
        const bool ok = fwStarted && !Update.isRunning() && !Update.hasError();
        r->send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"update\"}");
        if (ok) restartAt = millis() + 800;  // Antwort erst rausschicken, dann neu starten
        fwStarted = false;
      },
      [](AsyncWebServerRequest* r, const String&, size_t index, uint8_t* data, size_t len, bool final) {
        if (index == 0) {
          fwAuthOk = authValid(r);
          fwStarted = false;
          if (!fwAuthOk) return;
          if (Update.isRunning()) Update.abort();
          if (xSemaphoreTakeRecursive(mutex(), pdMS_TO_TICKS(LOCK_WAIT_MS)) == pdTRUE) {
            sys::pauseWatchdog();
            display::setState(SysState::Updating);
            xSemaphoreGiveRecursive(mutex());
          }
          fwStarted = Update.begin(UPDATE_SIZE_UNKNOWN);
          Serial.printf("[web] Update gestartet (%u Bytes erwartet): %s\n", (unsigned)r->contentLength(),
                        fwStarted ? "ok" : Update.errorString());
        }
        if (!fwAuthOk || !fwStarted) return;
        if (Update.write(data, len) != len) Serial.printf("[web] Update-Schreibfehler: %s\n", Update.errorString());
        const size_t total = r->contentLength();
        if (total && xSemaphoreTakeRecursive(mutex(), 0) == pdTRUE) {  // Fortschritt nur, wenn frei
          display::setProgress(min<size_t>(99, (index + len) * 100 / total));
          xSemaphoreGiveRecursive(mutex());
        }
        if (final) {
          Update.end(true);
          Serial.printf("[web] Update fertig: %s\n", Update.hasError() ? Update.errorString() : "ok");
          if (Update.hasError() && xSemaphoreTakeRecursive(mutex(), pdMS_TO_TICKS(LOCK_WAIT_MS)) == pdTRUE) {
            display::setState(SysState::Error);
            sys::resumeWatchdog();
            xSemaphoreGiveRecursive(mutex());
          }
        }
      });

  http.onNotFound([](AsyncWebServerRequest* r) {
    locked(r, [r] {
      if (alexa::handleApi(r)) return;  // Hue-API fuer Alexa (/api/...)
      r->send(404, "text/plain", "nicht gefunden");
    });
  });
}

void sendFrames(uint32_t now) {
  if (now - lastFrameSent < 100) return;  // max. 10 Bilder/s
  lastFrameSent = now;
  bool any = false;
  for (auto& c : cl) any |= c.id && c.wantFrames;
  if (!any) return;
  frameBuf[0] = 1;
  frameBuf[1] = display::brightness();
  memcpy(frameBuf + 2, display::frame(), LED_COUNT * 3);
  // Nur geaenderte Bilder schicken (die Uhrzeit steht meist still), sonst 1x pro Sekunde.
  const bool changed = memcmp(frameBuf, lastFrame, sizeof(frameBuf)) != 0;
  if (changed) {
    memcpy(lastFrame, frameBuf, sizeof(frameBuf));
    lastChange = now;
  }
  for (auto& c : cl) {
    if (!c.id || !c.wantFrames) continue;
    const uint32_t minGap = now < c.slowUntil ? 1000 : (changed ? 0 : 1000);
    if (now - c.lastSentTo < minGap) continue;
    if (!changed && c.lastSentTo > lastChange && now - c.lastSentTo < 1000) continue;
    // Handy kommt nicht hinterher (schwaches WLAN): Bild auslassen statt stauen
    // Bilder nur, wenn kaum etwas wartet -> Spiel- und Einstellungsnachrichten haben immer Platz.
    AsyncWebSocketClient* wc = ws.client(c.id);
    if (!wc || wc->queueLen() > 2) {
      c.slowUntil = now + 3000;
      continue;
    }
    c.lastSentTo = now;
    ws.binary(c.id, frameBuf, sizeof(frameBuf));
  }
}

}  // namespace

namespace web {

void lock() { xSemaphoreTakeRecursive(mutex(), portMAX_DELAY); }
void unlock() { xSemaphoreGiveRecursive(mutex()); }

void begin() {
  if (!routesReady) {
    mutex();
    evq = xQueueCreate(48, sizeof(Ev));
    fsOk = LittleFS.begin(true);
    if (!fsOk) Serial.println("[web] LittleFS fehlt - keine Fotos");
    setupRoutes();
    routesReady = true;
  }
  if (!listening) {
    http.begin();
    listening = true;
    Serial.println("[web] http://wordclock.local (asynchron, WebSocket /ws)");
  }
}

void end() {
  if (!listening) return;
  ws.closeAll();
  http.end();
  listening = false;
}

bool running() { return listening; }

void loop() {
  if (!routesReady) return;
  uint32_t now = millis();
  if (restartAt && (int32_t)(now - restartAt) >= 0) {
    Serial.println("[web] Neustart");
    delay(100);
    ESP.restart();
  }

  Ev e;
  for (uint8_t n = 0; n < 32 && xQueueReceive(evq, &e, 0) == pdTRUE; n++) {
    handleEv(e);
    free(e.data);
  }
  // Zeit NACH den Ereignissen neu lesen: handleEv stempelt mit millis(); ein aelteres "now"
  // ergaebe now - lastRx < 0 -> Ueberlauf -> Handy galt als ewig still und flog raus.
  now = millis();

  // Lebenszeichen: Browser antworten auf Pings selbst (auch ohne Tastendruck). Gesperrte
  // oder weggelegte Handys antworten nicht mehr -> Platz frei, Spiel pausiert.
  if (now - lastPing >= PING_MS) {
    lastPing = now;
    for (int s = 0; s < MAXC; s++) {
      if (!cl[s].id) continue;
      if (now - cl[s].lastAlive > DEAD_MS) {
        Serial.printf("[web] Handy %d antwortet nicht mehr -> trennen\n", s);
        dropSlot(s, true, "stumm");
        continue;
      }
      // Kein ws.ping(): Ping-Frames bringen die Ack-Zaehlung von ESPAsyncWebServer 3.9.6
      // durcheinander -> Nachrichten bleiben fuer immer in der Warteschlange haengen
      // (Handy bekam nichts mehr, flog bei 32 raus). Lebenszeichen der Seite: "hb" alle 3 s.
      web::sendTo(s, "{\"t\":\"p\"}");  // Lebenszeichen fuer die Seite (Pings sieht JavaScript nicht)
    }
  }

  // Immer einen Platz frei halten: sind (fast) alle belegt, fliegt die Seite raus, die am
  // laengsten nichts gesendet hat (Tab im Hintergrund). Sichtbare Seiten senden alle 5 s "hb".
  int used = 0;
  for (auto& c : cl) used += c.id != 0;
  if (used >= MAXC - 1 && now - lastEvict > 1000) {
    lastEvict = now;
    int oldest = -1;
    for (int s = 0; s < MAXC; s++)
      if (cl[s].id && now - cl[s].lastRx > STILL_MS && (oldest < 0 || cl[s].lastRx < cl[oldest].lastRx)) oldest = s;
    if (oldest >= 0) {
      Serial.printf("[web] Platz frei machen: Handy %d (still seit %lus)\n", oldest, (now - cl[oldest].lastRx) / 1000);
      dropSlot(oldest, true, "platz");
    }
  }

  static uint32_t lastCleanup = 0;
  if (now - lastCleanup > 2000) {
    lastCleanup = now;
    ws.cleanupClients(MAXC + 2);  // geschlossene Verbindungen aufraeumen
  }

  sendFrames(now);
}

void notifySettings() {
  if (!routesReady) return;
  const String s = settingsJson(true);
  for (int i = 0; i < MAXC; i++)
    if (cl[i].id) web::sendTo(i, s);
}

AsyncWebServer* server() { return &http; }

bool sendTo(uint8_t client, const String& text) {
  if (!routesReady || client >= MAXC || !cl[client].id) return false;
  AsyncWebSocketClient* wc = ws.client(cl[client].id);
  if (!wc || wc->queueLen() >= WS_MAX_QUEUED_MESSAGES - 2) return false;  // Rueckstand: spaeter nochmal
  return wc->text(text);
}

bool clientConnected(uint8_t client) { return routesReady && client < MAXC && cl[client].id != 0; }

}  // namespace web
