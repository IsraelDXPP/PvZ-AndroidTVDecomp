// AwardScreen -- the end-of-level award screen.
//
// Rewritten from the transcription against the IDA database of libGameMain.so;
// every body is the ARM's own control flow with the decompiler's temporaries
// and its libstdc++ string refcount loops folded back into Sexy::String.
// The PvZ-src counterpart (PvZ-src\Lawn\Widget\AwardScreen.cpp) carries only
// MouseDown and MouseUp, and both are reproduced here.
//
// Two things the binary does that the transcription hid:
//   * `GetNameString` / `GetToolTip` return their Sexy::String BY VALUE, so the
//     decompiler's first parameter is the destination of a struct return, not a
//     receiver. DrawAwardSeed calls them as the value-returning functions they
//     are.
//   * The `this + 256` the button constructors are handed is the
//     Sexy::ButtonListener base sub-object, which is `static_cast` here rather
//     than an offset.

#include "AwardScreen.h"
#include "ButtonListener.h"
#include "GameButton.h"
#include "GamepadApp.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "Image.h"
#include "ImageFont.h"
#include "LawnApp.h"
#include "LawnPlayerInfo.h"
#include "Level.h"
#include "Plant.h"
#include "StoreScreen.h"
#include "StretchableGameButton.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include "WidgetManager.h"
#include "ZenGarden.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// Sexy::DrawStringJustification, the values TodDrawString / TodDrawStringWrapped
// take. The enum has no header shared by the game files yet, so the two members
// this screen uses are named here, exactly as AlmanacDialog.cpp names them.
const int DS_ALIGN_CENTER = 2;
const int DS_ALIGN_CENTER_MIDDLE = 5;

namespace {
// ChallengePage values, from the authoritative PvZ\Lawn\Common\ConstEnums.h.
// Sexy\ReconTypes.h carries ChallengePage as a one-int surrogate struct with no
// enumerators, so the pages this screen asks about are named here, the same way
// ChallengeScreen.cpp and ContinueDialog.cpp name their own.
;
} // namespace

// ==========================================================================
// AwardScreen::Resize(int,int,int,int)
// @00143E60  4 bytes
// ==========================================================================
void AwardScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
  Sexy::Widget::Resize(theX, theY, theWidth, theHeight);
}

// ==========================================================================
// AwardScreen::MouseDown(int,int,int)
// @00143E64  34 bytes
// ==========================================================================
void AwardScreen::MouseDown(int theX, int theY, int theClickCount)
{
  // The click count arrives in r3; only a single click makes a sound. The sound
  // is the tape hiss the note screens play (the binary's `_ZN4Sexy9SOUND_TAPE`,
  // which this tree spells SOUND_TAP), and the call is LawnApp's own
  // PlaySample(int, bool), which is why the second argument is there.
  if (theClickCount == 1)
    mApp->PlaySample(SOUND_TAP, true);
}

// ==========================================================================
// AwardScreen::IsPaperNote(void)
// @00143E90  80 bytes
// ==========================================================================
int AwardScreen::IsPaperNote()
{
  if (mAwardType == AWARD_CREDITS_ZOMBIENOTE || mAwardType == AWARD_HELP_ZOMBIENOTE)
    return 1;

  if (!mApp->IsAdventureMode())
    return 0;

  int aLevel = mApp->mPlayerInfo->GetLevel();
  return aLevel == 10 || aLevel == 20 || aLevel == 30 || aLevel == 40 || aLevel == 50;
}

// ==========================================================================
// AwardScreen::DrawBottom(Sexy::Graphics *,Sexy::String const&,Sexy::String const&,Sexy::String const&)
// @00143EE0  232 bytes
// ==========================================================================
int AwardScreen::DrawBottom(Sexy::Graphics *theGraphics, Sexy::String const &theTitle, Sexy::String const &theSubtitle, Sexy::String const &theDescription)
{
  theGraphics->DrawImage(IMAGE_AWARDSCREEN_BACK, LawnApp::FULLSCREEN_RECT.mX,
                LawnApp::FULLSCREEN_RECT.mY);

  // vtable +0x6C is ImageFont::GetPointSize. Font does not declare it.
  Sexy::ImageFont *aTitleFont = static_cast<Sexy::ImageFont *>(FONT_DWARVENTODCRAFT24);
  int aTitleY = aTitleFont->GetPointSize() / 2 + 42;
  TodDrawString(theGraphics, theTitle, 400, aTitleY, FONT_DWARVENTODCRAFT24,
                Color(213, 159, 43), DS_ALIGN_CENTER);
  TodDrawString(theGraphics, theSubtitle, 400, 322, FONT_DWARVENTODCRAFT18,
                Color(213, 159, 43), DS_ALIGN_CENTER);

  TRect<int> aDescriptionRect(285, 361, 230, 90);
  return TodDrawStringWrapped(theGraphics, theDescription, aDescriptionRect, FONT_BRIANNETOD16,
                              Color(40, 50, 90), DS_ALIGN_CENTER_MIDDLE, false);
}

