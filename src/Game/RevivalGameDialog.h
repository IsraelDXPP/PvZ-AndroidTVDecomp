// RevivalGameDialog -- reconstructed from libGameMain.so.
//
// Polymorphic class deriving from LawnDialog (every slot this class does not
// override holds LawnDialog / Sexy::Dialog / Sexy::Widget / Sexy::WidgetContainer
// code). Its constructor calls `LawnDialog::LawnDialog(this, gLawnApp, 0, 17, 1,
// &title, &message, &footer, 1)` and then rewrites the two vptrs -- the primary
// one at +0x00 and the ButtonListener one at +0x100, which is where Dialog
// places its second base because Widget is 0x100 bytes. The destructor's
// non-virtual thunk adjusts `this` by -256 for the same reason.
//
// vtable order -- the class's own entries, in slot order (MSVC x86 collapses the
// Itanium D2/D0 pair into one destructor slot):
//   [  0] 0x13e620  RevivalGameDialog::~RevivalGameDialog
//   [  1] 0x13e654  RevivalGameDialog::~RevivalGameDialog
//   [ 29] 0x141154  RevivalGameDialog::AddedToManager
//   [ 30] 0x1412ac  RevivalGameDialog::RemovedFromManager
//   [ 61] 0x13ea20  RevivalGameDialog::GotFocus
//
// Members, from the constructor's own stores (0x1408A8):
//   * +0x120 is Dialog::mDialogLines -- the StrFormat result the ctor copies
//     there (`LDR.W R1,[R4,#0x120]` -> `StrFormat(&v35, v1, a3)` -> assign);
//   * +0x2E0 is LawnDialog::mApp, read as `LDR.W R0,[R0,#0x2E0]`;
//   * +0x2F0 / +0x2F4 are LawnDialog::mLawnYesButton / mLawnNoButton, whose
//     label is set through vtable slot 127 (`LDR.W R10,[R3,#0x1FC]`);
//   * +0x2FC is the Zombie the dialog was built for -- the constructor's first
//     argument, `STR.W R10,[R4,#0x2FC]`, and ButtonDepress reads it back as
//     `Board::ZombiesWon(mApp->mBoard, mZombie)`.
//
// The base has not been refactored yet, so the absolute offsets above only hold
// once LawnDialog's own size is right.

#pragma once

#include "ReconTypes.h"
// The transcribed bodies call methods on these, so the includes stay.
#include "Board.h"
#include "Dialog.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "Zombie.h"

class RevivalGameDialog : public LawnDialog
{
public:
    virtual ~RevivalGameDialog();

    // --- this class's own vtable entries, in table order -------------------
    virtual void AddedToManager(Sexy::WidgetManager *theManager);
    virtual void RemovedFromManager(Sexy::WidgetManager *theManager);
    virtual void GotFocus();                                   // [61]

    // --- members -----------------------------------------------------------
    // +0x120 is Dialog::mDialogLines; the dialog adds nothing before the base's
    // own tail, so the only member this class owns is the Zombie below.
    // TODO(recon): nothing in the binary names the Zombie field.
    Zombie *mZombie; // +0x2FC, the constructor's first argument

    // `MouseDrag` overrides Sexy::Dialog (Dialog.h:128) and `ButtonDepress`
    // overrides LawnDialog (LawnDialog.h:87) -- both are `void` there, so the
    // transcription's `int` returns are gone.
    RevivalGameDialog(Zombie *theZombie, int theZombieType);
    virtual void MouseDrag(int theX, int theY);
    virtual void ButtonDepress(int theId);
};
