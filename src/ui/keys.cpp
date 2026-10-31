// StickC Plus 2: three buttons, edges latched for the whole frame.
#include "keys.h"

namespace Keys {

static constexpr uint32_t kHoldMs = 180;

static bool s_aDown = false, s_bDown = false, s_pwrDown = false;
static bool s_aEdge = false, s_bEdge = false, s_pwrEdge = false;
static bool s_aClick = false, s_bClick = false, s_pwrClick = false;
static bool s_pwrHold = false;
static bool s_prevA = false, s_prevB = false, s_prevPwr = false;

void begin() {
    M5.update();
    s_prevA = M5.BtnA.isPressed();
    s_prevB = M5.BtnB.isPressed();
    s_prevPwr = M5.BtnPWR.isPressed();
}

void update() {
    M5.update();
    s_aDown = M5.BtnA.isPressed();
    s_bDown = M5.BtnB.isPressed();
    s_pwrDown = M5.BtnPWR.isPressed();
    s_aEdge = M5.BtnA.wasPressed();
    s_bEdge = M5.BtnB.wasPressed();
    s_pwrEdge = M5.BtnPWR.wasPressed();
    s_aClick = M5.BtnA.wasClicked();
    s_bClick = M5.BtnB.wasClicked();
    s_pwrClick = M5.BtnPWR.wasClicked();
    s_pwrHold = M5.BtnPWR.wasHold();
}

bool ok() { return s_pwrClick; }
bool back() { return s_pwrHold; }
bool next() { return s_aEdge; }
bool prev() { return s_bEdge; }
bool tapA() { return s_aClick; }
bool tapB() { return s_bClick; }
bool holdA() { return s_aDown && M5.BtnA.pressedFor(kHoldMs); }
bool holdB() { return s_bDown && M5.BtnB.pressedFor(kHoldMs); }

bool btnA() { return s_aDown; }
bool btnB() { return s_bDown; }
bool btnPWR() { return s_pwrDown; }

bool keyNewPress(bool& latch) {
    bool edge = s_aEdge || s_bEdge || s_pwrEdge || s_aClick || s_bClick || s_pwrClick || s_pwrHold;
    bool down = s_aDown || s_bDown || s_pwrDown;
    if (!down && !edge) {
        latch = false;
        return false;
    }
    if (!edge) return false;
    latch = true;
    return true;
}

bool isKeyPressed(char c) {
    switch (c) {
        case '.': return s_aDown;                 // down / next
        case ';': return s_bDown;                 // up / previous
        case 27: case '~': case '`': return s_pwrHold;
        case 'm': case 'M': return s_pwrClick;
        default: return false;
    }
}

char typedChar() { return 0; }

KeysState keysState() {
    KeysState st{};
    st.enter = s_pwrClick;
    st.del = s_pwrHold;
    st.fn = false;
    return st;
}

bool isPressedAny() { return s_aDown || s_bDown || s_pwrDown; }
bool isPressed() { return isPressedAny(); }

bool isChanged() {
    bool changed = (s_aDown != s_prevA) || (s_bDown != s_prevB) || (s_pwrDown != s_prevPwr);
    s_prevA = s_aDown;
    s_prevB = s_bDown;
    s_prevPwr = s_pwrDown;
    return changed;
}

}  // namespace Keys
