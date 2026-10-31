#pragma once
#include <M5Unified.h>

// Central physical-button scanner and event mapper for M5StickC Plus 2.
// All hardware sampling, edge detection, long presses and button chords live here.
namespace ButtonMapper {
void begin();
void update();
bool ok();
bool back();
bool next();
bool prev();
bool tapA();
bool tapB();
bool holdA();
bool holdB();
bool longA();
bool longB();
bool longPower();
bool comboAB();       // one-shot event when A+B become held together
bool bothAB();        // true while A+B are both physically held
bool keyNewPress(bool& latch);
bool btnA();
bool btnB();
bool btnPWR();
bool isKeyPressed(char c);
struct KeysState { char word[8] = {0}; bool enter = false; bool del = false; bool fn = false; };
char typedChar();
KeysState keysState();
bool isPressedAny();
bool isPressed();
bool isChanged();
}
