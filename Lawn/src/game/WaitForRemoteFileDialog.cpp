// WaitForRemoteFileDialog -- restored from libGameMain.so.
//
// Rewritten by hand in stage 2: the stage-1 body was the IDA decompilation with
// syntactic changes only. Every raw offset, hand-written vtable call and vptr
// store is gone; the members and the virtuals come from
// WaitForRemoteFileDialog.h and its bases.

#include "WaitForRemoteFileDialog.h"

#include "Dialog.h"
#include "LawnApp.h"
#include "SavingDingusDialog.h"
// ==========================================================================
// WaitForRemoteFileDialog::GetStatus()
// @0013E3C0  4 bytes
// ==========================================================================
// `MOVS R0, #0; BX LR` -- the constant 0. Update treats a result of 1000 or more
// as "finished", so this dialog never decides on its own that it is done and
// waits for the player instead. (The header used to claim the constant was 1000;
// WaitFor2PLoadDialog::GetStatus is the one that returns that.)
int WaitForRemoteFileDialog::GetStatus()
{
    return 0;
}

// ==========================================================================
// WaitForRemoteFileDialog::~WaitForRemoteFileDialog()
// @0013E8A8  34 bytes
// ==========================================================================
// What the dump spells out is the object's own vptr, the vptr of the
// Sexy::ButtonListener sub-object at +0x100 (Widget is 0x100 bytes, see
// Dialog.h) and the explicit `~Dialog()` call. A real base class makes the
// compiler emit both stores and run the base destructor itself.
WaitForRemoteFileDialog::~WaitForRemoteFileDialog()
{
}

// ==========================================================================
// WaitForRemoteFileDialog::WaitForRemoteFileDialog(int,LawnApp *,bool)
// @0013F880  134 bytes
// ==========================================================================
// The dump builds a Sexy::String from "[SAVING_LIVE_DIALOG]", passes it as the
// base constructor's 4th argument and then releases the temporary by hand --
// which is exactly what the base's parameter list already expresses. The two
// vptr stores after it are the compiler's. The base call is
// `SavingDingusDialog::SavingDingusDialog(this, a3, a4, 56, &text, 1.0, a4)`
// (_recon/ida/WaitForRemoteFileDialog.cpp:118).
WaitForRemoteFileDialog::WaitForRemoteFileDialog(int theRequestId, LawnApp *theApp, bool theModal)
    : SavingDingusDialog(theApp, theModal, 56, "[SAVING_LIVE_DIALOG]", 1.0f, theModal)
{
    mRequestId = theRequestId;
}
