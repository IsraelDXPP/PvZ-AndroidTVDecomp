// ImitaterDialog -- reconstructed from libGameMain.so.
//
// Inheritance chain: `class ImitaterDialog : public LawnDialog`
// (phase-1 group table; confirmed by the binary's ZTV14ImitaterDialog @0x67fdb0:
// its primary vtable is 134 slots, it has a secondary vtable at +0x100 with
// offset-to-top -0x100, and its destructor chains into the LawnDialog one).
//
// vtable -- this class overrides exactly nine slots:
//   [  0] 0x14bbbc  ImitaterDialog::~ImitaterDialog
//   [  1] 0x14bc94  ImitaterDialog::~ImitaterDialog
//   [ 31] 0x151254  ImitaterDialog::Update
//   [ 36] 0x15126c  ImitaterDialog::Draw
//   [ 54] 0x14b9e4  ImitaterDialog::DrawOverlay
//   [ 71] 0x14edbc  ImitaterDialog::KeyDown
//   [ 76] 0x14eac0  ImitaterDialog::MouseDown
//   [ 79] 0x14b98c  ImitaterDialog::MouseUp
//   [133] 0x14eb50  ImitaterDialog::OnKeyDown      (a NEW virtual this class appends)
// Every other slot still points at the LawnDialog / Sexy::Dialog / Sexy::Widget
// implementation. The return types below are the bases' own -- a same-named,
// same-parameter derived declaration is an implicit override, so a differing
// return type would be C2555. Game\ImitaterDialog.cpp now spells all five `void`
// overrides `void` as well.
//
// MSVC x86 has ONE virtual-destructor slot where Itanium/ARM has D1+D0; only the
// declaration order matters.
//
// Size 0x310, from the constructor call site (`an allocation of 0x310u` followed by
// ImitaterDialog::ImitaterDialog(int) @0x14bf3c).
// LawnDialog is no longer opaque: it is 0x2FC, so this class's own members are the
// five words at +0x2FC..+0x30F. The transcribed header used to place them at
// +0x2E0, which overlapped LawnDialog's mApp/mLawnYesButton/mLawnNoButton; that
// block is gone and every member below is named from its use in the bodies.
//
// Return types: RemoveToolTip, GetSeedPosition and ShowToolTip are void in the
// binary. Their transcribed prototypes (`int`, `SeedChooserScreen *`, `int`) came
// from the raw register state at the ARM `POP`/`BX LR`: 0x14c1f0 ends with `BX LR`
// with r0 untouched (=this), and both exits of 0x14c374 (`POP.W {R4-R8,PC}`) leave
// r0 holding the `a2 / NumColumns()` quotient from `__aeabi_idiv`. No caller reads
// any of the three values.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_IMITATERDIALOG_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_IMITATERDIALOG_H

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "GamepadApp.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "LawnDialog.h"
#include "SexyAppBase.h"
#include "ToolTipWidget.h"
#include "Widget.h"

// LawnApp is reached only through `LawnApp *mApp` and the base's constructor
// parameter, so it is declared rather than included: the include was half of the
// LawnApp.h <-> ImitaterDialog.h cycle.
// Sexy\ReconTypes.h already declares LawnApp.
class LawnApp;
// SeedChooserScreen is no longer named by any signature here (GetSeedPosition is
// void and GetNextSeedInDir takes the ReconTypes `SeedChooserScreen_SeedDir` shim),
// but SeedChooserScreen.h still includes this header, so the declaration stays as
// the cheap half of that edge.
class SeedChooserScreen;

class ImitaterDialog : public LawnDialog
{
public:
    virtual ~ImitaterDialog(); // [  0],[1] 0x000/0x004
    virtual void Update(); // [ 31] 0x07C
    virtual void Draw(Sexy::Graphics *theGraphics); // [ 36] 0x090
    virtual void DrawOverlay(Sexy::Graphics *theGraphics); // [ 54] 0x0D8
    virtual int KeyDown(Sexy::KeyCode theKey); // [ 71] 0x11C
    virtual void MouseDown(int theX, int theY, int theClickCount); // [ 76] 0x130
    virtual void MouseUp(int theX, int theY, int theClickCount); // [ 79] 0x13C
    virtual int OnKeyDown(Sexy::KeyCode theKey, unsigned int theFlags); // [133] 0x214 (new virtual)

    // ---- derived member area: +0x2FC..+0x30F (LawnDialog ends at 0x2FC) ----
    // Names are from the call sites: the constructor seeds all five, Draw/DrawOverlay
    // use the tool tip, and KeyDown/GameButtonDown carry the selected seed and the
    // gamepad index that GamepadToPlayerIndex converts.
    ToolTipWidget *mToolTipWidget; // +0x2FC, the shared ToolTipWidget this dialog owns
    int mToolTipSeed; // +0x300, the seed the tool tip is showing, -1 for none
    int mSelectedSeed; // +0x304, the highlighted seed packet, -1 for none
    int mPlayerIndex; // +0x308, the gamepad index this dialog belongs to
    HelpBarWidget *mHelpBar; // +0x30C, mApp->mHelpBarWidget
    // 0x310

    // The remaining methods. None of these owns a vtable slot.
    ImitaterDialog(int thePlayerIndex);
    void RemoveToolTip();
    void GetSeedPosition(int theIndex, int &theX, int &theY);
    int SeedHitTest(int theX, int theY);
    int GetNextSeedInDir(int theIndex, SeedChooserScreen_SeedDir theDir);
    int UpdateCursor();
    void ShowToolTip();
    // The disassembly of 0x14ec8c compares the second argument against mPlayerIndex
    // and switches on the first, so the first parameter IS the button: the
    // transcribed spelling named it `result` on the theory that it was the receiver.
    void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theButtonFlags);
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_IMITATERDIALOG_H
