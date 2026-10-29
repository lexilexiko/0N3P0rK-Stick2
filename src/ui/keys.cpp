// StickC Plus 2 button input вЂ” emulates a minimal keyboard over BtnA/BtnB/PWR.
#include "keys.h"

namespace Keys {

static char   s_lastChar = 0;
static uint32_t s_lastCharTime = 0;
static bool   s_prevA = false, s_prevB = false, s_prevPWR = false;

void begin() {
    M5.update();
    s_prevA  = M5.BtnA.isPressed();
    s_prevB  = M5.BtnB.isPressed();
    s_prevPWR = M5.BtnPWR.isPressed();
    s_lastChar = 0;
}

void update() {
    M5.update();
    s_prevA  = M5.BtnA.isPressed();
    s_prevB  = M5.BtnB.isPressed();
    s_prevPWR = M5.BtnPWR.isPressed();
    // Clear the latched char after a short debounce window so a single press
    // produces one typed char.
    if (s_lastChar && (millis() - s_lastCharTime) > 180) {
        s_lastChar = 0;
    }
}

char typedChar() {
    if (M5.BtnA.wasPressed())  { s_lastChar = 'a'; s_lastCharTime = millis(); return 'a'; }
    if (M5.BtnB.wasPressed())  { s_lastChar = 27; s_lastCharTime = millis(); return 27; } // ESC
    if (M5.BtnPWR.wasClicked()){ s_lastChar = 'm'; s_lastCharTime = millis(); return 'm'; }
    return s_lastChar;
}

KeysState keysState() {
    KeysState st{};
    char c = typedChar();
    if (c) st.word[0] = c;
    st.enter = M5.BtnA.wasPressed();
    st.del   = M5.BtnB.wasPressed();
    st.fn    = M5.BtnPWR.wasClicked();
    return st;
}

bool isPressedAny() {
    return M5.BtnA.isPressed() || M5.BtnB.isPressed() || M5.BtnPWR.isPressed();
}

bool isPressed() {
    return isPressedAny();
}

bool isChanged() {
    bool curA  = M5.BtnA.isPressed();
    bool curB  = M5.BtnB.isPressed();
    bool curPWR = M5.BtnPWR.isPressed();
    return (curA != s_prevA) || (curB != s_prevB) || (curPWR != s_prevPWR);
}

}  // namespace Keys
