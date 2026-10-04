// CheatDialog -- restored from libGameMain.so.
//
// Rewritten by hand in stage 2: the stage-1 body was the IDA decompilation with
// syntactic changes only. Every raw offset, hand-written vtable call and vptr
// store is gone; the members and the virtuals come from CheatDialog.h and its
// bases (LawnDialog / Sexy::Dialog / Sexy::Widget, and the listener base).

#include "CheatDialog.h"

#include "Dialog.h"
#include "EditWidget.h"
#include "Font.h"
#include "Graphics.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "LawnPlayerInfo.h"
#include "Level.h"
#include "ReanimationWidget.h"
#include "WidgetManager.h"

// The libc ctype table the binary reaches as `ctype_`: one byte per entry,
// indexed by the character value plus one (the body spells the +1 itself).
// ReconData.h declares it and ReconData.cpp defines it, so it must not be
// redeclared here: a file-local `ctype_` is a different type and is C2373.
// TODO(recon): no platform table is wired up, so every class bit reads as 0.

// ==========================================================================
// CheatDialog::EditWidgetText(int,Sexy::String const&)
// @001919C0  24 bytes
// ==========================================================================
int CheatDialog::EditWidgetText(int theId)
{
    // The receiver is this object, not a1: IDA's prototype dropped the implicit
    // parameter. mApp's vtable at byte 16 is slot 4, LawnApp::ButtonDepress,
    // called with the dialog's own id plus the framework's 2000 convention.
    mApp->ButtonDepress(mId + 2000);
    return 0;
}

// ==========================================================================
// CheatDialog::~CheatDialog()
// @001919E0  56 bytes
// ==========================================================================
CheatDialog::~CheatDialog()
{
    // The three vptr stores and the explicit ~LawnDialog() call are the
    // compiler's; what is left is the owned EditWidget, released through its
    // deleting destructor behind a null test, i.e. `delete`.
    if ( mEditWidget != nullptr )
        delete mEditWidget;
}

// ==========================================================================
// CheatDialog::AllowChar(int,char)
// @00191A50  62 bytes
// ==========================================================================
bool CheatDialog::AllowChar(int theId, char theChar)
{
    (void)theId;
    // Digits, the cheat prefixes 'c'/'C' and 'f'/'F', and the level separator.
    if ( (ctype_[theChar + 1] & 4) != 0 )
        return true;
    if ( theChar == 'c' || theChar == '-' )
        return true;
    return theChar == 'C' || theChar == 'f' || theChar == 'F';
}

// ==========================================================================
// CheatDialog::CheatDialog(LawnApp *)
// @00191AA0  718 bytes
// ==========================================================================
CheatDialog::CheatDialog(LawnApp *theApp)
    : LawnDialog(theApp, nullptr, 36, true, "CHEAT", "Enter New Level:", "", 2)
{
    // The three Sexy::String temporaries the dump builds and then releases by
    // hand are the base constructor's header/lines/footer arguments; the vptr
    // stores that follow them are the compiler's.
    mVerticalCenterText = false;
    mApp = theApp;
    // CreateEditWidget is handed this dialog's listener sub-object at +0x2FC.
    // The binary's base there is an EditListener; CheatDialog.h models it as
    // Sexy::ButtonListener, so the sub-object is taken through that base.
    mEditWidget = CreateEditWidget(0, (Sexy::EditListener *)this, this);
    mEditWidget->mMaxChars = 12;
    mEditWidget->AddWidthCheckFont(FONT_BRIANNETOD12, 220);

    // The level the edit box starts with: "C<mode>" for a custom level,
    // "F<area>-<sub>" once the adventure is finished, "<area>-<sub>" otherwise.
    Sexy::String aLevelString;
    if ( mApp->mGameMode != 0 )
    {
        Sexy::String aFormatted;
        StrFormat(&aFormatted, "C%d", mApp->mGameMode);
        aLevelString = aFormatted;
    }
    else if ( mApp->HasFinishedAdventure() )
    {
        // LawnApp::GetStageString writes its result through the receiver, which
        // is the sret buffer the transcription kept as `this`.
        Sexy::String aStage = mApp->GetStageString(mApp->mPlayerInfo->GetLevel());
        Sexy::String aFormatted;
        StrFormat(&aFormatted, "F%s", aStage.c_str());
        aLevelString = aFormatted;
    }
    else
    {
        aLevelString = mApp->GetStageString(mApp->mPlayerInfo->GetLevel());
    }
    mEditWidget->SetText(aLevelString, true, true, true);
    CalcSize(110, 40, true);
}

