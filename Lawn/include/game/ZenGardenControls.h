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
// Draw overrides the Graphics* callback in GamepadControls.

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
    GameObjectType mSelectedTool; // +0x108, tool restored after its animation
    // size 0x10C

    virtual ~ZenGardenControls();                                        // [16] 0x040

    // ---- replacements for GamepadControls' slots ----
    virtual void OnButtonDown(int theButton, int thePlayerIndex, unsigned int theFlags); // [ 0] 0x000
    virtual bool OnKeyDown(Sexy::KeyCode theKey, unsigned int theFlags);                 // [ 3] 0x00C
    virtual int Update(float theDeltaTime);                                              // [ 5] 0x014
    virtual void OnButtonUp(int theButton, int thePlayerIndex, unsigned int theFlags);
    virtual void OnGameAxisMove(int theAxis, int theValue, int thePlayerIndex);
    void Draw(Sexy::Graphics *theGraphics) override;

    ZenGardenControls(Board * theBoard, int thePlayerIndex1, int thePlayerIndex2);
};
