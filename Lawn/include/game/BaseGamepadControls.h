// BaseGamepadControls -- reconstructed from libGameMain.so.
//
// Inheritance chain (authoritative header PvZ\Lawn\BaseGamepadControls.h:43,
// confirmed by the binary's ZTV19BaseGamepadControls @0x684d30):
//   class BaseGamepadControls : public Sexy::GamepadListener, public GameObject
//   (GameObject : SyncObject)
// Sub-object offsets, from the vtable's offset-to-top words:
//   +0x00  Sexy::GamepadListener   (0x04; its own vptr -- the primary base)
//   +0x04  GameObject              (0x34; vptr at +0x04, mApp lands at +0x14)
//   +0x38  BaseGamepadControls' own members
// The transcribed bodies confirm it: the destructor stores the GameObject vtable
// into `(char*)this + 4` and then calls `~GameObject()` on `(GameObject*)this + 4`
// (BaseGamepadControls.cpp:72-77), and `mApp` is read as `+20` (LawnApp.cpp side).
//
// Size 0xAC (43 ints). The authoritative header's own comment says "size = 43
// ints", and 4 + 0x34 + 0x74 == 0xAC. The previous 0x118 was larger than the
// size of the DERIVED GamepadControls (0x108, from `an allocation of 0x108u` at
// Board.cpp:18111), which cannot be right for a base class.
//
// vtable order exactly as the binary holds it (18 slots, 0x000..0x044). The two
// destructor entries land LAST (0x040/0x044), i.e. after every other virtual,
// because Sexy::GamepadListener does not declare a virtual destructor: the
// derived class's destructor is a NEW virtual and is therefore appended.
// MSVC x86 has ONE virtual-destructor slot where Itanium/ARM has D1+D0, so the
// declaration order below matters, not the absolute slot count.
//
//   [  0] 0x000  BaseGamepadControls::OnButtonDown
//   [  1] 0x004  __cxa_pure_virtual          <-- declared but never overridden
//   [  2] 0x008  __cxa_pure_virtual          <-- declared but never overridden
//   [  3] 0x00C  BaseGamepadControls::OnKeyDown
//   [  4] 0x010  BaseGamepadControls::OnKeyUp
//   [  5] 0x014  BaseGamepadControls::Update
//   [  6] 0x018  BaseGamepadControls::BeginDraw
//   [  7] 0x01C  BaseGamepadControls::Draw
//   [  8] 0x020  BaseGamepadControls::EndDraw
//   [  9] 0x024  BaseGamepadControls::MakeParentGraphicsFrame
//   [ 10] 0x028  BaseGamepadControls::EnterState
//   [ 11] 0x02C  BaseGamepadControls::ExitState
//   [ 12] 0x030  BaseGamepadControls::UpdateStates
//   [ 13] 0x034  BaseGamepadControls::GotoState
//   [ 14] 0x038  BaseGamepadControls::GetSnapToGridPos
//   [ 15] 0x03C  BaseGamepadControls::GetSnapToGridXY
//   [ 16] 0x040  BaseGamepadControls::~BaseGamepadControls
//   [ 17] 0x044  BaseGamepadControls::~BaseGamepadControls

#pragma once

#include "ReconTypes.h"
// Only complete types this header itself needs: GameObject is a base class and
// Graphics appears in method signatures. Board / Challenge / CursorObject are used
// through pointers only, so they are forward-declared below instead of included.
//
// Including Board.h here used to close an include cycle
//     BaseGamepadControls.h -> Board.h -> {Coin.h, Challenge.h}
//         -> GamepadControls.h -> BaseGamepadControls.h
// that could not be resolved by the guards: Board.h reached GamepadControls.h
// while BaseGamepadControls was still incomplete, so `class GamepadControls :
// public BaseGamepadControls` failed with C2504 and every size assertion below it
// collapsed to 1. BaseGamepadControls.cpp includes Board.h, Challenge.h and
// CursorObject.h itself, so nothing is lost.
#include "GameObject.h"
#include "Graphics.h"
// GetHitResult returns a HitResult by value, so the complete type is needed
// here. It is its own header rather than Board.h's, because Board.h includes
// this file.
#include "HitResult.h"

// Result of BaseGamepadControls::GetSnapToGridPos (authoritative header:29).
struct SnapToGridPosition
{
    float mX;
    float mY;
};

// Needed by the listener's return types and by this class's own member list.
class Board;
class Challenge;
class CursorObject;

