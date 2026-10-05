// ZenGardenControls -- reconstructed from libGameMain.so.
//
// Inheritance: `class ZenGardenControls : public GamepadControls`. The
// authoritative header PvZ\Lawn\GamepadControls.h declares it in the same file as
// GamepadControls, and this class's destructor (ZenGardenControls.cpp:48) calls
// `~GamepadControls()` on itself -- the only reason that call is well-formed is
// that GamepadControls is a base.
//
// SIZE 0x10C -- NOT the transcriber's 0x118:
//   * Board.cpp:18088 allocates `(ZenGardenControls *)operator new(0x10Cu)`;
//   * the authoritative header closes with "67 ints" = 268 = 0x10C;
//   * GamepadControls (0x108) + one word (mObjectType) = 0x10C.
// (Board.cpp:18096's `operator new(0x114u)` is TreeOfWisdomControls, the other
// GamepadControls sibling.)
//
// ---------------------------------------------------------------------------
// VTABLE: the same 18 slots as GamepadControls; this class replaces its own
// implementations. Slots [0] OnButtonDown, [3] OnKeyDown, [5] Update and [16] the
// destructor are the ones declared as overrides here.
// The `_DWORD *` returns this file used to carry for Update and Draw were
// decompiler artefacts: they differed from GamepadControls' `int Update(float)` /
// `Board * Draw(Sexy::Graphics *)` and were implicit overrides anyway, so they
// emitted C2555. An erroneous class declaration collapses sizeof() to 1, which was
// the only reason the layout assertion at the bottom failed -- measured with the
// offending declarations neutralised, sizeof(ZenGardenControls) is exactly 0x10C,
// i.e. GamepadControls (0x108) plus the single mObjectType word below.
// TODO(recon): ZenGardenControls.cpp:55/153 still spell Update and Draw `_DWORD *`
// and OnKeyDown `int`, and must be retyped to `int` / `Board *` / `bool`.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "BaseGamepadControls.h"
#include "game/Board.h"
#include "GameStats.h"
#include "GamepadControls.h"
#include "HelpBarWidget.h"

// Coin, CursorObject, LawnApp and ZenGarden are not named anywhere in this file,
// so all four includes were pure cycle. This header derives from GamepadControls,
// whose own LawnApp.h edge is cut in GamepadControls.h.
class Coin;
class CursorObject;
class ZenGarden;
class LawnApp;

class ZenGardenControls : public GamepadControls
{
public:
    // ---- own member, +0x108 (authoritative ordinal 66) ----
    // The base's GameObjectType (from GameObject): which zen-garden object the
    // cursor is on. Board.cpp:5442/5449 write it after a hit test.
    GameObjectType mObjectType; // +0x108
    // size 0x10C

    virtual ~ZenGardenControls();                                        // [16] 0x040

    // ---- replacements for GamepadControls' slots ----
    virtual void OnButtonDown(int theButton, int thePlayerIndex, unsigned int theFlags); // [ 0] 0x000
    virtual bool OnKeyDown(Sexy::KeyCode theKey, unsigned int theFlags);                 // [ 3] 0x00C
    virtual int Update(float theDeltaTime);                                              // [ 5] 0x014
    virtual void OnButtonUp(int theButton, int thePlayerIndex, unsigned int theFlags);
    virtual void OnGameAxisMove(int theAxis, int theValue, int thePlayerIndex);
    Board * Draw(Sexy::Graphics * g);

    ZenGardenControls(Board * theBoard, int thePlayerIndex1, int thePlayerIndex2);
};