// ==========================================================================
// AwardScreen::DrawAwardSeed(Sexy::Graphics *)
// @00143FE0  462 bytes
// ==========================================================================
void AwardScreen::DrawAwardSeed(Sexy::Graphics *theGraphics)
{
  // The seed the level's award is: one behind the level number, because level 1
  // awards the first seed.
  int aLevel = mApp->mPlayerInfo->GetLevel();
  int aSeedType = mApp->GetAwardSeedForLevel(aLevel - 1);

  // GetNameString and GetToolTip both return their Sexy::String by value, so the
  // binary's r0 is the destination of a struct return, not a receiver -- the
  // `(SeedType, SeedType)` shape the dump shows is the sret slot followed by the
  // real arguments.
  Sexy::String aName = Plant::GetNameString((SeedType)aSeedType, SEED_NONE);

  Sexy::String aToolTip;
  if (mApp->IsTrialStageLocked() == 0
      || aSeedType == SEED_TANGLEKELP
      || aSeedType <= SEED_LILYPAD)
  {
    aToolTip = Plant::GetToolTip((SeedType)aSeedType);
  }
  else
  {
    aToolTip.Assign("[AVAILABLE_IN_FULL_VERSION]", 27);
  }

  Sexy::String aHeader("[NEW_PLANT]");
  DrawBottom(theGraphics, aHeader, aName, aToolTip);

  // The packet is drawn at double size around its own cell.
  theGraphics->SetScale(2.0f, 2.0f, 350.0f, 129.0f);
  DrawSeedPacket(theGraphics, 350.0f, 129.0f, (SeedType)aSeedType, SEED_NONE, 0.0f, 255,
                 true, false, false, true);
  theGraphics->SetScale(1.0f, 1.0f, 0.0f, 0.0f);
}

