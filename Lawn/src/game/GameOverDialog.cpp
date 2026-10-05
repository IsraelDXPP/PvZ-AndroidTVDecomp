// GameOverDialog -- restored from libGameMain.so.
//
// Rewritten by hand in stage 2: the stage-1 body was the IDA decompilation with
// syntactic changes only. Every raw offset, hand-written vtable call and vptr
// store is gone; the members and the virtuals come from GameOverDialog.h and its
// bases. There is no PvZ-src counterpart for this file.

#include "GameOverDialog.h"

// The constructor reaches into the app's board to drop three of its hints, and
// LawnApp.h only forward-declares Board, so the complete type is needed here.
#include "game/Board.h"
#include "Dialog.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "WidgetManager.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// ==========================================================================
// GameOverDialog::~GameOverDialog()
// @0013E68C  34 bytes
// ==========================================================================
GameOverDialog::~GameOverDialog()
{
  // The primary and the ButtonListener vptrs are the compiler's own work now
  // that Dialog really derives from Sexy::ButtonListener, and LawnDialog's
  // destructor runs by itself.
}

// ==========================================================================
// GameOverDialog::GotFocus(void)
// @0013E9D8  34 bytes
// ==========================================================================
void GameOverDialog::GotFocus()
{
  LawnDialog::GotFocus();
  // Nothing in this container holds the focus yet, so hand it to the retry
  // button and raise it (Widget::SetFocus, slot 57).
  if ( mFocusedChildWidget == nullptr )
    SetFocus(mLawnYesButton, true);
}

// ==========================================================================
// GameOverDialog::MouseDrag(int,int)
// @0013EAB0  4 bytes
// ==========================================================================
void GameOverDialog::MouseDrag(int theX, int theY)
{
  Sexy::Dialog::MouseDrag(theX, theY);
}

// ==========================================================================
// GameOverDialog::ButtonDepress(int)
// @0013EB80  212 bytes
// ==========================================================================
void GameOverDialog::ButtonDepress(int theId)
{
  if ( theId == 1001 )
  {
    mApp->PostLeaveLevel();
    // The current game mode decides which screen the player falls back to, so
    // it is read before the level is torn down.
    mApp->IsSurvivalEndless(mApp->mGameMode);
    mApp->KillDialogById(16);
    mApp->KillBoard();
    if ( mApp->IsSurvivalMode() )
    {
      mApp->ShowChallengeScreen(ChallengePage(0));  // CHALLENGE_PAGE_SURVIVAL
    }
    else if ( mApp->IsPuzzleMode() )
    {
      mApp->ShowChallengeScreen(ChallengePage(4));  // CHALLENGE_PAGE_PUZZLE
    }
    else if ( mApp->IsAdventureMode() )
    {
      mApp->ShowGameSelector();
    }
    else if ( mApp->IsCoopMode() )
    {
      mApp->ShowChallengeScreen(ChallengePage(3));  // CHALLENGE_PAGE_COOP
    }
    else
    {
      mApp->ShowChallengeScreen(ChallengePage(1));  // CHALLENGE_PAGE_CHALLENGE
    }
  }
  else if ( theId == 1000 )
  {
    mApp->PostLeaveLevel();
    mApp->PostEnterLevel();
    mApp->KillDialogById(16);
    mApp->EndLevel();
  }
}

// ==========================================================================
// GameOverDialog::GameOverDialog(Sexy::String const&,bool)
// @00140258  574 bytes
// ==========================================================================
GameOverDialog::GameOverDialog(const Sexy::String& theMessage, bool theShowTrophy)
    // The base is a real base now, so the dump's placement-new of LawnDialog
    // over `this` and the two string temporaries it was handed (StringCtor is
    // StringCtor) are this initialiser list. The vptr stores at +0x00 and
    // +0x100 (Dialog's ButtonListener sub-object) are compiler-generated.
    : LawnDialog((LawnApp *)gLawnApp, nullptr, 16, true, "[GAME_OVER]", theMessage, "", 1)
{

  // The two dialog buttons the base created, relabelled.
  mLawnYesButton->SetLabel("[TRY_AGAIN]");
  mLawnNoButton->SetLabel("[QUIT_BUTTON]");

  if ( theShowTrophy )
  {
    // The survival and puzzle screens pass the challenge's own name as the
    // dialog text; the sixth word of the challenge definition record is it.
    const ChallengeDefinition *aChallengeDef = mApp->GetCurrentChallengeDef();
    Sexy::String aChallengeName;
    TodStringTranslate(&aChallengeName, aChallengeDef->mChallengeName);
    mDialogLines = aChallengeName;
  }
  // An empty header string means the dialog has no title bar, so the layout
  // gets the extra fifteen pixels that bar would have used.
  if ( theMessage.empty() )
    mSpaceAfterHeader += 15;
  CalcSize(300, 30, true);
  // vtable slot 50: this class does not override Resize, so this is
  // LawnDialog's, which centres the dialog on the 800x600 screen.
  Resize((800 - mWidth) / 2, (600 - mHeight) / 2, mWidth, mHeight);
  // The dialog's own backdrop is hidden -- LawnDialog draws the standard one --
  // and the QR code is only shown when the app asks for it.
  mClip = false;
  mDrawQRCode = mApp->NeedShowQRCode();
  // The app's board, if there is one, drops the three "show me" hints.
  Board *aBoard = (Board *)((LawnApp *)gLawnApp)->mBoard;
  if ( aBoard != nullptr )
  {
    aBoard->mShowShovel = false;
    aBoard->mShowButter = false;
    aBoard->mShowHammer = false;
  }
}

// ==========================================================================
// GameOverDialog::AddedToManager(Sexy::WidgetManager *)
// @00141138  4 bytes
// ==========================================================================
void GameOverDialog::AddedToManager(Sexy::WidgetManager *theManager)
{
  LawnDialog::AddedToManager(theManager);
}

// ==========================================================================
// GameOverDialog::RemovedFromManager(Sexy::WidgetManager *)
// @00141290  4 bytes
// ==========================================================================
void GameOverDialog::RemovedFromManager(Sexy::WidgetManager *theManager)
{
  LawnDialog::RemovedFromManager(theManager);
}
