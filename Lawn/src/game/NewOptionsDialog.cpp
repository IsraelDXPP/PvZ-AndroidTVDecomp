// NewOptionsDialog -- reconstructed from libGameMain.so.

#include "NewOptionsDialog.h"

#include "game/Board.h"
#include "Challenge.h"
// The bodies read members of Board::mGamepadControls[i], which GamepadControls.h
// only forward-declares, so the complete type is needed here.
#include "ConfirmBackToMainDialog.h"
#include "CutScene.h"
#include "Dialog.h"
#include "GamepadApp.h"
#include "GamepadControls.h"
#include "Graphics.h"
#include "Image.h"
// The constructor scales the label font through ImageFont::SetScale.
#include "ImageFont.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "SexyAppBase.h"
#include "TodFoley.h"
#include "VSResultsMenu.h"
#include "Widget.h"
#include "WidgetManager.h"
#include "ZenGarden.h"
// ==========================================================================
// NewOptionsDialog::GetPreferredHeight(int)
// @00141304  14 bytes
// ==========================================================================
int NewOptionsDialog::GetPreferredHeight(int theWidth)
{
  (void)theWidth;
  return (IMAGE_OPTIONS_MENUBACK)->GetWidth();
}

// ==========================================================================
// NewOptionsDialog::SliderVal(int,double)
// @0014131C  2 bytes
// ==========================================================================
void NewOptionsDialog::SliderVal(int theId, double theValue)
{
  // Empty in the binary too: this dialog has no sliders, but SliderListener is
  // one of its bases, so the override exists.
}

// ==========================================================================
// NewOptionsDialog::CheckboxChecked(int,bool)
// @00141328  2 bytes
// ==========================================================================
void NewOptionsDialog::CheckboxChecked(int theId, bool theChecked)
{
  // Empty in the binary too, for the same reason as SliderVal.
}

// ==========================================================================
// NewOptionsDialog::~NewOptionsDialog()
// @00141334  160 bytes
// ==========================================================================
NewOptionsDialog::~NewOptionsDialog()
{
  // The binary frees each button through slot 1 of its vtable (the deleting
  // destructor), in exactly this order. The four vptr stores it performs
  // (+0x00, +0x100, +0x2FC, +0x300) are the compiler's own and are left to the
  // implicit base destructor calls.
  delete mHelpDialogButton;
  delete mStoreButton;
  delete mRestartButton;
  delete mMainMenuButton;
  delete mAchievementsButton;
  delete mLeaderboardsButton;
  delete mUnlockButton;
  delete mAlmanacButton;
  delete mBackToGameButton;
}

