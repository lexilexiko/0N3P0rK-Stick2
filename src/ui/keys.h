#pragma once
#include <M5Unified.h>

// StickC Plus 2 input вЂ” no QWERTY keyboard. Navigation uses:
//   BtnA (GPIO 37) = SELECT / advance / "a"
//   BtnB (GPIO 39) = BACK / escape / ESC(27)
//   BtnPWR (GPIO 35) = MENU / minimize / power

namespace Keys {

void begin();       // call once in setup()
void update();      // call each loop вЂ” refreshes button edges

// Logical keys (menu navigation)
inline bool keyEsc()  { return M5.BtnB.wasPressed(); }          // back
inline bool keyEnter(){ return M5.BtnA.wasPressed(); }          // select
inline bool keyMin()  { return M5.BtnPWR.wasHold(); }           // minimize / power

// Edge detection for "any new press" вЂ” used by main loop dispatch.
inline bool keyNewPress(bool& latch) {
    bool pressed = M5.BtnA.isPressed() || M5.BtnB.isPressed() || M5.BtnPWR.isPressed();
    if (!pressed) { latch = false; return false; }
    bool changed = M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnPWR.wasPressed();
    if (latch && !changed) return false;
    latch = true;
    return true;
}

// Raw button states (gameplay / direct use)
inline bool btnA()  { return M5.BtnA.isPressed(); }
inline bool btnB()  { return M5.BtnB.isPressed(); }
inline bool btnPWR(){ return M5.BtnPWR.isPressed(); }

// Compat shim: map logical buttons to single chars used by old keyboard hotkeys.
// Only BtnA('a'/' ') and BtnB(ESC 27/'~') and BtnPWR('m') are real on StickC Plus 2.
inline bool isKeyPressed(char c) {
    switch (c) {
        case ' ': case 'a': case 'A': return btnA();
        case 27: case '~': case '`': return btnB();   // esc / back
        case 'm': case 'M': return btnPWR();          // menu
        default: return false;
    }
}

// Emulated keyboard state for app.cpp dispatch.
struct KeysState {
    char word[8] = {0};
    bool enter = false;
    bool del = false;
    bool fn  = false;
};

// Return the last typed char (from buttons) вЂ” 0 if none.
char typedChar();
// Fill st with current emulated state.
KeysState keysState();
bool isPressedAny();
bool isPressed();   // any button currently held (alias for isPressedAny)
bool isChanged();

}  // namespace Keys

// Back-compat aliases so existing call sites keep reading naturally.
inline bool keyEsc()  { return Keys::keyEsc(); }
inline bool keyMin()  { return Keys::keyMin(); }
inline bool keyNewPress(bool& latch) { return Keys::keyNewPress(latch); }

