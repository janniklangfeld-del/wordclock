#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <FastLED.h>

// "MJ Arcade" – Spiele auf der Uhr, Handys sind die Controller.
//
// Die Uhr ist der Schiedsrichter: Sie nimmt die Tasten aller Spieler ueber den
// WebSocket (Port 81) an, rechnet das Spiel und zeigt es auf den 11 x 10
// Buchstabenfeldern. Die Handys zeigen nur Controller, Punktestand und Statistik.
//
// Ablauf: Lobby (Plaetze belegen, "Bereit") -> 3-2-1 -> Spiel -> Ergebnis.
// Faellt ein Spieler aus der Verbindung, pausiert das Spiel, bis jemand den
// Platz wieder belegt.
//
// Protokoll (Text, JSON) Handy -> Uhr:
//   {"t":"g.open","game":"tetris","mode":"1p|2p|bot"}   Lobby fuer ein Spiel oeffnen
//   {"t":"g.join","slot":0|1,"name":"MJ"}              Platz belegen (-1 = erster freier)
//   {"t":"g.leave"} {"t":"g.ready","on":true} {"t":"g.pause"} {"t":"g.again"} {"t":"g.quit"}
// Binaer Handy -> Uhr (Taste, so schnell wie moeglich): [2, Taste, gedrueckt 0/1]
//   Taste: 0 hoch, 1 runter, 2 links, 3 rechts, 4 A (Aktion/Drehen), 5 B (Fallen lassen)
// Uhr -> Handy: {"t":"game", ...} bei jeder Aenderung (pro Handy, mit "you" = eigener Platz)
namespace games {

enum Key : uint8_t { K_UP, K_DOWN, K_LEFT, K_RIGHT, K_A, K_B, K_COUNT };

void begin();
void loop();
bool active();  // Spielhalle hat die Anzeige (Lobby, Spiel, Ergebnis)
bool busy();    // Countdown oder Spiel laeuft -> Hintergrundarbeit (OTA, Speichern) pausieren
void render(CRGB* leds, uint32_t now);  // nur wenn active()
uint8_t brightness();

// vom WebSocket (web.cpp)
void onText(uint8_t client, JsonObjectConst msg);
void onKey(uint8_t client, uint8_t key, bool down);
void onConnect(uint8_t client);
void onDisconnect(uint8_t client);

void recordsJson(JsonObject out);  // Rekorde + aktueller Stand fuer GET /v1/games

}  // namespace games
