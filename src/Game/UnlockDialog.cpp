// UnlockDialog -- reconstructed from libGameMain.so.
//
// The bodies below are the transcribed IDA decompilation, rewritten by hand into
// real C++ against the reconstructed headers.  Every slot call was re-derived
// from the binary's own vtable, because the transcriber rendered
//
//     (*(fn **)(*(_DWORD *)((char *)this + N)))(...)
//
// for calls that are really `vptr[N / 4]`.  Two sites in this file were of that
// shape and both were wrong as written:
//
//     GotFocus    +912 -> vptr[57]  0xE4  Sexy::Widget::SetFocus(Widget*, bool)
//     constructor +800 -> vptr[50]  0xC8  LawnDialog::Resize(int,int,int,int)
//
// (0x390 / 4 == 0xE4 and 0x320 / 4 == 0xC8; the transcriber had multiplied the
// vtable byte offset by 4 a second time.)

#include "UnlockDialog.h"

#include "ChallengeScreen.h"
#include "Dialog.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "LawnPlayerInfo.h"
#include "MainMenu.h"
#include "WidgetManager.h"

// ==========================================================================
// UnlockDialog::~UnlockDialog()
// @0013E5B4  34 bytes
// ==========================================================================
// The binary stores this class's vtable and the Dialog sub-object's vtable and
// then calls the LawnDialog base destructor; the compiler emits all three for a
// real class chain, so the body is empty.
UnlockDialog::~UnlockDialog()
{
}

// ==========================================================================
// UnlockDialog::GotFocus()
// @0013EA44  34 bytes
// ==========================================================================
// `BL LawnDialog::GotFocus / LDR R3,[R4,#0xDC] / CBZ` -- if there is already a
// focused child widget, nothing else happens; otherwise the yes button takes the
// focus with force = true (`MOVS R2,#1` is SetFocus's second parameter).
void UnlockDialog::GotFocus()
{
    LawnDialog::GotFocus();

    if (mFocusedChildWidget != nullptr)
        return;

    SetFocus(mLawnYesButton, true);
}

// ==========================================================================
// UnlockDialog::MouseDrag(int,int)
// @0013EABC  4 bytes
// ==========================================================================
void UnlockDialog::MouseDrag(int theX, int theY)
{
    Sexy::Dialog::MouseDrag(theX, theY);
}

// ==========================================================================
// UnlockDialog::ButtonDepress(int)
// @0013ECCC  270 bytes
// ==========================================================================
void UnlockDialog::ButtonDepress(int theId)
{
    // 1001 is the cancel button: the dialog just closes.
    if (theId == 1001)
    {
        mApp->KillDialog(18);
        return;
    }

    if (theId != 1000)
        return;

    // 1000 is the "unlock" button.  GotoBuyTool returns non-zero when it really
    // opened the shop page, and the rest only runs in that case.
    switch (mUnlockedItemId)
    {
    case 1:
        if (mApp->GotoBuyTool(7) == 0)
            return;
        break;
    case 2:
        if (mApp->GotoBuyTool(8) == 0)
            return;
        break;
    case 3:
        if (mApp->GotoBuyTool(10) == 0)
            return;
        break;
    default:
        return;
    }

    mApp->LawnMessageBox(51, "[MODE_UNLOCK]", "[MODEGAME_UNLOCK_MESSAGE]", "[DIALOG_BUTTON_OK]",
                         gEmptyCString, 3);
    mWidgetManager->SetFocus(this);

    switch (mUnlockedItemId)
    {
    case 1:
        mApp->mPlayerInfo->SetFlag(16, 1);
        mApp->mGameSelector->SyncProfile(true);
        break;
    case 2:
        mApp->mPlayerInfo->SetFlag(32, 1);
        mApp->mGameSelector->SyncProfile(true);
        break;
    case 3:
        {
            // Unlock the next survival challenge button: re-enable it and give it
            // the finger highlight (+0x76 is Widget::mDisabled, +0x98 is
            // Widget::mDoFinger).
            ChallengeScreen *aChallengeScreen = mApp->mChallengeScreen;
            int aNext = aChallengeScreen->mSurvivalCount + 1;
            Sexy::ButtonWidget *aButton = aChallengeScreen->mChallengeButtons[aChallengeScreen->mSurvivalCount];
            aButton->mDoFinger = true;
            aButton->mDisabled = false;

            // The binary wraps 10 to 12 here (the two survival sub-pages).
            aChallengeScreen->mSurvivalCount = (aNext == 10) ? 12 : aNext;
            aChallengeScreen->SaveSurvivalCount();
        }
        break;
    default:
        break;
    }

    mApp->KillDialog(18);
}

// ==========================================================================
// UnlockDialog::UnlockDialog(int,int)
// @00140B7C  566 bytes
// ==========================================================================
// The three `StringCtor` calls and the refcount dance that follows each of them
// are the construction and destruction of three temporary Sexy::String objects,
// which real C++ does by itself; the strings are passed straight to the
// LawnDialog base constructor here.
//
// `mDialogLines` doubles as the format string, exactly as the binary does it
// (`LDR.W R1,[R4,#0x120]` before the StrFormat call, then the result is assigned
// back to it).
UnlockDialog::UnlockDialog(int theLevel, int theItemId)
    : LawnDialog((LawnApp *)gLawnApp, nullptr, 18, 1, Sexy::String("[MODE_LOCKED]"),
                 Sexy::String("[UNLOCK_MODE_MSG]"), Sexy::String(gEmptyCString), 1)
{
    // mDialogLines doubles as the format string, exactly as the binary does it
    // (`LDR.W R1,[R4,#0x120]` before the StrFormat call, then the result is
    // assigned back to it).  StrFormat formats into its own buffer before it
    // assigns, so reusing the member as both source and destination is safe.
    StrFormat(&mDialogLines, mDialogLines.c_str(), theLevel);
    mUnlockedItemId = theItemId;

    mLawnYesButton->SetLabel("[UNLOCK_MODE_YES_BUTTON]");
    mLawnNoButton->SetLabel("[UNLOCK_MODE_NO_BUTTON]");

    CalcSize(300, 30, true);

    // Centre the dialog on the 800x600 screen without changing its size.
    Resize((800 - mWidth) / 2, (600 - mHeight) / 2, mWidth, mHeight);
}

// ==========================================================================
// UnlockDialog::AddedToManager(Sexy::WidgetManager *)
// @00141158  4 bytes
// ==========================================================================
void UnlockDialog::AddedToManager(Sexy::WidgetManager *theWidgetManager)
{
    LawnDialog::AddedToManager(theWidgetManager);
}

// ==========================================================================
// UnlockDialog::RemovedFromManager(Sexy::WidgetManager *)
// @001412B0  4 bytes
// ==========================================================================
void UnlockDialog::RemovedFromManager(Sexy::WidgetManager *theWidgetManager)
{
    LawnDialog::RemovedFromManager(theWidgetManager);
}