// ==========================================================================
// NewOptionsDialog::NewOptionsDialog(LawnApp *,bool,unsigned int)
// @0014140C  2382 bytes
// ==========================================================================
NewOptionsDialog::NewOptionsDialog(LawnApp *theApp, bool theFromMenu, unsigned int theGamepadIndex)
  : LawnDialog(theApp, nullptr, 2, true, "[GAME_PAUSED]", "", "", 0)
{
  Sexy::Color aBackColor(255, 255, 100);

  mFromMenu = theFromMenu;
  mGamepadIndex = theGamepadIndex;
  SetColor(3, aBackColor);
  mHelpDialogButton = (GameButton *)MakeButton(2, (ButtonListener *)this, this, "[HELP AND OPTIONS]");
  // In the Zen Garden or the Tree of Wisdom, without a gamepad, the third
  // button opens the shop. A menu-opened dialog always shows the almanac label.
  bool aShopInstead = !mFromMenu
      && (mApp->mGameMode == GAMEMODE_CHALLENGE_ZEN_GARDEN
          || mApp->mGameMode == GAMEMODE_TREE_OF_WISDOM)
      && !mApp->HasGamepad();
  if (aShopInstead)
    mAlmanacButton = (GameButton *)MakeButton(3, (ButtonListener *)this, this, "[SHOP_BUTTON]");
  else
    mAlmanacButton = (GameButton *)MakeButton(3, (ButtonListener *)this, this, "[VIEW_ALMANAC_BUTTON]");
  if (mApp->mGameMode == GAMEMODE_MP_VS)
    mRestartButton = (GameButton *)MakeButton(5, (ButtonListener *)this, this, "[CONCEDE_VS_BUTTON]");
  else
    mRestartButton = (GameButton *)MakeButton(5, (ButtonListener *)this, this, "[RESTART_LEVEL_BUTTON]");
  mMainMenuButton = (GameButton *)MakeButton(4, (ButtonListener *)this, this, "[MAIN_MENU_BUTTON]");
  mAchievementsButton = (GameButton *)MakeButton(1, (ButtonListener *)this, this, "[ACHIEVEMENTS]");
  mLeaderboardsButton = (GameButton *)MakeButton(0, (ButtonListener *)this, this, "[LEADERBOARDS]");
  // Widget slot 42 is SetVisible and slot 48 is SetDisabled; the binary hides
  // both buttons and disables them (the achievements and leaderboards screens
  // are not part of this build).
  mAchievementsButton->SetVisible(false);
  mAchievementsButton->SetDisabled(true);
  mLeaderboardsButton->SetVisible(false);
  mLeaderboardsButton->SetDisabled(true);
  mUnlockButton = (GameButton *)MakeButton(7, (ButtonListener *)this, this, "[DROP_P2]");
  mStoreButton = (GameButton *)MakeButton(8, (ButtonListener *)this, this, "[UNLOCK_FULL_VERSION]");
  mBackToGameButton = (GameButton *)MakeButton(1000, (ButtonListener *)this, this, "[BACK_TO_GAME]");
  // The focus ring, one link per button, in the order the binary sets it.
  mHelpDialogButton->SetFocusLinks(mBackToGameButton, mLeaderboardsButton, nullptr, nullptr);
  mLeaderboardsButton->SetFocusLinks(mHelpDialogButton, mAchievementsButton, nullptr, nullptr);
  mAchievementsButton->SetFocusLinks(mLeaderboardsButton, mAlmanacButton, nullptr, nullptr);
  mAlmanacButton->SetFocusLinks(mAchievementsButton, mUnlockButton, nullptr, nullptr);
  mUnlockButton->SetFocusLinks(mAlmanacButton, mRestartButton, nullptr, nullptr);
  mRestartButton->SetFocusLinks(mUnlockButton, mStoreButton, nullptr, nullptr);
  mStoreButton->SetFocusLinks(mRestartButton, mMainMenuButton, nullptr, nullptr);
  mMainMenuButton->SetFocusLinks(mStoreButton, mBackToGameButton, nullptr, nullptr);
  mBackToGameButton->SetFocusLinks(mMainMenuButton, mHelpDialogButton, nullptr, nullptr);
  if (mFromMenu)
  {
    mRestartButton->SetVisible(false);
    mBackToGameButton->SetLabel("[DIALOG_BUTTON_OK]");
    if (!mApp->HasFinishedAdventure() || mApp->IsTrialStageLocked() != 0)
      mMainMenuButton->SetVisible(false);
    else
      mMainMenuButton->SetLabel("[CREDITS]");
  }
  // GAMEMODE_CHALLENGE_ICE (43), GAMEMODE_CHALLENGE_ZEN_GARDEN (44) and
  // GAMEMODE_TREE_OF_WISDOM (51) are the three modes the restart button is
  // never offered in.
  if (mApp->mGameMode == GAMEMODE_CHALLENGE_ICE
      || mApp->mGameMode == GAMEMODE_CHALLENGE_ZEN_GARDEN
      || mApp->mGameMode == GAMEMODE_TREE_OF_WISDOM)
    mRestartButton->SetVisible(false);
  // At the level intro the restart button only comes back for a survival
  // re-pick, which is the one intro the player is allowed to rewind from.
  if (mApp->mGameScene == SCENE_LEVEL_INTRO
      && !mApp->mBoard->mCutScene->IsSurvivalRepick())
    mRestartButton->SetVisible(false);
  if (mApp->mGameMode == GAMEMODE_MP_VS && mApp->mBoard != nullptr
      && mApp->mBoard->mLevelAwardSpawned)
  {
    mRestartButton->SetVisible(false);
  }
  if (mApp->IsFullVersion() != 0)
  {
    mUnlockButton->SetVisible(false);
    // From a menu, the Tree of Wisdom still shows the almanac and the Zen
    // Garden hides it. Every other menu open hides it. In a level it is hidden
    // during the intro, in those two modes, and when the profile cannot open it.
    bool aShowAlmanac = true;
    if (mFromMenu && mApp->mGameMode == GAMEMODE_CHALLENGE_ZEN_GARDEN)
      aShowAlmanac = false;
    else if (mFromMenu && mApp->mGameMode == GAMEMODE_TREE_OF_WISDOM)
      aShowAlmanac = true;
    else if (mApp->CanShowAlmanac() == 0
        || mApp->mGameScene == SCENE_LEVEL_INTRO
        || mApp->mGameMode == GAMEMODE_CHALLENGE_ZEN_GARDEN
        || mApp->mGameMode == GAMEMODE_TREE_OF_WISDOM
        || mFromMenu)
      aShowAlmanac = false;
    mAlmanacButton->SetVisible(aShowAlmanac);
  }
  else
  {
    mUnlockButton->SetVisible(true);
    mAlmanacButton->SetVisible(false);
  }
  // The store is offered in adventure mode while the level is being played, and
  // only when no second gamepad has claimed the second player slot (+0xA7C).
  if (mApp->mGameScene != SCENE_PLAYING || mApp->mGameMode != GAMEMODE_ADVENTURE
      || mApp->mSecondPlayerGamepadIndex == -1)
    mStoreButton->SetVisible(false);
  else
    mStoreButton->SetVisible(true);
  mFontId = FONT_DWARVENTODCRAFT18;
  ((Sexy::ImageFont *)FONT_DWARVENTODCRAFT18)->SetScale(
      ((Sexy::SexyAppBase *)gSexyAppBase)->GetDouble("OPTION_DLG_LABEL_FONT_SCALE", 1.0));
}

