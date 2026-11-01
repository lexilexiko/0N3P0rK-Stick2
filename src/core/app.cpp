#include "app.h"
#include "../ui/task_manager.h"
#include "../ui/display.h"
#include "../ui/menu.h"
#include "../ui/keys.h"
#include "../ui/loot_menu.h"
#include "../ui/settings_menu.h"
#include "../ui/screenshot.h"
#include "../modes/evilpig.h"
#include "../modes/pigpass.h"
#include "../modes/spectrum.h"
#include "../modes/xfer.h"
#include "../modes/filemgr.h"
#include "../piglet/avatar.h"
#include "../piglet/cards_table.h"
#include "../piglet/props.h"
#include "../piglet/credits.h"
#include "../piglet/mood.h"
#include "../piglet/wolf.h"
#include "../piglet/seasonal_fx.h"
#include "../audio/sfx.h"
#include "../core/config.h"
#include <stdio.h>

namespace App {

static AppMode s_mode = AppMode::FARM;
static bool s_winHid = false;
static bool s_minLatch = false;
static bool s_capBarLatch = false;
static bool s_capWasRunning = false;
// Bottom-bar detail mode used while Cap::isRunning() and no overlay is open.
// 0 = full 8-widget carousel (the "big info" we had by default),
// 1 = simple one-liner (LITE/PIN/AGG + CH + HS),
// 2 = line 2 hidden entirely.
static uint8_t s_capBarMode = 0;

bool overlayMode() {
    return s_mode == AppMode::LOOT || s_mode == AppMode::EVILPIG ||
           s_mode == AppMode::PIGPASS || s_mode == AppMode::SPECTRUM ||
           s_mode == AppMode::FILEMGR || s_mode == AppMode::XFER ||
           s_mode == AppMode::TASKS ||
           s_mode == AppMode::PIG ||
           s_mode == AppMode::TUNE || s_mode == AppMode::WIFI;
}

bool windowHidden() { return s_winHid && overlayMode(); }

void setWindowHidden(bool hid) {
    s_winHid = hid && overlayMode();
}

void begin() {
    s_mode = AppMode::FARM;
    Menu::begin();
    TaskManager::begin();
}

AppMode mode() { return s_mode; }

const char* modeName() {
    switch (s_mode) {
        case AppMode::FARM:     return "FARM";
        case AppMode::MENU:     return "MENU";
        case AppMode::ATTACK:   return "ATTACK";
        case AppMode::LOOT:     return "LOOT";
        case AppMode::WIFI:     return "WIFI";
        case AppMode::PIG:      return "PIG";
        case AppMode::TUNE:     return "TUNE";
        case AppMode::EVILPIG:  return "EVILPIG";
        case AppMode::PIGPASS:  return "PIGPASS";
        case AppMode::SPECTRUM: return "SPEC";
        case AppMode::FILEMGR:  return "FILES";
        case AppMode::XFER:     return "XFER";
        case AppMode::TASKS:    return "TASKS";
        default:                return "?";
    }
}

void setMode(AppMode m) {
    if (s_mode == m) return;
    if (s_mode == AppMode::LOOT) LootMenu::hide();
    if (s_mode == AppMode::PIG || s_mode == AppMode::TUNE ||
        s_mode == AppMode::WIFI) SettingsMenu::hide();
    if (s_mode == AppMode::EVILPIG && EvilPigMode::isRunning()) EvilPigMode::stop();
    if (s_mode == AppMode::PIGPASS && PigpassMode::isRunning()) PigpassMode::stop();
    if (s_mode == AppMode::SPECTRUM && SpectrumMode::isRunning()) SpectrumMode::stop();
    if (s_mode == AppMode::FILEMGR && FileMgrMode::isRunning()) FileMgrMode::stop();
    if (s_mode == AppMode::XFER && XferMode::isRunning()) XferMode::stop();
    if (s_mode == AppMode::TASKS && TaskManager::isRunning()) TaskManager::stop();
    s_winHid = false;
    s_mode = m;
    Menu::onEnter(m);
    if (m == AppMode::LOOT) LootMenu::show();
    if (m == AppMode::EVILPIG) EvilPigMode::start();
    if (m == AppMode::PIGPASS) PigpassMode::start();
    if (m == AppMode::SPECTRUM) SpectrumMode::start();
    if (m == AppMode::FILEMGR) FileMgrMode::start();
    if (m == AppMode::XFER) XferMode::start();
    if (m == AppMode::TASKS) TaskManager::start();
    SFX::play(m == AppMode::FARM ? SFX::MODE_EXIT : SFX::MODE_ENTER);
}

// SETTINGS в†’ ANIM TEST: - previous / = next demo (OnePork lab).
static void animTestPoll() {
    if (!Config::personality().animTest) return;
    static bool minusWas = false;
    static bool eqWas = false;
    bool minus = ButtonMapper::btnB() || ButtonMapper::isKeyPressed('-') || ButtonMapper::isKeyPressed('_');
    bool eq = ButtonMapper::btnA() || ButtonMapper::isKeyPressed('=') || ButtonMapper::isKeyPressed('+');
    bool prev = minus && !minusWas;
    bool next = eq && !eqWas;
    minusWas = minus;
    eqWas = eq;
    if (!prev && !next) return;

    static const char* const kAnimNames[] = {
        "NEUTRAL", "HAPPY", "EXCITED", "HUNTING", "SLEEPY", "SAD", "ANGRY",
        "BLINK", "SNIFF", "JUMP", "PERK UP", "FLINCH", "SPIN", "PAW SCRATCH",
        "TAIL WIGGLE", "SPARKLES", "ATTACK HOP", "WAVE IN", "WAVE OUT",
        "FACE LEFT", "FACE RIGHT", "TREE ON", "TREE OFF", "WOLF",
        "SIT", "PLAY DEAD", "STAND",
        "PROP HIVE", "PROP SNOWMAN", "PROP FOX", "PROP FIRE",
        "PROP CAT", "PROP SKULL", "PROP CLEAR", "METEOR SHOWER",
    };
    static const uint8_t kAnimCount =
        (uint8_t)(sizeof(kAnimNames) / sizeof(kAnimNames[0]));
    static int8_t s_animIdx = 0;
    if (next) s_animIdx = (int8_t)((s_animIdx + 1) % kAnimCount);
    else s_animIdx = (int8_t)((s_animIdx - 1 + kAnimCount) % kAnimCount);

    Avatar::setAttackShake(false, false);
    Avatar::setThunderFlash(false);
    Avatar::setMicDance(0.0f);
    Avatar::waveRipple(WaveMode::NONE, 0);

    switch (s_animIdx) {
        case 0: Avatar::setState(AvatarState::NEUTRAL); break;
        case 1: Avatar::setState(AvatarState::HAPPY); break;
        case 2: Avatar::setState(AvatarState::EXCITED); break;
        case 3: Avatar::setState(AvatarState::HUNTING); break;
        case 4: Avatar::setState(AvatarState::SLEEPY); break;
        case 5: Avatar::setState(AvatarState::SAD); break;
        case 6: Avatar::setState(AvatarState::ANGRY); break;
        case 7: Avatar::setState(AvatarState::HAPPY); Avatar::blink(); break;
        case 8: Avatar::setState(AvatarState::HAPPY); Avatar::sniff(); break;
        case 9: Avatar::setState(AvatarState::EXCITED); Avatar::cuteJump(); break;
        case 10: Avatar::setState(AvatarState::HAPPY); Avatar::perkUp(); break;
        case 11: Avatar::setState(AvatarState::SAD); Avatar::flinch(); break;
        case 12: Avatar::setState(AvatarState::EXCITED); Avatar::spin(); break;
        case 13: Avatar::setState(AvatarState::NEUTRAL); Avatar::pawScratch(); break;
        case 14: Avatar::setState(AvatarState::HAPPY); Avatar::triggerTailWiggle(); break;
        case 15: Avatar::setState(AvatarState::EXCITED); Avatar::triggerSparkles(8); break;
        case 16: Avatar::setState(AvatarState::HUNTING); Avatar::attackHop(); break;
        case 17: Avatar::setState(AvatarState::HUNTING); Avatar::waveRipple(WaveMode::INCOMING, 4); break;
        case 18: Avatar::setState(AvatarState::ANGRY); Avatar::waveRipple(WaveMode::OUTGOING, 4); break;
        case 19: Avatar::setState(AvatarState::NEUTRAL); Avatar::setFacingLeft(); break;
        case 20: Avatar::setState(AvatarState::NEUTRAL); Avatar::setFacingRight(); break;
        case 21: Avatar::setState(AvatarState::HAPPY); Avatar::showTree(5); break;
        case 22: Avatar::setState(AvatarState::NEUTRAL); Avatar::hideTree(); break;
        case 23:
            Avatar::setPlayDead(false);
            Avatar::setSitting(false);
            Avatar::setState(AvatarState::EXCITED);
            Wolf::spawnNow();
            break;
        case 24:
            Avatar::setPlayDead(false);
            Avatar::setSitting(true);
            Avatar::setState(AvatarState::HAPPY);
            break;
        case 25:
            Avatar::setSitting(false);
            Avatar::setPlayDead(true);
            break;
        case 26:
            Avatar::setSitting(false);
            Avatar::setPlayDead(false);
            Avatar::setState(AvatarState::NEUTRAL);
            break;
        case 27: Props::forceDemo(0); break;  // hive
        case 28: Props::forceDemo(1); break;  // snowman
        case 29: Props::forceDemo(2); break;  // fox
        case 30: Props::forceDemo(3); break;  // fire
        case 31: Props::forceDemo(4); break;  // cat
        case 32: Props::forceDemo(5); break;  // skull
        case 33: Props::forceDemo(6); break;  // clear
        case 34: SeasonalFx::triggerMeteorShower(); break;
        default: break;
    }

    char msg[36];
    snprintf(msg, sizeof(msg), "ANIM %d/%u  %s",
             (int)(s_animIdx + 1), (unsigned)kAnimCount, kAnimNames[s_animIdx]);
    Display::showToast(msg, 1200);
}

// Same idle roam as OnePork:
//   , left hold    / right hold
//   ; jump         SPACE attack-hop
//   . sit hold
//   ANIM TEST: - previous  = next
static void farmPoll() {
    if (Credits::isPlaying()) return;  // unskippable credits
    if (CardsTable::isActive()) return;  // duel owns keys + draw
    animTestPoll();
    // Hold A/B to walk, tap A to jump, tap B to sit, and tap PWR to attack.
    bool left  = ButtonMapper::holdB();
    bool right = ButtonMapper::holdA();
    bool jumpKey = ButtonMapper::tapA();
    bool attackKey = ButtonMapper::ok();
    bool sitKey = ButtonMapper::tapB();

    static bool idleJumpWas = false;
    static bool idleAttackWas = false;
    static bool sitWasDown = false;
    static bool digPressArmed = false;
    static uint32_t idleMoveMs = 0;
    bool jumpEdge = jumpKey && !idleJumpWas;
    bool attackEdge = attackKey && !idleAttackWas;
    bool sitEdge = sitKey && !sitWasDown;
    idleJumpWas = jumpKey;
    idleAttackWas = attackKey;
    sitWasDown = sitKey;

    const bool locked = Avatar::isControlLocked() || Avatar::isPlayDead();
    const bool airborne = Avatar::isJumping() || Avatar::isAttackHopping();

    if (sitEdge) {
        digPressArmed = !locked && !airborne && Avatar::isSitPoseComplete();
        if (!locked && !airborne && !Avatar::isSitting()) Avatar::setSitting(true);
    }
    if (!sitKey || locked || airborne || left || right || jumpKey || attackKey)
        digPressArmed = false;

    const bool digging = digPressArmed && sitKey;
    Avatar::setDigging(digging);

    uint32_t nowMove = millis();
    if (nowMove - idleMoveMs >= 12) {
        idleMoveMs = nowMove;
        static int lastHold = 0;
        int hold = 0;
        if (!locked && left && !right) hold = -1;
        else if (!locked && right && !left) hold = 1;
        if (hold != 0) Avatar::playerWalkHold(hold);
        else if (lastHold != 0) Avatar::playerWalkHold(0);
        lastHold = hold;
    }

    if (!locked && jumpEdge && !Avatar::isJumping() && !Avatar::isAttackHopping()) {
        Avatar::setPlayDead(false);
        Avatar::setSitting(false);
        Avatar::cuteJump();
        Avatar::tryStompTree();
        Avatar::setState(AvatarState::HAPPY);
        Mood::play();
    }
    if (!locked && attackEdge && !Avatar::isAttackHopping() && !Avatar::isJumping()) {
        Avatar::setPlayDead(false);
        Avatar::setSitting(false);
        Avatar::attackHop();
        Avatar::setState(AvatarState::HUNTING);
        Mood::play();
    }
}

void loop() {
    // GPIO0 is the StickC Plus2 mic clock. Reading it as a button floats
    // and was turning the backlight off, which looks like a frozen scene.
    if (Display::isScreenForcedOff()) return;

    // Any key wakes the dimmed backlight (not only isChange, not only FARM).
    if (ButtonMapper::isPressed() || ButtonMapper::isChanged())
        Display::resetDimTimer();

    if (!SettingsMenu::isTyping())
        Screenshot::poll();

    if (s_mode == AppMode::FARM || windowHidden()) farmPoll();
    // A short PWR click attacks; a long PWR press opens the menu.
    if (s_mode == AppMode::FARM && ButtonMapper::back()) {
        setMode(AppMode::MENU);
        return;
    }

    // Backspace = minimize overlay, except in file manager where it navigates back.
    if (overlayMode() && s_mode != AppMode::FILEMGR &&
        !SettingsMenu::isTyping()) {
        if (ButtonMapper::keyNewPress(s_minLatch)) {
            if (false) {
                s_winHid = !s_winHid;
                SFX::play(SFX::MENU_CLICK);
                Display::showToast(s_winHid ? "MIN" : "WIN", 500);
            } else if (s_winHid && ButtonMapper::back()) {
                setMode(AppMode::MENU);
            } else if (s_winHid && Menu::tryHotkey()) {
            }
        }
    }

    if (s_mode == AppMode::LOOT) {
        LootMenu::update();
        if (!LootMenu::isActive()) setMode(AppMode::MENU);
    } else if (s_mode == AppMode::EVILPIG) {
        EvilPigMode::update();
        if (!EvilPigMode::isRunning()) setMode(AppMode::MENU);
    } else if (s_mode == AppMode::PIGPASS) {
        PigpassMode::update();
        if (!PigpassMode::isRunning()) setMode(AppMode::MENU);
    } else if (s_mode == AppMode::SPECTRUM) {
        SpectrumMode::update();
        if (!SpectrumMode::isRunning()) setMode(AppMode::MENU);
    } else if (s_mode == AppMode::FILEMGR) {
        FileMgrMode::update();
        if (!FileMgrMode::isRunning()) setMode(AppMode::MENU);
    } else if (s_mode == AppMode::XFER) {
        XferMode::update();
        if (!XferMode::isRunning()) setMode(AppMode::MENU);
    } else if (s_mode == AppMode::TASKS) {
        TaskManager::update();
        if (!TaskManager::isRunning()) setMode(AppMode::MENU);
    } else if (s_mode == AppMode::PIG || s_mode == AppMode::TUNE ||
               s_mode == AppMode::WIFI) {
        SettingsMenu::update();
        if (!SettingsMenu::isActive()) setMode(AppMode::MENU);
    } else if (s_mode == AppMode::MENU) {
        Menu::update();
        return;
    }

    if (!ButtonMapper::isPressedAny() || !ButtonMapper::isChanged())
        return;

    if (s_mode == AppMode::LOOT || s_mode == AppMode::EVILPIG ||
        s_mode == AppMode::PIGPASS ||
        s_mode == AppMode::SPECTRUM ||
        s_mode == AppMode::PIG || s_mode == AppMode::TUNE ||
        s_mode == AppMode::WIFI || s_mode == AppMode::TASKS) return;

    char typed = ButtonMapper::typedChar();
    bool back = ButtonMapper::back();

    if (s_mode == AppMode::FARM) return;

    if (back) {
        setMode(AppMode::MENU);
        return;
    }

    Menu::handleKey(typed, false, false, false);
}

}
