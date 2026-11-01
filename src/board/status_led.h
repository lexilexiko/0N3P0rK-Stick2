#pragma once

#include <Arduino.h>

namespace StatusLed {
// StickC Plus 2 red LED shares GPIO 19 with the IR emitter; use M5.Power.setLed().
void begin();
void update();
bool isEnabled();
void setEnabled(bool enabled);
// Call after a frame has been successfully accepted by the capture writer.
void captureEvent();
}
