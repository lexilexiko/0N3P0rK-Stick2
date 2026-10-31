#pragma once
#include "button_mapper.h"

// Compatibility facade. New code should use ButtonMapper directly.
namespace Keys {
using KeysState = ButtonMapper::KeysState;
inline void begin(){ ButtonMapper::begin(); }
inline void update(){ ButtonMapper::update(); }
inline bool ok(){ return ButtonMapper::ok(); }
inline bool back(){ return ButtonMapper::back(); }
inline bool next(){ return ButtonMapper::next(); }
inline bool prev(){ return ButtonMapper::prev(); }
inline bool tapA(){ return ButtonMapper::tapA(); }
inline bool tapB(){ return ButtonMapper::tapB(); }
inline bool holdA(){ return ButtonMapper::holdA(); }
inline bool holdB(){ return ButtonMapper::holdB(); }
inline bool longA(){ return ButtonMapper::longA(); }
inline bool longB(){ return ButtonMapper::longB(); }
inline bool longPower(){ return ButtonMapper::longPower(); }
inline bool comboAB(){ return ButtonMapper::comboAB(); }
inline bool bothAB(){ return ButtonMapper::bothAB(); }
inline bool keyNewPress(bool& l){ return ButtonMapper::keyNewPress(l); }
inline bool btnA(){ return ButtonMapper::btnA(); }
inline bool btnB(){ return ButtonMapper::btnB(); }
inline bool btnPWR(){ return ButtonMapper::btnPWR(); }
inline bool isKeyPressed(char c){ return ButtonMapper::isKeyPressed(c); }
inline char typedChar(){ return ButtonMapper::typedChar(); }
inline KeysState keysState(){ return ButtonMapper::keysState(); }
inline bool isPressedAny(){ return ButtonMapper::isPressedAny(); }
inline bool isPressed(){ return ButtonMapper::isPressed(); }
inline bool isChanged(){ return ButtonMapper::isChanged(); }
}
inline bool keyEsc(){ return ButtonMapper::back(); }
inline bool keyEnter(){ return ButtonMapper::ok(); }
inline bool keyMin(){ return false; }
inline bool keyNewPress(bool& latch){ return ButtonMapper::keyNewPress(latch); }