namespace Sexy {

// GamepadListener -- the abstract gamepad-event interface. The binary's
// BaseGamepadControls vtable starts with its slots, and slots [1]/[2] stay at
// __cxa_pure_virtual because nothing in the hierarchy overrides them.
// The return types below are the ones the authoritative vtable records
// (PvZ\Lawn\BaseGamepadControls.h:68-74: slot [0] void, slot [3] bool) and they
// MUST match BaseGamepadControls' own declarations exactly: a same-named,
// same-parameter-type-list declaration in a derived class is an implicit override
// whether or not it says `virtual`, and a differing return type is C2555 -- which
// makes the whole class ill-formed and collapses sizeof() to 1.
// TODO(recon): neither pure virtual can be named from the symbols.
class GamepadListener
{
public:
    virtual void OnButtonDown(int theButton, int thePlayerIndex, unsigned int theFlags) = 0; // [0] 0x000
    virtual void OnButtonUp(int theButton, int thePlayerIndex, unsigned int theFlags) = 0; // [1] 0x004
    virtual void OnGameAxisMove(int theAxis, int theValue, int thePlayerIndex) = 0; // [2] 0x008
    virtual bool OnKeyDown(Sexy::KeyCode theKey, unsigned int theFlags) = 0; // [3] 0x00C
    virtual int OnKeyUp(Sexy::KeyCode theKey, unsigned int theFlags) = 0;    // [4] 0x010
};

} // namespace Sexy

class SeedBank;
namespace Sexy { class Graphics; }

class BaseGamepadControls : public Sexy::GamepadListener, public GameObject
{
public:
    // Movement state machine (authoritative header:45). mGamepadState holds one
    // of these; BaseGamepadControls.cpp:114/150/160/177 compares it with 3/6/10.
    enum MovementState
    {
        MOVEMENT_STATE_NONE = 0,
        MOVEMENT_STATE_NORMAL = 1,
        MOVEMENT_STATE_RESERVED_2 = 2,
        MOVEMENT_STATE_BUTTER_HELD = 3,
        MOVEMENT_STATE_BUTTER_RELEASED = 4,
        MOVEMENT_STATE_GRID_STEP = 5,
        MOVEMENT_STATE_SELECT_SEED = 6,
        MOVEMENT_STATE_PLANT_CURSOR = 7,
        MOVEMENT_STATE_DIG_HOLD = 8,
        MOVEMENT_STATE_RESERVED_9 = 9,
        MOVEMENT_STATE_DIG_INDICATOR_WAIT = 10,
    };

    // The slots this class implements, in vtable order. Slots [1]/[2] are left
    // abstract (the binary holds __cxa_pure_virtual there).
    // The return types of slots [0], [3] and [6] were `Board **`, `Challenge *`
    // and `int`, which the transcribed bodies had drifted away from and which no
    // longer matched the authoritative vtable (PvZ\Lawn\BaseGamepadControls.h:68-74:
    // slot [0] void, slot [3] bool, slot [6] bool). A derived declaration with the
    // same name and parameter-type-list is an implicit override whether or not it
    // is spelled `virtual`, so those three mismatches emitted C2555 in
    // GamepadControls.h and made the whole hierarchy's sizeof() collapse to 1.
    // TODO(recon): the transcribed bodies (BaseGamepadControls.cpp:21/97/111) and
    // the remaining loose shapes (`int Update(float)` vs `void Update(float)`,
    // `int OnKeyUp` vs `bool OnKeyUp`, `void Draw(void)` vs `void Draw(Sexy::Graphics *)`,
    // `int BeginDraw(void)` vs `bool BeginDraw(Sexy::Graphics *)`) must be retyped
    // together with the declarations.
    virtual void OnButtonDown(int theButton, int thePlayerIndex, unsigned int theFlags); // [ 0] 0x000
    virtual void OnButtonUp(int theButton, int thePlayerIndex, unsigned int theFlags); // [ 1] 0x004
    virtual void OnGameAxisMove(int theAxis, int theValue, int thePlayerIndex); // [ 2] 0x008
    virtual bool OnKeyDown(Sexy::KeyCode theKey, unsigned int theFlags); // [ 3] 0x00C
    virtual int OnKeyUp(Sexy::KeyCode theKey, unsigned int theFlags);    // [ 4] 0x010
    virtual int Update(float theDeltaTime);                         // [ 5] 0x014
    virtual bool BeginDraw();                                        // [ 6] 0x018
    virtual void Draw();                                             // [ 7] 0x01C
    virtual void EndDraw();                                          // [ 8] 0x020
    virtual void MakeParentGraphicsFrame();                          // [ 9] 0x024
    virtual int EnterState(int theState);                            // [10] 0x028
    virtual int ExitState();                                         // [11] 0x02C
    virtual int UpdateStates(float theDeltaTime);                    // [12] 0x030
    virtual int GotoState(MovementState theState);                   // [13] 0x034
    // sret: ARM passes a by-value result's destination in r0, so the dump's
    // `this` in both of these IS that destination and the object arrives in the
    // next register. The transcribed `BaseGamepadControls *` return was the
    // destination handed back in r0. Same shape as GetHitResult below.
    virtual SnapToGridPosition GetSnapToGridPos();                   // [14] 0x038
    virtual SnapToGridPosition GetSnapToGridXY();                    // [15] 0x03C
    virtual ~BaseGamepadControls();                                  // [16],[17] 0x040/0x044

