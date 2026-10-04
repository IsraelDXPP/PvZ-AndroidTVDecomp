// BuySunMoneyDialog -- restored from libGameMain.so.
//
// Rewritten by hand in stage 2: the stage-1 body was the IDA decompilation with
// syntactic changes only. Every raw offset, hand-written vtable call and vptr
// store is gone; the members and the virtuals come from BuySunMoneyDialog.h and
// its bases.

#include "BuySunMoneyDialog.h"

#include "Dialog.h"
#include "GameButton.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "WidgetManager.h"
#include "Zombie.h"
// ==========================================================================
// BuySunMoneyDialog::~BuySunMoneyDialog()
// @0013E548  34 bytes
// ==========================================================================
// The two stores are the object's own vptr and the vptr of the
// Sexy::ButtonListener sub-object at +0x100 (Widget is 0x100 bytes, see
// Dialog.h); a real base class makes the compiler emit both, and the explicit
// `~LawnDialog()` call is the base destructor it already runs.
BuySunMoneyDialog::~BuySunMoneyDialog()
{
}

// ==========================================================================
// BuySunMoneyDialog::GotFocus()
// @0013EA68  34 bytes
// ==========================================================================
// The dump hands the focus to the Yes button (LawnDialog's +0x2F0) only when
// nothing holds it: it reads Widget::mFocusedChildWidget at +0xDC and calls
// through vptr byte 0xE4 -- slot 57, Widget::SetFocus.
void BuySunMoneyDialog::GotFocus()
{
    LawnDialog::GotFocus();

    if (mFocusedChildWidget == nullptr)
        SetFocus(mLawnYesButton, true);
}

// ==========================================================================
// BuySunMoneyDialog::MouseDrag(int,int)
// @0013EAC0  4 bytes
// ==========================================================================
// A bare tail call to Dialog's own implementation (0x301e68), not a virtual one.
void BuySunMoneyDialog::MouseDrag(int x, int y)
{
    Sexy::Dialog::MouseDrag(x, y);
}

// ==========================================================================
// BuySunMoneyDialog::ButtonDepress(int)
// @0013EE58  144 bytes
// ==========================================================================
// 1000 is the OK button and 1001 the Cancel button of the two-button dialog the
// constructor builds.
//
// Both branches end the same way: `mApp->KillDialog(84)` -- the dump's
// `*(_DWORD *)(vptr + 428)`, byte 428 being slot 107, which LawnApp.h:463 names
// KillDialog -- where 84 is the id the constructor passes to LawnDialog.
//
// The other indirect call, `*(_DWORD *)(*(_DWORD *)(this + 24) + 48)`, is
// WidgetContainer::mWidgetManager at +0x18 and slot 12, i.e.
// `mWidgetManager->SetFocus(this)` -- and the receiver really is `this`, so the
// dialog takes the focus back after the message box has gone.
void BuySunMoneyDialog::ButtonDepress(int theId)
{
    if (theId == 1001)
    {
        mApp->KillDialog(84);
        mZombie->BuyRevivalItem();
    }
    else if (theId == 1000)
    {
        if (mApp->GotoBuyTool((E_SHOP_PACKET)11) != 0)
        {
            mApp->LawnMessageBox(51,
                                 "[SHOP_BUYSUNMONEY_SUCCEED]",
                                 "[SHOP_BUYSUNMONEY_SUCCEEDMSG]",
                                 "[DIALOG_BUTTON_OK]",
                                 "",
                                 3);
            mWidgetManager->SetFocus(this);
            mApp->AddBuySunMoneyCount();
            mApp->KillDialog(84);
            mZombie->BuySunMoneyItem();
        }
    }
}

// ==========================================================================
// BuySunMoneyDialog::BuySunMoneyDialog(Zombie *,int)
// @00140E30  590 bytes
// ==========================================================================
// The three Sexy::String temporaries the dump builds and then releases by hand
// are the base constructor's header/lines/footer arguments; the two vptr stores
// after them are the compiler's.
//
// The message the dialog shows is the "[SHOP_BUYSUNMONEY_MSG]" line run through
// StrFormat with the price (`50 * mApp->mInitialSunMoney`) and the caller's
// second argument, and the result replaces mDialogLines before the two buttons
// get their labels.
BuySunMoneyDialog::BuySunMoneyDialog(Zombie *theZombie, int theSunAmount)
    : LawnDialog(gLawnApp, nullptr, 84, true,
                 "[SHOP_BUYSUNMONEY_LABEL]", "[SHOP_BUYSUNMONEY_MSG]", "", 1)
{
    // The dump reads the string's character data before formatting and tests
    // nothing; StrFormat writes into a separate temporary that is then assigned.
    Sexy::String aFormatted;
    StrFormat(&aFormatted, mDialogLines.c_str(), 50, theSunAmount, 50 * mApp->mInitialSunMoney);
    mDialogLines = aFormatted;

    mZombie = theZombie;

    // vptr byte 0x1FC -- slot 127, GameButton::SetLabel.
    mLawnYesButton->SetLabel("[DIALOG_BUTTON_OK]");
    mLawnNoButton->SetLabel("[DIALOG_BUTTON_CANCEL]");

    CalcSize(300, 30, true);
    // vptr byte 200 -- slot 50, Widget::Resize. The dump's prototype lists only
    // the two coordinates; the size arguments are the unchanged mWidth/mHeight.
    Resize((800 - mWidth) / 2, (600 - mHeight) / 2, mWidth, mHeight);
}

// ==========================================================================
// BuySunMoneyDialog::AddedToManager(Sexy::WidgetManager *)
// @0014115C  4 bytes
// ==========================================================================
// A bare tail call to LawnDialog's implementation (0x1410fc); the class only
// exists to make the slot non-abstract.
void BuySunMoneyDialog::AddedToManager(Sexy::WidgetManager * theWidgetManager)
{
    LawnDialog::AddedToManager(theWidgetManager);
}

// ==========================================================================
// BuySunMoneyDialog::RemovedFromManager(Sexy::WidgetManager *)
// @001412B4  4 bytes
// ==========================================================================
// As above: a bare tail call to LawnDialog::RemovedFromManager (0x141254).
void BuySunMoneyDialog::RemovedFromManager(Sexy::WidgetManager * theWidgetManager)
{
    LawnDialog::RemovedFromManager(theWidgetManager);
}
