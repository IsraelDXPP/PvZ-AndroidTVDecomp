// UnlockDialog -- reconstructed from libGameMain.so.
//
// Polymorphic class derived from LawnDialog (Game\LawnDialog.h): the constructor
// calls LawnDialog::LawnDialog and then stores its own vtable, and the destructor
// calls LawnDialog::~LawnDialog.
//
// vtable order -- this is what the binary actually holds, and `virtual` must be
// declared in this order:
//   [  0] 0x13e5b4  UnlockDialog::~UnlockDialog
//   [  1] 0x13e5e8  UnlockDialog::~UnlockDialog
//   [ 29] 0x141158  UnlockDialog::AddedToManager
//   [ 30] 0x1412b0  UnlockDialog::RemovedFromManager
//   [ 61] 0x13ea44  UnlockDialog::GotFocus
//   [ 76]           UnlockDialog::MouseDrag        (declared after GotFocus)
//   [  4]           UnlockDialog::ButtonDepress    (declared last)
// Slots 2..28, 31..60 and the rest of the Widget / Dialog slots are inherited and
// must NOT be re-declared here.
// Itanium/ARM gives the destructor two slots; MSVC x86 gives it one, so the
// generated absolute slot numbers are one lower than the comments above.
//
// Layout: in the binary the first own member sits at +0x2FC, i.e. immediately
// after LawnDialog (whose last declared word is ordinal 191). The constructor
// stores its second argument there:
//     +0x2FC  int mUnlockedItemId
//     +0x300 .. +0x39F  unknown; the object is 0x3A0 in this transcription.
// TODO(recon): none of the words after +0x2FC has been named yet. LawnDialog
// itself is still declared as 0x710 in this tree (another group owns it), so the
// derived field below only lands at +0x2FC once that header is rebuilt.

#pragma once

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept as-is so other files' transitive dependencies do not change.
#include "Dialog.h"
#include "LawnDialog.h"

// ChallengeScreen, LawnApp, LawnPlayerInfo and MainMenu are not named anywhere in
// this file, so all four includes were pure cycle. Removing them here breaks the
// two-edge ChallengeScreen.h <-> UnlockDialog.h pair, the longer
// ChallengeScreen -> UnlockDialog -> MainMenu -> StoreScreen chains, and the last
// edge of LawnPlayerInfo.h -> GameStats.h -> SeedBank.h -> CutScene.h ->
// ChallengeScreen.h -> UnlockDialog.h -> LawnPlayerInfo.h.
class ChallengeScreen;
class MainMenu;
class LawnPlayerInfo;
class LawnApp;

class UnlockDialog : public LawnDialog
{
public:
    virtual ~UnlockDialog();

    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager);
    virtual void RemovedFromManager(Sexy::WidgetManager *theWidgetManager);
    virtual void GotFocus();
    virtual void MouseDrag(int theX, int theY);
    virtual void ButtonDepress(int theId);

    UnlockDialog(int theLevel, int theItemId);

    int mUnlockedItemId;
};
