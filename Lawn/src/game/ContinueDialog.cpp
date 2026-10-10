// ContinueDialog -- restored from libGameMain.so.
//
// Rewritten by hand in stage 2: the stage-1 body was the IDA decompilation with
// syntactic changes only. Every raw offset, hand-written vtable call and vptr
// store is gone; the members and the virtuals come from ContinueDialog.h and its
// bases. There is no PvZ-src counterpart for this file.

#include "ContinueDialog.h"

#include "game/Board.h"
#include "Dialog.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "LawnStoneButton.h"
#include "LawnPlayerInfo.h"
#include "ReanimationWidget.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"
#include "Zombie.h"

namespace
{
// ChallengePage values, from the authoritative PvZ\Lawn\Common\ConstEnums.h.
// Sexy\ReconTypes.h carries ChallengePage as a one-int surrogate struct with no
// enumerators, so the values this file passes to ShowChallengeScreen are named
// here, the same way ChallengeScreen.cpp names its own.
;
} // namespace

// ==========================================================================
// ContinueDialog::GotGameFocus()
// @001B73BC  2 bytes
// ==========================================================================
void ContinueDialog::GotGameFocus() {}

// ==========================================================================
// ContinueDialog::~ContinueDialog()
// @001B73C0  60 bytes
// ==========================================================================
ContinueDialog::~ContinueDialog()
{
    // The two dialog buttons belong to this dialog; the dump deletes each through
    // its virtual deleting destructor (`(*(vptr + 4))(button)`), which is what
    // `delete` emits. The two vptr stores are compiler-generated now that the
    // bases are real.
    delete mContinueButton;
    delete mNewGameButton;
}

// ==========================================================================
// ContinueDialog::ContinueDialog(LawnApp *)
// @001B7434  872 bytes
// ==========================================================================
ContinueDialog::ContinueDialog(LawnApp *theApp)
    // The dump's placement-new of LawnDialog over `this` and the three string
    // temporaries it was handed (StringCtor is StringCtor) are this initialiser
    // list. The two vptr stores are compiler-generated.
    :
    LawnDialog(theApp, nullptr, 39, true, "[CONTINUE_GAME_HEADER]", "", "[DIALOG_BUTTON_CANCEL]", 3)
{
    if (theApp->IsAdventureMode())
    {
        // The ARM tests `IsAdventureMode` and jumps AWAY to the other branch when
        // it is zero (0x1b74d8/0x1b74da), so adventure is the one that restarts
        // the level; the PC reference source agrees.
        Sexy::String aText;
        TodStringTranslate(&aText, "[CONTINUE_GAME_OR_RESTART]");
        mDialogLines = aText;

        mContinueButton = MakeButton(ContinueDialog_Continue,
            static_cast<Sexy::ButtonListener *>(this),
            this,
            Sexy::String("[CONTINUE_BUTTON]"));
        mNewGameButton = MakeButton(ContinueDialog_NewGame,
            static_cast<Sexy::ButtonListener *>(this),
            this,
            Sexy::String("[RESTART_LEVEL_BUTTON]"));
    }
    else
    {
        Sexy::String aText;
        TodStringTranslate(&aText, "[CONTINUE_GAME]");
        mDialogLines = aText;

        mContinueButton = MakeButton(ContinueDialog_Continue,
            static_cast<Sexy::ButtonListener *>(this),
            this,
            Sexy::String("[CONTINUE_BUTTON]"));
        mNewGameButton = MakeButton(
            ContinueDialog_NewGame, static_cast<Sexy::ButtonListener *>(this), this, Sexy::String("[NEW_GAME_BUTTON]"));
    }

    // vtable slot 59 (Widget::SetFocusLinks), in the ARM's own order: the
    // continue button and the new-game button both hang off the dialog's yes
    // button (the ARM reads +0x2F0 at 0x1b7572/0x1b758c), and the yes button
    // points back at the continue button.
    mContinueButton->SetFocusLinks(nullptr, mLawnYesButton, nullptr, mNewGameButton);
    mNewGameButton->SetFocusLinks(nullptr, mLawnYesButton, mContinueButton, nullptr);
    mLawnYesButton->SetFocusLinks(mContinueButton, nullptr, nullptr, nullptr);

    mTallBottom = true;
    mDrawQRCode = mApp->NeedShowQRCode();
    CalcSize(10, 60, true);
}

// ==========================================================================
// ContinueDialog::RestartLoopingSounds(void)
// @001B784C  88 bytes
// ==========================================================================
int ContinueDialog::RestartLoopingSounds()
{
    // The rain loop is restarted when this is the raining-seeds challenge or a
    // stormy night level. The ARM jumps straight to PlayFoley when the mode IS
    // 90 (0x1b785a/0x1b785c) and also when IsStormyNightLevel is non-zero
    // (0x1b7862), so this is a disjunction, not a conjunction.
    if (mApp->mGameMode == GAMEMODE_CHALLENGE_RAINING_SEEDS || mApp->IsStormyNightLevel())
        mApp->PlayFoley(FOLEY_RAIN);

    Zombie *aZombie = nullptr;
    int result;
    while ((result = mApp->mBoard->IterateZombies(aZombie)) != 0)
    {
        // Every zombie that had gone quiet is told to start its own loop again.
        if (aZombie->mPlayingSong)
            aZombie->StartZombieSound();
    }
    return result;
}

