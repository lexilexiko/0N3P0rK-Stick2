#include "status_led.h"
#include <M5Unified.h>
#include <Preferences.h>

bool statusLedWolfIsVisible();

namespace StatusLed {
namespace {
constexpr uint32_t CAPTURE_ACTIVITY_MS = 1200;
constexpr uint32_t BLINK_PERIOD_MS = 500;
constexpr uint32_t BLINK_ON_MS = 120;
Preferences prefs;
bool enabled = true;
bool initialized = false;
bool outputOn = false;
bool captureSeen = false;
uint32_t lastCaptureMs = 0;
bool activityActive = false;
uint32_t activityStartedMs = 0;

void writeLed(bool on) {
    if (!initialized || outputOn == on) return;
    outputOn = on;
    M5.Power.setLed(on ? 255 : 0);
}
}

void begin() {
    M5.Power.setLed(0);
    outputOn = false;
    initialized = true;
    if (prefs.begin("statusled", false)) {
        enabled = prefs.getBool("enabled", true);
        prefs.end();
    } else {
        enabled = true;
    }
    captureSeen = false;
    activityActive = false;
    writeLed(false);
}

bool isEnabled() { return enabled; }

void setEnabled(bool value) {
    enabled = value;
    captureSeen = false;
    activityActive = false;
    if (prefs.begin("statusled", false)) {
        prefs.putBool("enabled", enabled);
        prefs.end();
    }
    if (!enabled) writeLed(false);
}

void captureEvent() {
    if (!enabled) return;
    captureSeen = true;
    lastCaptureMs = millis();
}

void update() {
    if (!initialized) return;
    if (!enabled) {
        captureSeen = false;
        activityActive = false;
        writeLed(false);
        return;
    }

    const uint32_t now = millis();
    const bool captureActive = captureSeen &&
        (uint32_t)(now - lastCaptureMs) < CAPTURE_ACTIVITY_MS;
    if (!captureActive) captureSeen = false;

    if (!captureActive && !statusLedWolfIsVisible()) {
        activityActive = false;
        writeLed(false);
        return;
    }

    if (!activityActive) {
        activityActive = true;
        activityStartedMs = now;
    }
    const uint32_t blinkMs = (now - activityStartedMs) % BLINK_PERIOD_MS;
    writeLed(blinkMs < BLINK_ON_MS);
}
}
