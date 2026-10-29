#include "led.h"
#include "../core/config.h"
#include "../core/app.h"
#include "../cap/sniffer.h"

// StickC Plus 2 has no programmable WS2812 RGB LED (GPIO 21 does not exist).
// The only LED is a red indicator on GPIO 19 shared with the IR emitter, and a
// non-programmable green sleep LED. To keep the LED API intact without
// interfering with IR, the LED is a no-op status stub on this board.

namespace Led {

static bool s_inited = false;

void applyBrightness() { (void)0; }

void begin() {
    s_inited = true;
}

void off() { }

void setRgb(uint8_t, uint8_t, uint8_t) { }

void pulse(uint8_t, uint8_t, uint8_t, uint16_t) { }

void update() {
    const PersonalityConfig& p = Config::personality();
    if (!p.ledEnabled) return;
    // No RGB LED available on StickC Plus 2 — stay dark.
}

}  // namespace Led
