#include "settings.h"
#include <Preferences.h>

Settings settings;

void Settings::load() {
  Preferences p;
  p.begin("wordclock", true);
  adminPassword = p.getString("adminpw", adminPassword);
  autoUpdate = p.getBool("autoupd", autoUpdate);
  brightness = p.getUChar("bright", brightness);
  p.end();
  if (adminPassword.isEmpty()) adminPassword = "wordclock";
}

void Settings::save() const {
  Preferences p;
  p.begin("wordclock", false);
  p.putString("adminpw", adminPassword);
  p.putBool("autoupd", autoUpdate);
  p.putUChar("bright", brightness);
  p.end();
}
