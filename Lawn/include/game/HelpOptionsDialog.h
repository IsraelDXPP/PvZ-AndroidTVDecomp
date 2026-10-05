// HelpOptionsDialog -- reconstructed from libGameMain.so.
//
// Real polymorphic class: `class HelpOptionsDialog : public LawnDialog`
// (LawnDialog itself derives from Sexy::Dialog). Evidence: the destructor at
// _recon\ida\HelpOptionsDialog.cpp:92 calls `LawnDialog::~LawnDialog(this)`
// directly, and the constructor calls LawnDialog's constructor at :217.
//
// The RTTI export _ZTV17HelpOptionsDialog is LawnDialog's primary table with
// these entries replaced:
//   [  0] 0x16811c  HelpOptionsDialog::~HelpOptionsDialog     (D2)
//   [  1] 0x168198  HelpOptionsDialog::~HelpOptionsDialog     (D0)
//   [ 29] 0x168b88  HelpOptionsDialog::AddedToManager         (override)
//   [ 30] 0x168c30  HelpOptionsDialog::RemovedFromManager     (override)
//   [ 31] 0x168e90  HelpOptionsDialog::Update                 (override)
//   [ 36] 0x168f88  HelpOptionsDialog::Draw                   (override)
//   [ 50] 0x168cb4  HelpOptionsDialog::Resize                 (override)
//   [ 61] 0x1690d4  HelpOptionsDialog::GotFocus               (override)
//   [ 71] 0x168f6c  HelpOptionsDialog::KeyDown                (override)
// MSVC x86 gives the virtual destructor ONE slot where Itanium gives two, so
// absolute indices shift by one; only the declaration order matters.
//
// ButtonPress / ButtonDepress reach this class through the ButtonListener
// sub-object's own vtable (the secondary vptr at +0x100, see the non-virtual
// thunk at 0x16804C), but LawnDialog already declares both as virtuals, so they
// are ordinary overrides of slots [128]/[129] here and add no slot of their own.
// The return type must stay `void` -- an `int` return differs from
// LawnDialog's `void` and is C2555, which makes the whole class ill-formed and
// collapses sizeof() to 1, so the layout assertion below could never pass.
//
// Size 0x318, from the allocation site
//     LawnApp.cpp:5763   HelpOptionsDialog allocation, 0x318 bytes
// which is LawnDialog (0x2FC, "115: 191" in the authoritative header) plus seven
// pointers. The old opaque blob said 0x3A0.

//
// Members, from _recon\ida\HelpOptionsDialog.cpp (the constructor at :254-338
// and the destructor at :75-91):
//   +0x2FC = mApp         ctor stores argument 2; also used by ButtonDepress
//   +0x300 = MakeButton(0, "[HOW_TO_PLAY]")
//   +0x304 = MakeButton(1, "[CONTROLS]")
//   +0x308 = MakeButton(2, "[SETTINGS]")
//   +0x30C = MakeButton(3, "[CREDITS]")
//   +0x310 = MakeButton(4, "[BACK]")
//   +0x314 is never written in this build but is inside the allocation.

#pragma once

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept so other files' transitive dependencies do not change.
#include "Dialog.h"
#include "LawnDialog.h"

// Only pointers to these are stored, so a forward declaration is enough and it
// keeps GameButton.h (which pulls in ButtonWidget.h and GamepadApp.h) out of
// this header's transitive closure.
class GameButton;

// LawnApp is reached only through `LawnApp *mApp` and the constructor's pointer
// parameter, so it is declared rather than included: the include was half of the
// LawnApp.h <-> HelpOptionsDialog.h cycle. Sexy\ReconTypes.h already carries it.
class LawnApp;

class HelpOptionsDialog : public LawnDialog
{
public:
    virtual ~HelpOptionsDialog();

    // Overrides of Dialog/WidgetContainer slots.
    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager); // [29] 0x168b88
    virtual void RemovedFromManager(Sexy::WidgetManager *theWidgetManager); // [30] 0x168c30
    virtual void Update();                                              // [31] 0x168e90
    virtual void Draw(Sexy::Graphics *g);                               // [36] 0x168f88
    virtual void Resize(int theX, int theY, int theWidth, int theHeight); // [50] 0x168cb4
    virtual void GotFocus();                                            // [61] 0x1690d4
    virtual int KeyDown(Sexy::KeyCode theKey);                         // [71] 0x168f6c
    virtual void ButtonPress(int theId);                                // [128] 0x168028
    virtual void ButtonDepress(int theId);                              // [129] 0x168ecc

    explicit HelpOptionsDialog(LawnApp *theApp);

    // --- helpers that are NOT in the vtable ---------------------------------
    int GetPreferredHeight(int theWidth);
    void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theModifiers);

    LawnApp *mApp;                  // +0x2FC
    GameButton *mHowToPlayButton;   // +0x300, id 0, "[HOW_TO_PLAY]"
    GameButton *mSwitchUserButton;  // +0x304, id 1, the binary labels it "[CONTROLS]"
    GameButton *mSettingsButton;    // +0x308, id 2, "[SETTINGS]"
    GameButton *mCreditsButton;     // +0x30C, id 3, "[CREDITS]"
    GameButton *mBackButton;        // +0x310, id 4, "[BACK]"
    GameButton *mExtraButton;       // +0x314, inside the allocation, never written here
    // size 0x318
};

