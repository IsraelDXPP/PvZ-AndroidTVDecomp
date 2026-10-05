// SavingFileDialog -- reconstructed from libGameMain.so.
//
// Polymorphic class derived from SavingDingusDialog (Game\SavingDingusDialog.h):
// the constructor calls SavingDingusDialog::SavingDingusDialog and then stores
// its own vtable, and the destructor calls Sexy::Dialog::~Dialog.
//
// vtable order -- this is what the binary actually holds, and `virtual` must be
// declared in this order:
//   [  0] 0x13e7d0  SavingFileDialog::~SavingFileDialog
//   [  1] 0x13e804  SavingFileDialog::~SavingFileDialog
// Slots 2..79 (and the 52 that follow) are the inherited SavingDingusDialog /
// Dialog / Widget slots and must NOT be re-declared here.
// Itanium/ARM gives the destructor two slots; MSVC x86 gives it one, so the
// generated absolute slot number is one lower than the comment above.
//
// The class adds no member: the constructor only stores the two vptrs
// (this + 0 and this + 256, the Sexy::ButtonListener subobject) and nothing
// else, and the destructor does the same. So sizeof(SavingFileDialog) equals
// sizeof(SavingDingusDialog).
//
// TODO(recon): this tree still declares SavingDingusDialog as 0x840 bytes; the
// binary's own constructor shows +0x100 as the ButtonListener subobject, which
// means the real size is far smaller. The object size will settle once that
// header is rebuilt (it belongs to another group).

#pragma once

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept as-is so other files' transitive dependencies do not change.
#include "Dialog.h"
#include "SavingDingusDialog.h"
#include <string>
#include <vector>
#include <map>
#include <set>

class SavingFileDialog : public SavingDingusDialog
{
public:
    virtual ~SavingFileDialog();

    int GetStatus();
    SavingFileDialog(LawnApp *theApp, bool theModal, Sexy::String theMessage);
};
