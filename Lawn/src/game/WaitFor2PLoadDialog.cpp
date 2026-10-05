// WaitFor2PLoadDialog -- reconstructed from libGameMain.so.
//
// Stage 2: the bodies are the IDA decompilation rewritten as ordinary C++
// against the reconstructed headers (see build/phase2-g5-brief.md).

#include "WaitFor2PLoadDialog.h"

#include "Dialog.h"
#include "LawnApp.h"
#include "SavingDingusDialog.h"

// ==========================================================================
// WaitFor2PLoadDialog::GetStatus()
// @0013E3C8  28 bytes
// ==========================================================================
int WaitFor2PLoadDialog::GetStatus()
{
    // The dump's word read 2352 bytes into the LawnApp is LawnApp::mPlayer2Info
    // (+0x930), and the virtual call at byte 48 of its table is slot 12,
    // Sexy::PlayerInfo::IsLoading. Still loading (or no player-2 record at all)
    // reads as "not finished".
    Sexy::DefaultPlayerInfo *aPlayer2Info = mApp->mPlayer2Info;
    if (aPlayer2Info == nullptr)
        return 1000;

    return aPlayer2Info->IsLoading() ? 1000 : 0;
}

// ==========================================================================
// WaitFor2PLoadDialog::~WaitFor2PLoadDialog()
// @0013E83C  34 bytes
// ==========================================================================
WaitFor2PLoadDialog::~WaitFor2PLoadDialog()
{
    // Empty on purpose: the two vptr stores the dump spells out (the object's own
    // at +0x00 and the ButtonListener sub-object's at +0x100) and the
    // `((Sexy::Dialog *)this)->~Dialog()` call are all compiler-generated now that
    // the base chain is real.
}

// ==========================================================================
// WaitFor2PLoadDialog::WaitFor2PLoadDialog(LawnApp *)
// @0013F9C0  120 bytes
// ==========================================================================
WaitFor2PLoadDialog::WaitFor2PLoadDialog(LawnApp *theApp)
    : SavingDingusDialog(theApp, true, 69, Sexy::String("[LOADING_P2_PROFILE]"), 0.0f, true)
{
    // The dump's placement-new of SavingDingusDialog over `this` plus the string
    // temporary and its release block (`v8 - 12 != (int *)Sexy::gEmptyStringData`) are the base
    // constructor and the temporary's destructor, both emitted by the compiler.
    // The vptr stores at +0x00/+0x100 are emitted after the base, as the dump does.
}