// ==========================================================================
// AwardScreen::Draw(Sexy::Graphics *)
// @001441F4  5352 bytes
// ==========================================================================
void AwardScreen::Draw(Sexy::Graphics *theGraphics)
{
  // The award type and the level are read once and drive every branch below.
  int aLevel = mApp->mPlayerInfo->GetLevel();
  int aAwardType = mAwardType;

  theGraphics->PushState();
  theGraphics->SetLinearBlend(true);

  // ---- credits zombie note: the boss background, dimmed, plus the note -----
  if (aAwardType == AWARD_CREDITS_ZOMBIENOTE)
  {
    theGraphics->SetColor(Color(125, 200, 255, 255));
    theGraphics->SetColorizeImages(true);
    theGraphics->DrawImage(IMAGE_BACKGROUND6BOSS, -900, -400, 2800, 1200);
    theGraphics->SetColorizeImages(false);
    theGraphics->SetColor(Color(0, 0, 0, 100));
    // The dark band across the lower part of the screen: the x and the width
    // are the fullscreen rect's own first and third words.
    theGraphics->FillRect(LawnApp::FULLSCREEN_RECT.mX, 525, LawnApp::FULLSCREEN_RECT.mWidth, 600);
    theGraphics->DrawImage(IMAGE_ZOMBIE_NOTE, 75, 60);
    theGraphics->DrawImage(IMAGE_CREDITS_ZOMBIENOTE, 149, 103, 475, 325);
  }
  // ---- help zombie note: the day background and the help note --------------
  else if (aAwardType == AWARD_HELP_ZOMBIENOTE)
  {
    theGraphics->DrawImage(IMAGE_BACKGROUND1, -700, -300, 2800, 1200);
    theGraphics->DrawImage(IMAGE_ZOMBIE_NOTE, 80, 80);
    theGraphics->DrawImage(IMAGE_ZOMBIE_NOTE_HELP, 131, 132);
  }
  else if (mApp->IsAdventureMode() == 0)
  {
    // Every non-adventure award is a trophy; only the caption changes.
    if (mApp->EarnedGoldTrophy() != 0)
    {
      Sexy::String aTitle("[BEAT_GAME_MESSAGE1]");
      Sexy::String aSubTitle("[GOLD_SUNFLOWER_TROPHY]");
      Sexy::String aDescription("[BEAT_GAME_MESSAGE2]");
      DrawBottom(theGraphics, aTitle, aSubTitle, aDescription);
      // Cel 1 is the gold sunflower, drawn small.
      TodDrawImageCelCenterScaledF(theGraphics, IMAGE_SUNFLOWER_TROPHY, 325.0f,
                                   65.0f, 1, 0.6f, 0.6f);
    }
    else
    {
      Sexy::String aTitle("[GOT_TROPHY]");
      Sexy::String aSubTitle("[TROPHY]");
      Sexy::String aDescription;
      if (mApp->IsSurvivalMode() != 0)
      {
        int aTrophies = mApp->GetNumTrophies(CHALLENGE_PAGE_SURVIVAL);
        if (aTrophies <= 7)
          aDescription.Assign("[YOU_UNLOCKED_A_SURVIVAL]", 25);
        else if (aTrophies == 10)
          aDescription.Assign("[YOU_UNLOCKED_ENDLESS_SURVIVAL]", 31);
        else
          aDescription.Assign("[EARN_MORE_TROPHIES_FOR_ENDLESS_SURVIVAL]", 40);
      }
      else if (mApp->IsScaryPotterLevel() != 0)
      {
        aDescription.Assign("[UNLOCKED_VASEBREAKER_LEVEL]", 28);
      }
      else if (mApp->IsPuzzleMode() != 0)
      {
        aDescription.Assign("[UNLOCKED_I_ZOMBIE_LEVEL]", 24);
      }
      else if (mApp->GetNumTrophies(CHALLENGE_PAGE_CHALLENGE) > 17)
      {
        aDescription.Assign("[GET_MORE_TROPHIES]", 19);
      }
      else
      {
        aDescription.Assign("[CHALLENGE_UNLOCKED]", 20);
      }

      DrawBottom(theGraphics, aTitle, aSubTitle, aDescription);

      // The trophy is centred on x = 400 by its own width and sits at y = 137.
      Sexy::Image *aTrophy = IMAGE_TROPHY_HI_RES;
      theGraphics->DrawImage(aTrophy, 400 - aTrophy->GetWidth() / 2, 137);
    }
  }
  else
  {
    // ---- adventure mode: one caption per milestone level ------------------
    // Only the milestone levels draw anything of their own; the levels that
    // fall to `default` are the two the seed-or-trophy block below handles.
    switch (aLevel)
    {
      case 5:
      {
        Sexy::String aTitle("[GOT_SHOVEL]");
        Sexy::String aSubTitle("[SHOVEL]");
        Sexy::String aDescription("[SHOVEL_DESCRIPTION]");
        DrawBottom(theGraphics, aTitle, aSubTitle, aDescription);
        Sexy::Image *aShovel = IMAGE_SHOVEL_HI_RES;
        theGraphics->DrawImage(aShovel, 400 - aShovel->GetWidth() / 2, 137);
        break;
      }
      case 10:
        theGraphics->DrawImage(IMAGE_BACKGROUND1, -700, -300, 2800, 1200);
        theGraphics->DrawImage(IMAGE_ZOMBIE_NOTE, 80, 80);
        theGraphics->DrawImage(IMAGE_ZOMBIE_NOTE1, 131, 132);
        TodDrawString(theGraphics, Sexy::String("[FOUND_NOTE]"), 400, 70,
                      FONT_DWARVENTODCRAFT24, Color(255, 200, 0, 255),
                      DS_ALIGN_CENTER);
        break;
      case 15:
      {
        Sexy::String aTitle("[FOUND_SUBURBAN_ALMANAC]");
        Sexy::String aSubTitle("[SUBURBAN_ALMANAC]");
        Sexy::String aDescription("[SUBURBAN_ALMANAC_DESCRIPTION]");
        DrawBottom(theGraphics, aTitle, aSubTitle, aDescription);
        Sexy::Image *anAlmanac = IMAGE_ALMANAC;
        theGraphics->DrawImage(anAlmanac, 400 - anAlmanac->GetWidth() / 2, 160);
        break;
      }
      case 20:
        theGraphics->DrawImage(IMAGE_BACKGROUND2, -700, -300, 2800, 1200);
        theGraphics->DrawImage(IMAGE_ZOMBIE_NOTE, 80, 80);
        theGraphics->DrawImage(IMAGE_ZOMBIE_NOTE2, 133, 127);
        TodDrawString(theGraphics, Sexy::String("[FOUND_NOTE]"), 400, 70,
                      FONT_DWARVENTODCRAFT24, Color(255, 200, 0, 255),
                      DS_ALIGN_CENTER);
        break;
      case 25:
      {
        Sexy::String aTitle("[FOUND_KEYS]");
        Sexy::String aSubTitle("[KEYS]");
        Sexy::String aDescription("[KEYS_DESCRIPTION]");
        DrawBottom(theGraphics, aTitle, aSubTitle, aDescription);
        Sexy::Image *aKeys = IMAGE_CARKEYS;
        theGraphics->DrawImage(aKeys, 400 - aKeys->GetWidth() / 2, 160);
        break;
      }
      case 30:
        theGraphics->DrawImage(IMAGE_BACKGROUND1, -700, -300, 2800, 1200);
        theGraphics->DrawImage(IMAGE_ZOMBIE_NOTE, 80, 80);
        theGraphics->DrawImage(IMAGE_ZOMBIE_NOTE3, 120, 117);
        TodDrawString(theGraphics, Sexy::String("[FOUND_NOTE]"), 400, 70,
                      FONT_DWARVENTODCRAFT24, Color(255, 200, 0, 255),
                      DS_ALIGN_CENTER);
        break;
      case 35:
      {
        Sexy::String aTitle("[FOUND_TACO]");
        Sexy::String aSubTitle("[TACO]");
        Sexy::String aDescription("[TACO_DESCRIPTION]");
        DrawBottom(theGraphics, aTitle, aSubTitle, aDescription);
        Sexy::Image *aTaco = IMAGE_TACO;
        theGraphics->DrawImage(aTaco, 400 - aTaco->GetWidth() / 2, 160);
        break;
      }
      case 40:
        theGraphics->DrawImage(IMAGE_BACKGROUND2, -700, -300, 2800, 1200);
        theGraphics->DrawImage(IMAGE_ZOMBIE_NOTE, 80, 80);
        theGraphics->DrawImage(IMAGE_ZOMBIE_NOTE4, 102, 117);
        TodDrawString(theGraphics, Sexy::String("[FOUND_NOTE]"), 400, 70,
                      FONT_DWARVENTODCRAFT24, Color(255, 200, 0, 255),
                      DS_ALIGN_CENTER);
        break;
      case 45:
      {
        Sexy::String aTitle("[FOUND_WATERING_CAN]");
        Sexy::String aSubTitle("[WATERING_CAN]");
        Sexy::String aDescription("[WATERING_CAN_DESCRIPTION]");
        DrawBottom(theGraphics, aTitle, aSubTitle, aDescription);
        Sexy::Image *aCan = IMAGE_WATERINGCAN;
        theGraphics->DrawImage(aCan, 400 - aCan->GetWidth() / 2, 160);
        break;
      }
      default:
        // Every other level: the last level draws the final note, level 1 that
        // finished the adventure draws the silver sunflower, and everything
        // else draws the seed the level awards.
        if (aLevel != 50)
        {
          if (aLevel != 1 || mApp->HasFinishedAdventure() == 0)
          {
            DrawAwardSeed(theGraphics);
          }
          else
          {
            Sexy::String aTitle("[WIN_MESSAGE1]");
            Sexy::String aSubTitle("[SILVER_SUNFLOWER_TROPHY]");
            Sexy::String aDescription("[WIN_MESSAGE2]");
            DrawBottom(theGraphics, aTitle, aSubTitle, aDescription);
            // Cel 0 is the silver sunflower.
            TodDrawImageCelCenterScaledF(theGraphics, IMAGE_SUNFLOWER_TROPHY,
                                         325.0f, 65.0f, 0, 0.7f, 0.7f);
          }
        }
        else
        {
          theGraphics->DrawImage(IMAGE_BACKGROUND1, -700, -300, 2800, 1200);
          theGraphics->DrawImage(IMAGE_ZOMBIE_NOTE, 80, 80);
          theGraphics->DrawImage(IMAGE_ZOMBIE_FINAL_NOTE, 114, 138);
          TodDrawString(theGraphics, Sexy::String("[FOUND_NOTE]"), 400, 70,
                        FONT_DWARVENTODCRAFT24, Color(255, 200, 0, 255),
                        DS_ALIGN_CENTER);
        }
        break;
    }
  }

  // The start button is drawn by the screen itself, shifted by the button's own
  // position, so the fade below covers it too.
  theGraphics->PopState();
  theGraphics->PushState();
  theGraphics->Translate(mStartButton->mX, mStartButton->mY);
  mStartButton->Draw(theGraphics);
  theGraphics->PopState();

  // The screen fades in as the counter runs down from 180, and the fullscreen
  // wash is black for a paper note and white for everything else.
  int anAlpha = TodAnimateCurve(180, 0, mFadeInCounter, 255, 0.0f, 1);
  int aWash = IsPaperNote() ? 0 : 255;
  theGraphics->SetColor(Color(aWash, aWash, aWash, anAlpha));
  theGraphics->FillRect(LawnApp::FULLSCREEN_RECT);
}

