// WaitForLeaderboardsDialog -- reconstructed from libGameMain.so.
//
// Real inheritance: `class WaitForLeaderboardsDialog : public SavingDingusDialog`
// (SavingDingusDialog : Sexy::Dialog : (Sexy::Widget, Sexy::ButtonListener)). The
// vtable confirms it: slots 29..31 and 36 are SavingDingusDialog's own addresses,
// and slot 131 -- the pure virtual SavingDingusDialog declares -- holds this
// class's GetStatus.
//
// Size 0x2F4, from `(WaitForLeaderboardsDialog *)operator new of 0x2F4 bytes` at
// LawnApp.cpp:9479. SavingDingusDialog ends at 0x2F0, so one field remains. The old
// placeholder's 0x110 was far too small.
//
// vtable order -- slots 0..130 come from the bases and are NOT redeclared here.
// This class contributes exactly one entry: it overrides SavingDingusDialog's pure
// virtual at slot 131:
//   [131] 0x13e3bc  WaitForLeaderboardsDialog::GetStatus

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_WAITFORLEADERBOARDSDIALOG_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_WAITFORLEADERBOARDSDIALOG_H

#include "ReconTypes.h"
// The base class, plus the classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Dialog.h"
#include "SavingDingusDialog.h"

class WaitForLeaderboardsDialog : public SavingDingusDialog
{
public:
    virtual ~WaitForLeaderboardsDialog();

    // Fills the slot SavingDingusDialog leaves pure.
    virtual int GetStatus(); // [131] 0x13e3bc

    // Not a vtable slot; the signature is the one the transcribed body defines.
    explicit WaitForLeaderboardsDialog(LawnApp *theApp);
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_WAITFORLEADERBOARDSDIALOG_H
