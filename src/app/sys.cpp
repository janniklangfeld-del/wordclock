#include "sys.h"
#include <Preferences.h>
#include <esp_system.h>
#include <esp_task_wdt.h>

namespace {
const char* reason = "?";
uint32_t bootCount = 0, crashCount = 0;
bool watched = false;
}  // namespace

namespace sys {

void begin() {
  bool crash = false;
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON: reason = "Strom eingesteckt"; break;
    case ESP_RST_SW: reason = "Neustart (Update/Knopf)"; break;
    case ESP_RST_PANIC: reason = "Absturz"; crash = true; break;
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT: reason = "Wachhund (Uhr hing)"; crash = true; break;
    case ESP_RST_BROWNOUT: reason = "Spannungseinbruch (Netzteil/Kabel)"; crash = true; break;
    case ESP_RST_DEEPSLEEP: reason = "Tiefschlaf"; break;
    case ESP_RST_EXT: reason = "Reset-Taste"; break;
    default: reason = "unbekannt"; break;
  }
  Preferences p;
  p.begin("sys", false);
  bootCount = p.getUInt("boots", 0) + 1;
  crashCount = p.getUInt("crashes", 0) + (crash ? 1 : 0);
  p.putUInt("boots", bootCount);
  p.putUInt("crashes", crashCount);
  p.end();
  Serial.printf("[sys] Start #%lu, Grund: %s (unerwartet bisher: %lu)\n", (unsigned long)bootCount, reason,
                (unsigned long)crashCount);
  esp_task_wdt_init(WDT_SECONDS, true);  // Zeitlimit fuer alle ueberwachten Tasks
  resumeWatchdog();
}

void feed() {
  if (watched) esp_task_wdt_reset();
}

void pauseWatchdog() {
  if (!watched) return;
  esp_task_wdt_delete(NULL);
  watched = false;
}

void resumeWatchdog() {
  if (watched) return;
  watched = esp_task_wdt_add(NULL) == ESP_OK;
  feed();
}

const char* resetReason() { return reason; }
uint32_t boots() { return bootCount; }
uint32_t crashes() { return crashCount; }

}  // namespace sys
