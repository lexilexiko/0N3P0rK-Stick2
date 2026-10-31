#pragma once

#include <Arduino.h>

namespace StatusLed {
// Built-in StickC Plus 2 indicator is a single red LED on GPIO 19 (active low).
void begin();
void update();
bool isEnabled();
void setEnabled(bool enabled);
// Call after a frame has been successfully accepted by the capture writer.
void captureEvent();
}