// ==========================================================================
// NewOptionsDialog::AddedToManager(Sexy::WidgetManager *)
// @00141E98  134 bytes
// ==========================================================================
void NewOptionsDialog::AddedToManager(Sexy::WidgetManager *theManager)
{
  Sexy::Dialog::AddedToManager(theManager);
  // TV @0x141E98 calls this dialog's AddWidget slot, so focus stays in its subtree.
  AddWidget(mHelpDialogButton);
  AddWidget(mLeaderboardsButton);
  AddWidget(mAchievementsButton);
  AddWidget(mAlmanacButton);
  AddWidget(mUnlockButton);
  AddWidget(mRestartButton);
  AddWidget(mStoreButton);
  AddWidget(mMainMenuButton);
  AddWidget(mBackToGameButton);
  SetFocus(mBackToGameButton, true);
}

// ==========================================================================
// NewOptionsDialog::RemovedFromManager(Sexy::WidgetManager *)
// @00141F20  118 bytes
// ==========================================================================
void NewOptionsDialog::RemovedFromManager(Sexy::WidgetManager *theManager)
{
  Sexy::Dialog::RemovedFromManager(theManager);
  // TV @0x141F20 removes the same children through the dialog's RemoveWidget slot.
  RemoveWidget(mHelpDialogButton);
  RemoveWidget(mLeaderboardsButton);
  RemoveWidget(mAchievementsButton);
  RemoveWidget(mAlmanacButton);
  RemoveWidget(mUnlockButton);
  RemoveWidget(mRestartButton);
  RemoveWidget(mStoreButton);
  RemoveWidget(mMainMenuButton);
  RemoveWidget(mBackToGameButton);
}