// ==========================================================================
// AwardScreen::StartButtonPressed(void)
// @00145714  530 bytes
// ==========================================================================
void AwardScreen::StartButtonPressed()
{
  // The screen only handles the press while its own dialog is not up.
  if (mApp->GetDialog(6) != nullptr)
    return;

  int anAwardType = mAwardType;
  if (anAwardType == AWARD_CREDITS_ZOMBIENOTE)
  {
    mApp->KillAwardScreen();
    mApp->ShowCreditScreen(false);
    return;
  }

  if (anAwardType == AWARD_HELP_ZOMBIENOTE)
  {
    mApp->KillAwardScreen();
    mApp->ShowGameSelector();
    return;
  }

  if (mApp->IsCoopMode())
  {
    mApp->KillAwardScreen();
    mApp->ShowChallengeScreen(CHALLENGE_PAGE_SURVIVAL);
    return;
  }
  if (mApp->IsSurvivalMode() != 0)
  {
    mApp->KillAwardScreen();
    mApp->ShowChallengeScreen(CHALLENGE_PAGE_SURVIVAL);
    return;
  }
  if (mApp->IsPuzzleMode() != 0)
  {
    mApp->KillAwardScreen();
    mApp->ShowChallengeScreen(CHALLENGE_PAGE_PUZZLE);
    return;
  }
  if (mApp->IsChallengeMode() != 0)
  {
    mApp->KillAwardScreen();
    mApp->ShowChallengeScreen(CHALLENGE_PAGE_CHALLENGE);
    return;
  }

  // Adventure mode: what the button does depends on which milestone the player
  // has just passed. Everything not listed falls through to the restart below.
  switch (mApp->mPlayerInfo->GetLevel())
  {
    case 1:
      if (mApp->HasFinishedAdventure() != 0)
      {
        mApp->KillAwardScreen();
        mApp->ShowAwardScreen(AWARD_CREDITS_ZOMBIENOTE);
        return;
      }
      break;
    case 7:
      if (mApp->IsFullVersion() == 0)
      {
        mApp->KillAwardScreen();
        mApp->mTrialType = 1;
        mApp->PreNewGame(GAMEMODE_UPSELL, false);
        return;
      }
      break;
    case 15:
      // The almanac opens and is told to wait for its own result.
      mApp->DoAlmanacDialog(SEED_NONE, ZOMBIE_INVALID)->WaitForResult(true);
      break;
    case 25:
    {
      StoreScreen *aStore = mApp->ShowStoreScreen();
      aStore->SetupForIntro(301);
      aStore->WaitForResult(true);
      // The zen garden shop jumps straight to the tree; the other store pages
      // fall through to the restart.
      if (aStore->mGoToTreeNow)
      {
        mApp->KillAwardScreen();
        mApp->ShowGameSelector();
        return;
      }
      break;
    }
    case 35:
    {
      StoreScreen *aStore = mApp->ShowStoreScreen();
      aStore->SetupForIntro(601);
      aStore->WaitForResult(true);
      break;
    }
    case 42:
    {
      StoreScreen *aStore = mApp->ShowStoreScreen();
      aStore->SetupForIntro(3100);
      aStore->WaitForResult(true);
      break;
    }
    case 45:
      mApp->KillAwardScreen();
      mApp->ClearSecondPlayer();
      mApp->PreNewGame(GAMEMODE_CHALLENGE_ZEN_GARDEN, false);
      mApp->mZenGarden->SetupForZenTutorial();
      return;
    default:
      break;
  }

  mApp->KillAwardScreen();
  // A two-player adventure continues into the versus board; anything else
  // restarts the plain adventure.
  mApp->PreNewGame(mApp->mGameMode == GAMEMODE_ADVENTURE_TWO_PLAYER
                       ? GAMEMODE_ADVENTURE_TWO_PLAYER
                       : GAMEMODE_ADVENTURE,
                   false);
}

