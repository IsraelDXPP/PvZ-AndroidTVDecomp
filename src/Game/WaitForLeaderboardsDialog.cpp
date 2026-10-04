// WaitForLeaderboardsDialog -- restored from libGameMain.so.
//
// Rewritten by hand in stage 2: the stage-1 body was the IDA decompilation with
// syntactic changes only. Every raw offset, hand-written vtable call and vptr
// store is gone; the members and the virtuals come from
// WaitForLeaderboardsDialog.h.

#include "WaitForLeaderboardsDialog.h"

#include "Dialog.h"
#include "LawnApp.h"
#include "SavingDingusDialog.h"
// ==========================================================================
// WaitForLeaderboardsDialog::GetStatus()
// @0013E3BC  4 bytes
// ==========================================================================
int WaitForLeaderboardsDialog::GetStatus()
{
    return 0;
}

// ==========================================================================
// WaitForLeaderboardsDialog::~WaitForLeaderboardsDialog()
// @0013E914  34 bytes
// ==========================================================================
// The two stores are the object's own vptr and the secondary-base vptr of the
// `Sexy::ButtonListener` sub-object at +0x100 (Widget is 0x100 bytes, see
// Dialog.h). A real base class makes the compiler emit both, and the explicit
// `~Dialog()` call is likewise the base destructor the compiler already runs.
WaitForLeaderboardsDialog::~WaitForLeaderboardsDialog()
{
}

// ==========================================================================
// WaitForLeaderboardsDialog::WaitForLeaderboardsDialog(LawnApp *)
// @0013F7E8  122 bytes
// ==========================================================================
// The dump builds a `Sexy::String` from the title literal, hands it to the base
// constructor as the 5th argument and then releases the temporary; that whole
// dance is what the `SavingDingusDialog` parameter list already expresses, so
// the body is the constructor's initializer list.
WaitForLeaderboardsDialog::WaitForLeaderboardsDialog(LawnApp *theApp)
    : SavingDingusDialog(theApp, true, 57, Sexy::String("[LEADERBOARDS_DIALOG]"), 1.0f, true)
{
}
