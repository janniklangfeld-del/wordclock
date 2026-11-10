#include "net.h"
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include "display.h"
#include "hardware.h"
#include "settings.h"
#include "web.h"

namespace {
WiFiManager wm;
WiFiManagerParameter paramAdminPw("adminpw", "Admin-Passwort (fuer Updates, leer = unveraendert)", "", 32);

constexpr uint32_t LOST_BEFORE_PORTAL_MS = 5UL * 60 * 1000;
constexpr uint32_t BUTTON_HOLD_MS = 5000;

bool wasOnline = false;
bool portalForOutage = false;  // Portal wurde wegen WLAN-Ausfall geoeffnet
bool portalRequested = false;
uint32_t portalOpenedAt = 0;
uint32_t offlineSince = 0;
constexpr uint32_t MANUAL_PORTAL_MAX_MS = 10UL * 60 * 1000;

void onOnline() {
  wasOnline = true;
  offlineSince = 0;
  Serial.printf("[net] online: %s  IP %s  RSSI %d dBm\n", WiFi.SSID().c_str(),
                WiFi.localIP().toString().c_str(), WiFi.RSSI());
  MDNS.end();
  if (MDNS.begin(net::HOSTNAME)) MDNS.addService("http", "tcp", 80);
  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "de.pool.ntp.org", "pool.ntp.org");
  web::begin();
  display::setState(SysState::Online);
}

void onSaveParams() {
  String pw = paramAdminPw.getValue();
  pw.trim();
  if (pw.length() >= 4) {
    settings.adminPassword = pw;
    settings.save();
    Serial.println("[net] Admin-Passwort gespeichert");
  }
}

void checkButton() {
  static uint32_t pressedSince = 0;
  if (digitalRead(PIN_BUTTON) == LOW) {
    if (!pressedSince) pressedSince = millis();
    if (millis() - pressedSince > BUTTON_HOLD_MS && !wm.getConfigPortalActive()) {
      Serial.println("[net] Taste gehalten -> Portal");
      portalRequested = true;
    }
  } else {
    pressedSince = 0;
  }
}
}  // namespace

namespace net {

void begin() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.setAutoReconnect(true);

  wm.setDebugOutput(false);
  wm.setHostname(HOSTNAME);
  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(0);     // Portal bleibt offen, bis eingerichtet
  wm.setConnectTimeout(20);
  wm.setCaptivePortalEnable(true);
  wm.setTitle("Wordclock");
  wm.addParameter(&paramAdminPw);
  wm.setSaveParamsCallback(onSaveParams);
  std::vector<const char*> menu = {"wifi", "info", "restart"};
  wm.setMenu(menu);

  display::setState(SysState::Connecting);
  if (wm.autoConnect(AP_NAME)) {
    onOnline();
  } else {
    Serial.printf("[net] kein WLAN -> Portal '%s' (http://192.168.4.1)\n", AP_NAME);
    display::setState(SysState::Portal);
  }
}

void openPortal() {
  web::end();  // Port 80 fuer das Portal freigeben
  portalForOutage = false;
  wm.startConfigPortal(AP_NAME);
  portalOpenedAt = millis();
  display::setState(SysState::Portal);
}

void requestPortal() { portalRequested = true; }

void loop() {
  wm.process();
  checkButton();
  if (portalRequested) {
    portalRequested = false;
    if (!wm.getConfigPortalActive()) openPortal();
  }

  bool connected = WiFi.status() == WL_CONNECTED;
  bool portal = wm.getConfigPortalActive();

  if (connected && !wasOnline) {
    if (portal) wm.stopConfigPortal();
    onOnline();
  } else if (connected && portal && portalForOutage) {
    Serial.println("[net] WLAN wieder da -> Portal schliessen");
    wm.stopConfigPortal();
    portalForOutage = false;
    display::setState(SysState::Online);
  } else if (!connected && wasOnline) {
    if (!offlineSince) {
      offlineSince = millis();
      Serial.println("[net] WLAN verloren, versuche Reconnect");
      display::setState(SysState::Connecting);
    } else if (!portal && millis() - offlineSince > LOST_BEFORE_PORTAL_MS) {
      Serial.println("[net] seit 5 min offline -> Portal zusaetzlich oeffnen");
      openPortal();
      portalForOutage = true;
    }
  } else if (connected && offlineSince) {
    offlineSince = 0;
    display::setState(SysState::Online);
  }

  // Manuell geoeffnetes Portal schliesst sich nach 10 min, wenn das WLAN noch geht.
  if (connected && portal && !portalForOutage && millis() - portalOpenedAt > MANUAL_PORTAL_MAX_MS) {
    Serial.println("[net] Portal-Zeitlimit -> schliessen");
    wm.stopConfigPortal();
    display::setState(SysState::Online);
  }
  if (connected && wasOnline && !portal && !web::running()) web::begin();
}

bool online() { return WiFi.status() == WL_CONNECTED; }
bool portalActive() { return wm.getConfigPortalActive(); }

}  // namespace net