// ==========================================================================
// AwardScreen::MouseUp(int,int,int)
// @00145928  10 bytes
// ==========================================================================
void AwardScreen::MouseUp(int theX, int theY, int theClickCount)
{
  if (theClickCount == 1)
    StartButtonPressed();
}

// ==========================================================================
// AwardScreen::KeyDown(Sexy::KeyCode)
// @00145934  104 bytes
// ==========================================================================
int AwardScreen::KeyDown(Sexy::KeyCode theKey)
{
  // The event's own gamepad index decides whether this screen handles the key:
  // only the pad that owns the focused player's profile may press start.
  int aGamepadIndex = 0;
  if (mEvent != nullptr)
    aGamepadIndex = mEvent->modifiers;

  int aPad = mApp->GetGamepadIndex(aGamepadIndex, 0);
  if (mApp->HasGamepad() != 0 && mApp->mPlayerInfo->GetId() != aPad)
  {
    int aSecondPad = mApp->mSecondPlayerGamepadIndex;
    if (aSecondPad == -1 || aPad != aSecondPad)
      return 0;
  }

  if (theKey != Sexy::KEYCODE_RETURN && theKey != Sexy::KEYCODE_GAMEPAD_A)
    return 0;

  StartButtonPressed();
  return 1;
}

// ==========================================================================
// AwardScreen::GameButtonDown(Sexy::GamepadButton,int,unsigned int)
// @0014599C  58 bytes
// ==========================================================================
void AwardScreen::GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags)
{
  // `theFlags` is the gamepad index the press came from, so the button only acts when
  // that pad owns the focused player's profile.
  if ((mApp->mPlayerInfo->GetId() == (int)theFlags
       || (mApp->mSecondPlayerGamepadIndex != -1
           && (int)theFlags == mApp->mSecondPlayerGamepadIndex))
      && thePlayerIndex == GAMEPAD_BUTTON_A)
  {
    StartButtonPressed();
  }
}

