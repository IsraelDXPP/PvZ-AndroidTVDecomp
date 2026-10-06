// TreeOfWisdomControls -- reconstructed from libGameMain.so.
//
// Real inheritance: `class TreeOfWisdomControls : public GamepadControls`.
// Evidence: _recon/vtables.json, RTTI `20TreeOfWisdomControls` (vtable_ea
// 0x684e10) records `bases: [["15GamepadControls", 0]]`, and the authoritative
// header PvZ\Lawn\GamepadControls.h:161 declares the same base. The constructor
// @0x1C30A4 also writes the second base's vtable pointer at +4
// (`*((_DWORD *)this + 1) = &off_684E68;`), which is the GamepadListener subobject
// GamepadControls itself derives from.
//
// SIZE 0x114, from the allocation site `operator new(0x114u)` at Board.cpp:18093.
// GamepadControls is 0x108 (66 ints, Board.cpp:18111) and this class adds the three
// words the authoritative header lists at indices 66/67/68, i.e. +0x108/+0x10C/+0x110.
//
// VTABLE -- 18 slots @0x684e10. EVERY slot is the base's: slots 0..15 are the
// GamepadControls slots and 16/17 are the destructor pair, so this class declares no
// virtual of its own beyond the destructor (the rule for a derived class is to
// declare only the virtuals it adds).
//   [  0] 0x1bd5e4  TreeOfWisdomControls::OnButtonDown
//   [  1] 0x1bce9c  TreeOfWisdomControls::OnButtonUp
//   [  2] 0x1bcea0  TreeOfWisdomControls::OnGameAxisMove
//   [  3] 0x1c1e34  TreeOfWisdomControls::OnKeyDown
//   [  4] 0x1bd52c  GamepadControls::OnKeyUp            (base implementation)
//   [  5] 0x1bdabc  TreeOfWisdomControls::Update
//   [  6] 0x1bce38  TreeOfWisdomControls::BeginDraw
//   [  7] 0x1bd758  TreeOfWisdomControls::Draw
//   [  8] 0x1bce44  TreeOfWisdomControls::EndDraw
//   [  9] 0x1bce50  TreeOfWisdomControls::MakeParentGraphicsFrame
//   [ 10] 0x1bd32c  BaseGamepadControls::EnterState    (base implementation)
//   [ 11] 0x1bd584  BaseGamepadControls::ExitState     (base implementation)
//   [ 12] 0x1c0de8  GamepadControls::UpdateStates      (base implementation)
//   [ 13] 0x1bce78  BaseGamepadControls::GotoState     (base implementation)
//   [ 14] 0x1bd258  BaseGamepadControls::GetSnapToGridPos (base implementation)
//   [ 15] 0x1bd168  BaseGamepadControls::GetSnapToGridXY  (base implementation)
//   [ 16] 0x1bd000  TreeOfWisdomControls::~TreeOfWisdomControls   (D1)
//   [ 17] 0x1bd030  TreeOfWisdomControls::~TreeOfWisdomControls   (D0)
// The Itanium table has a D0/D1 destructor pair; MSVC x86 keeps one slot, so the
// order is unchanged and the class still adds nothing of its own.
//
// Draw callbacks override the Graphics* methods in GamepadControls.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "BaseGamepadControls.h"
#include "game/Board.h"
#include "Challenge.h"
#include "GamepadControls.h"
#include "Graphics.h"
#include "SexyTransform2D.h"

// LawnApp is not named in this file, so the include was pure cycle. Its base
// GamepadControls.h used to carry the same edge; this is the other half.
class LawnApp;

class TreeOfWisdomControls : public GamepadControls
{
public:
    // Slots 16/17, one slot under MSVC x86. Declared here because the class defines
    // it (Game\TreeOfWisdomControls.cpp:67).
    virtual ~TreeOfWisdomControls();

    // Overrides of base slots, in the binary's slot order.
    virtual void OnButtonDown(int theButton, int thePlayerIndex, unsigned int theFlags); // [ 0]
    virtual void OnGameAxisMove(int theAxis, int theValue, int thePlayerIndex);   // [ 2]
    virtual bool OnKeyDown(Sexy::KeyCode theKey, unsigned int theFlags); // [ 3]
    virtual int Update(float theFrac);                          // [ 5]
    bool BeginDraw(Sexy::Graphics *theGraphics) override;    // [ 6]
    virtual void MakeParentGraphicsFrame();                // [ 9]

    virtual void OnButtonUp(int theButton, int thePlayerIndex, unsigned int theFlags); // [ 1]
    void Draw(Sexy::Graphics *theGraphics) override;         // [ 7]
    void EndDraw(Sexy::Graphics *theGraphics) override;      // [ 8]

    // ---- members, absolute offsets (base GamepadControls = 0x108) ----
    // True while the cursor is on the tree and the player owns tree food.
    bool mCursorOnTree; // +0x108
    // The tool in hand, and the tool from the previous tick. A change redraws
    // the board's help text.
    int mSelectedTool;  // +0x10C
    int mPreviousTool;  // +0x110
    // Size 0x114.

    TreeOfWisdomControls(Board *theBoard, int thePlayerIndex1, int thePlayerIndex2);
};

// Layout check: GamepadControls (0x108) + three words = 0x114, the size of the
// `operator new(0x114u)` at Board.cpp:18093.
