// WaitFor2PLoadDialog -- reconstructed from libGameMain.so.
//
// Real inheritance: `class WaitFor2PLoadDialog : public SavingDingusDialog`
// (SavingDingusDialog : Sexy::Dialog : (Sexy::Widget, Sexy::ButtonListener)). The
// vtable confirms it: slots 29..31 and 36 are SavingDingusDialog's own addresses,
// and slot 131 -- the pure virtual SavingDingusDialog declares -- holds this
// class's GetStatus.
//
// Size 0x2F8. SavingDingusDialog ends at 0x2F0 (its own header's "Was 0x2F0"), so
// two fields remain, which is why this object is one word larger than its sibling
// WaitForLeaderboardsDialog.
//
// vtable order -- slots 0..130 come from the bases and are NOT redeclared here.
// This class contributes exactly one entry: it overrides SavingDingusDialog's pure
// virtual at slot 131:
//   [131] 0x13e3c8  WaitFor2PLoadDialog::GetStatus

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_WAITFOR2PLOADDIALOG_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_WAITFOR2PLOADDIALOG_H

#include "ReconTypes.h"
// The base class, plus the classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Dialog.h"
#include "SavingDingusDialog.h"

class WaitFor2PLoadDialog : public SavingDingusDialog
{
public:
    virtual ~WaitFor2PLoadDialog();

    // Fills the slot SavingDingusDialog leaves pure.
    virtual int GetStatus(); // [131] 0x13e3c8

    // Not a vtable slot; the signature is the one the transcribed body defines.
    explicit WaitFor2PLoadDialog(LawnApp *theApp);
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_WAITFOR2PLOADDIALOG_H
