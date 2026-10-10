// SavingFileDialog -- transcribed from libGameMain.so
// Rewritten by hand in stage 2; see build/phase2-kickoff.md.

#include "SavingFileDialog.h"
#include "Dialog.h"
#include "LawnApp.h"
#include "SavingDingusDialog.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// ==========================================================================
// SavingFileDialog::GetStatus()
// @0013E3C4  4 bytes
// ==========================================================================
int SavingFileDialog::GetStatus()
{
    return 0;
}

// ==========================================================================
// SavingFileDialog::~SavingFileDialog()
// @0013E7D0  34 bytes
// ==========================================================================
// The dump restores two vptrs (this + 0 and the Sexy::ButtonListener subobject at
// this + 256) and then calls Sexy::Dialog::~Dialog explicitly. With a real base
// class the compiler emits all of that, so the body is empty.
SavingFileDialog::~SavingFileDialog() {}

// ==========================================================================
// SavingFileDialog::SavingFileDialog(LawnApp *,bool,Sexy::String)
// @0013F924  128 bytes
// ==========================================================================
// The dump builds a temporary copy of `a4` and passes it to the base constructor;
// `a4` is already a by-value parameter, so it is handed over directly.
SavingFileDialog::SavingFileDialog(LawnApp *theApp, bool theModal, Sexy::String theMessage) :
    SavingDingusDialog(theApp, theModal, 68, theMessage, 3.0f, true)
{
}
