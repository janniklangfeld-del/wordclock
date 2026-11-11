#include "settings.h"
#include <Preferences.h>

Settings settings;

namespace {
const char* const MODES[] = {"clock", "effect", "custom"};
const char* const COLOR_MODES[] = {"solid", "rainbow", "gradient", "cycle", "words"};
const char* const GRAD_DIRS[] = {"h", "v", "d"};
const char* const BGS[] = {"off", "color", "effect"};
const char* const EFFECTS[] = {"rainbow", "plasma", "sparkle", "matrix", "fire", "breathe", "hearts", "confetti"};
const char* const TRANSITIONS[] = {"none", "fade", "wipe", "diag", "random", "matrix", "type"};
const char* const PREFIXES[] = {"always", "hourly", "never"};
const char* const QUARTERS[] = {"nach", "viertel"};
const char* const THREE_QUARTERS[] = {"vor", "dreiviertel"};
const char* const TWENTIES[] = {"zwanzig", "halb"};

bool dirty = false;
uint32_t dirtySince = 0;

String colorStr(uint32_t c) {
  char b[8];
  snprintf(b, sizeof(b), "#%06lX", (unsigned long)(c & 0xFFFFFF));
  return b;
}

String hmStr(uint16_t m) {
  char b[6];
  snprintf(b, sizeof(b), "%02u:%02u", m / 60, m % 60);
  return b;
}

template <size_t N>
void setEnum(JsonObjectConst o, const char* key, const char* const (&list)[N], uint8_t& field, uint32_t flag,
             uint32_t& ch) {
  const char* s = o[key].as<const char*>();
  if (!s) return;
  for (size_t i = 0; i < N; i++) {
    if (strcmp(s, list[i]) == 0) {
      if (field != i) { field = i; ch |= flag; }
      return;
    }
  }
}

template <typename T>
void setNum(JsonObjectConst o, const char* key, T& field, long lo, long hi, uint32_t flag, uint32_t& ch) {
  JsonVariantConst v = o[key];
  if (!v.is<long>() && !v.is<float>()) return;
  long x = constrain(lroundf(v.as<float>()), lo, hi);
  if ((long)field != x) { field = (T)x; ch |= flag; }
}

void setBool(JsonObjectConst o, const char* key, bool& field, uint32_t flag, uint32_t& ch) {
  JsonVariantConst v = o[key];
  if (!v.is<bool>()) return;
  bool b = v.as<bool>();
  if (field != b) { field = b; ch |= flag; }
}

void setColor(JsonObjectConst o, const char* key, uint32_t& field, uint32_t flag, uint32_t& ch) {
  const char* s = o[key].as<const char*>();
  if (!s || s[0] != '#' || strlen(s) != 7) return;
  char* end = nullptr;
  uint32_t c = strtoul(s + 1, &end, 16);
  if (*end) return;
  if (field != c) { field = c; ch |= flag; }
}

void setString(JsonObjectConst o, const char* key, String& field, size_t maxLen, uint32_t flag, uint32_t& ch) {
  const char* s = o[key].as<const char*>();
  if (!s || strlen(s) > maxLen) return;
  String v(s);
  v.trim();
  if (v.length() && field != v) { field = v; ch |= flag; }
}

void setTime(JsonObjectConst o, const char* key, uint16_t& field, uint32_t flag, uint32_t& ch) {
  const char* s = o[key].as<const char*>();
  int h, m;
  if (!s || sscanf(s, "%d:%d", &h, &m) != 2 || h < 0 || h > 23 || m < 0 || m > 59) return;
  uint16_t v = h * 60 + m;
  if (field != v) { field = v; ch |= flag; }
}
}  // namespace

void Settings::toJson(JsonObject o) const {
  o["name"] = name;
  o["power"] = power;
  o["brightness"] = brightness;
  o["mode"] = MODES[mode];
  o["color"] = colorStr(color);
  o["color2"] = colorStr(color2);
  o["colorMode"] = COLOR_MODES[colorMode];
  o["gradDir"] = GRAD_DIRS[gradDir];
  o["speed"] = speed;
  o["bg"] = BGS[bg];
  o["bgColor"] = colorStr(bgColor);
  o["bgLevel"] = bgLevel;
  o["effect"] = EFFECTS[effect];
  o["transition"] = TRANSITIONS[transition];
  o["transMs"] = transMs;
  o["dots"] = dots;
  o["dotsOwnColor"] = dotsOwnColor;
  o["dotColor"] = colorStr(dotColor);
  o["custom"] = custom;
  o["customStyle"] = customScroll ? "scroll" : "letters";
  o["scrollText"] = scrollText;
  o["scrollSpeed"] = scrollSpeed;
  o["prefix"] = PREFIXES[prefix];
  o["quarter"] = QUARTERS[quarter];
  o["threeQuarter"] = THREE_QUARTERS[threeQuarter];
  o["twenty"] = TWENTIES[twenty];
  o["nightOn"] = nightOn;
  o["nightFrom"] = hmStr(nightFrom);
  o["nightTo"] = hmStr(nightTo);
  o["nightBri"] = nightBri;
  o["nightOff"] = nightOff;
  o["autoOn"] = autoOn;
  o["autoMin"] = autoMin;
  o["autoMax"] = autoMax;
  o["autoLux"] = autoLux;
  o["bdayOn"] = bdayOn;
  char b[6];
  snprintf(b, sizeof(b), "%02u.%02u", bdayDay, bdayMonth);
  o["bday"] = b;
  o["introOn"] = introOn;
  o["introText"] = introText;
  o["tz"] = tz;
  o["ntp"] = ntp;
  o["alexa"] = alexa;
  o["alexaName"] = alexaName;
  o["autoUpdate"] = autoUpdate;
}

