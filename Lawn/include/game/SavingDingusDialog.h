// SavingDingusDialog -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class SavingDingusDialog : public Sexy::Dialog`.
// The binary's RTTI base for _ZTV18SavingDingusDialog is N4Sexy6DialogE, and the
// table's Dialog entries (OrderInManagerChanged [41], SetColor [45], Resize [50],
// KeyDown [71], KeyUp [72], MouseDown [76]/[77], MouseUp [78]/[79]) confirm it.
//
// Object size 0x2EC. Sexy::Dialog is 0x2E0 and the constructor stores three
// members right after it (SavingDingusDialog.cpp:104-107). The transcribed object
// was 0x840 because the Update body at :152 reads (this + 2096) as a function
// pointer; the same literal 2064 appears in StoreScreen, BuyMoneyDialog,
// ContinueDialog and NewOptionsDialog, so it is a mis-typed base access, not a
// member of this class.
//
// vtable order -- the slots this class occupies, in the binary's order:
//   [  0] 0x13e6f8  SavingDingusDialog::~SavingDingusDialog
//   [  1] 0x13e72c  SavingDingusDialog::~SavingDingusDialog
//   [ 29] 0x141160  SavingDingusDialog::AddedToManager
//   [ 30] 0x141174  SavingDingusDialog::RemovedFromManager
//   [ 31] 0x1411c8  SavingDingusDialog::Update
//   [ 36] 0x13eb10  SavingDingusDialog::Draw
//   [131]            GetStatus -- PURE
// Slot 131 is one past Dialog's own 131 slots (0..130), so it is new here. Its
// entry in this class's own table (0x67e088) is `__cxa_pure_virtual` @0x2c50d8,
// and all five derived classes override it with a GetStatus:
//   0x13e3c0 WaitForRemoteFileDialog, 0x13e3c4 SavingFileDialog,
//   0x13e3bc WaitForLeaderboardsDialog, 0x13e3c8 WaitFor2PLoadDialog,
//   0x13e3e4 SavingProfileDialog.
// Update calls it and treats a result >= 1000 as "finished", which is why it is
// the class's one pure virtual.
//
// MSVC x86 gives a virtual destructor one slot where Itanium and ARM give it two,
// so the absolute slot numbers run one lower here; only the order is copied.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Dialog.h"
#include "Font.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// SavingDingusDialog is the base of SavingFileDialog, SavingProfileDialog,
// WaitForLeaderboardsDialog, WaitForRemoteFileDialog and WaitFor2PLoadDialog, and
// LawnApp.h includes all of those. That made this an include cycle with a
// particularly wide blast radius: entering from ANY of the five derived headers
// reached LawnApp.h, which came back to SavingDingusDialog.h while it was still
// mid-parse, so `#pragma once` skipped the definition and all five derived
// classes reported C2504 in one go (17 diagnostics in a full build).
//
// LawnApp is only ever a pointer here (`LawnApp *mApp` and the constructor's
// first parameter), so the include is replaced by a declaration. Sexy\ReconTypes.h
// already carries it.
class LawnApp;

class SavingDingusDialog : public Sexy::Dialog
{
public:
    virtual ~SavingDingusDialog();
    virtual void AddedToManager(Sexy::WidgetManager *theManager);
    virtual void RemovedFromManager(Sexy::WidgetManager *theManager);
    virtual void Update();
    virtual void Draw(Sexy::Graphics *theGraphics);

    // slot 131, new in this class and pure here. Returns 1000 once whatever this
    // dialog waits for has finished; Update polls it and leaves the dialog up
    // while the result stays below that.
    virtual int GetStatus() = 0;

    // +0x2E0 .. +0x2EB (the first 0x2E0 bytes are Sexy::Dialog).
    float mDingusTimer;   // +0x2E0, constructor takes it as the 5th argument and
                          // Update subtracts 0.01 from it each frame
    LawnApp *mApp;        // +0x2E4
    // Constructor's last argument; Draw checks it before DrawDialogFade, so it is
    // the "draw the fade behind the dingus" switch.
    // TODO(recon): name it once the caller in LawnApp.cpp is reconstructed.
    bool mDrawFade;       // +0x2E8, DrawDialogFade runs only when this is set
    // size 0x2EC

    SavingDingusDialog(LawnApp *theApp, bool theModal, int theDialogId, Sexy::String theText, float theTimer, bool theDrawFade);
};
