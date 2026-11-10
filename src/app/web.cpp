#include "web.h"
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include "display.h"
#include "net.h"
#include "ota.h"
#include "settings.h"

namespace {
WebServer* server = nullptr;

bool auth() {
  if (server->authenticate("admin", settings.adminPassword.c_str())) return true;
  server->requestAuthentication(BASIC_AUTH, "Wordclock");
  return false;
}

String uptime() {
  uint32_t s = millis() / 1000;
  char buf[32];
  snprintf(buf, sizeof(buf), "%lud %02lu:%02lu", (unsigned long)(s / 86400), (unsigned long)(s / 3600 % 24),
           (unsigned long)(s / 60 % 60));
  return buf;
}

String timeNow() {
  struct tm t;
  if (!getLocalTime(&t, 10)) return "(noch keine NTP-Zeit)";
  char buf[32];
  strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S", &t);
  return buf;
}

const char* PAGE_HEAD = R"(<!doctype html><html lang=de><meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1"><title>Wordclock</title>
<style>body{font-family:system-ui,sans-serif;max-width:32rem;margin:1rem auto;padding:0 1rem;background:#111;color:#eee}
td{padding:.2rem .6rem .2rem 0}button,input{font-size:1rem;padding:.4rem .8rem;margin:.2rem 0}
form{display:inline}a{color:#9cf}</style><h1>Wordclock</h1>)";

void handleRoot() {
  String h = PAGE_HEAD;
  h += "<table>";
  h += "<tr><td>Firmware</td><td>" FW_VERSION "</td></tr>";
  h += "<tr><td>Uhrzeit</td><td>" + timeNow() + "</td></tr>";
  h += "<tr><td>WLAN</td><td>" + WiFi.SSID() + " (" + String(WiFi.RSSI()) + " dBm)</td></tr>";
  h += "<tr><td>IP</td><td>" + WiFi.localIP().toString() + "</td></tr>";
  h += "<tr><td>Laufzeit</td><td>" + uptime() + "</td></tr>";
  h += "<tr><td>Update-Quelle</td><td>" GITHUB_REPO "</td></tr>";
  h += "<tr><td>Update-Status</td><td>" + ota::lastResult() + "</td></tr>";
  h += "<tr><td>Auto-Update</td><td>" + String(settings.autoUpdate ? "an" : "aus") + "</td></tr>";
  h += "</table><p>";
  h += "<form method=post action=/check><button>Jetzt auf Update pruefen</button></form> ";
  h += "<form method=post action=/autoupdate><button>Auto-Update " + String(settings.autoUpdate ? "aus" : "an") +
       "schalten</button></form><br>";
  h += "<form method=post action=/test><button>LED-Test</button></form> ";
  h += "<form method=post action=/wifi><button>WLAN aendern</button></form> ";
  h += "<form method=post action=/reboot><button>Neustart</button></form>";
  h += "<p><a href=/update>Firmware-Datei hochladen</a></p>";
  server->send(200, "text/html", h);
}

void redirectHome(const char* msg) {
  String h = PAGE_HEAD;
  h += "<p>" + String(msg) + "</p><p><a href=/>zurueck</a></p><meta http-equiv=refresh content='3;url=/'>";
  server->send(200, "text/html", h);
}

void setupRoutes() {
  server->on("/", HTTP_GET, handleRoot);
  server->on("/check", HTTP_POST, [] {
    if (!auth()) return;
    ota::requestCheck();
    redirectHome("Update-Pruefung gestartet. Bei einem Update startet die Uhr neu.");
  });
  server->on("/autoupdate", HTTP_POST, [] {
    if (!auth()) return;
    settings.autoUpdate = !settings.autoUpdate;
    settings.save();
    redirectHome(settings.autoUpdate ? "Auto-Update an" : "Auto-Update aus");
  });
  server->on("/test", HTTP_POST, [] {
    display::testPattern();
    redirectHome("LED-Test laeuft");
  });
  server->on("/wifi", HTTP_POST, [] {
    if (!auth()) return;
    redirectHome("Einrichtungsportal startet: mit WLAN 'Wordclock-Setup' verbinden.");
    net::requestPortal();
  });
  server->on("/reboot", HTTP_POST, [] {
    if (!auth()) return;
    redirectHome("Neustart ...");
    delay(500);
    ESP.restart();
  });
  server->on("/update", HTTP_GET, [] {
    if (!auth()) return;
    String h = PAGE_HEAD;
    h += "<p>firmware.bin auswaehlen (aus GitHub Release oder .pio/build/wordclock/):</p>"
         "<form method=post action=/update enctype=multipart/form-data>"
         "<input type=file name=fw accept=.bin><br><button>Hochladen</button></form>";
    server->send(200, "text/html", h);
  });
  server->on(
      "/update", HTTP_POST,
      [] {
        if (!auth()) return;
        bool ok = !Update.hasError();
        redirectHome(ok ? "Update ok – Neustart" : "Update fehlgeschlagen");
        if (ok) { delay(500); ESP.restart(); }
      },
      [] {
        if (!server->authenticate("admin", settings.adminPassword.c_str())) return;
        HTTPUpload& up = server->upload();
        if (up.status == UPLOAD_FILE_START) {
          display::setState(SysState::Updating);
          Update.begin(UPDATE_SIZE_UNKNOWN);
        } else if (up.status == UPLOAD_FILE_WRITE) {
          Update.write(up.buf, up.currentSize);
        } else if (up.status == UPLOAD_FILE_END) {
          Update.end(true);
          if (Update.hasError()) display::setState(SysState::Error);
        }
      });
  server->onNotFound([] { server->send(404, "text/plain", "nicht gefunden"); });
}
}  // namespace

namespace web {

void begin() {
  if (server) return;
  server = new WebServer(80);
  setupRoutes();
  server->begin();
  Serial.println("[web] http://wordclock.local");
}

void end() {
  if (!server) return;
  server->stop();
  delete server;
  server = nullptr;
}

bool running() { return server != nullptr; }

void loop() {
  if (server) server->handleClient();
}

}  // namespace web
