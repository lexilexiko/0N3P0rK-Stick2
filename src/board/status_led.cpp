#include "status_led.h"
#include <Preferences.h>

bool statusLedWolfIsVisible();

namespace StatusLed {
namespace {
constexpr uint8_t LED_PIN = 19;
constexpr uint32_t CAPTURE_PULSE_MS = 120;
Preferences prefs;
bool enabled = true;
bool initialized = false;
bool outputOn = false;
uint32_t captureUntil = 0;

void writeLed(bool on) {
    if (!initialized || outputOn == on) return;
    outputOn = on;
    // GPIO 19 LED is active-low.
    digitalWrite(LED_PIN, on ? LOW : HIGH);
}
}

void begin() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    outputOn = false;
    initialized = true;
    if (prefs.begin("statusled", false)) {
        enabled = prefs.getBool("enabled", true);
        prefs.end();
    } else {
        enabled = true;
    }
    captureUntil = 0;
    writeLed(false);
}

bool isEnabled() { return enabled; }

void setEnabled(bool value) {
    enabled = value;
    captureUntil = 0;
    if (prefs.begin("statusled", false)) {
        prefs.putBool("enabled", enabled);
        prefs.end();
    }
    if (!enabled) writeLed(false);
}

void captureEvent() {
    if (!enabled) return;
    captureUntil = millis() + CAPTURE_PULSE_MS;
}

void update() {
    if (!initialized) return;
    if (!enabled) {
        writeLed(false);
        return;
    }

    // A visible wolf takes priority over capture activity: steady red.
    if (statusLedWolfIsVisible()) {
        writeLed(true);
        return;
    }

    const bool pulse = captureUntil != 0 && (int32_t)(captureUntil - millis()) > 0;
    if (!pulse) captureUntil = 0;
    writeLed(pulse);
}
}