// ==========================================================================
// CheatDialog::ApplyCheat(void)
// @00191E04  538 bytes
// ==========================================================================
int CheatDialog::ApplyCheat()
{
    const char *aText = mEditWidget->mString.c_str();
    int aLevel;
    if ( sscanf(aText, "c%d", &aLevel) == 1 || sscanf(aText, "C%d", &aLevel) == 1 )
    {
        // A custom level number, carried in the mode word and clamped to 0..90.
        mApp->KillBoard();
        if ( aLevel < 0 )
            aLevel = 0;
        else if ( aLevel >= 90 )
            aLevel = 90;
        mApp->mGameMode = (GameMode)aLevel;
        return 1;
    }

    // Every other form names a level: "<area>-<sub>" combines into
    // 10 * (area - 1) + sub, and a leading 'f'/'F' additionally flags the level.
    int aArea;
    int aSub;
    int aNewLevel = -1;
    int aFlag = 0;
    if ( sscanf(aText, "f%d-%d", &aArea, &aSub) == 2 || sscanf(aText, "F%d-%d", &aArea, &aSub) == 2 )
    {
        aNewLevel = aSub + 10 * (aArea - 1);
        aFlag = 1;
    }
    else if ( sscanf(aText, "f%d", &aNewLevel) == 1 || sscanf(aText, "F%d", &aNewLevel) == 1 )
    {
        aFlag = 1;
    }
    else if ( sscanf(aText, "%d-%d", &aArea, &aSub) == 2 )
    {
        aNewLevel = aSub + 10 * (aArea - 1);
    }
    else
    {
        sscanf(aText, "%d", &aNewLevel);
    }

    if ( aNewLevel <= 0 )
    {
        mApp->DoDialog(38, true, "Enter Level",
                       "Invalid Level. Do 'number' or 'area-subarea' or 'Cnumber' or 'Farea-subarea'.",
                       "OK", 3);
        return 0;
    }

    mApp->mGameMode = GAMEMODE_ADVENTURE;
    mApp->mGameScene = SCENE_LEVEL_INTRO;
    mApp->mPlayerInfo->SetLevel(aNewLevel);
    if ( aFlag != 0 )
        mApp->mPlayerInfo->SetFlag(aFlag, aFlag);
    mApp->TryToSaveProfile();
    return 1;
}

// ==========================================================================
// CheatDialog::GetPreferredHeight(int)
// @00192080  10 bytes
// ==========================================================================
int CheatDialog::GetPreferredHeight(int theWidth)
{
    return Sexy::Dialog::GetPreferredHeight(theWidth) + 40;
}

// ==========================================================================
// CheatDialog::AddedToManager(Sexy::WidgetManager *)
// @0019208C  38 bytes
// ==========================================================================
void CheatDialog::AddedToManager(Sexy::WidgetManager *theWidgetManager)
{
    LawnDialog::AddedToManager(theWidgetManager);
    AddWidget(mEditWidget);
    SetFocus(mEditWidget, true);
}

// ==========================================================================
// CheatDialog::RemovedFromManager(Sexy::WidgetManager *)
// @001920B4  22 bytes
// ==========================================================================
void CheatDialog::RemovedFromManager(Sexy::WidgetManager *theWidgetManager)
{
    LawnDialog::RemovedFromManager(theWidgetManager);
    RemoveWidget(mEditWidget);
}

// ==========================================================================
// CheatDialog::Resize(int,int,int,int)
// @001920CC  64 bytes
// ==========================================================================
void CheatDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    // The edit box sits inside the content insets, above the two buttons.
    LawnDialog::Resize(theX, theY, theWidth, theHeight);
    mEditWidget->Resize(mContentInsets.mLeft + 12,
                        mWidth - 155,
                        mHeight - mContentInsets.mLeft - mContentInsets.mRight - 24,
                        28);
}

// ==========================================================================
// CheatDialog::Draw(Sexy::Graphics *)
// @0019210C  24 bytes
// ==========================================================================
void CheatDialog::Draw(Sexy::Graphics *theGraphics)
{
    LawnDialog::Draw(theGraphics);
    DrawEditBox(theGraphics, mEditWidget);
}

// ==========================================================================
// CheatDialog::GotFocus()
// @00192124  48 bytes
// ==========================================================================
void CheatDialog::GotFocus()
{
    // Focus the edit box unless something else already holds the focus. The dump
    // reads mFocusedChildWidget twice -- once before the base call and once
    // after -- because LawnDialog::GotFocus can move it, so both reads are kept.
    Widget *aFocusedBefore = mFocusedChildWidget;
    LawnDialog::GotFocus();

    if ( aFocusedBefore == nullptr || mFocusedChildWidget == nullptr ||
         mFocusedChildWidget == mEditWidget )
        SetFocus(mEditWidget, true);
}