// ==========================================================================
// NewOptionsDialog::Resize(int,int,int,int)
// @00141F98  666 bytes
// ==========================================================================
void NewOptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
  // Every button is repositioned through Widget::Resize, and its visibility
  // (Widget::mVisible at +0x74) decides whether it takes a row: the first two
  // buttons always do, and the rest pack upwards from wherever the previous one
  // ended. 166/212/258 are the accumulated row offsets.
  int aRowTop = theWidth - 85;
  int anX = theWidth / 2 - (theWidth - 85) / 2 + 6;
  mHelpDialogButton->Resize(anX, 120, theWidth - 85, 46);
  int aFirstFreeRow;
  int aNumButtons;
  int aFilledButtons;
  int aRowY;
  if (mLeaderboardsButton->mVisible)
  {
    aFirstFreeRow = 258;
    aNumButtons = 3;
    mLeaderboardsButton->Resize(anX, 166, aRowTop, 46);
    aFilledButtons = 2;
    aRowY = 212;
  }
  else
  {
    aFirstFreeRow = 212;
    aNumButtons = 2;
    aFilledButtons = 1;
    aRowY = 166;
  }
  if (mAchievementsButton->mVisible)
  {
    aFilledButtons = aNumButtons;
    mAchievementsButton->Resize(anX, aRowY, aRowTop, 46);
    ++aNumButtons;
    aRowY = aFirstFreeRow;
    aFirstFreeRow += 46;
  }
  int aNextY = aRowY;
  if (mApp->IsFullVersion() != 0)
  {
    if (mAlmanacButton->mVisible)
    {
      aFilledButtons = aNumButtons;
      mAlmanacButton->Resize(anX, aRowY, aRowTop, 46);
      ++aNumButtons;
      aNextY = aFirstFreeRow;
      aFirstFreeRow += 46;
    }
  }
  else
  {
    aFilledButtons = aNumButtons++;
    mStoreButton->Resize(anX, aRowY, aRowTop, 46);
    aNextY = aFirstFreeRow;
    aFirstFreeRow += 46;
  }
  if (mApp->mGameMode != GAMEMODE_ADVENTURE || mApp->mPlayerInfo->mId == -1)
  {
    if (mRestartButton->mVisible)
    {
      int aRestartY = aFirstFreeRow;
      aFirstFreeRow = aNextY + 92;
      aNumButtons = aFilledButtons + 2;
      mRestartButton->Resize(anX, aRestartY, aRowTop, 46);
    }
    int aMainMenuY = aFirstFreeRow;
    aFirstFreeRow = aNextY + 92;
    int aLastButtonRow = aFilledButtons + 4;
    mMainMenuButton->Resize(anX, aMainMenuY, aRowTop, 46);
    mBackToGameButton->Resize(30, aMainMenuY + 98, 350, 46);
    aRowY = aLastButtonRow;
  }
  else
  {
    int aStoreY = aNextY + 92;
    int aSavedY = aNextY;
    mStoreButton->Resize(anX, aFirstFreeRow, aRowTop, 46);
    if (mRestartButton->mVisible)
    {
      int aRestartY = aStoreY;
      aNumButtons = aFilledButtons + 3;
      aStoreY = aSavedY + 138;
      mRestartButton->Resize(anX, aRestartY, aRowTop, 46);
    }
    else
    {
      aNumButtons = aFilledButtons + 2;
    }
    mMainMenuButton->Resize(anX, aStoreY, aRowTop, 46);
    int aBackY = aStoreY + 98;
    aRowY = aNumButtons + 2;
    mBackToGameButton->Resize(30, aBackY, 350, 46);
  }
  // The dialog is as tall as the stacked rows plus the framed top and bottom
  // pieces: the centre-left strip is repeated until it covers the remainder.
  int aContentHeight = 46 * aRowY + 171;
  Sexy::Image *aBottomLeft = mTallBottom ? IMAGE_DIALOG_BIGBOTTOMLEFT
                                         : IMAGE_DIALOG_BOTTOMLEFT;
  int aBottomHeight = aBottomLeft->mHeight;
  int aTopHeight = (IMAGE_DIALOG_TOPLEFT)->mHeight;
  int aCenterHeight = (IMAGE_DIALOG_CENTERLEFT)->mHeight;
  int aRemainder = aContentHeight - aTopHeight - aBottomHeight - 45;
  int aStripCount = aRemainder / aCenterHeight;
  if (aRemainder % aCenterHeight != 0)
    ++aStripCount;
  int aTotalHeight = aTopHeight + aCenterHeight * aStripCount + aBottomHeight;
  mBackToGameButton->Resize(30, aTotalHeight - 40, 350, 46);
  // The binary tail-calls Sexy::Dialog::Resize directly, not through the vtable.
  Sexy::Dialog::Resize(theX, theY, theWidth, aTotalHeight + 45);
}

// ==========================================================================
// NewOptionsDialog::Update(void)
// @00142248  4 bytes
// ==========================================================================
void NewOptionsDialog::Update()
{
  LawnDialog::Update();
}

// ==========================================================================
// NewOptionsDialog::Draw(Sexy::Graphics *)
// @0014224C  4 bytes
// ==========================================================================
void NewOptionsDialog::Draw(Sexy::Graphics *theGraphics)
{
  LawnDialog::Draw(theGraphics);
}