// ==========================================================================
// ContinueDialog::ButtonDepress(int)
// @001B78A4  302 bytes
// ==========================================================================
void ContinueDialog::ButtonDepress(int theId)
{
    if (theId != ContinueDialog_Continue)
    {
        if (theId == ContinueDialog_NewGame)
        {
            mApp->DoRestartConfirmDialog();
            return;
        }

        // Any other id is the cancel button: leave the level and go back to the
        // screen the mode came from.
        mApp->PostLeaveLevel();
        mApp->KillDialogById(mId);
        if (mApp->IsAdventureMode())
            mApp->ShowGameSelector();
        else if (mApp->IsSurvivalMode())
        {
            mApp->KillBoard();
            mApp->ShowChallengeScreen(CHALLENGE_PAGE_SURVIVAL);
        }
        else if (mApp->IsPuzzleMode())
        {
            mApp->KillBoard();
            mApp->ShowChallengeScreen(CHALLENGE_PAGE_PUZZLE);
        }
        else
        {
            mApp->KillBoard();
            mApp->ShowChallengeScreen(CHALLENGE_PAGE_CHALLENGE);
        }
        return;
    }

    // Continue: unless the survival run is between stages, the save for the
    // current mode and profile is erased, because the level is being replayed.
    if (mApp->mBoard->mNextSurvivalStageCounter != 1)
    {
        Sexy::String aSavedGameName;
        GetSavedGameName(
            &aSavedGameName, mApp->mGameMode, mApp->mPlayerInfo->GetProfileId(), mApp->mPlayerInfo->GetId());
        mApp->EraseFile(aSavedGameName);
    }
    RestartLoopingSounds();
    mApp->KillDialogById(mId);
}

// ==========================================================================
// ContinueDialog::GetPreferredHeight(int)
// @001B79F0  10 bytes
// ==========================================================================
int ContinueDialog::GetPreferredHeight(int theWidth)
{
    return Sexy::Dialog::GetPreferredHeight(theWidth) + 40;
}

// ==========================================================================
// ContinueDialog::Resize(int,int,int,int)
// @001B79FC  98 bytes
// ==========================================================================
void ContinueDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    LawnDialog::Resize(theX, theY, theWidth, theHeight);

    // ContinueDialog::Resize @0x1B79FC places both choices above Cancel.
    int buttonHeight = mLawnYesButton->mHeight;
    int buttonY = mLawnYesButton->mY - buttonHeight;
    int buttonWidth = mLawnYesButton->mWidth / 2 + 10;
    mContinueButton->Resize(mLawnYesButton->mX - 20, buttonY, buttonWidth, buttonHeight);
    mNewGameButton->Resize(
        mLawnYesButton->mX + mLawnYesButton->mWidth - buttonWidth + 20, buttonY, buttonWidth, buttonHeight);
}

// ==========================================================================
// ContinueDialog::AddedToManager(Sexy::WidgetManager *)
// @001B7A60  34 bytes
// ==========================================================================
void ContinueDialog::AddedToManager(Sexy::WidgetManager *theManager)
{
    LawnDialog::AddedToManager(theManager);
    AddWidget(mContinueButton);
    AddWidget(mNewGameButton);
}

// ==========================================================================
// ContinueDialog::RemovedFromManager(Sexy::WidgetManager *)
// @001B7A84  34 bytes
// ==========================================================================
void ContinueDialog::RemovedFromManager(Sexy::WidgetManager *theManager)
{
    LawnDialog::RemovedFromManager(theManager);
    RemoveWidget(mContinueButton);
    RemoveWidget(mNewGameButton);
}

// ==========================================================================
// ContinueDialog::KeyDown(Sexy::KeyCode)
// @001B7AA8  4 bytes
// ==========================================================================
int ContinueDialog::KeyDown(Sexy::KeyCode theKey)
{
    return LawnDialog::KeyDown(theKey);
}

// ==========================================================================
// ContinueDialog::KeyUp(Sexy::KeyCode)
// @001B7AAC  32 bytes
// ==========================================================================
int ContinueDialog::KeyUp(Sexy::KeyCode theKey)
{
    if (theKey != Sexy::KEYCODE_ESCAPE)
        return Sexy::Dialog::KeyUp(theKey);
    // Escape presses the yes button: the ARM reads the button's widget id
    // (+0x70, WidgetContainer::mWidgetId) from the word at +0x2F0, which is
    // mLawnYesButton, and calls the base's own ButtonDepress (slot 129) with it,
    // so this class's override is not re-entered.
    Sexy::Dialog::ButtonDepress(mLawnYesButton->mWidgetId);
    return 1;
}

// ==========================================================================
// ContinueDialog::GotFocus()
// @001B7ACC  34 bytes
// ==========================================================================
void ContinueDialog::GotFocus()
{
    bool hadFocusedChild = mFocusedChildWidget != nullptr;
    LawnDialog::GotFocus();
    // Nothing in this container holds the focus yet, so hand it to the continue
    // button and raise it (Widget::SetFocus, slot 57).
    if (!hadFocusedChild)
        SetFocus(mContinueButton, true);
}