// ==========================================================================
// AwardScreen::KeyChar(char)
// @001459D8  172 bytes
// ==========================================================================
int AwardScreen::KeyChar(char theChar)
{
  // Enter, space and escape all press the button.
  if (theChar == '\r' || theChar == ' ' || theChar == 27)
    StartButtonPressed();

  // The cheat keys only work while the app's debug-key flag is set and the
  // global kill switch is clear.
  if (mApp->mDebugKeysEnabled && gDisableDebugKeys == 0)
  {
    Sexy::String aMessage;
    StrFormat(&aMessage, "Award cheat key '%c'", theChar);

    // '0' is a fortune and '$' a token amount; any other key does nothing.
    int aCoins;
    if (theChar == '0')
      aCoins = 50000;
    else if (theChar == '$')
      aCoins = 100;
    else
      return 1;

    mApp->mPlayerInfo->AddCoins(aCoins);
    mApp->PlaySample(SOUND_BUTTONCLICK, true);
  }
  return 1;
}

// ==========================================================================
// AwardScreen::~AwardScreen()
// @00145B04  102 bytes
// ==========================================================================
AwardScreen::~AwardScreen()
{
  // The screen owns its start button, and every resource it loaded is its own
  // to release. The two vptr stores the transcription carries at the top are the
  // compiler's (the base destructors restore them) and are not written here.
  if (mStartButton != nullptr)
    delete mStartButton;

  // mLoadedResources is a std::set<Sexy::String>. A node is the four-word
  // base plus the stored string at +0x10. Walk in order, release each name,
  // then erase the tree. The binary's next-node call is RbTreeNextConst, the same
  // _Rb_tree_increment as Sexy::RbTree::_M_increment.
  struct ResourceNameNode : Sexy::RbTree::NodeBase
  {
    Sexy::String mName;
  };
  for (ResourceNameNode *aNode = static_cast<ResourceNameNode *>(mLoadedResources.mHeader.mLeft);
       aNode != static_cast<ResourceNameNode *>(&mLoadedResources.mHeader);
       aNode = static_cast<ResourceNameNode *>(Sexy::RbTree::_M_increment(aNode)))
  {
    TodDeleteResources(aNode->mName);
  }
  Sexy::RbTree::_M_erase(mLoadedResources.mHeader.mParent);
}