// ==========================================================================
// NewOptionsDialog::ButtonPress(int)
// @00142250  50 bytes
// ==========================================================================
void NewOptionsDialog::ButtonPress(int theId)
{
  if (theId == 1000)
  {
    mApp->PlaySample(SOUND_GRAVEBUTTON, true);
    Sexy::Dialog::ButtonDepress(1000);
  }
}

// ==========================================================================
// NewOptionsDialog::ButtonDepress(int)
// @00142294  648 bytes
// ==========================================================================
void NewOptionsDialog::ButtonDepress(int theId)
{
  mApp->PlaySample(SOUND_GRAVEBUTTON, true);
  Sexy::Dialog::ButtonDepress(theId);
  switch (theId)
  {
    case NewOptionsDialog_House:
      if (!mFromMenu)
      {
        mApp->ShowHouseScreenFromGame();
        mWidgetManager->PutInfront(this, mApp->mBoard);
      }
      return;
    case NewOptionsDialog_HelpDialog:
      mApp->DoHelpDialog(1);
      return;
    case NewOptionsDialog_Almanac:
      // A menu-opened dialog, and any mode other than the garden or the tree,
      // opens the almanac. Those two modes open their own stores and then close
      // this dialog.
      if (mFromMenu
          || (mApp->mGameMode != GAMEMODE_CHALLENGE_ZEN_GARDEN
              && mApp->mGameMode != GAMEMODE_TREE_OF_WISDOM))
      {
        mApp->DoAlmanacDialog(SEED_NONE, ZOMBIE_INVALID)->WaitForResult(true);
        return;
      }
      if (mApp->mGameMode == GAMEMODE_CHALLENGE_ZEN_GARDEN)
        mApp->mZenGarden->OpenStore();
      else
        mApp->mBoard->mChallenge->TreeOfWisdomOpenStore();
      ButtonDepress(1000);
      return;
    case NewOptionsDialog_MainMenu:
      if (mFromMenu)
      {
        mApp->KillNewOptionsDialog();
        mApp->KillGameSelector();
        mApp->ShowAwardScreen(AWARD_CREDITS_ZOMBIENOTE);
        return;
      }
      {
        Board *aBoard = mApp->mBoard;
        if (aBoard != nullptr
            && (mApp->mGameMode == GAMEMODE_CHALLENGE_ZEN_GARDEN
                || mApp->mGameMode == GAMEMODE_TREE_OF_WISDOM))
        {
          mApp->DoConfirmBackToMain(false);
          mApp->KillNewOptionsDialog();
          return;
        }
        bool aNeedConfirm = false;
        if (aBoard != nullptr && aBoard->NeedSaveGame())
          aNeedConfirm = true;
        else if (aBoard != nullptr && aBoard->mCutScene != nullptr && aBoard->mCutScene->IsSurvivalRepick())
          aNeedConfirm = true;
        if (aNeedConfirm)
          mApp->DoConfirmBackToMain(true);
        else
        {
          mApp->SetBoardResult(BOARDRESULT_QUIT);
          mApp->DoBackToMain();
        }
      }
      return;
    case NewOptionsDialog_Restart:
      if ( mApp->mBoard != nullptr )
      {
        // LawnApp::DoConfirmRestartDialog hands back a plain Sexy::Dialog built
        // by DoDialog(24, ...); the binary then reads LawnDialog::mLawnYesButton
        // (+0x2F0) and Dialog::mId (+0x2CC) off it, so it is a LawnDialog.
        LawnDialog *aConfirmDialog = (LawnDialog *)mApp->DoConfirmRestartDialog();
        if ( aConfirmDialog->WaitForResult(false) == 1000 )
        {
          mApp->mMusic->StopAllMusic();
          mApp->mSoundSystem->CancelPausedFoley();
          if ( mApp->mGameMode == GAMEMODE_MP_VS )
          {
            // In a versus match the surrender hands the game to the other side:
            // the confirm dialog's yes-button tag (+0x34C) is compared with the
            // gamepad that owns player one to decide which side won.
            int aButtonTag = aConfirmDialog->mLawnYesButton->mGamepadIndex;
            int aFirstGamepadIndex = mApp->mBoard->mGamepadControls[0]->mGamepadIndex;
            mApp->KillNewOptionsDialog();
            mApp->SetBoardResult(aButtonTag == aFirstGamepadIndex ? BOARDRESULT_VS_ZOMBIE_WON
                                                                  : BOARDRESULT_VS_PLANT_WON);
            mApp->mGameScene = aButtonTag == aFirstGamepadIndex ? SCENE_ZOMBIES_WON : SCENE_PLANTS_WON;
            mApp->ShowVSResultsScreen();
            mApp->mVSResultsMenu->InitFromBoard(mApp->mBoard);
            mApp->KillBoard();
          }
          else
          {
            mApp->mGameScene = SCENE_LEVEL_INTRO;
            mApp->mMusic->StopAllMusic();
            mApp->mSoundSystem->CancelPausedFoley();
            mApp->KillNewOptionsDialog();
            mApp->SetBoardResult(BOARDRESULT_RESTART);
            GameMode aGameMode = mApp->mGameMode;
            mApp->mKilledYetiAndRestarted = mApp->mBoard->mKilledYeti;
            mApp->PreNewGame(aGameMode, false);
          }
        }
        mApp->KillDialog(aConfirmDialog->mId);
      }
      return;
    case NewOptionsDialog_Update:
      mApp->CheckForUpdates();
      return;
    case NewOptionsDialog_ClearSecondPlayer:
      mApp->mBoard->RefreshSeedPacketFromCursor(1);
      mApp->mBoard->ClearCursor(1);
      mApp->ClearSecondPlayer();
      // The binary clears the SECOND player's gamepad index, not the first's.
      mApp->mBoard->mGamepadControls[1]->mGamepadIndex = -1;
      ButtonDepress(1000);
      return;
    default:
      return;
  }
}

