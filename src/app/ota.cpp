#include "ota.h"
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "display.h"
#include "net.h"
#include "settings.h"

// Root-Zertifikate fuer github.com / *.githubusercontent.com (certs/github_roots.pem,
// per board_build.embed_txtfiles eingebettet). CI prueft, dass sie noch passen.
extern const char github_roots_pem[] asm("_binary_certs_github_roots_pem_start");

namespace {
constexpr uint32_t FIRST_CHECK_DELAY_MS = 60UL * 1000;
constexpr uint32_t CHECK_INTERVAL_MS = 6UL * 60 * 60 * 1000;

uint32_t lastCheck = 0;
bool checkedOnce = false;
bool checkRequested = false;
String result = "noch nicht geprueft";
String latest = "";

struct Ver {
  int major = -1, minor = 0, patch = 0;
  bool valid() const { return major >= 0; }
  bool operator>(const Ver& o) const {
    if (major != o.major) return major > o.major;
    if (minor != o.minor) return minor > o.minor;
    return patch > o.patch;
  }
};

// Nur reine Release-Versionen "1.2.3" gelten. Entwicklungs-Builds ("abc123-dirty",
// "0.0.0-dev") sind ungueltig -> automatische Updates aus, damit ein per USB
// geflashter Test-Stand nicht sofort ueberschrieben wird.
Ver parse(String s) {
  Ver v;
  s.trim();
  if (s.startsWith("v")) s.remove(0, 1);
  int a, b, c;
  char rest;
  if (sscanf(s.c_str(), "%d.%d.%d%c", &a, &b, &c, &rest) == 3) {
    v.major = a; v.minor = b; v.patch = c;
  }
  return v;
}

// Liefert den Tag des neuesten Releases ("v1.2.3") ohne GitHub-API (kein Rate-Limit,
// kein JSON): /releases/latest leitet per 302 auf /releases/tag/<tag> um.
String fetchLatestTag(WiFiClientSecure& client) {
  HTTPClient http;
  String url = String("https://github.com/") + GITHUB_REPO + "/releases/latest";
  if (!http.begin(client, url)) return "";
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  http.setUserAgent("wordclock/" FW_VERSION);
  const char* headers[] = {"Location"};
  http.collectHeaders(headers, 1);
  int code = http.sendRequest("HEAD");
  String loc = http.header("Location");
  http.end();
  if (code != 301 && code != 302) {
    result = "GitHub antwortet " + String(code);
    return "";
  }
  int i = loc.lastIndexOf("/tag/");
  return i >= 0 ? loc.substring(i + 5) : "";
}

void checkAndUpdate(bool manual) {
  if (strlen(GITHUB_REPO) == 0) { result = "kein GitHub-Repo einkompiliert"; return; }
  if (!net::online()) { result = "offline"; return; }

  Ver current = parse(FW_VERSION);
  if (!current.valid() && !manual) { result = "Entwicklungs-Build: Auto-Update aus"; return; }

  WiFiClientSecure client;
  client.setCACert(github_roots_pem);
  client.setTimeout(15);

  String tag = fetchLatestTag(client);
  if (tag.isEmpty()) {
    if (result.startsWith("GitHub")) return;
    result = "kein Release gefunden / TLS-Fehler";
    return;
  }
  latest = tag;
  Ver remote = parse(tag);
  if (!remote.valid()) { result = "Release-Tag ungueltig: " + tag; return; }
  if (current.valid() && !(remote > current)) { result = "aktuell (" + tag + ")"; return; }

  Serial.printf("[ota] Update %s -> %s\n", FW_VERSION, tag.c_str());
  String url = String("https://github.com/") + GITHUB_REPO + "/releases/download/" + tag + "/firmware.bin";

  display::setState(SysState::Updating);
  httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  httpUpdate.rebootOnUpdate(true);
  httpUpdate.onProgress([](int done, int total) {
    if (total > 0) display::setProgress(done * 100 / total);
  });
  t_httpUpdate_return r = httpUpdate.update(client, url);
  // Bei Erfolg startet der ESP32 neu und kommt hier nicht an.
  result = "Update fehlgeschlagen: " + httpUpdate.getLastErrorString();
  Serial.println("[ota] " + result);
  display::setState(r == HTTP_UPDATE_NO_UPDATES ? SysState::Online : SysState::Error);
}

void setupArduinoOta() {
  ArduinoOTA.setHostname(net::HOSTNAME);
  ArduinoOTA.setPassword(settings.adminPassword.c_str());
  ArduinoOTA.onStart([] { display::setState(SysState::Updating); });
  ArduinoOTA.onProgress([](unsigned done, unsigned total) { display::setProgress(done * 100 / total); });
  ArduinoOTA.onError([](ota_error_t) { display::setState(SysState::Error); });
  ArduinoOTA.setMdnsEnabled(false);  // mDNS macht net.cpp
  ArduinoOTA.begin();
}
}  // namespace

namespace ota {

void begin() { setupArduinoOta(); }

void loop() {
  ArduinoOTA.handle();
  uint32_t now = millis();
  bool due = settings.autoUpdate &&
             ((!checkedOnce && now > FIRST_CHECK_DELAY_MS) || (checkedOnce && now - lastCheck > CHECK_INTERVAL_MS));
  if ((due || checkRequested) && net::online()) {
    bool manual = checkRequested;
    checkRequested = false;
    checkedOnce = true;
    lastCheck = now;
    checkAndUpdate(manual);
    Serial.println("[ota] " + result);
  }
}

void requestCheck() { checkRequested = true; }
String lastResult() { return result; }
String latestKnownVersion() { return latest; }

}  // namespace ota
