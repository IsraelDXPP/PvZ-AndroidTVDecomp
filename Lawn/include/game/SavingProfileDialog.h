// SavingProfileDialog -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class SavingProfileDialog : public SavingDingusDialog`.
// This is the one place where the binary's RTTI disagrees with the community
// restoration, which has no entry for the class at all: the RTTI base list for
// _ZTV19SavingProfileDialog is exactly `18SavingDingusDialog`, and the table
// confirms it by keeping SavingDingusDialog::Update at slot [31] while overriding
// only AddedToManager, RemovedFromManager and Draw.
//
// It adds no data members of its own: SavingProfileDialog.cpp touches +0x2E4 and
// +0x2E8, which are SavingDingusDialog::mApp and ::mUnk2E8, and nothing else above
// the Sexy::Dialog area. Size is therefore 0x2EC, the same as its base.
// The transcribed object was 0x2F8.
//
// vtable order -- the slots this class occupies, in the binary's order:
//   [  0] 0x13e764  SavingProfileDialog::~SavingProfileDialog
//   [  1] 0x13e798  SavingProfileDialog::~SavingProfileDialog
//   [ 29] 0x13eb34  SavingProfileDialog::AddedToManager
//   [ 30] 0x13eb38  SavingProfileDialog::RemovedFromManager
//   [ 36] 0x13e998  SavingProfileDialog::Draw
//   [131] 0x13e3e4  SavingProfileDialog::GetStatus
// Slot 131 is the framework's "which dialog is this" query; the body returns 1000.
//
// MSVC x86 gives a virtual destructor one slot where Itanium and ARM give it two,
// so the absolute slot numbers run one lower here; only the order is copied.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_SAVINGPROFILEDIALOG_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_SAVINGPROFILEDIALOG_H

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Dialog.h"
#include "SavingDingusDialog.h"
#include "WidgetContainer.h"

// LawnApp is only the constructor's pointer parameter
// (`SavingProfileDialog(LawnApp *, bool)`), so it is declared rather than
// included: the include was half of the LawnApp.h <-> SavingProfileDialog.h
// cycle. Sexy\ReconTypes.h already carries it.
class LawnApp;

class SavingProfileDialog : public SavingDingusDialog
{
public:
    virtual ~SavingProfileDialog();
    virtual void AddedToManager(Sexy::WidgetManager *theManager);
    virtual void RemovedFromManager(Sexy::WidgetManager *theManager);
    virtual void Draw(Sexy::Graphics *theGraphics);
    virtual int GetStatus();

    SavingProfileDialog(LawnApp *theApp, bool theDrawFade);
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_SAVINGPROFILEDIALOG_H
