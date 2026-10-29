#pragma once
#include <Arduino.h>
#include <LittleFS.h>

namespace SDLayout {
inline bool usingNewLayout() { return true; }
inline void setUseNewLayout(bool) {}

inline const char* rootDir() { return "/0N3P0rK"; }
inline const char* handshakesDir() { return "/0N3P0rK/handshakes"; }
inline const char* passworldDir() { return "/0N3P0rK/Passworld"; }
inline const char* pigpassDir() { return "/0N3P0rK/pigpass"; }
inline const char* pigpassResultsPath() { return "/0N3P0rK/pigpass/cracked.txt"; }
inline const char* pigpassCheckpointPath() { return "/0N3P0rK/pigpass/checkpoint.txt"; }
inline const char* pigpassLastWordlistPath() { return "/0N3P0rK/pigpass/last_wl.txt"; }
inline const char* evilpigDir() { return "/0N3P0rK/evilpig"; }
inline const char* evilpigCredsPath() { return "/0N3P0rK/evilpig/creds.csv"; }
inline const char* screenshotsDir() { return "/0N3P0rK/screenshots"; }
inline const char* inspectorDir() { return "/0N3P0rK/inspector"; }

inline void ensureDirs() {
    LittleFS.mkdir("/0N3P0rK");
    LittleFS.mkdir("/0N3P0rK/handshakes");
    LittleFS.mkdir("/0N3P0rK/wpa-sec");
    LittleFS.mkdir("/0N3P0rK/pwncrack");
    LittleFS.mkdir("/0N3P0rK/ohc");
    LittleFS.mkdir("/0N3P0rK/inspector");
    LittleFS.mkdir("/0N3P0rK/pigpass");
    LittleFS.mkdir("/0N3P0rK/Passworld");
    LittleFS.mkdir("/0N3P0rK/evilpig");
    LittleFS.mkdir("/0N3P0rK/ir");
    LittleFS.mkdir("/0N3P0rK/wolf");
    LittleFS.mkdir("/0N3P0rK/talk");
    LittleFS.mkdir("/0N3P0rK/music");
    LittleFS.mkdir("/0N3P0rK/screenshots");
}
}