// ==========================================================================
// AwardScreen::LoadResources(Sexy::String const&)
// @00145CE0  130 bytes
// ==========================================================================
int AwardScreen::LoadResources(Sexy::String const& theGroup)
{
  // The set of names this screen has already loaded, so asking for one twice
  // is a no-op. find walks from the root at +0x120 and returns the header
  // node when the name is absent. A hit returns 0 without loading. A miss
  // loads, and on success inserts; the binary discards the inserted node
  // and returns TodLoadResources' result.
  if (Sexy::RbTree::find(&mLoadedResources, &theGroup) != &mLoadedResources.mHeader)
    return 0;

  int aResult = TodLoadResources(theGroup);
  if (aResult != 0)
    Sexy::RbTree::_M_insert_unique(&mLoadedResources, &theGroup);
  return aResult;
}

// ==========================================================================
// AwardScreen::AwardScreen(LawnApp *,AwardType)
// @00145D64  3418 bytes
// ==========================================================================
AwardScreen::AwardScreen(LawnApp *theApp, AwardType theAward)
{
  // The class' own state. The two vptrs are the compiler's, not the body's, so
  // they are not written here; everything else the ARM stores in one run is.
  mApp = theApp;
  mAwardType = theAward;
  mFadeInCounter = 180;
  mLoadedResources.mHeader.mColor = 0;
  mLoadedResources.mHeader.mParent = nullptr;
  mLoadedResources.mHeader.mLeft = &mLoadedResources.mHeader;
  mLoadedResources.mHeader.mRight = &mLoadedResources.mHeader;
  mLoadedResources.mCount = 0;
  mClip = false;

  int aLevel = mApp->mPlayerInfo->GetLevel();

  // The screen's own resource plus, per award kind, the background and notes it
  // will draw. Every name goes through LoadResources, which is a no-op for one
  // already loaded.
  LoadResources(Sexy::String("DelayLoad_AwardScreen"));
  if (mAwardType == AWARD_CREDITS_ZOMBIENOTE)
  {
    LoadResources(Sexy::String("DelayLoad_Background6"));
    LoadResources(Sexy::String("DelayLoad_ZombieNote"));
    LoadResources(Sexy::String("DelayLoad_Credits"));
  }
  else if (mAwardType == AWARD_HELP_ZOMBIENOTE)
  {
    LoadResources(Sexy::String("DelayLoad_Background1"));
    LoadResources(Sexy::String("DelayLoad_ZombieNote"));
    LoadResources(Sexy::String("DelayLoad_ZombieNoteHelp"));
  }
  else if (mApp->IsAdventureMode() != 0)
  {
    switch (aLevel)
    {
      case 10:
        LoadResources(Sexy::String("DelayLoad_Background1"));
        LoadResources(Sexy::String("DelayLoad_ZombieNote"));
        LoadResources(Sexy::String("DelayLoad_ZombieNote1"));
        break;
      case 20:
        LoadResources(Sexy::String("DelayLoad_Background2"));
        LoadResources(Sexy::String("DelayLoad_ZombieNote"));
        LoadResources(Sexy::String("DelayLoad_ZombieNote2"));
        break;
      case 30:
        LoadResources(Sexy::String("DelayLoad_Background1"));
        LoadResources(Sexy::String("DelayLoad_ZombieNote"));
        LoadResources(Sexy::String("DelayLoad_ZombieNote3"));
        break;
      case 40:
        LoadResources(Sexy::String("DelayLoad_Background2"));
        LoadResources(Sexy::String("DelayLoad_ZombieNote"));
        LoadResources(Sexy::String("DelayLoad_ZombieNote4"));
        break;
      case 50:
        LoadResources(Sexy::String("DelayLoad_Background1"));
        LoadResources(Sexy::String("DelayLoad_ZombieNote"));
        LoadResources(Sexy::String("DelayLoad_ZombieFinalNote"));
        break;
      default:
        break;
    }
  }

  // The start button. The credits note gets a plain GameButton with the rolling
  // credits label, everything else a stretchable one.
  if (mAwardType == AWARD_CREDITS_ZOMBIENOTE)
  {
    GameButton *aButton = new GameButton(100, static_cast<Sexy::ButtonListener *>(this));
    mStartButton = aButton;
    aButton->SetLabel(Sexy::String("[ROLL_CREDITS]"));
    aButton->mButtonImage = IMAGE_CREDITS_PLAYBUTTON;
    aButton->mOverImage = nullptr;
    aButton->mDownImage = nullptr;
    aButton->mDisabledImage = nullptr;
    aButton->SetFont(FONT_HOUSEOFTERROR20);
    aButton->mColors[0] = Color(255, 255, 255, 255);
    aButton->mColors[1] = Color(255, 255, 255, 255);
    aButton->mIsOver = true;
    aButton->Resize(325, 515, 210, 60);
    // GameButton draws itself 42 pixels below its own y while the wobble runs.
    aButton->mTextOffsetY = 42;
  }
  else
  {
    StretchableGameButton *aButton = new StretchableGameButton(
        100, static_cast<Sexy::ButtonListener *>(this), Sexy::String(""),
        mApp->mWidth / 2, 500, 210, 42, (BUTTON_ALIGNMENT_X)0, 0, 0, nullptr);
    mStartButton = aButton;
    aButton->mButtonImage = IMAGE_SEEDCHOOSER_BUTTON_DISABLED;
    aButton->mOverImage = IMAGE_SEEDCHOOSER_BUTTON;
    aButton->mDownImage = nullptr;
    aButton->mDisabledImage = nullptr;
    aButton->SetFont(FONT_BRIANNETOD16);
    aButton->mColors[0] = Color(213, 159, 43, 255);
    aButton->mColors[1] = Color(213, 159, 43, 255);
    aButton->mIsOver = true;

    // The label says what pressing it does, which depends on how the award was
    // reached.
    if (mAwardType == AWARD_HELP_ZOMBIENOTE)
    {
      aButton->SetLabel(Sexy::String("[MAIN_MENU_BUTTON]"));
    }
    else if (mApp->IsAdventureMode() != 0)
    {
      if (aLevel == 1 && mApp->HasFinishedAdventure() != 0)
        aButton->SetLabel(Sexy::String("[CONTINUE_BUTTON]"));
      else if (aLevel == 15)
        aButton->SetLabel(Sexy::String("[VIEW_ALMANAC_BUTTON]"));
      else if (aLevel == 25 || aLevel == 35 || aLevel == 45)
        aButton->SetLabel(Sexy::String("[CONTINUE_BUTTON]"));
      else
        aButton->SetLabel(Sexy::String("[NEXT_LEVEL_BUTTON]"));
    }
    else
    {
      aButton->SetLabel(Sexy::String("[MAIN_MENU_BUTTON]"));
    }
  }

  mApp->mHelpBarWidget->ClearButtons(0);
  mApp->HideHelpBarWidget();

  // A paper note plays the paper foley and stops the music the level was
  // playing; every other award makes sure the seed-choosing tune is running.
  if (IsPaperNote() != 0)
  {
    mApp->mMusic->StopAllMusic();
    mStartButton->mY += 20;
    mApp->PlayFoley(FOLEY_PAPER);
  }
  else
  {
    // Music2 hides MakeSureMusicIsPlaying with a wrong parameter list, so the
    // call goes through the Music base that owns the slot. Tune 6 is
    // MUSIC_TUNE_CHOOSE_YOUR_SEEDS, which is the tune the seed-choosing screens
    // play; Game\Music.h carries the slot table but not the MusicTune enum, so
    // the value is spelled the way AlmanacDialog.cpp spells it.
    Music *aMusic = mApp->mMusic;
    aMusic->MakeSureMusicIsPlaying(MUSIC_TUNE_CHOOSE_YOUR_SEEDS); // MUSIC_TUNE_CHOOSE_YOUR_SEEDS
  }
}

