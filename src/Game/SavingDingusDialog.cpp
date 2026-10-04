// SavingDingusDialog -- restored from libGameMain.so.
//
// Rewritten by hand in stage 2: the stage-1 body was the IDA decompilation with
// syntactic changes only. Every raw offset, hand-written vtable call and vptr
// store is gone; the members and the virtuals come from SavingDingusDialog.h and
// its bases.

#include "SavingDingusDialog.h"

#include "Dialog.h"
#include "Font.h"
#include "Graphics.h"
#include "LawnApp.h"
#include "WidgetManager.h"

// ==========================================================================
// SavingDingusDialog::~SavingDingusDialog()
// @0013E6F8  34 bytes
// ==========================================================================
// The two stores are the object's own vptr and the vptr of the
// Sexy::ButtonListener sub-object at +0x100 (Widget is 0x100 bytes, see
// Dialog.h); a real base class makes the compiler emit both, and the explicit
// `~Dialog()` call is the base destructor it already runs.
SavingDingusDialog::~SavingDingusDialog()
{
}

// ==========================================================================
// SavingDingusDialog::Draw(Sexy::Graphics *)
// @0013EB10  34 bytes
// ==========================================================================
// mDrawFade is the constructor's last argument, checked before the fade, so it is
// the "draw the dialog fade behind the dingus" switch.
void SavingDingusDialog::Draw(Sexy::Graphics *theGraphics)
{
    if (mDrawFade)
        mApp->DrawDialogFade(theGraphics);

    mApp->DrawSavingDingus(theGraphics);
}

// ==========================================================================
// SavingDingusDialog::SavingDingusDialog(LawnApp *,bool,int,Sexy::String,float,bool)
// @0013F634  352 bytes
// ==========================================================================
// The dump builds three empty Sexy::Strings and then releases them by hand; they
// are the base constructor's header/lines/footer arguments. The vptr stores
// after them are the compiler's.
//
// The base call is
// `Sexy::Dialog::Dialog(this, 0, 0, a4, a3, &hdr, &lines, &footer, 0)` -- the
// component image and the dialog image are both null, the id is the 3rd argument
// and the modal flag the 2nd.
SavingDingusDialog::SavingDingusDialog(LawnApp *theApp, bool theModal, int theDialogId, Sexy::String theText, float theTimer, bool theDrawFade)
    : Sexy::Dialog(nullptr, nullptr, theDialogId, theModal, "", "", "", 0),
      mDingusTimer(theTimer),
      mApp(theApp),
      mDrawFade(theDrawFade)
{
    // The dump calls Dialog::SetHeaderFont directly (0x301c78) but reaches
    // SetLinesFont through the vptr (byte 0x1F0 -- slot 124).
    Sexy::Dialog::SetHeaderFont(FONT_DWARVENTODCRAFT24);
    SetLinesFont(FONT_BRIANNETOD16);

    // The dump tests the string's rep length (`*(_DWORD *)(*a5 - 12) != 0`),
    // i.e. "is the text non-empty".
    if (!theText.empty())
        mApp->SetDingusText(theText);
}

// ==========================================================================
// SavingDingusDialog::AddedToManager(Sexy::WidgetManager *)
// @00141160  20 bytes
// ==========================================================================
void SavingDingusDialog::AddedToManager(Sexy::WidgetManager *theManager)
{
    Sexy::Dialog::AddedToManager(theManager);
    mApp->ShowSavingDingus();
}

// ==========================================================================
// SavingDingusDialog::RemovedFromManager(Sexy::WidgetManager *)
// @00141174  20 bytes
// ==========================================================================
void SavingDingusDialog::RemovedFromManager(Sexy::WidgetManager *theManager)
{
    Sexy::Dialog::RemovedFromManager(theManager);
    mApp->HideSavingDingus();
}

// ==========================================================================
// SavingDingusDialog::Update()
// @001411C8  82 bytes
// ==========================================================================
// The dingus runs on a countdown and then polls GetStatus -- the pure virtual
// this class leaves to its five derived classes (see SavingDingusDialog.h). A
// result of 1000 or more means "finished": a modal dialog records it as its
// result, a non-modal one is killed by id.
//
// The dump's `*(_DWORD *)this + 524` is the vptr plus 524 bytes, i.e. slot 131;
// the transcriber read 2096 as an object offset, which is what made this class
// look 0x840 bytes wide.
void SavingDingusDialog::Update()
{
    Sexy::Dialog::Update();

    mDingusTimer -= 0.01f;
    if (mDingusTimer <= 0.0f)
    {
        int aStatus = GetStatus();
        if (aStatus >= 1000)
        {
            if (mIsModal)
                mResult = aStatus;
            else
                mApp->KillDialog(mId);
        }
    }
}
