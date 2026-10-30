// board/board.cpp
#include "board.h"
#include <Arduino.h>
#include <M5Unified.h>
#include <esp_system.h>

namespace Board {

const char* name() {
    return BOARD_NAME;
}

const char* modelLabel() {
    return "StickC Plus 2";
}

bool isAdv() { return false; }

const char* chip() {
    const char* m = ESP.getChipModel();
    return m ? m : "ESP32";
}

uint8_t defaultButtonGpio() {
    return (uint8_t)BUTTON_GPIO_DEFAULT;
}

bool gpioAllowed(int gpio) {
    if (gpio < 0) return false;
#if CONFIG_IDF_TARGET_ESP32C3
    return gpio <= 21;
#elif CONFIG_IDF_TARGET_ESP32C6
    return gpio <= 30;
#elif CONFIG_IDF_TARGET_ESP32S3
    return gpio <= 48;
#elif CONFIG_IDF_TARGET_ESP32S2
    return gpio <= 46;
#else
    if (gpio >= 6 && gpio <= 11) return false;
    return gpio <= 39;
#endif
}

uint32_t flashBytes() {
    return (uint32_t)ESP.getFlashChipSize();
}

void startKeyboard() {
    // No QWERTY keyboard on StickC Plus 2 вЂ” buttons are handled by Keys::begin().
    // Keep the HOLD pin (GPIO 4) high so the device stays powered on battery.
    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);
    Serial.printf("[KB] StickC Plus 2 (BtnA=37 BtnB=39 PWR=35)\n");
}

} // namespace Board

