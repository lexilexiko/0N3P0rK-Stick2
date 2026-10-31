#pragma once
#include <M5Unified.h>

// StickC Plus 2 has three buttons. One scheme everywhere:
//   A (GPIO 37)  next / down / walk right. Short tap on the farm = jump.
//   B (GPIO 39)  previous / up / walk left. Short tap on the farm = sit.
//   PWR (GPIO 35) short = OK / open the menu. Hold = back.
// The old keyboard letters (; . enter ESC) are aliases of these three.

namespace Keys {

void begin();
void update();

bool ok();        // PWR short click
bool back();      // PWR hold
bool next();      // A press edge
bool prev();      // B press edge
bool tapA();      // A short click
bool tapB();      // B short click
bool holdA();     // A held long enough to walk
bool holdB();     // B held long enough to walk

bool keyNewPress(bool& latch);

bool btnA();
bool btnB();
bool btnPWR();

bool isKeyPressed(char c);

struct KeysState {
    char word[8] = {0};
    bool enter = false;
    bool del = false;
    bool fn  = false;
};

char typedChar();
KeysState keysState();
bool isPressedAny();
bool isPressed();
bool isChanged();

}  // namespace Keys

inline bool keyEsc()  { return Keys::back(); }
inline bool keyEnter(){ return Keys::ok(); }
inline bool keyMin()  { return false; }
inline bool keyNewPress(bool& latch) { return Keys::keyNewPress(latch); }
