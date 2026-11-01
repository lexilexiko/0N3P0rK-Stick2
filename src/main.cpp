// 0N3P0rK - Tamagotchi pig + barn on M5StickC Plus 2

#include <M5Unified.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <nvs.h>
#include "build_info.h"
#include "core/config.h"
#include "core/xp.h"
#include "core/app.h"
#include "ui/display.h"
#include "ui/menu.h"
#include "piglet/avatar.h"
#include "piglet/mood.h"
#include "audio/sfx.h"
#include "storage/littlefs_ops.h"
#include "board/board.h"
#include "net/ap_sta.h"
#include "cap/sniffer.h"
#include "modes/evilpig.h"
#include "modes/pigpass.h"
#include "ui/keys.h"
#include "board/status_led.h"
#include "piglet/wolf.h"
#include "piglet/scene_layers.h"

static void bootLed(bool on) {
    // StickC Plus 2 red LED is GPIO 19, active low.
    pinMode(19, OUTPUT);
    digitalWrite(19, on ? LOW : HIGH);
}

static void bootBlinks(int n) {
    for (int i = 0; i < n; ++i) {
        bootLed(true);
        delay(140);
        bootLed(false);
        delay(140);
    }
}

static void bootMark(const char* step) {
    Serial.println(step);
    Serial.flush();
}

static void preInitWiFiDriverEarly() {
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true, false);
    WiFi.setSleep(false);
    delay(50);
}

static void setupHeapLayout() {
    size_t beforeLargest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    Serial.printf("[BOOT] pre-fence largest=%u\n", (unsigned)beforeLargest);
    Serial.flush();

    // PSRAM on the Plus2 module is not enabled in this build, so this
    // fence comes from internal RAM. 80 KB here can reset the chip before
    // anything is drawn. Take at most a third of the largest block.
    size_t fenceSize = beforeLargest / 3;
    if (fenceSize > 48000) fenceSize = 48000;
    void* fence = fenceSize >= 4096
        ? heap_caps_malloc(fenceSize, MALLOC_CAP_8BIT) : nullptr;
    preInitWiFiDriverEarly();
    if (fence) heap_caps_free(fence);

    Serial.printf("[BOOT] post-fence free=%u largest=%u\n",
                  (unsigned)ESP.getFreeHeap(),
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    Serial.flush();
}

void setup() {
    // Plus2 dies on battery unless HOLD stays high, and the backlight is
    // GPIO 27 (not the AXP chip of the old StickC).
    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);
    pinMode(27, OUTPUT);
    digitalWrite(27, HIGH);
    bootBlinks(1);   // 1 blink: CPU is running, setup() started

    Serial.begin(115200);
    delay(80);
    Serial.println();
    Serial.printf(">>> %s v%s build=%s\n", ON3PORK_NAME, ON3PORK_VERSION, ON3PORK_BUILD);
    Serial.flush();

    // A previous boot may have saved "this is a StickC" into M5GFX NVS.
    // That skips the Plus2 probe and leaves the panel dark.
    nvs_handle_t nvs;
    if (nvs_open("M5GFX", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_erase_key(nvs, "AUTODETECT");
        nvs_commit(nvs);
        nvs_close(nvs);
    }
    bootMark("[BOOT] before M5.begin");

    auto cfg = M5.config();
    // If the panel probe misses, the default fallback is Atom Lite and
    // M5.begin() then scans every Grove display (external_display = 0xFFFF).
    cfg.fallback_board = m5::board_t::board_M5StickCPlus2;
    cfg.external_display_value = 0;
    // Plus2 speaker is a passive piezo on GPIO2. The I2S buzzer path
    // squeals and keeps a task awake. Tones go out through LEDC instead.
    // GPIO0 is the mic clock, not a button.
    cfg.internal_spk = false;
    cfg.internal_mic = false;
    M5.begin(cfg);
    // Autodetect restores GPIO state. Hold must stay high or Plus2 cuts power.
    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);
    bootBlinks(2);   // 2 blinks: M5.begin returned
    Serial.printf("[BOOT] M5 board=%d (Plus2 is 5)\n", (int)M5.getBoard());
    M5.Display.setRotation(3);
    M5.Display.setBrightness(255);
    M5.Display.fillScreen(TFT_RED);
    M5.Display.setTextColor(TFT_WHITE, TFT_RED);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(8, 40);
    M5.Display.print("BOOT");
    Serial.printf("[LCD] %dx%d\n", M5.Display.width(), M5.Display.height());
    Serial.flush();
    delay(400);
    Board::startKeyboard();

    setupHeapLayout();
    yield();

    if (!Storage::begin()) {
        bootMark("[BOOT] LittleFS failed - continuing");
    } else {
        bootMark("[BOOT] LittleFS ok");
    }
    ButtonMapper::begin();
    Serial.printf("[BOOT] board=%s\n", Board::modelLabel());
    Serial.flush();

    Config::init();
    StatusLed::begin();
    XP::begin();
    M5.Display.setBrightness(255);

    bootMark("[BOOT] display init");
    Display::init();
    bootBlinks(3);   // 3 blinks: canvas exists
    SFX::init();
    bootMark("[BOOT] avatar");
    Avatar::init();
    Display::showBootSplash();
    Display::refreshBrightness();
    Mood::init();
    Menu::begin();
    App::begin();
    yield();

    bootMark("[BOOT] net");
    Net::begin();
    Storage::loadKeysIntoNet();
    Cap::begin();
    EvilPigMode::init();
    PigpassMode::init();

    Net::Status s = Net::status();
    Serial.printf("[BOOT] wifi mode ssid=%s ip=%s\n", s.ssid, s.ip);
    Serial.printf("[BOOT] pig=%s ready\n", Config::personality().name);
    Serial.flush();
    bootLed(false);
}

static unsigned long s_lastHeapLog = 0;

// StatusLed calls this lightweight bridge to avoid coupling its driver to the farm scene.
bool statusLedWolfIsVisible() {
    return App::mode() == AppMode::FARM && SceneLayers::wolf && Wolf::isActive();
}

void loop() {
    M5.update();
    ButtonMapper::update();
    App::loop();
    Display::update();
    StatusLed::update();
    SFX::update();
    XP::tick();
    Cap::loop();

    if (millis() - s_lastHeapLog > 30000) {
        s_lastHeapLog = millis();
        Serial.printf("[LOOP] heap=%u largest=%u cap=%s\n",
                      (unsigned)ESP.getFreeHeap(),
                      (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
                      Cap::runMode() == Cap::RunMode::Aggressive ? "AGG" :
                      (Cap::isRunning() ? "LITE" : "off"));
        Serial.flush();
    }
    delay(5);
    yield();
}