// ==========================================================================
// AwardScreen::Update(void)
// @00146ACC  72 bytes
// ==========================================================================
void AwardScreen::Update()
{
  Sexy::Widget::Update();

  // While the app has a modal dialog up the screen is frozen; otherwise the
  // counter runs down and the whole screen is marked dirty each frame.
  if (mApp->GetDialogCount() <= 0)
  {
    mStartButton->Update();
    mStartButton->mIsOver = true;
    MarkDirty();
    if (mFadeInCounter > 0)
      --mFadeInCounter;
  }
}

// ==========================================================================
// AwardScreen::GotFocus(void)
// @00146B14  4 bytes
// ==========================================================================
void AwardScreen::GotFocus()
{
  Sexy::Widget::GotFocus();
}

// ==========================================================================
// AwardScreen::AddedToManager(Sexy::WidgetManager *)
// @00146B18  4 bytes
// ==========================================================================
void AwardScreen::AddedToManager(Sexy::WidgetManager *theWidgetManager)
{
  Sexy::WidgetContainer::AddedToManager(theWidgetManager);
}

// ==========================================================================
// AwardScreen::RemovedFromManager(Sexy::WidgetManager *)
// @00146B1C  4 bytes
// ==========================================================================
void AwardScreen::RemovedFromManager(Sexy::WidgetManager *theWidgetManager)
{
  Sexy::WidgetContainer::RemovedFromManager(theWidgetManager);
}