uint32_t Settings::applyJson(JsonObjectConst o) {
  uint32_t ch = 0;
  setString(o, "name", name, 40, CH_DISPLAY, ch);
  setBool(o, "power", power, CH_DISPLAY, ch);
  setNum(o, "brightness", brightness, 1, 255, CH_DISPLAY, ch);
  setEnum(o, "mode", MODES, mode, CH_DISPLAY, ch);
  setColor(o, "color", color, CH_DISPLAY, ch);
  setColor(o, "color2", color2, CH_DISPLAY, ch);
  setEnum(o, "colorMode", COLOR_MODES, colorMode, CH_DISPLAY, ch);
  setEnum(o, "gradDir", GRAD_DIRS, gradDir, CH_DISPLAY, ch);
  setNum(o, "speed", speed, 1, 100, CH_DISPLAY, ch);
  setEnum(o, "bg", BGS, bg, CH_DISPLAY, ch);
  setColor(o, "bgColor", bgColor, CH_DISPLAY, ch);
  setNum(o, "bgLevel", bgLevel, 0, 100, CH_DISPLAY, ch);
  setEnum(o, "effect", EFFECTS, effect, CH_DISPLAY, ch);
  setEnum(o, "transition", TRANSITIONS, transition, CH_DISPLAY, ch);
  setNum(o, "transMs", transMs, 200, 5000, CH_DISPLAY, ch);
  setBool(o, "dots", dots, CH_DISPLAY, ch);
  setBool(o, "dotsOwnColor", dotsOwnColor, CH_DISPLAY, ch);
  setColor(o, "dotColor", dotColor, CH_DISPLAY, ch);
  if (const char* c = o["custom"].as<const char*>()) {
    String v;
    for (size_t i = 0; c[i] && i < 110; i++) v += (c[i] == '1') ? '1' : '0';
    if (v != custom) { custom = v; ch |= CH_DISPLAY; }
  }
  if (const char* cs = o["customStyle"].as<const char*>()) {
    bool v = !strcmp(cs, "scroll");
    if (v != customScroll) { customScroll = v; ch |= CH_DISPLAY; }
  }
  setString(o, "scrollText", scrollText, 60, CH_DISPLAY, ch);
  setNum(o, "scrollSpeed", scrollSpeed, 1, 100, CH_DISPLAY, ch);
  setEnum(o, "prefix", PREFIXES, prefix, CH_DISPLAY, ch);
  setEnum(o, "quarter", QUARTERS, quarter, CH_DISPLAY, ch);
  setEnum(o, "threeQuarter", THREE_QUARTERS, threeQuarter, CH_DISPLAY, ch);
  setEnum(o, "twenty", TWENTIES, twenty, CH_DISPLAY, ch);
  setBool(o, "nightOn", nightOn, CH_DISPLAY, ch);
  setTime(o, "nightFrom", nightFrom, CH_DISPLAY, ch);
  setTime(o, "nightTo", nightTo, CH_DISPLAY, ch);
  setNum(o, "nightBri", nightBri, 1, 255, CH_DISPLAY, ch);
  setBool(o, "nightOff", nightOff, CH_DISPLAY, ch);
  setBool(o, "autoOn", autoOn, CH_DISPLAY, ch);
  setNum(o, "autoMin", autoMin, 1, 255, CH_DISPLAY, ch);
  setNum(o, "autoMax", autoMax, 1, 255, CH_DISPLAY, ch);
  setNum(o, "autoLux", autoLux, 10, 5000, CH_DISPLAY, ch);
  setBool(o, "bdayOn", bdayOn, CH_DISPLAY, ch);
  if (const char* b = o["bday"].as<const char*>()) {
    int d, m;
    if (sscanf(b, "%d.%d", &d, &m) == 2 && d >= 1 && d <= 31 && m >= 1 && m <= 12 &&
        (d != bdayDay || m != bdayMonth)) {
      bdayDay = d;
      bdayMonth = m;
      ch |= CH_DISPLAY;
    }
  }
  setBool(o, "introOn", introOn, CH_SYSTEM, ch);
  setString(o, "introText", introText, 24, CH_SYSTEM, ch);
  setString(o, "tz", tz, 60, CH_TIME, ch);
  setString(o, "ntp", ntp, 60, CH_TIME, ch);
  setBool(o, "alexa", alexa, CH_ALEXA, ch);
  setString(o, "alexaName", alexaName, 32, CH_ALEXA, ch);
  setBool(o, "autoUpdate", autoUpdate, CH_SYSTEM, ch);
  return ch;
}

void Settings::load() {
  Preferences p;
  p.begin("wordclock", true);
  String json = p.getString("cfg", "");
  adminPassword = p.getString("adminpw", adminPassword);
  p.end();
  if (adminPassword.isEmpty()) adminPassword = "wordclock";
  if (json.length()) {
    JsonDocument doc;
    if (!deserializeJson(doc, json)) applyJson(doc.as<JsonObjectConst>());
  }
}

void Settings::save() const {
  JsonDocument doc;
  toJson(doc.to<JsonObject>());
  String json;
  serializeJson(doc, json);
  Preferences p;
  p.begin("wordclock", false);
  p.putString("cfg", json);
  p.putString("adminpw", adminPassword);
  p.end();
  dirty = false;
}

void Settings::requestSave() {
  dirty = true;
  dirtySince = millis();
}

void Settings::loop() {
  if (dirty && millis() - dirtySince > 2000) save();
}