// ==========================================================================
// NewOptionsDialog::GameButtonDown(Sexy::GamepadButton,int,unsigned int)
// @0014252C  56 bytes
// ==========================================================================
void NewOptionsDialog::GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags)
{
  (void)thePlayerIndex;
  if (theButton == GAMEPAD_BUTTON_START)
  {
    Sexy::Dialog::ButtonDepress(1000);
    return;
  }
  if ((theFlags & 1) == 0 && theButton == GAMEPAD_BUTTON_B)
  {
    mApp->PlaySample(SOUND_GRAVEBUTTON, true);
    Sexy::Dialog::ButtonDepress(1000);
  }
}

// ==========================================================================
// NewOptionsDialog::KeyDown(Sexy::KeyCode)
// @0014256C  152 bytes
// ==========================================================================
int NewOptionsDialog::KeyDown(Sexy::KeyCode theKey)
{
  // Only the gamepad that opened this dialog may drive it: the top-level
  // widget's mEvent carries the device id in its `lateModifiers` word (+8), and
  // the dialog's own mGamepadIndex has to match the resulting gamepad slot.
  Sexy::Widget *aTopLevel = GetTopLevel();
  int aGamepadIndex;
  if ( aTopLevel->mEvent == nullptr
    || (aGamepadIndex = mApp->GetGamepadIndex((int)aTopLevel->mEvent->lateModifiers, 0)) == -1 )
  {
    if ( mGamepadIndex != 0 )
      return 1;
  }
  else if ( mGamepadIndex != aGamepadIndex )
  {
    return 1;
  }
  // The three closes are the gamepad C button (0x132), Escape (0x1B) and the
  // gamepad Start button (0x13B).
  if (theKey == Sexy::KEYCODE_GAMEPAD_C || theKey == Sexy::KEYCODE_ESCAPE
    || theKey == Sexy::KEYCODE_GAMEPAD_START)
  {
    mApp->PlaySample(SOUND_GRAVEBUTTON, true);
    Sexy::Dialog::ButtonDepress(1001);
    return 1;
  }
  if (LawnDialog::KeyDown(theKey) != 0)
    return 1;
  if (theKey != Sexy::KEYCODE_SPACE && theKey != Sexy::KEYCODE_RETURN)
    return 0;
  Sexy::Dialog::ButtonDepress(1000);
  return 1;
}

// ==========================================================================
// NewOptionsDialog::GotFocus(void)
// @0014260C  4 bytes
// ==========================================================================
void NewOptionsDialog::GotFocus()
{
  LawnDialog::GotFocus();
}

// ==========================================================================
// NewOptionsDialog::OrderInManagerChanged(void)
// @00142610  4 bytes
// ==========================================================================
void NewOptionsDialog::OrderInManagerChanged()
{
  Sexy::Dialog::OrderInManagerChanged();
}