    // Member area +0x38 .. +0xAB (the ordinals are 4-byte units in the
    // authoritative table; the byte values it writes directly are kept as-is).
    Board *mBoard;                   // +0x38 (ordinal 14; shadows GameObject::mBoard at +0x18)
    bool mDrawCursorFrame;           // +0x3C
    float mCursorHighlightAnimPhase; // +0x40 (ordinal 16; BaseGamepadControls.cpp:522)
    int mGamepadFrameCounter;        // +0x44 (ordinal 17; BaseGamepadControls.cpp:2606)
    float mGamepadAccLeftX;          // +0x48
    float mGamepadAccLeftY;          // +0x4C
    float mGridCenterPositionX;      // +0x50 (ordinal 20; BaseGamepadControls.cpp:298)
    float mGridCenterPositionY;      // +0x54 (ordinal 21)
    float mRightPositionX;           // +0x58 (ordinal 22)
    float mRightPositionY;           // +0x5C (ordinal 23)
    MovementState mGamepadState;     // +0x60 (ordinal 24; the switch selector)
    float mMovementStateTimer;       // +0x64
    float mDigIndicatorPercentage;   // +0x68 (ordinal 26; +2.3 per dig key press)
    float mCursorPositionX;          // +0x6C (ordinal 27)
    float mCursorPositionY;          // +0x70 (ordinal 28)
    float mGamepadVelocityLeftX;     // +0x74 (ordinal 29)
    float mGamepadVelocityLeftY;     // +0x78 (ordinal 30)
    float mGamepadVelocityRightX;    // +0x7C (ordinal 31)
    float mGamepadVelocityRightY;    // +0x80 (ordinal 32)
    float mInputLeftStickX;          // +0x84
    float mInputLeftStickY;          // +0x88
    float mInputRightStickX;         // +0x8C
    float mInputRightStickY;         // +0x90
    int mPlayerIndex;                // +0x94 (ordinal 37; indexes CursorObject/CursorPreview)
    int mGamepadIndex;               // +0x98 (ordinal 38; equal to mPlayerIndex in practice)
    float mCursorPositionYJitter;    // +0x9C (ordinal 39)
    float mCursorJitterAnimPhase;    // +0xA0 (ordinal 40)
    int mButterCursorMoveCounter;    // +0xA4
    int mDerivedSyncBlockPadding;    // +0xA8 (no semantic access found yet)
    // 0xAC (43 ints)

    // The remaining methods. None of these owns a vtable slot.
    int GetCursorPosition(float & theX, float & theY);
    int GetCursorPosition(int & theX, int & theY);
    int GetGamepadVelocity(float & theX, float & theY);
    // sret again: the dump's `this` is the destination, the object is the second
    // register. Verified at 0x1BE920.
    SnapToGridPosition GetCursorGridPosition();
    // _ZN19BaseGamepadControls9DrawFrameEPN4Sexy8GraphicsENS_16FrameRenderPhaseE.
    // The float indexes in the body are mTransX / mTransY (Graphics +8 / +0xC).
    Board * DrawFrame(Sexy::Graphics *theGraphics, int thePhase);
    // sret: ARM passes an 8-byte struct's destination in r0, so the dump's
    // `this` IS that destination and the body ends `return this;` right after
    // `Board::MouseHitTest(x, y, this, 0)`. Declared as a pointer return, MSVC
    // hands the result back in EAX and the callee writes 8 bytes over the first
    // two words of the GamepadControls object instead. Verified at 0x1C0B6C.
    HitResult GetHitResult();
    BaseGamepadControls(Board * theBoard, int thePlayerIndex, int theGamepadIndex);
};

// Layout check: 0x04 (GamepadListener vptr) + 0x34 (GameObject) + 0x74 (own members).
