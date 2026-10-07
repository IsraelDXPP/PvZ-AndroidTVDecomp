// ContinueDialog -- reconstructed from libGameMain.so.
//
// `class ContinueDialog : public LawnDialog`.
//   The class has no header in the authoritative tree, so the base comes from the
//   vtable: slots 31/36 hold `LawnDialog::Update` (0x141188) and `LawnDialog::Draw`
//   (0x13ef00), slot 41 holds `Sexy::Dialog::OrderInManagerChanged` (0x301d28),
//   slot 45 `Sexy::Dialog::SetColor` (0x301f84) and slots 76..79
//   `Sexy::Dialog::MouseDown/MouseUp` -- the whole dialog chain is inherited.
//   The .cpp confirms the base extent: it reads +0x2E0 as a `LawnApp *`, which is
//   exactly `LawnDialog::mApp`, and LawnDialog is 0x2FC bytes.
//   The PC reference source (LawnProject\src\Lawn\ContinueDialog.h) is
//   `class ContinueDialog : public LawnDialog` too.
//   The `non-virtual thunk to ContinueDialog::~ContinueDialog` and
//   `... ButtonDepress` symbols the binary carries are the second-base adjustment
//   for `Sexy::ButtonListener`, which Dialog inherits.
//
// SIZE 0x304, NOT 0x820.
//   LawnDialog's 0x2FC plus the two dialog buttons the PC source names; the .cpp's
//   largest own member access is +0x300 (read back at ContinueDialog.cpp:185). The
//   only bigger offset in the file, +2064, is a LawnApp member reached through
//   `mApp`, not a member of this class.
//
// VTABLE -- 134 slots; the six own slots are:
//   [  0] 0x1b7418  ~ContinueDialog (deleting)
//   [ 29] 0x1b7a60  AddedToManager   [ 61] 0x1b7acc  GotFocus
//   [ 30] 0x1b7a84  RemovedFromManager  [ 71] 0x1b7aa8  KeyDown
//   [ 50] 0x1b79fc  Resize           [ 72] 0x1b7aac  KeyUp
// `ButtonDepress` is a `Sexy::ButtonListener` slot inherited through Dialog, not a
// new slot of this class.
// The Itanium table has a D0/D1 destructor pair; MSVC x86 keeps one slot, so only
// one `virtual ~ContinueDialog()` is declared.
//
// NOTE: the transcribed .cpp spells these with `int` returns; the base classes
// declare `void`, which is what an override needs, so the declarations below use
// void. Phase 2 aligns the bodies.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Dialog.h"
#include "LawnDialog.h"
#include "SexyAppBase.h"
#include "Zombie.h"

// Board is not named in this file, so the include was pure cycle
// (Board.h -> LawnApp.h -> ContinueDialog.h -> Board.h); it is kept as a
// declaration. LawnApp is likewise only the constructor's pointer parameter and
// is covered by the declaration at the end of this block.
class Board;

// LawnApp is only the constructor's pointer parameter here, so it is declared
// rather than included: the include was half of the LawnApp.h <-> ContinueDialog.h
// cycle. Sexy\ReconTypes.h already carries this declaration.
class LawnApp;

class ContinueDialog : public LawnDialog
{
public:
    enum
    {
        ContinueDialog_Continue = 0,
        ContinueDialog_NewGame,
    };

    virtual ~ContinueDialog();

    // slot 29
    virtual void AddedToManager(Sexy::WidgetManager * theWidgetManager);
    // slot 30
    virtual void RemovedFromManager(Sexy::WidgetManager * theWidgetManager);
    // slot 50
    virtual void Resize(int theX, int theY, int theWidth, int theHeight);
    // slot 61
    virtual void GotFocus();
    // slot 71
    virtual int KeyDown(Sexy::KeyCode theKey);
    // slot 72
    virtual int KeyUp(Sexy::KeyCode theKey);
    // `Sexy::ButtonListener` slot, reached through Dialog's second base.
    virtual void ButtonDepress(int theId);

    // ---- members, absolute offsets (base LawnDialog = 0x2FC) ----
    LawnStoneButton * mContinueButton;
    LawnStoneButton * mNewGameButton;

    // ---- methods that are not vtable slots (kept from the transcription) ----
    void GotGameFocus();
    explicit ContinueDialog(LawnApp *theApp);
    int RestartLoopingSounds();
    int GetPreferredHeight(int theWidth);
};
