// WaitForRemoteFileDialog -- reconstructed from libGameMain.so.
//
// Polymorphic class deriving from SavingDingusDialog -- every slot it does not
// override holds SavingDingusDialog / Sexy::Dialog / Sexy::Widget /
// Sexy::WidgetContainer code. The constructor builds the
// "[SAVING_LIVE_DIALOG]" string, chains to
// `SavingDingusDialog::SavingDingusDialog(this, arg, arg, 56, &text, 1.0, arg)`
// and then rewrites the two vptrs: the primary one at +0x00 and the
// ButtonListener one at +0x100, which is where Dialog places its second base
// because Widget is 0x100 bytes. The destructor's non-virtual thunk adjusts
// `this` by -256 for the same reason.
//
// vtable order -- the class's own entries are the two Itanium destructor slots
// and nothing else; MSVC x86 collapses them into ONE `virtual ~` slot:
//   [  0] 0x13e8a8  WaitForRemoteFileDialog::~WaitForRemoteFileDialog
//   [  1] 0x13e8dc  WaitForRemoteFileDialog::~WaitForRemoteFileDialog
//   [131] 0x13e3c0  WaitForRemoteFileDialog::GetStatus
// GetStatus fills the slot SavingDingusDialog leaves pure (see that header). Its
// address is 0x13e3c0 and _recon/vtables.json records it at index 131 of this
// class's table; the earlier note here claimed the function was not virtual and
// returned 1000, and both halves of that were wrong -- the body is
// `MOVS R0, #0` (see _recon/ida/WaitForRemoteFileDialog.cpp:14-16), i.e. the
// constant 0, so this dialog never decides that it is finished and waits for the
// player. WaitFor2PLoadDialog::GetStatus is the one that returns 1000.
//
// Members: the constructor stores its first argument into word 189 (+0x2F4),
// and no body reaches anything higher. The base has not been refactored yet, so
// that absolute offset only holds once SavingDingusDialog's own size is right.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_WAITFORREMOTEFILEDIALOG_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_WAITFORREMOTEFILEDIALOG_H

#include "ReconTypes.h"
// The transcribed bodies call methods on these, so the includes stay.
#include "Dialog.h"
#include "SavingDingusDialog.h"

class WaitForRemoteFileDialog : public SavingDingusDialog
{
public:
    virtual ~WaitForRemoteFileDialog();

    int mRequestId;

    virtual int GetStatus();
    WaitForRemoteFileDialog(int theRequestId, LawnApp *theApp, bool theModal);
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_WAITFORREMOTEFILEDIALOG_H
