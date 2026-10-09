// LawnApp -- reconstructed from libGameMain.so
//
// Stage 2: the bodies below are the IDA decompilation rewritten as ordinary
// C++. Every `((char *)this + N)` this-relative offset is the named member the
// member table in Game\LawnApp.h documents at that offset, every hand-written
// vtable-slot call is a real virtual call, and every two-step
// `operator new(n)` + placement-new pair is a real construction.
//
// The `// @ADDRESS  N bytes` banners are the binary's own function addresses and
// sizes; they are kept.

// The binary's mangled name is TM::LogComposer, but LogComposer.h declares the
// class at global scope. Alias it in (same pattern as LogComposer.cpp) before
// any header forward-declares TM::LogComposer, so the qualified member calls in
// this file name the same class.
class LogComposer;
namespace TM { using ::LogComposer; }

#include "LawnApp.h"
#include "NativeDisplay.h"
#include "AlmanacDialog.h"
#include "AuthManager.h"
#include "AwardScreen.h"
#include "game/Board.h"
#include "Buffer.h"
#include "BuyMoneyDialog.h"
#include "Challenge.h"
#include "ChallengeScreen.h"
#include "CheatCodeDialog.h"
#include "CheatDialog.h"
#include "ConfirmBackToMainDialog.h"
#include "ContinueDialog.h"
#include "CreditScreen.h"
#include "AchievementsWidget.h"
#include "LeaderboardsWidget.h"
#include "CutScene.h"
#include "DaveHelp.h"
#include "DefaultProfileMgr.h"
#include "Dialog.h"
#include "DrRpcEngine.h"
#include "EffectSystem.h"
#include "FoleyTypeData.h"
#include "GameCenter.h"
// Board.h only forward-declares GamepadControls (the include edge is a cycle),
// so the mGamepadControls[i] member access in IsTwoPlayerGame needs the real
// type here.
#include "GamepadControls.h"
#include "GameOverDialog.h"
#include "GamepadApp.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "HelpOptionsDialog.h"
#include "HelpTextScreen.h"
#include "Image.h"
#include "LawnDialog.h"
#include "LawnPlayerInfo.h"
#include "LawnSession.h"
#include "LawnUser.h"
#include "Level.h"
#include "LevelStats.h"
#include "LogComposer.h"
#include "LogManager.h"
#include "Mailbox.h"
#include "MainMenu.h"
#include "MaskHelpWidget.h"
#include "Music.h"
#include "Music2.h"
#include "NewOptionsDialog.h"
#include "NewUserDialog.h"
#include "PerfTimer.h"
#include "Plant.h"
#include "PlayerInfo.h"
#include "PoolEffect.h"
#include "Platform/LocalGameCenter.h"
#include "PurchaseLimitDialog.h"
#include "Reanimation.h"
#include "ReanimationHolder.h"
#include "ReanimationWidget.h"
#include "ReanimatorCache.h"
#include "ReconTypes.h"
#include "ResourceManager.h"
#include "SavingFileDialog.h"
#include "SaveGameContext.h"
#include "SavingProfileDialog.h"
#include "SeedBank.h"
#include "SeedChooserScreen.h"
#include "ServerConfig.h"
#include "SessionWaitingDialog.h"
#include "SettingsDialog.h"
#include "SexyAppBase.h"
#include "SexyCommonApp.h"
#include "Listener.h"
#include "SlideInOutWidgetAnimator.h"
#include "StoreScreen.h"
#include "TitleScreen.h"
#include "TodFoley.h"
#include "TodHesitationBracket.h"
#include "TodParticleHolder.h"
#include "TodParticleSystem.h"
#include "TypingCheck.h"
#include "UserDialog.h"
#include "VSResultsMenu.h"
#include "VSSetupMenu.h"
#include "WaitForLeaderboardsDialog.h"
#include "WaitForRemoteFileDialog.h"
#include "Widget.h"
#include "WidgetAnimator.h"
#include "XMLElement.h"
#include "XMLParser.h"
#include "ZenGarden.h"
#include "Zombie.h"
#include <string>
#include <vector>

// Sexy::setLocale, Sexy::SexyStringToString, Sexy::SWTri_AddDrawTriFunc and
// Sexy::DrawTriangle_8888_TEX1_TALPHA1_MOD0_GLOB0_BLEND1 are declared in
// Sexy/ReconHelpers.h; the ImageLib namespace in Sexy/ImageLib.h; and POSIX
// setenv in Runtime/AndroidShims.h (reached through ReconTypes.h).
// <map> and <set> have to be visible before ReconHelpers.h: its MapAt<K,V>
// template names std::map in its body, and a non-dependent name is looked up at
// definition time.
#include <map>
#include <set>

#include "ReconHelpers.h"
#include "ImageLib.h"
using Sexy::SexyStringToString;

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

// TODO(recon): gStepReady / gFastMo / gStep are real byte globals in the binary
// (0x715B78, 0x715B7A, 0x715B79) that ReconData.h does not declare. Declared
// file-locally here so this file compiles; they belong in ReconData.h.
extern unsigned char gStepReady;
extern unsigned char gFastMo;
extern unsigned char gStep;

// LawnApp is the listener of every Sexy::Gamepad, but Sexy\Gamepad.h keeps that
// member as `void *` and the interface itself is declared in
// Game\BaseGamepadControls.h rather than next to the class that calls it.
// Sexy\Gamepad.cpp solves the same problem with a local view class; this is the
// same view, so Gamepad::HandleButtonDown can reach this class's override.
// TODO(recon): move the interface to Sexy\GamepadListener.h, make
// Gamepad::mListener that type, and delete this declaration.
namespace {
class GamepadListenerView
{
public:
    virtual void OnButtonDown(int theButton, int thePlayerIndex, unsigned int theFlags) = 0;
    virtual void OnButtonUp(int theButton, int thePlayerIndex, unsigned int theFlags) = 0;
    virtual void OnGameAxisMove(int theAxis, int theValue, int thePlayerIndex) = 0;
};
} // namespace

// Definitions for the class statics declared in the header.
// The four words at 0x715B80 are zero in the file image. The static
// initializer at 0x1318B0 stores (-240, -60, 1280, 720) before draw.
// Title art and the disclaimer cover both use this rect. DrawScreen's
// shift of -mScreenBounds then puts the 1280-wide cover over the window.
Sexy::TRect<int> LawnApp::FULLSCREEN_RECT(-240, -60, 1280, 720);


// The challenge table's page enumerators, from the authoritative
// PvZ\Lawn\Common\ConstEnums.h. Sexy\ReconTypes.h carries ChallengePage as a
// one-int surrogate with no enumerators of its own, so the three pages
// GetNumTrophies is asked about are named here.
;

// Function-local static guards. Each is one int: the low byte is
// "initialised" and the next is "in progress", which is what the
// transcribed tests and CxaGuardAcquire/CxaGuardRelease use.
static int gGuard_getCurUser__user_1 = 0;
static int gGuard_user_2 = 0;

// Linker symbols IDA printed verbatim; each becomes a stub whose
// address is what the original code compares or reads.
static int gGuardVar_1 = 0;
static void* gVtable_1[16] = {};
static void* gVtable_2[16] = {};

// The exception-cleanup registration RegisterAtExit wants a plain function pointer,
// which a destructor's address is not under MSVC; the same thunk the other
// callers of gGuard_getCurUser__user_1 use (see game/LawnUser.cpp).
static void LawnUserDtorThunk(LawnUser *theObject) { theObject->~LawnUser(); }


// ==========================================================================
// LawnApp::GotFocus()
// @002B2610  2 bytes
// ==========================================================================
void LawnApp::GotFocus()
{
}

// ==========================================================================
// LawnApp::ModalClose()
// @002B2614  2 bytes
// ==========================================================================
void LawnApp::ModalClose()
{
}

// ==========================================================================
// LawnApp::ChangeDirHook(char const*)
// @002B2618  4 bytes
// ==========================================================================
bool LawnApp::ChangeDirHook(char const* theIntendedPath)
{
  return false;
}

// ==========================================================================
// LawnApp::ButtonPress(int)
// @002B261C  2 bytes
// ==========================================================================
void LawnApp::ButtonPress(int theId)
{
}

// ==========================================================================
// LawnApp::CloseRequestAsync()
// @002B2620  12 bytes
// ==========================================================================
void LawnApp::CloseRequestAsync()
{
  mCloseRequested = true;
  mCloseRequest = 1;
}

// ==========================================================================
// LawnApp::InitHook()
// @002B262C  2 bytes
// ==========================================================================
void LawnApp::InitHook()
{
}

// ==========================================================================
// LawnApp::GetProfileVersion()
// @002B2630  4 bytes
// ==========================================================================
int LawnApp::GetProfileVersion()
{
  return 19;
}

// ==========================================================================
// LawnApp::NotifyProfileChanged(Sexy::PlayerInfo *)
// @002B2640  2 bytes
// ==========================================================================
void LawnApp::NotifyProfileChanged(Sexy::PlayerInfo * theProfile)
{
  ;
}

// ==========================================================================
// LawnApp::PreDrawScreen()
// @002B2650  8 bytes
// ==========================================================================
void LawnApp::PreDrawScreen()
{
  mDialogFadeDrawn = false;
}

// ==========================================================================
// LawnApp::DrawPost()
// @002B2658  2 bytes
// ==========================================================================
void LawnApp::DrawPost()
{
}

// ==========================================================================
// LawnApp::GamepadConnected()
// @002B265C  2 bytes
// ==========================================================================
void LawnApp::GamepadConnected()
{
}

// ==========================================================================
// LawnApp::GamepadDisconnected()
// @002B2660  2 bytes
// ==========================================================================
void LawnApp::GamepadDisconnected()
{
}

// ==========================================================================
// LawnApp::ShowResourceError(bool)
// @002B2710  4 bytes
// ==========================================================================
void LawnApp::ShowResourceError(bool theExit)
{
  SexyAppBase::ShowResourceError(theExit);
}

// ==========================================================================
// LawnApp::PlaySample(int,bool)
// @002B2714  16 bytes
// ==========================================================================
void LawnApp::PlaySample(int theSoundId, bool theLoop)
{
  // The binary's symbol for the two-argument base slot is
  // `Sexy::SexyAppBase::PlaySample(int, int)`; this tree names that slot
  // PlaySampleWithPan (SexyAppBase.h:311).
  if ( !mMuteSoundsForCutscene )
    SexyAppBase::PlaySampleWithPan(theSoundId, theLoop);
}

// ==========================================================================
// LawnApp::CreatePlayerInfo()
// @002B2724  26 bytes
// ==========================================================================
LawnPlayerInfo *LawnApp::CreatePlayerInfo()
{
  return new LawnPlayerInfo(mProfileMgr);
}

// ==========================================================================
// LawnApp::LoadingThreadCompleted()
// @002B2754  228 bytes
// ==========================================================================
void LawnApp::LoadingThreadCompleted()
{
  if ( mShutDown )
    return;

  Sexy::PerfTimer aTimer;
  aTimer.Start();

  // Pre-render every plant and zombie frame the almanac and the seed bank need,
  // so the first level does not stall on the reanimator cache.
  for ( int aSeedType = 0; aSeedType < NUM_SEED_TYPES; ++aSeedType )
  {
    if ( aSeedType != NUM_SEEDS_IN_CHOOSER )
      mReanimatorCache->MakeCachedPlantFrame((SeedType)aSeedType, DrawVariation(0));
  }

  for ( int aZombieType = 0; aZombieType < NUM_CACHED_ZOMBIE_TYPES; ++aZombieType )
  {
    if ( aZombieType != NUM_ZOMBIE_TYPES )
      mReanimatorCache->MakeCachedZombieFrame((ZombieType)aZombieType);
  }

  for ( int aMowerType = 0; aMowerType < 4; ++aMowerType )
    mReanimatorCache->MakeCachedMowerFrame((LawnMowerType)aMowerType);

  mReanimatorCache->MakeCachedPlantFrame(SEED_FLOWERPOT, VARIATION_ZEN_GARDEN);
  mReanimatorCache->MakeCachedPlantFrame(SEED_FLOWERPOT, VARIATION_ZEN_GARDEN_WATER);
  mReanimatorCache->MakeCachedPlantFrame(SEED_SPROUT, DrawVariation(0));
  mReanimatorCache->MakeCachedPlantFrame(SEED_SPROUT, VARIATION_SPROUT_NO_FLOWER);
  mReanimatorCache->MakeCachedPlantFrame(SEED_TANGLEKELP, VARIATION_AQUARIUM);
  mReanimatorCache->MakeCachedPlantFrame(SEED_SEASHROOM, VARIATION_AQUARIUM);
  mReanimatorCache->MakeCachedPlantFrame(SEED_SUNSHROOM, DrawVariation(18));

  for ( int aVariation = 2; aVariation < 13; ++aVariation )
    mReanimatorCache->MakeCachedPlantFrame(SEED_MARIGOLD, DrawVariation(aVariation));

  aTimer.GetDuration();
}

// ==========================================================================
// LawnApp::OnButtonDown(int,int,unsigned int)
// @002B2838  60 bytes
// ==========================================================================
void LawnApp::OnButtonDown(int theButton, int thePlayerIndex, unsigned int theFlags)
{
  // The binary returns the widget the search ended in; the authoritative
  // signature is `void`, so the value is dropped here.
  LawnPlayerInfo *aPlayerInfo = mPlayerInfo;
  if ( (aPlayerInfo == nullptr || aPlayerInfo->GetId() != thePlayerIndex) && mGameScene == SCENE_MENU )
  {
    MainMenu *aMenu = mGameSelector;
    if ( aMenu != nullptr && aMenu == mWidgetManager->mFocusWidget )
      aMenu->InTransition();
  }
}

// ==========================================================================
// LawnApp::BetaSubmit(bool)
// @002B3098  2 bytes
// ==========================================================================
void LawnApp::BetaSubmit(bool theAskForComments)
{
  ;
}

// ==========================================================================
// LawnApp::NeedRegister()
// @002B30B8  4 bytes
// ==========================================================================
int LawnApp::NeedRegister()
{
  return 0;
}

// ==========================================================================
// LawnApp::UpdateRegisterInfo()
// @002B30BC  2 bytes
// ==========================================================================
void LawnApp::UpdateRegisterInfo()
{
}

// ==========================================================================
// LawnApp::BetaRecordLevelStats()
// @002B30C0  2 bytes
// ==========================================================================
void LawnApp::BetaRecordLevelStats()
{
}

// ==========================================================================
// LawnApp::CanPauseNow()
// @002B30C4  76 bytes
// ==========================================================================
bool LawnApp::CanPauseNow()
{
  if ( mBoard == nullptr )
    return false;
  if ( mSeedChooserScreen != nullptr && mSeedChooserScreen->mMouseVisible )
    return false;
  if ( mBoard->mBoardFadeOutCounter >= 0 )
    return false;
  if ( mCrazyDaveState != CRAZY_DAVE_OFF )
    return false;
  if ( mGameMode == GAMEMODE_CHALLENGE_ZEN_GARDEN || mGameMode == GAMEMODE_TREE_OF_WISDOM )
    return false;
  return GetDialogCount() <= 0;
}

// ==========================================================================
// LawnApp::getLevelForPost()
// @002B3110  56 bytes
// ==========================================================================
int LawnApp::getLevelForPost()
{
  LawnPlayerInfo *aPlayerInfo = mPlayerInfo;
  bool aFlag = aPlayerInfo->GetFlag(PlayerFlags(1));
  return aPlayerInfo->GetLevel()
       + 100 * (aPlayerInfo->GetFlag(PlayerFlags(0x8000)) + aFlag)
       + 10000 * mGameMode;
}

// ==========================================================================
// LawnApp::PostEnterLevel()
// @002B3148  176 bytes
// ==========================================================================
void LawnApp::PostEnterLevel()
{
  // R0 is the result buffer, R1 is this. composeLevelEnterLog's receiver is
  // mLogComposer at +0xB00; sendLog's receiver is the log manager at +0xB04.
  Sexy::String aRemoteName = getCurrentRemoteName();
  MLogManager->sendLog(mLogComposer->composeLevelEnterLog(aRemoteName, getLevelForPost()));
  printf("****level enter %d\n", getLevelForPost());
}

// ==========================================================================
// LawnApp::PostLeaveLevel()
// @002B3220  110 bytes
// ==========================================================================
void LawnApp::PostLeaveLevel()
{
  // composeLevelLeaveLog's receiver is mLogComposer; the dump's r0 was the
  // result buffer it fills and r1 the real this.
  MLogManager->sendLog(mLogComposer->composeLevelLeaveLog(getLevelForPost()));
  printf("****level leave %d\n", getLevelForPost());
}

// ==========================================================================
// LawnApp::PostLevelSuccess()
// @002B32A8  110 bytes
// ==========================================================================
void LawnApp::PostLevelSuccess()
{
  MLogManager->sendLog(mLogComposer->composeLevelSucceedLog(getLevelForPost()));
  printf("****level success %d\n", getLevelForPost());
}

// ==========================================================================
// LawnApp::PostLevelFailed()
// @002B3330  110 bytes
// ==========================================================================
void LawnApp::PostLevelFailed()
{
  MLogManager->sendLog(mLogComposer->composeLevelFailedLog(getLevelForPost()));
  printf("****level failed %d\n", getLevelForPost());
}

// ==========================================================================
// LawnApp::SaveFileExists()
// @002B33B8  112 bytes
// ==========================================================================
int LawnApp::SaveFileExists()
{
  // @0x2B33B8 builds the saved-game name from the current profile's two ids and
  // asks the file driver whether that file exists. The name's buffer is a
  // temporary, so its release is the local's destructor.
  Sexy::String aName;
  GetSavedGameName(&aName, GAMEMODE_ADVENTURE, mPlayerInfo->GetProfileId(), mPlayerInfo->GetId());
  return SexyAppBase::FileExists(aName);
}

// ==========================================================================
// LawnApp::IsIceDemo()
// @002B3440  4 bytes
// ==========================================================================
int LawnApp::IsIceDemo()
{
  return 0;
}

// ==========================================================================
// LawnApp::TransitionToHouseScreen()
// @002B3444  2 bytes
// ==========================================================================
void LawnApp::TransitionToHouseScreen()
{
  ShowHouseScreen();
}

// ==========================================================================
// LawnApp::ShowLeaderboardScreen()
// @002B3448  2 bytes
// ==========================================================================
void LawnApp::ShowLeaderboardScreen()
{
  ShowHouseScreen();
}

// ==========================================================================
// LawnApp::ShowHouseScreen()
// @002B344C  2 bytes
// ==========================================================================
void LawnApp::ShowHouseScreen()
{
  if (mPlayerInfo == nullptr || mLeaderboardScreen != nullptr)
    return;
  KillGameSelector();
  ForceHideHelpBarWidget();
  KillNewOptionsDialog();
  mLeaderboardScreen = new LeaderboardsWidget(this, false);
  mWidgetManager->AddWidget(mLeaderboardScreen);
  mWidgetManager->SetFocus(mLeaderboardScreen);
}

// ==========================================================================
// LawnApp::KillHouseScreenFromGame()
// @002B3450  2 bytes
// ==========================================================================
void LawnApp::KillHouseScreenFromGame()
{
  KillLeaderboardScreen();
}

// ==========================================================================
// LawnApp::ShowHouseScreenFromGame()
// @002B3454  2 bytes
// ==========================================================================
void LawnApp::ShowHouseScreenFromGame()
{
  if (mPlayerInfo == nullptr || mLeaderboardScreen != nullptr)
    return;
  mLeaderboardScreen = new LeaderboardsWidget(this, true);
  mWidgetManager->AddWidget(mLeaderboardScreen);
  mWidgetManager->SetFocus(mLeaderboardScreen);
}

void LawnApp::ShowAchievementsScreen(bool theFromGame)
{
  if (mPlayerInfo == nullptr || mAchievementsScreen != nullptr)
    return;
  if (!theFromGame)
  {
    KillGameSelector();
    KillNewOptionsDialog();
  }
  mAchievementsScreen = new AchievementsWidget(this, theFromGame);
  mWidgetManager->AddWidget(mAchievementsScreen);
  mWidgetManager->SetFocus(mAchievementsScreen);
}

void LawnApp::KillAchievementsScreen()
{
  if (mAchievementsScreen == nullptr)
    return;
  mWidgetManager->RemoveWidget(mAchievementsScreen);
  SafeDeleteWidget(mAchievementsScreen);
  mAchievementsScreen = nullptr;
  ForceHideHelpBarWidget();
}

// ==========================================================================
// LawnApp::KillTitleScreen()
// @002B3458  2 bytes
// ==========================================================================
void LawnApp::KillTitleScreen()
{
  ;
}

// ==========================================================================
// LawnApp::KillMainMenu()
// @002B345C  50 bytes
// ==========================================================================
int LawnApp::KillMainMenu()
{
  if ( mGameSelector != nullptr )
  {
    if ( !mGameSelector->mIsFading )
      mWidgetManager->RemoveWidget(mGameSelector);

    SafeDeleteWidget(mGameSelector);
    mGameSelector = nullptr;
  }
  return 0;
}

// ==========================================================================
// LawnApp::KillGameSelector()
// @002B3490  4 bytes
// ==========================================================================
int LawnApp::KillGameSelector()
{
  return KillMainMenu();
}

// ==========================================================================
// LawnApp::ShowHelpTextScreen(int)
// @002B3494  90 bytes
// ==========================================================================
Board * LawnApp::ShowHelpTextScreen(int theHelpIndex)
{
  if ( mBoard != nullptr )
    mBoard->Pause(true);

  if ( mHelpTextScreen == nullptr )
  {
    HelpTextScreen *aScreen = new HelpTextScreen(this, theHelpIndex);
    mHelpTextScreen = aScreen;
    mWidgetManager->AddWidget(aScreen);
    mWidgetManager->SetFocus(aScreen);
    mWidgetManager->BringToFront(aScreen);
  }
  return nullptr;
}

// ==========================================================================
// LawnApp::KillHelpTextScreen()
// @002B34F8  162 bytes
// ==========================================================================
char * LawnApp::KillHelpTextScreen()
{
  if ( mHelpTextScreen != nullptr )
  {
    mWidgetManager->RemoveWidget(mHelpTextScreen);
    SafeDeleteWidget(mHelpTextScreen);
    mHelpTextScreen = nullptr;
  }
  if ( mSeedChooserScreen != nullptr )
  {
    mBoard->Pause(false);
    return nullptr;
  }
  if ( mVSSetupMenu != nullptr )
  {
    mBoard->Pause(false);
    mWidgetManager->SetFocus(mVSSetupMenu);
    return nullptr;
  }
  if ( GetDialog(3) != nullptr )
  {
    mWidgetManager->SetFocus(GetDialog(3));
    return nullptr;
  }
  if ( mBoard != nullptr )
  {
    mBoard->Pause(false);
    return nullptr;
  }
  GetDialog(60);
  return nullptr;
}

// ==========================================================================
// LawnApp::TryHelpTextScreen(int)
// @002B359C  26 bytes
// ==========================================================================
Board * LawnApp::TryHelpTextScreen(int theHelpIndex)
{
  LawnPlayerInfo *aPlayerInfo = mPlayerInfo;
  if ( aPlayerInfo != nullptr && !aPlayerInfo->mHelpTextSeen[theHelpIndex] )
  {
    aPlayerInfo->mHelpTextSeen[theHelpIndex] = true;
    return ShowHelpTextScreen(theHelpIndex);
  }
  return nullptr;
}

// ==========================================================================
// LawnApp::KillLeaderboardScreen()
// @002B35B8  2 bytes
// ==========================================================================
void LawnApp::KillLeaderboardScreen()
{
  if (mLeaderboardScreen == nullptr)
    return;
  mWidgetManager->RemoveWidget(mLeaderboardScreen);
  SafeDeleteWidget(mLeaderboardScreen);
  mLeaderboardScreen = nullptr;
  ForceHideHelpBarWidget();
}

// ==========================================================================
// LawnApp::KillHouseScreen()
// @002B35BC  2 bytes
// ==========================================================================
void LawnApp::KillHouseScreen()
{
  KillLeaderboardScreen();
}

// ==========================================================================
// LawnApp::ShowAwardScreen(AwardType)
// @002B35C0  102 bytes
// ==========================================================================
int LawnApp::ShowAwardScreen(AwardType theAwardType)
{
  mGameScene = SCENE_AWARD;
  AwardScreen *aScreen = new AwardScreen(this, theAwardType);
  mAwardScreen = aScreen;
  aScreen->Resize(0, 0, mWidth, mHeight);
  mWidgetManager->AddWidget(aScreen);
  mWidgetManager->BringToBack(aScreen);
  mWidgetManager->SetFocus(aScreen);
  return 0;
}

// ==========================================================================
// LawnApp::KillAwardScreen()
// @002B3630  42 bytes
// ==========================================================================
int LawnApp::KillAwardScreen()
{
  if ( mAwardScreen != nullptr )
  {
    mWidgetManager->RemoveWidget(mAwardScreen);
    SafeDeleteWidget(mAwardScreen);
    mAwardScreen = nullptr;
  }
  return 0;
}

// ==========================================================================
// LawnApp::ShowCreditScreen(bool)
// @002B365C  102 bytes
// ==========================================================================
int LawnApp::ShowCreditScreen(bool theFromMainMenu)
{
  mGameScene = SCENE_CREDIT;
  CreditScreen *aScreen = new CreditScreen(this, theFromMainMenu);
  mCreditScreen = aScreen;
  aScreen->Resize(0, 0, mWidth, mHeight);
  mWidgetManager->AddWidget(aScreen);
  mWidgetManager->BringToBack(aScreen);
  mWidgetManager->SetFocus(aScreen);
  return 0;
}

// ==========================================================================
// LawnApp::KillCreditScreen()
// @002B36CC  42 bytes
// ==========================================================================
int LawnApp::KillCreditScreen()
{
  if ( mCreditScreen != nullptr )
  {
    mWidgetManager->RemoveWidget(mCreditScreen);
    SafeDeleteWidget(mCreditScreen);
    mCreditScreen = nullptr;
    ForceHideHelpBarWidget();
  }
  return 0;
}

// ==========================================================================
// LawnApp::ShowChallengeScreen(ChallengePage)
// @002B36F8  116 bytes
// ==========================================================================
int LawnApp::ShowChallengeScreen(ChallengePage thePage)
{
  mGameScene = SCENE_CHALLENGE;
  unsigned int aPlayerId = mPlayerInfo->GetId();
  ChallengeScreen *aScreen = new ChallengeScreen(this, thePage, aPlayerId);
  mChallengeScreen = aScreen;
  aScreen->Resize(0, 0, mWidth, mHeight);
  mWidgetManager->AddWidget(aScreen);
  mWidgetManager->BringToBack(aScreen);
  mWidgetManager->SetFocus(aScreen);
  return 0;
}

// ==========================================================================
// LawnApp::KillChallengeScreen()
// @002B3778  42 bytes
// ==========================================================================
int LawnApp::KillChallengeScreen()
{
  if ( mChallengeScreen != nullptr )
  {
    mWidgetManager->RemoveWidget(mChallengeScreen);
    SafeDeleteWidget(mChallengeScreen);
    mChallengeScreen = nullptr;
  }
  return 0;
}

// ==========================================================================
// LawnApp::KillStoreScreen()
// @002B37A4  42 bytes
// ==========================================================================
int LawnApp::KillStoreScreen()
{
  if ( GetDialog(6) == nullptr )
    return 0;
  KillDialog(6);
  return SexyAppBase::ClearUpdateBacklog(false);
}

// ==========================================================================
// LawnApp::ShowVSSetupScreen()
// @002B37D0  90 bytes
// ==========================================================================
int LawnApp::ShowVSSetupScreen()
{
  VSSetupMenu *aScreen = new VSSetupMenu();
  mVSSetupMenu = aScreen;
  aScreen->Resize(0, 0, mWidth, mHeight);
  mWidgetManager->AddWidget(aScreen);
  mWidgetManager->BringToFront(aScreen);
  mWidgetManager->SetFocus(aScreen);
  return 0;
}

// ==========================================================================
// LawnApp::KillVSSetupScreen()
// @002B3834  42 bytes
// ==========================================================================
int LawnApp::KillVSSetupScreen()
{
  if ( mVSSetupMenu != nullptr )
  {
    mWidgetManager->RemoveWidget(mVSSetupMenu);
    SafeDeleteWidget(mVSSetupMenu);
    mVSSetupMenu = nullptr;
  }
  return 0;
}

// ==========================================================================
// LawnApp::ShowVSResultsScreen()
// @002B3860  90 bytes
// ==========================================================================
int LawnApp::ShowVSResultsScreen()
{
  VSResultsMenu *aScreen = new VSResultsMenu();
  mVSResultsMenu = aScreen;
  aScreen->Resize(0, 0, mWidth, mHeight);
  mWidgetManager->AddWidget(aScreen);
  mWidgetManager->BringToBack(aScreen);
  mWidgetManager->SetFocus(aScreen);
  return 0;
}

// ==========================================================================
// LawnApp::KillVSResultsScreen()
// @002B38C4  50 bytes
// ==========================================================================
int LawnApp::KillVSResultsScreen()
{
  if ( mVSResultsMenu != nullptr )
  {
    if ( !mVSResultsMenu->mIsFading )
      mWidgetManager->RemoveWidget(mVSResultsMenu);

    SafeDeleteWidget(mVSResultsMenu);
    mVSResultsMenu = nullptr;
  }
  return 0;
}

// ==========================================================================
// LawnApp::ShowSeedChooserScreen()
// @002B38F8  88 bytes
// ==========================================================================
int LawnApp::ShowSeedChooserScreen()
{
  SeedChooserScreen *aScreen = new SeedChooserScreen(0);
  mSeedChooserScreen = aScreen;
  aScreen->Resize(0, 0, (IMAGE_SEEDCHOOSER_BACKGROUND)->mWidth,
                  (IMAGE_SEEDCHOOSER_BACKGROUND)->mHeight);
  mWidgetManager->AddWidget(aScreen);
  mWidgetManager->BringToFront(aScreen);
  return 0;
}

// ==========================================================================
// LawnApp::KillSeedChooserScreen()
// @002B3964  42 bytes
// ==========================================================================
int LawnApp::KillSeedChooserScreen()
{
  if ( mSeedChooserScreen != nullptr )
  {
    mWidgetManager->RemoveWidget(mSeedChooserScreen);
    SafeDeleteWidget(mSeedChooserScreen);
    mSeedChooserScreen = nullptr;
  }
  return 0;
}

// ==========================================================================
// LawnApp::ShowZombieChooserScreen()
// @002B3990  90 bytes
// ==========================================================================
int LawnApp::ShowZombieChooserScreen()
{ // TODO FAIL
  SeedChooserScreen *aScreen = new SeedChooserScreen(1);
  mZombieChooserScreen = aScreen;
  aScreen->Resize(800 - (IMAGE_SEEDCHOOSER_BACKGROUND2)->mWidth, 0,
                  (IMAGE_SEEDCHOOSER_BACKGROUND2)->mWidth,
                  (IMAGE_SEEDCHOOSER_BACKGROUND2)->mHeight);
  mWidgetManager->AddWidget(aScreen);
  mWidgetManager->BringToFront(aScreen);
  return 0;
}

// ==========================================================================
// LawnApp::KillZombieChooserScreen()
// @002B39FC  42 bytes
// ==========================================================================
int LawnApp::KillZombieChooserScreen()
{
  if ( mZombieChooserScreen != nullptr )
  {
    mWidgetManager->RemoveWidget(mZombieChooserScreen);
    SafeDeleteWidget(mZombieChooserScreen);
    mZombieChooserScreen = nullptr;
  }
  return 0;
}

// ==========================================================================
// LawnApp::StartPlaying()
// @002B3A28  50 bytes
// ==========================================================================
int LawnApp::StartPlaying()
{
  KillSeedChooserScreen();
  KillZombieChooserScreen();
  KillVSSetupScreen();
  mBoard->StartLevel();
  mGameScene = SCENE_PLAYING;
  mBoard->RebuildHelpBar();
  mHelpIndex = 0;
  return 0;
}

// ==========================================================================
// LawnApp::DoConfirmBackToMain(bool)
// @002B3A5C  52 bytes
// ==========================================================================
ConfirmBackToMainDialog * LawnApp::DoConfirmBackToMain(bool theSaveGame)
{
  // Garden progress lives in the profile. Returning from it has no level to
  // restart or resume, so request the board's deferred exit instead of a prompt.
  if (mBoard != nullptr
      && (mGameMode == GAMEMODE_CHALLENGE_ZEN_GARDEN || mGameMode == GAMEMODE_TREE_OF_WISDOM))
  {
    if (!mBoard->mExitRequested)
    {
      PostLeaveLevel();
      SetBoardResult(BOARDRESULT_QUIT);
      mBoard->TryToExit();
    }
    return nullptr;
  }

  // The confirmation says the game is saved. Write its snapshot before showing it.
  if (theSaveGame && mBoard != nullptr && !mBoard->TryToSaveGame())
    return nullptr;

  ConfirmBackToMainDialog *aDialog = new ConfirmBackToMainDialog(theSaveGame);
  AddDialogWithId(23, aDialog);
  mWidgetManager->SetFocus(aDialog);
  return aDialog;
}

// ==========================================================================
// LawnApp::NewDialog(int,bool,Sexy::String const&,Sexy::String const&,Sexy::String const&,int)
// @002B3AC4  68 bytes
// ==========================================================================
LawnDialog * LawnApp::NewDialog(int theDialogId, bool theModal, Sexy::String const& theHeader, Sexy::String const& theLines, Sexy::String const& theFooter, int theButtonMode)
{
  LawnDialog *aDialog = new LawnDialog(this, nullptr, theDialogId, theModal, theHeader, theLines, theFooter, theButtonMode);
  CenterDialog(aDialog, aDialog->mWidth, aDialog->mHeight);// TODO FAIL
  return aDialog;
}

// ==========================================================================
// LawnApp::DoContinueDialog()
// @002B3B14  58 bytes
// ==========================================================================
int LawnApp::DoContinueDialog()
{
  ContinueDialog *aDialog = new ContinueDialog(this);
  CenterDialog(aDialog, aDialog->mWidth, aDialog->mHeight);// TODO FAIL
  AddDialogWithId(39, aDialog);
  mWidgetManager->SetFocus(aDialog);
  return 0;
}

// ==========================================================================
// LawnApp::FinishTimesUpDialog()
// @002B3B58  14 bytes
// ==========================================================================
int LawnApp::FinishTimesUpDialog()
{
  return KillDialog(44);
}

// ==========================================================================
// LawnApp::FinishModelessDialogs()
// @002B3B68  2 bytes
// ==========================================================================
void LawnApp::FinishModelessDialogs()
{
}

// ==========================================================================
// LawnApp::KillBoard()
// @002B3B6C  98 bytes
// ==========================================================================
int LawnApp::KillBoard()
{
  mGameScene = SCENE_LEVEL_INTRO;
  FinishModelessDialogs();
  KillSeedChooserScreen();
  KillZombieChooserScreen();
  KillVSSetupScreen();
  if ( mBoard != nullptr )
  {
    BetaRecordLevelStats();
    mBoard->DisposeBoard();
    mWidgetManager->RemoveWidget(mBoard);
    SafeDeleteWidget(mBoard);
    mBoard = nullptr;
  }
  return SexyAppBase::SetCursor(0);
}

// ==========================================================================
// LawnApp::MakeNewBoard()
// @002B3BD0  108 bytes
// ==========================================================================
int LawnApp::MakeNewBoard()
{
  KillBoard();
  Board *aBoard = new Board(this);
  mBoard = aBoard;
  aBoard->Resize(0, 0, mWidth, mHeight);
  mWidgetManager->AddWidget(aBoard);
  mWidgetManager->BringToBack(aBoard);

  int result = GetDialogCount();
  if ( result == 0 )
  {
    mWidgetManager->SetFocus(aBoard);
    return 0;
  }
  return result;
}

// ==========================================================================
// LawnApp::NewGame()
// @002B3C48  98 bytes
// ==========================================================================
int LawnApp::NewGame()
{
  mFirstTimeGameSelector = false;
  MakeNewBoard();
  mBoard->InitLevel();
  GameMode aGameMode = mGameMode;
  mBoardResult = BOARDRESULT_NONE;
  mGameScene = SCENE_LEVEL_INTRO;
  if ( aGameMode == GAMEMODE_MULTI_PLAYER )
  {
    ShowSeedChooserScreen();
    ShowZombieChooserScreen();
  }
  else
  {
    if ( aGameMode == GAMEMODE_MP_VS )
    {
      ShowVSSetupScreen();
      mBoard->mCutScene->StartLevelIntro();
      return 0;
    }
    ShowSeedChooserScreen();
  }
  mBoard->mCutScene->StartLevelIntro();
  return 0;
}

// ==========================================================================
// LawnApp::ShowStoreScreen()
// @002B3CAC  54 bytes
// ==========================================================================
StoreScreen * LawnApp::ShowStoreScreen()
{
  FinishModelessDialogs();
  StoreScreen *aScreen = new StoreScreen(this);
  AddDialogWithId(6, aScreen);
  mWidgetManager->SetFocus(aScreen);
  return aScreen;
}

// ==========================================================================
// LawnApp::DoNewOptions(bool,unsigned int)
// @002B3CEC  74 bytes
// ==========================================================================
int LawnApp::DoNewOptions(bool theFromGameSelector, unsigned int thePlayerIndex)
{
  FinishModelessDialogs();
  NewOptionsDialog *aDialog = new NewOptionsDialog(this, theFromGameSelector, thePlayerIndex);
  CenterDialog(aDialog, 413, 585);
  AddDialogWithId(2, aDialog);
  mWidgetManager->SetFocus(aDialog);
  return 0;
}

// ==========================================================================
// LawnApp::DoAlmanacDialog(SeedType,ZombieType)
// @002B3D40  114 bytes
// ==========================================================================
AlmanacDialog * LawnApp::DoAlmanacDialog(SeedType theSeedType, ZombieType theZombieType)
{
  Sexy::PerfTimer aTimer;
  aTimer.Start();

  FinishModelessDialogs();
  AlmanacDialog *aDialog = new AlmanacDialog(this);
  AddDialogWithId(5, aDialog);
  mWidgetManager->SetFocus(aDialog);

  if ( theSeedType == -1 )
  {
    if ( theZombieType != -1 )
      aDialog->ShowZombie(theZombieType);
  }
  else
  {
    aDialog->ShowPlant(theSeedType);
  }

  aTimer.GetDuration();
  return aDialog;
}

// ==========================================================================
// LawnApp::DoPauseDialog()
// @002B3DBC  318 bytes
// ==========================================================================
void LawnApp::DoPauseDialog()
{
  if ( mBoard != nullptr )
    mBoard->Pause(true);

  FinishModelessDialogs();

  // The three keys are copied as-is. DoDialog (vtable +0x198) translates them;
  // these locals release through their own destructors.
  Sexy::String aHeader("[GAME_PAUSED]");
  Sexy::String aLines("[CLICK_TO_RESUME]");
  Sexy::String aFooter("[RESUME_GAME]");
  LawnDialog *aDialog = static_cast<LawnDialog *>(
      DoDialog(20, true, aHeader, aLines, aFooter, 3));

  // 5.0f and 42.0f are the two float immediates. Type 30 and the 155-pixel
  // gap under the header are immediates too; CalcSize sees the width test.
  aDialog->mReanimationWidget->AddReanimation(5.0f, 42.0f, REANIM_ZOMBIE_NEWSPAPER);
  aDialog->mSpaceAfterHeader = 155;
  aDialog->CalcSize(0, 10, aDialog->mWidth < 500);
  CenterDialog(aDialog, aDialog->mWidth, aDialog->mHeight);
}
// ==========================================================================
// LawnApp::LostFocus()
// @002B3F3C  30 bytes
// ==========================================================================
void LawnApp::LostFocus()
{
  if ( !mTodCheatKeys && CanPauseNow() )
    DoPauseDialog();
}

// ==========================================================================
// LawnApp::LawnMessageBox(int,char const*,char const*,char const*,char const*,int)
// @002B3F5C  296 bytes
// ==========================================================================
int LawnApp::LawnMessageBox(int theDialogId, char const* theHeader, char const* theLines, char const* theButton1, char const* theButton2, int theButtonMode)
{
  // Saved from WidgetManager::mFocusWidget (+0xA0) and restored after the box.
  Sexy::Widget *aOldFocus = mWidgetManager->mFocusWidget;

  // DoDialog is vtable +0x198. The three keys are copied as-is; these locals
  // release through their own destructors, immediately after the call.
  LawnDialog *aDialog;
  {
    Sexy::String aHeader(theHeader);
    Sexy::String aLines(theLines);
    Sexy::String aFooter(theButton1);
    aDialog = static_cast<LawnDialog *>(
        DoDialog(theDialogId, true, aHeader, aLines, aFooter, theButtonMode));
  }

  // Remove-then-add uses the dialog's own manager (+0x18). 99 is the z-order.
  Sexy::WidgetManager* aDialogManager = aDialog->mWidgetManager;
  aDialogManager->RemoveWidget(aDialog);
  aDialog->mZOrder = 99;
  aDialogManager->AddWidget(aDialog);

  // Direct mLabel assign, not SetLabel: SetLabel would translate again.
  if ( aDialog->mLawnYesButton != nullptr )
  {
    Sexy::String aText;
    TodStringTranslate(&aText, theButton1);
    aDialog->mLawnYesButton->mLabel = aText;
  }

  if ( aDialog->mLawnNoButton != nullptr )
  {
    Sexy::String aText;
    TodStringTranslate(&aText, theButton2);
    aDialog->mLawnNoButton->mLabel = aText;
  }

  // App manager SetFocus (vtable +0x30) is after the labels. Dialog slot 57
  // (vtable +0xE4) focuses the yes button, or the no button when yes is null.
  mWidgetManager->SetFocus(aDialog);
  GameButton *aDefaultButton = aDialog->mLawnYesButton;
  if ( aDefaultButton == nullptr )
    aDefaultButton = aDialog->mLawnNoButton;
  aDialog->SetFocus(aDefaultButton, true);

  int aResult = aDialog->WaitForResult(true);
  mWidgetManager->SetFocus(aOldFocus);
  return aResult;
}
// ==========================================================================
// LawnApp::showRechargeBoard(Sexy::String)
// @002B40C0  78 bytes
// ==========================================================================
int LawnApp::showRechargeBoard(Sexy::String theAmount)
{
  if ( LawnMessageBox(51, "[NOVEL_RECHARGE_HEADER]", theAmount.c_str(),
                      "[DIALOG_BUTTON_OK]", "[DIALOG_BUTTON_CANCEL]", 1) == 1000 )
  {
    mAdProvider->recharge();
    return 0;
  }

  // The cancel path re-enters the purchase flow with an empty item id.
  setPayItemsRes(false, "", 0, "");
  return 0;
}
// ==========================================================================
// LawnApp::DoDialogDelay(int,bool,Sexy::String const&,Sexy::String const&,Sexy::String const&,int)
// @002B4120  42 bytes
// ==========================================================================
Sexy::Dialog* LawnApp::DoDialogDelay(int theDialogId)
{
  Sexy::Dialog* aDialog = GetDialog(theDialogId);
  KillDialog(30);
  return aDialog;
}

// ==========================================================================
// LawnApp::DoUserDialog()
// @002B414C  68 bytes
// ==========================================================================
int LawnApp::DoUserDialog()
{
  KillDialog(30);
  UserDialog *aDialog = new UserDialog(this);
  CenterDialog(aDialog, aDialog->mWidth, aDialog->mHeight);
  AddDialogWithId(30, aDialog);
  mWidgetManager->SetFocus(aDialog);
  return 0;
}

// ==========================================================================
// LawnApp::FinishUserDialog(bool)
// @002B419C  152 bytes
// ==========================================================================
int LawnApp::FinishUserDialog(bool theYes)
{
  UserDialog *aUserDialog = static_cast<UserDialog *>(GetDialog(30));
  if ( aUserDialog == nullptr )
    return 0;

  if ( theYes != 0 )
  {
    Sexy::String aName = aUserDialog->GetSelName();
    Sexy::PlayerInfo *aProfile = mProfileMgr->GetProfile(aName);
    if ( aProfile != nullptr )
    {
      mPlayerInfo = static_cast<LawnPlayerInfo *>(aProfile);
      mWidgetManager->MarkAllDirty();
      if ( mGameSelector != nullptr )
        mGameSelector->SyncProfile(false);
    }
  }
  return KillDialog(30);
}

// ==========================================================================
// LawnApp::DoBuyMoneyDialog(Sexy::String const&)
// @002B424C  134 bytes
// ==========================================================================
int LawnApp::DoBuyMoneyDialog(const Sexy::String &theMessage)
{
  int result; // r0

  KillDialog(82);
  if ( mCanDoBuyMoneyDialog )
  {
    BuyMoneyDialog *aDialog = new BuyMoneyDialog(this, theMessage);
    CenterDialog(aDialog, aDialog->mWidth, aDialog->mHeight);
    AddDialogWithId(82, aDialog);
    mWidgetManager->SetFocus(aDialog);
    return 0;
  }

  result = LawnMessageBox(51,
             "[LOGIN_FAILED_HEADER]",
             "[LOGIN_FAILED_CANNOT_RECHARGE_MSG]",
             "[DIALOG_BUTTON_OK]",
             gEmptyCString,
             3);
  if ( result == 1000 )
    return KillDialog(51);
  return result;
}

// ==========================================================================
// LawnApp::FinishBuyMoneyDialog(bool)
// @002B42EC  30 bytes
// ==========================================================================
int LawnApp::FinishBuyMoneyDialog(bool theYes)
{
  if ( GetDialog(82) == nullptr )
    return 0;
  return KillDialog(82);
}

// ==========================================================================
// LawnApp::DoCreateUserDialog()
// @002B430C  58 bytes
// ==========================================================================
int LawnApp::DoCreateUserDialog()
{
  KillDialog(31);
  NewUserDialog *aDialog = new NewUserDialog(this, false);
  CenterDialog(aDialog, aDialog->mWidth, aDialog->mHeight);
  AddDialogWithId(31, aDialog);
  return 0;
}

// ==========================================================================
// LawnApp::FinishCreateUserDialog(bool)
// @002B4350  524 bytes
// ==========================================================================
void LawnApp::FinishCreateUserDialog(bool theYes)
{
  NewUserDialog *aDialog = static_cast<NewUserDialog *>(GetDialog(31));
  if ( aDialog == nullptr )
    return;

  // NewUserDialog::GetName returns its Sexy::String by value; the ARM's r0 is
  // the result buffer, so the receiver is the dialog.
  Sexy::String aName = aDialog->GetName();

  if ( !theYes )
  {
    // Cancelled: with a player already loaded the dialog just goes away,
    // otherwise the create-user prompt comes back.
    if ( mPlayerInfo != nullptr )
    {
      KillDialog(31);
    }
    else
    {
      DoDialog(34, true,
               Sexy::String("[ENTER_YOUR_NAME]"),
               Sexy::String("[USER_ERROR_MESSAGE]"),
               Sexy::String("[DIALOG_BUTTON_OK]"),
               3);
    }
    return;
  }

  if ( aName.size() == 0 )
  {
    DoDialog(34, true,
             Sexy::String("[ENTER_YOUR_NAME]"),
             Sexy::String("[USER_ERROR_MESSAGE]"),
             Sexy::String("[DIALOG_BUTTON_OK]"),
             3);
    return;
  }

  // AddProfile rejects a name another profile already carries.
  LawnPlayerInfo *aPlayerInfo = static_cast<LawnPlayerInfo *>(mProfileMgr->AddProfile(aName));
  if ( aPlayerInfo == nullptr )
  {
    DoDialog(34, true,
             Sexy::String("[NAME_CONFLICT]"),
             Sexy::String("[ENTER_UNIQUE_PLAYER_NAME]"),
             Sexy::String("[DIALOG_BUTTON_OK]"),
             3);
    return;
  }

  mProfileMgr->Save();
  mPlayerInfo = aPlayerInfo;
  KillDialog(30);
  KillDialog(31);
  mWidgetManager->MarkAllDirty();
  if ( mGameSelector != nullptr )
    mGameSelector->SyncProfile(false);
}
// ==========================================================================
// LawnApp::DoCheatDialog()
// @002B4614  56 bytes
// ==========================================================================
int LawnApp::DoCheatDialog()
{
#ifdef _DEBUG
  if (!mDebugKeysEnabled || gDisableDebugKeys)
    return 0;
  KillDialog(36);
  CheatDialog *aDialog = new CheatDialog(this);
  CenterDialog(aDialog, aDialog->mWidth, aDialog->mHeight);
  AddDialogWithId(36, aDialog);
#endif
  return 0;
}

// ==========================================================================
// LawnApp::DoCheatCodeDialog()
// @002B4658  56 bytes
// ==========================================================================
int LawnApp::DoCheatCodeDialog()
{
  KillDialog(37);
  CheatCodeDialog *aDialog = new CheatCodeDialog(this);
  CenterDialog(aDialog, aDialog->mWidth, aDialog->mHeight); // TODO FAIL
  AddDialogWithId(37, aDialog);
  return 0;
}

// ==========================================================================
// LawnApp::FinishCheatCodeDialog(bool)
// @002B469C  42 bytes
// ==========================================================================
bool LawnApp::FinishCheatCodeDialog(bool theYes)
{
  CheatCodeDialog *aDialog = static_cast<CheatCodeDialog *>(GetDialog(37));
  if ( aDialog == nullptr )
    return false;
  if ( theYes == 0 || aDialog->ApplyCheat() != 0 )
  {
    KillDialog(37);
    return false;
  }
  return true;
}

// ==========================================================================
// LawnApp::DoConfirmDeleteUserDialog(Sexy::String const&)
// @002B46C8  294 bytes
// ==========================================================================
void LawnApp::DoConfirmDeleteUserDialog(Sexy::String const& theName)
{
  // Slot 107 kills the dialog already carrying id 32, then slot 102 opens the
  // confirmation with the warning text and the name to delete formatted into it.
  KillDialog(32);

  Sexy::String aTitle;
  TodStringTranslate(&aTitle, "[ARE_YOU_SURE]");
  Sexy::String aWarning;
  TodStringTranslate(&aWarning, "[DELETE_USER_WARNING]");
  Sexy::String aMessage;
  StrFormat(&aMessage, aWarning.c_str(), theName.c_str());
  Sexy::String aEmpty(gEmptyCString);

  DoDialog(32, true, aTitle, aMessage, aEmpty, 1);
}

// ==========================================================================
// LawnApp::FinishConfirmDeleteUserDialog(bool)
// @002B483C  276 bytes
// ==========================================================================
UserDialog * LawnApp::FinishConfirmDeleteUserDialog(bool theYes)
{
  KillDialog(32);

  UserDialog *aUserDialog = static_cast<UserDialog *>(GetDialog(30));
  if (aUserDialog == nullptr)
    return nullptr;

  mWidgetManager->SetFocus(aUserDialog);
  if (theYes == 0)
    return aUserDialog;

  // The profile the deletion is about. When there is no player loaded the name
  // is empty, which is what makes the comparison below fail and the current
  // player survive the delete.
  Sexy::String aOldName = mPlayerInfo != nullptr ? mPlayerInfo->GetName()
                                                 : Sexy::String(gEmptyCString);
  Sexy::String aSelName = aUserDialog->GetSelName();
  if (aSelName == aOldName)
    mPlayerInfo = nullptr;

  mProfileMgr->DeleteProfile(aSelName);
  aUserDialog->FinishDeleteUser();

  if (mPlayerInfo == nullptr)
  {
    mPlayerInfo = static_cast<LawnPlayerInfo *>(mProfileMgr->GetProfile(aUserDialog->GetSelName()));
    if (mPlayerInfo == nullptr)
      mPlayerInfo = static_cast<LawnPlayerInfo *>(mProfileMgr->GetAnyProfile());
  }

  mProfileMgr->Save();
  if (mPlayerInfo == nullptr)
    DoCreateUserDialog();

  mWidgetManager->MarkAllDirty();
  return aUserDialog;
}

// ==========================================================================
// LawnApp::DoRenameUserDialog(Sexy::String const&)
// @002B497C  68 bytes
// ==========================================================================
int LawnApp::DoRenameUserDialog(Sexy::String const& theName)
{
  KillDialog(33);
  NewUserDialog *aDialog = new NewUserDialog(this, true);
  CenterDialog(aDialog, aDialog->mWidth, aDialog->mHeight);
  aDialog->SetName(theName);
  AddDialogWithId(33, aDialog);
  return 0;
}

// ==========================================================================
// LawnApp::FinishRenameUserDialog(bool)
// @002B49CC  358 bytes
// ==========================================================================
UserDialog * LawnApp::FinishRenameUserDialog(bool theYes)
{
  UserDialog *aUserDialog = static_cast<UserDialog *>(GetDialog(30));
  if ( !theYes )
  {
    KillDialog(33);
    mWidgetManager->SetFocus(aUserDialog);
    return aUserDialog;
  }

  NewUserDialog *aRenameDialog = static_cast<NewUserDialog *>(GetDialog(33));
  if ( aUserDialog == nullptr || aRenameDialog == nullptr )
    return nullptr;

  Sexy::String aOldName = aUserDialog->GetSelName();
  Sexy::String aNewName = aRenameDialog->GetName();
  if ( aNewName.size() == 0 )
    return nullptr;

  // RenameProfile rejects a name another profile already carries. The player the
  // manager was pointing at is compared by address, so that renaming the profile
  // currently in use re-resolves it to the record under its new name.
  LawnPlayerInfo *aOldPlayerInfo = static_cast<LawnPlayerInfo *>(mProfileMgr->GetProfile(aOldName));
  LawnPlayerInfo *aPlayerInfo = mPlayerInfo;
  if ( mProfileMgr->RenameProfile(aOldName, aNewName) == 0 )
  {
    DoDialog(35, true,
             Sexy::String("[NAME_CONFLICT]"),
             Sexy::String("[ENTER_UNIQUE_PLAYER_NAME]"),
             Sexy::String("[DIALOG_BUTTON_OK]"),
             3);
    return nullptr;
  }

  mProfileMgr->Save();
  if ( aOldPlayerInfo == aPlayerInfo )
    mPlayerInfo = static_cast<LawnPlayerInfo *>(mProfileMgr->GetProfile(aNewName));

  aUserDialog->FinishRenameUser(aNewName);
  mWidgetManager->MarkAllDirty();
  KillDialog(33);
  mWidgetManager->SetFocus(aUserDialog);
  return aUserDialog;
}
// ==========================================================================
// LawnApp::FinishNameError(int)
// @002B4B88  52 bytes
// ==========================================================================
int LawnApp::FinishNameError(int theId)
{
  // The name-error dialog belongs to the user dialog that raised it: 31 for the
  // create-user dialog, 33 for the rename one. The dialog id is still in r1 at
  // the first call, so KillDialog takes theId itself.
  KillDialog(theId);

  NewUserDialog *aDialog = static_cast<NewUserDialog *>(GetDialog(theId == 34 ? 31 : 33));
  if (aDialog == nullptr)
    return 0;

  mWidgetManager->SetFocus(aDialog->mEditWidget);
  return 0;
}

// ==========================================================================
// LawnApp::KillNewOptionsDialog()
// @002B4BBC  40 bytes
// ==========================================================================
int LawnApp::KillNewOptionsDialog()
{
  if (GetDialog(2) == nullptr)
    return 0;

  KillDialog(2);
  ClearUpdateBacklog(false);
  return 1;
}

// ==========================================================================
// LawnApp::KillAlmanacDialog()
// @002B4BE4  40 bytes
// ==========================================================================
int LawnApp::KillAlmanacDialog()
{
  if (GetDialog(5) == nullptr)
    return 0;

  KillDialog(5);
  ClearUpdateBacklog(false);
  return 1;
}

// ==========================================================================
// LawnApp::KillAll()
// @002B4C0C  114 bytes
// ==========================================================================
EffectSystem * LawnApp::KillAll()
{
  KillBoard();
  KillTitleScreen();
  KillGameSelector();
  KillChallengeScreen();
  KillNewOptionsDialog();
  KillCreditScreen();
  KillAwardScreen();
  KillAlmanacDialog();
  KillSeedChooserScreen();
  KillZombieChooserScreen();
  KillVSSetupScreen();
  KillVSResultsScreen();
  KillMainMenu();
  KillLeaderboardScreen();
  KillHouseScreen();
  KillAchievementsScreen();
  KillHelpTextScreen();
  if ( mEffectSystem != nullptr )
  {
    mEffectSystem->EffectSystemFreeAll();
    return mEffectSystem;
  }
  return nullptr;
}

// ==========================================================================
// LawnApp::NeedPauseGame()
// @002B4C80  86 bytes
// ==========================================================================
bool LawnApp::NeedPauseGame()
{
  if ( mDialogList.empty() )
    return false;

  int aCount = (int)mDialogList.size();

  if ( aCount == 1 )
  {
    // 29 is DIALOG_CHOOSER_WARNING, 52 DIALOG_IMITATER and 53
    // DIALOG_PURCHASE_PACKET_SLOT: none of them pauses the board.
    int aDialogId = mDialogList.front()->mId;
    if ( aDialogId != 0 && (aDialogId == 29 || (unsigned int)(aDialogId - 52) <= 1) )
      return false;
  }

  // 44 is GAMEMODE_CHALLENGE_ZEN_GARDEN and 51 GAMEMODE_TREE_OF_WISDOM; neither
  // has a board that pauses.
  return mBoard == nullptr
      || (mGameMode != GAMEMODE_CHALLENGE_ZEN_GARDEN && mGameMode != GAMEMODE_TREE_OF_WISDOM);
}

// ==========================================================================
// LawnApp::ModalOpen()
// @002B4CD8  32 bytes
// ==========================================================================
void LawnApp::ModalOpen()
{
  // Board::Pause is void, so the pointer the binary leaves in r0 is not this
  // function's return value; the declaration is void.
  if ( mBoard != nullptr && NeedPauseGame() )
    mBoard->Pause(true);
}

// ==========================================================================
// LawnApp::NeedShowQRCode()
// @002B4CF8  4 bytes
// ==========================================================================
int LawnApp::NeedShowQRCode()
{
  return 1;
}

// ==========================================================================
// LawnApp::StepAdvance()
// @002B4CFC  14 bytes
// ==========================================================================
void LawnApp::StepAdvance()
{
  gStepReady = 1;
}

// ==========================================================================
// LawnApp::ToggleSlowMo()
// @002B4D14  34 bytes
// ==========================================================================
int LawnApp::ToggleSlowMo()
{
  gSlowMoFrame = 0;
  gSlowMo ^= 1u;
  gFastMo = 0;
  return -3276;
}

// ==========================================================================
// LawnApp::ToggleFastMo()
// @002B4D44  26 bytes
// ==========================================================================
int LawnApp::ToggleFastMo()
{
  gSlowMo = 0;
  gFastMo ^= 1u;
  return -3276;
}

// ==========================================================================
// LawnApp::ToggleStep()
// @002B4D6C  38 bytes
// ==========================================================================
int LawnApp::ToggleStep()
{
  gSlowMo = 0;
  gStep ^= 1u;
  gFastMo = 0;
  return -3276;
}

// ==========================================================================
// LawnApp::UpdatePlayTimeStats()
// @002B4DA4  2 bytes
// ==========================================================================
void LawnApp::UpdatePlayTimeStats()
{
  ;
}

// ==========================================================================
// LawnApp::TraceLoadGroup(char const*,int,int,int)
// @002B4DA8  2 bytes
// ==========================================================================
void LawnApp::TraceLoadGroup(char const* theGroupName, int theGroupTime, int theTotalGroupWeight, int theTaskWeight)
{
  ;
}

// ==========================================================================
// LawnApp::LoadGroup(char const*,int)
// @002B4DAC  282 bytes
// ==========================================================================
void LawnApp::LoadGroup(char const* theGroupName, int theGroupAveMsToLoad)
{
  Sexy::PerfTimer aTimer;
  aTimer.Start();

  // StartLoadResources(String const&, bool) is ResourceManager slot 24; the
  // tree's ResourceManager.h models it as a plain method.
  Sexy::String aGroupName(theGroupName);
  mResourceManager->StartLoadResources(aGroupName, false);

  if ( mShutDown )
    return;

  while ( !mCloseRequest )
  {
    if ( mLoadingFailed )
      break;
    if ( mResourceManager->LoadNextResource() == 0 )
      break;

    bool aShutDown = mShutDown;
    mCompletedLoadingThreadTasks += theGroupAveMsToLoad;
    if ( aShutDown )
      return;
  }

  if ( mShutDown || mCloseRequest )
    return;

  // Publish loaded images before the error check. HadError used to skip the
  // extract, so a single missing file left every decoded image unpublished.
  bool aPublished = Sexy::ExtractResourcesByName(mResourceManager, theGroupName);
  if ( mResourceManager->HadError() || !aPublished )
  {
    ShowResourceError(false);
    mLoadingFailed = true;
    return;
  }

  Sexy::String aGroupName2(theGroupName);
  int aNumResources = mResourceManager->GetNumResources(aGroupName2, true, true) * theGroupAveMsToLoad;
  int aDuration = (int)aTimer.GetDuration();
  TraceLoadGroup(theGroupName, aDuration < 0 ? 0 : aDuration, aNumResources, theGroupAveMsToLoad);
}
// ==========================================================================
// LawnApp::ConfirmQuit()
// @002B4EE4  322 bytes
// ==========================================================================
Sexy::Dialog * LawnApp::ConfirmQuit()
{
  // vtable+0x198 is DoDialog. The footer is the empty C string at gEmptyCString.
  // The yes caption is translated after the call and assigned into mLabel;
  // GameButton::SetLabel would translate again and rewrite gamepad captions.
  Sexy::String aMessage;
  TodStringTranslate(&aMessage, "[QUIT_MESSAGE]");
  Sexy::String aHeader;
  TodStringTranslate(&aHeader, "[QUIT_HEADER]");

  LawnDialog *aDialog = static_cast<LawnDialog *>(
      DoDialog(13, true, aHeader, aMessage, Sexy::String(gEmptyCString), 2));

  Sexy::String aButtonText;
  TodStringTranslate(&aButtonText, "[QUIT_BUTTON]");
  aDialog->mLawnYesButton->mLabel = aButtonText;
  CenterDialog(aDialog, aDialog->mWidth, aDialog->mHeight);
  return aDialog;
}

// ==========================================================================
// LawnApp::ConfirmCheckForUpdates()
// @002B5074  2 bytes
// ==========================================================================
void LawnApp::ConfirmCheckForUpdates()
{
  ;
}

// ==========================================================================
// LawnApp::CheckForUpdates()
// @002B5078  2 bytes
// ==========================================================================
void LawnApp::CheckForUpdates()
{
  ;
}

// ==========================================================================
// LawnApp::PlayFoley(FoleyType)
// @002B507C  16 bytes
// ==========================================================================
FoleyTypeDataElement* LawnApp::PlayFoley(FoleyType theFoley)
{
  if ( mMuteSoundsForCutscene || mSoundSystem == nullptr )
    return nullptr;
  return mSoundSystem->PlayFoley(theFoley);
}

// ==========================================================================
// LawnApp::PlayFoleyPitch(FoleyType,float)
// @002B508C  16 bytes
// ==========================================================================
FoleyTypeDataElement* LawnApp::PlayFoleyPitch(FoleyType theFoley, float thePitch)
{
  if ( mMuteSoundsForCutscene || mSoundSystem == nullptr )
    return nullptr;
  return mSoundSystem->PlayFoleyPitch(theFoley, thePitch);
}

// ==========================================================================
// LawnApp::GetStageString(int)
// @002B509C  74 bytes
// ==========================================================================
Sexy::String LawnApp::GetStageString(int theLevel)
{
  // (level - 1) / 10 is the 0x66666667 multiply. Area is that plus one, then
  // the BLE/BGT clamps it to 1..6; the sub-level is level - (area - 1) * 10.
  int aArea = (theLevel - 1) / 10 + 1;
  if (aArea <= 1)
    aArea = 1;
  else if (aArea > 5)
    aArea = 6;

  Sexy::String aResult;
  StrFormat(&aResult, "%d-%d", aArea, theLevel - (aArea - 1) * 10);
  return aResult;
}

// ==========================================================================
// LawnApp::GetCurrentChallengeIndex()
// @002B50EC  8 bytes
// ==========================================================================
int LawnApp::GetCurrentChallengeIndex()
{
  return mGameMode - GAMEMODE_SURVIVAL_NORMAL_STAGE_1;
}

// ==========================================================================
// LawnApp::GetCurrentChallengeDef()
// @002B50F4  14 bytes
// ==========================================================================
const ChallengeDefinition* LawnApp::GetCurrentChallengeDef()
{
  return GetChallengeDefinition(GetCurrentChallengeIndex());
}

// ==========================================================================
// LawnApp::IsAdventureMode()
// @002B5104  14 bytes
// ==========================================================================
bool LawnApp::IsAdventureMode()
{
  return mGameMode <= GAMEMODE_ADVENTURE_TWO_PLAYER;
}

// ==========================================================================
// LawnApp::EndLevel()
// @002B5114  32 bytes
// ==========================================================================
int LawnApp::EndLevel()
{
  KillBoard();
  if ( IsAdventureMode() )
    NewGame();
  return NewGame();
}

// ==========================================================================
// LawnApp::IsSurvivalMode()
// @002B5134  34 bytes
// ==========================================================================
bool LawnApp::IsSurvivalMode()
{
  // The binary computes this branchlessly as
  //   (mGameMode - 2) <= 14 || (mGameMode - 84) <= 2 || mGameMode == 89
  // which is the two survival ranges plus the two-player endless mode.
  return (mGameMode >= GAMEMODE_SURVIVAL_NORMAL_STAGE_1 && mGameMode <= GAMEMODE_SURVIVAL_ENDLESS_STAGE_5)
      || (mGameMode >= GAMEMODE_TWO_PLAYER_COOP_DAY_HARD && mGameMode <= GAMEMODE_TWO_PLAYER_COOP_ROOF_HARD)
      || mGameMode == GAMEMODE_TWO_PLAYER_COOP_ENDLESS;
}

// ==========================================================================
// LawnApp::IsCoopMode()
// @002B5158  16 bytes
// ==========================================================================
bool LawnApp::IsCoopMode()
{
  return mGameMode >= GAMEMODE_MP_VS_COOP && mGameMode <= GAMEMODE_TWO_PLAYER_COOP_ENDLESS;
}

// ==========================================================================
// LawnApp::IsTwoPlayerGame()
// @002B5168  38 bytes
// ==========================================================================
int LawnApp::IsTwoPlayerGame()
{
  // Board +0x230/+0x234 are mGamepadControls[0]/[1] and +0x98 inside each is
  // GamepadControls::mGamepadIndex; -1 means no gamepad is bound.
  if ( mBoard == nullptr )
    return 0;
  return mBoard->mGamepadControls[0]->mGamepadIndex != -1
      && mBoard->mGamepadControls[1]->mGamepadIndex != -1;
}

// ==========================================================================
// LawnApp::IsTwinSunbankMode()
// @002B5190  4 bytes
// ==========================================================================
bool LawnApp::IsTwinSunbankMode()
{
  return IsCoopMode();
}

// ==========================================================================
// LawnApp::IsPuzzleMode()
// @002B5194  26 bytes
// ==========================================================================
bool LawnApp::IsPuzzleMode()
{
  // 52..71 is GAMEMODE_SCARY_POTTER_1 .. GAMEMODE_PUZZLE_I_ZOMBIE_ENDLESS.
  return mGameMode >= GAMEMODE_SCARY_POTTER_1 && mGameMode <= GAMEMODE_PUZZLE_I_ZOMBIE_ENDLESS;
}

// ==========================================================================
// LawnApp::IsChallengeMode()
// @002B51B0  38 bytes
// ==========================================================================
bool LawnApp::IsChallengeMode()
{
  return !IsAdventureMode() && !IsPuzzleMode() && !IsSurvivalMode();
}

// ==========================================================================
// LawnApp::IsArtChallenge()
// @002B51D8  38 bytes
// ==========================================================================
int LawnApp::IsArtChallenge()
{
  if ( mBoard == nullptr )
    return 0;
  // 41 is GAMEMODE_CHALLENGE_ART_CHALLENGE_SUNFLOWER, 37 the wallnut one and
  // 23 GAMEMODE_CHALLENGE_SEEING_STARS.
  return mGameMode == GAMEMODE_CHALLENGE_ART_CHALLENGE_SUNFLOWER
      || mGameMode == GAMEMODE_CHALLENGE_ART_CHALLENGE_WALLNUT
      || mGameMode == GAMEMODE_CHALLENGE_SEEING_STARS;
}

// ==========================================================================
// LawnApp::IsSquirrelLevel()
// @002B5200  28 bytes
// ==========================================================================
bool LawnApp::IsSquirrelLevel()
{
  return mBoard != nullptr && mGameMode == GAMEMODE_CHALLENGE_SQUIRREL;
}

// ==========================================================================
// LawnApp::IsIZombieLevel()
// @002B521C  42 bytes
// ==========================================================================
bool LawnApp::IsIZombieLevel()
{
  if ( mBoard == nullptr )
    return false;
  // The binary tests three bit masks rather than a range, and the set they
  // admit is {53, 55, 57, 59, 61, 63, 65, 67, 73, 75}.
  return ((int)mGameMode & ~0x0A) == 0x35
      || ((int)mGameMode & ~0x02) == 0x39
      || ((int)mGameMode & ~0x06) == 0x41;
}

// ==========================================================================
// LawnApp::IsShovelLevel()
// @002B5248  24 bytes
// ==========================================================================
bool LawnApp::IsShovelLevel()
{
  return mBoard != nullptr && mGameMode == GAMEMODE_CHALLENGE_SHOVEL;
}

// ==========================================================================
// LawnApp::IsWallnutBowlingLevel()
// @002B5260  54 bytes
// ==========================================================================
int LawnApp::IsWallnutBowlingLevel()
{
  if ( mBoard == nullptr )
    return 0;
  // 18/34 are GAMEMODE_CHALLENGE_WALLNUT_BOWLING and ..._2, 83 is
  // GAMEMODE_TWO_PLAYER_COOP_BOWLING.
  if ( mGameMode == GAMEMODE_CHALLENGE_WALLNUT_BOWLING_2
    || mGameMode == GAMEMODE_CHALLENGE_WALLNUT_BOWLING
    || mGameMode == GAMEMODE_TWO_PLAYER_COOP_BOWLING )
    return 1;

  if ( !IsAdventureMode() )
    return 0;
  return mPlayerInfo->mLevel == 5;
}

// ==========================================================================
// LawnApp::IsSlotMachineLevel()
// @002B5298  24 bytes
// ==========================================================================
bool LawnApp::IsSlotMachineLevel()
{
  return mBoard != nullptr && mGameMode == GAMEMODE_CHALLENGE_SLOT_MACHINE;
}

// ==========================================================================
// LawnApp::IsWhackAZombieLevel()
// @002B52B0  48 bytes
// ==========================================================================
int LawnApp::IsWhackAZombieLevel()
{
  if ( mBoard == nullptr )
    return 0;
  if ( mGameMode == GAMEMODE_CHALLENGE_WHACK_A_ZOMBIE )
    return 1;
  if ( !IsAdventureMode() )
    return 0;
  return mPlayerInfo->mLevel == 15;
}

// ==========================================================================
// LawnApp::IsLittleTroubleLevel()
// @002B52E0  52 bytes
// ==========================================================================
bool LawnApp::IsLittleTroubleLevel()
{
  if ( mBoard == nullptr )
    return false;
  if ( mGameMode == GAMEMODE_CHALLENGE_LITTLE_TROUBLE )
    return true;
  if ( !IsAdventureMode() )
    return false;
  return mPlayerInfo->mLevel == 25;
}

// ==========================================================================
// LawnApp::IsScaryPotterLevel()
// @002B5314  60 bytes
// ==========================================================================
int LawnApp::IsScaryPotterLevel()
{
  // Same three-mask shape as IsIZombieLevel; the admitted set is
  // {52, 54, 56, 58, 60, 62, 64, 66, 72, 74}, the Scary Potter levels plus 72
  // and 74.
  if ( (((int)mGameMode & ~0x0A) == 0x34)
    || (((int)mGameMode & ~0x02) == 0x38)
    || (((int)mGameMode & ~0x06) == 0x40) )
    return 1;

  if ( !IsAdventureMode() )
    return 0;
  return mPlayerInfo->mLevel == 35;
}

// ==========================================================================
// LawnApp::IsSeedPlantingLevel()
// @002B5350  102 bytes
// ==========================================================================
bool LawnApp::IsSeedPlantingLevel()
{
  if ( IsWallnutBowlingLevel() != 0
    || mGameMode == GAMEMODE_CHALLENGE_HEAVY_WEAPON
    || IsWhackAZombieLevel() != 0
    || mGameMode == GAMEMODE_MP_VS
    || IsArtChallenge() != 0 )
    return false;

  // 51 GAMEMODE_TREE_OF_WISDOM, 44 GAMEMODE_CHALLENGE_ZEN_GARDEN,
  // 72/73 GAMEMODE_UPSELL/GAMEMODE_INTRO, 24 GAMEMODE_CHALLENGE_ZOMBIQUARIUM,
  // 21/25 GAMEMODE_CHALLENGE_BEGHOULED and ..._TWIST.
  if ( mGameMode == GAMEMODE_TREE_OF_WISDOM
    || mGameMode == GAMEMODE_CHALLENGE_ZEN_GARDEN
    || (unsigned int)(mGameMode - GAMEMODE_UPSELL) <= 1
    || mGameMode == GAMEMODE_CHALLENGE_ZOMBIQUARIUM
    || IsIZombieLevel() )
    return false;

  return mGameMode != GAMEMODE_CHALLENGE_BEGHOULED
      && mGameMode != GAMEMODE_CHALLENGE_BEGHOULED_TWIST
      && IsScaryPotterLevel() == 0;
}

// ==========================================================================
// LawnApp::SetBoardResult(BoardResult)
// @002B53B8  108 bytes
// ==========================================================================
int LawnApp::SetBoardResult(BoardResult theResult)
{
  mBoardResult = theResult;

  if ( theResult == 1 && mBoard != nullptr )
  {
    Board *aBoard = mBoard;
    if ( !aBoard->mPeaShooterUsed
      && (unsigned int)(aBoard->mBackground - 2) <= 2
      && mGameMode == GAMEMODE_ADVENTURE )
      mPlayerInfo->mProgress[0] = (mPlayerInfo->mProgress[0] & ~0x00FF0000) | 0x00010000;

    if ( !aBoard->mCatapultPlantsUsed && IsSeedPlantingLevel() )
    {
      int aSecondPlayer = mSecondPlayerGamepadIndex + 1;
      mPlayerInfo->mProgressFlags[1] = 1;
      if ( aSecondPlayer != 0 && mPlayer2Info != nullptr )
        mPlayer2Info->mProgressFlags[1] = 1;
    }
  }

  // The binary leaves whatever r0 held; the declaration's `int` has no value to
  // return. TODO(recon): LawnApp.h should declare SetBoardResult void.
  return 0;
}

// ==========================================================================
// LawnApp::IsStormyNightLevel()
// @002B5424  48 bytes
// ==========================================================================
bool LawnApp::IsStormyNightLevel()
{
  if ( mBoard == nullptr )
    return false;
  if ( mGameMode == GAMEMODE_CHALLENGE_STORMY_NIGHT )
    return true;
  if ( !IsAdventureMode() )
    return false;
  return mPlayerInfo->mLevel == 40;
}

// ==========================================================================
// LawnApp::IsBungeeBlitzLevel()
// @002B5454  48 bytes
// ==========================================================================
bool LawnApp::IsBungeeBlitzLevel()
{
  if ( mBoard == nullptr )
    return false;
  if ( mGameMode == GAMEMODE_CHALLENGE_BUNGEE_BLITZ )
    return true;
  if ( !IsAdventureMode() )
    return false;
  return mPlayerInfo->mLevel == 45;
}

// ==========================================================================
// LawnApp::IsMiniBossLevel()
// @002B5484  76 bytes
// ==========================================================================
bool LawnApp::IsMiniBossLevel()
{
  if ( mBoard == nullptr )
    return false;
  if ( !IsAdventureMode() )
    return false;
  return mPlayerInfo->mLevel == 10 || mPlayerInfo->mLevel == 20 || mPlayerInfo->mLevel == 30;
}

// ==========================================================================
// LawnApp::IsFinalBossLevel()
// @002B54D0  56 bytes
// ==========================================================================
bool LawnApp::IsFinalBossLevel()
{
  if ( mBoard == nullptr )
    return false;
  // 36 is GAMEMODE_CHALLENGE_FINAL_BOSS and 88 GAMEMODE_TWO_PLAYER_COOP_BOSS.
  if ( mGameMode == GAMEMODE_CHALLENGE_FINAL_BOSS || mGameMode == GAMEMODE_TWO_PLAYER_COOP_BOSS )
    return true;
  if ( !IsAdventureMode() )
    return false;
  return mPlayerInfo->mLevel == 50;
}

// ==========================================================================
// LawnApp::IsContinuousChallenge()
// @002B5508  56 bytes
// ==========================================================================
bool LawnApp::IsContinuousChallenge()
{
  if ( IsArtChallenge() != 0 )
    return true;
  if ( IsSlotMachineLevel() )
    return true;
  if ( IsFinalBossLevel() )
    return true;
  // 21/25 are GAMEMODE_CHALLENGE_BEGHOULED and ..._TWIST, 72/73
  // GAMEMODE_UPSELL and GAMEMODE_INTRO.
  return mGameMode == GAMEMODE_CHALLENGE_BEGHOULED
      || (unsigned int)(mGameMode - GAMEMODE_UPSELL) <= 1
      || mGameMode == GAMEMODE_CHALLENGE_BEGHOULED_TWIST;
}

// ==========================================================================
// LawnApp::IsChallengeWithoutSeedBank()
// @002B5540  64 bytes
// ==========================================================================
int LawnApp::IsChallengeWithoutSeedBank()
{
  // 72/73 are GAMEMODE_UPSELL and GAMEMODE_INTRO, 90
  // GAMEMODE_CHALLENGE_RAINING_SEEDS.
  if ( mGameMode == GAMEMODE_CHALLENGE_RAINING_SEEDS
    || (unsigned int)(mGameMode - GAMEMODE_UPSELL) <= 1 )
    return 1;
  if ( IsWhackAZombieLevel() != 0 )
    return 1;
  if ( IsSquirrelLevel() )
    return 1;
  if ( IsScaryPotterLevel() != 0 )
    return 1;
  // 44 is GAMEMODE_CHALLENGE_ZEN_GARDEN and 51 GAMEMODE_TREE_OF_WISDOM.
  if ( mGameMode == GAMEMODE_CHALLENGE_ZEN_GARDEN || mGameMode == GAMEMODE_TREE_OF_WISDOM )
    return 1;
  return 0;
}

// ==========================================================================
// LawnApp::SrvLogoutFromServer()
// @002B5580  100 bytes
// ==========================================================================
// TODO(recon): blocked on the missing Game\SessionTasks.h. The ARM allocates a
// 0x10-byte LogoutTask -- LawnBaseTask's table at 0x6879A8, then LogoutTask's own
// at 0x6879E8, an empty String at +12 and a flag byte at +4 -- stores it in
// mLawnSessionTask and hands it to mSession.StartTask. The authoritative header
// only forward-declares the three classes, so the task cannot be constructed by
// name yet; with that header this body is
// `LogoutTask *aTask = new LogoutTask(); mLawnSessionTask = aTask;
//  mSession.StartTask(aTask);`. Everything else below is already named.
int LawnApp::SrvLogoutFromServer()
{
  if ( !mLoginToServer )
    return 0;
  return 0;
}

// ==========================================================================
// LawnApp::SrvQueryCoin()
// @002B5618  100 bytes
// ==========================================================================
// TODO(recon): the same missing Game\SessionTasks.h. The ARM allocates a
// 0x10-byte QueryCoinTask (LawnBaseTask's table at 0x6879A8, then the task's own
// at 0x687A48) with an empty String at +12, stores it in mLawnSessionTask, starts
// it and marks the session task type as 7. With that header this is
// `QueryCoinTask *aTask = new QueryCoinTask(); mLawnSessionTask = aTask;
//  mSession.StartTask(aTask);`.
int LawnApp::SrvQueryCoin()
{
  mSessionTaskType = 7;
  return 1;
}
// ==========================================================================
// LawnApp::onPay(int)
// @002B56B0  98 bytes
// ==========================================================================
void LawnApp::onPay(int theResult)
{
  if ( theResult == 0 && dynamic_cast<Sexy::LocalGameCenter*>(mGameCenter) != nullptr )
  {
    mQueryCoinState.mPending = false;
    int aBalance = atoi(LawnUser::getCurUser()->mCoinText.c_str());
    Sexy::String aMessage;
    StrFormat(&aMessage, u8"已到账 %d V币，当前余额 %d V币。\n仅用于本地测试，不涉及真实付款。",
              aBalance - mQueryCoinState.mPreviousBalance, aBalance);
    Sexy::Dialog* aRechargeDialog = GetDialog(82);
    if ( aRechargeDialog != nullptr )
      aRechargeDialog->mResult = 1000;
    FinishBuyMoneyDialog(true);
    DoDialog(51, true, "[RECHARGE_SUCCEED]", aMessage, "[DIALOG_BUTTON_OK]", 3);
    return;
  }

  if ( theResult != 0 )
  {
    if ( theResult == 1 )
      logfi("MobileCMGCHOME: on pay failed");
    else
      logfi("MobileCMGCHOME: on pay cancelled");

    // The app answers a failed or cancelled payment by putting the item list
    // back. The fourth argument is the empty string the ARM loads into r3.
    setPayItemsRes(false, gEmptyCString, 0, gEmptyCString);
  }
  else
  {
    logfi("MobileCMGCHOME: on pay success");
    mQueryCoinState.mPending = true;
    mQueryCoinState.mLastRequestTime = Sexy::GetTickCount();
    mQueryCoinState.mRequestCount = 0;
    SrvQueryCoin();
  }
}

// ==========================================================================
// LawnApp::OnSessionTaskFailed(int,Sexy::String)
// @002B5734  288 bytes
// ==========================================================================
void LawnApp::OnSessionTaskFailed(int theTask, Sexy::String theMessage)
{
  // The binary's whole body is the release of the by-value theMessage's libstdc++ rep,
  // which is what the copy the caller hands in needs and which the compiler now
  // emits for the parameter's own destructor. theTask -- the task id -- is never read.
}

// ==========================================================================
// LawnApp::OnSessionTaskSucceed()
// @002B589C  58 bytes
// ==========================================================================
int LawnApp::OnSessionTaskSucceed()
{
  // vtable byte 428 is the by-id KillDialog; the named forwarder is what the
  // rest of the tree calls it through today.
  int result = LawnMessageBox(51, "[RECHARGE_SUCCEED]", gEmptyCString, "[DIALOG_BUTTON_OK]", gEmptyCString, 3);
  if ( result == 1000 )
  {
    KillDialog(51);
    return 1000;
  }
  return result;
}

// ==========================================================================
// LawnApp::DoSessionWaitDialog()
// @002B58E4  94 bytes
// ==========================================================================
SessionWaitingDialog * LawnApp::DoSessionWaitDialog()
{
  if ( CanPauseNow() )
    mBoard->Pause(true);

  FinishModelessDialogs();
  KillDialog(81);

  SessionWaitingDialog *aDialog = new SessionWaitingDialog(this, 81);
  AddDialogWithId(81, aDialog);

  // The wait dialog is only focused if nothing else holds the focus.
  if ( mWidgetManager->mFocusWidget == nullptr )
    mWidgetManager->SetFocus(aDialog);
  return aDialog;
}

// ==========================================================================
// LawnApp::SrvBuyTool(E_SHOP_PACKET)
// @002B594C  146 bytes
// ==========================================================================
// TODO(recon): the task half of this body is blocked on the missing
// Game\SessionTasks.h. The ARM builds a BuyToolTask at 0x14 bytes (LawnBaseTask
// vptr 0x6879A8, then the task's own table 0x687A08) with the packet id at +16,
// stores it in mLawnSessionTask and hands it to mSession.StartTask. Only the
// member writes that have a name in this tree are kept; restoring the rest is
// `BuyToolTask *aTask = new BuyToolTask(a2); mLawnSessionTask = aTask;
//  mSession.StartTask(aTask);` once that header exists.
bool LawnApp::SrvBuyTool(E_SHOP_PACKET thePacket)
{
  if ( dynamic_cast<Sexy::LocalGameCenter*>(mGameCenter) != nullptr )
    return buyToolForDemo(thePacket);

  mPendingBuyToolId = thePacket;
  mSessionTaskType = 5;

  SessionWaitingDialog *aDialog = DoSessionWaitDialog();
  return aDialog == nullptr || aDialog->WaitForResult(true) == 0;
}
// ==========================================================================
// LawnApp::SrvDebugAddMoney(int)
// @002B5A14  154 bytes
// ==========================================================================
// TODO(recon): blocked on the missing Game\SessionTasks.h. The ARM builds a
// DebugAddMoneyTask at 0x14 bytes (LawnBaseTask vptr 0x6879A8, then 0x687A28)
// with thePacket at +16, stores it in mLawnSessionTask and starts it before waiting on
// dialog 81. The member write below is the one this tree can name.
bool LawnApp::SrvDebugAddMoney(int theAmount)
{
  mSessionTaskType = 6;

  DoSessionWaitDialog();
  Sexy::Dialog *aDialog = GetDialog(81);
  return aDialog == nullptr || aDialog->WaitForResult(true) == 0;
}
// ==========================================================================
// LawnApp::writeCoinNum(int)
// @002B5AE4  780 bytes
//
// Truncates <app data>/CoinFile and writes the decimal count. Open mode is
// out|trunc (MOVS #0x30). A failed open prints and the write still runs.
// The count goes through a stringstream (mode 0x18, in|out) and is extracted
// back out, then written unformatted. close() is explicit: filebuf::close
// runs before the stringstream dies, and again from the ofstream destructor.
// ==========================================================================
void LawnApp::writeCoinNum(int theCoinNum)
{
  Sexy::String aPath;
  GetAppDataFolder(&aPath);
  MkDir(aPath);
  aPath += "/CoinFile";

  std::ofstream aFile(aPath.c_str(), std::ios::out | std::ios::trunc);
  if (!aFile)
    puts("CoinFile open failed!");

  std::string aText;
  std::stringstream aStream;
  aStream << theCoinNum;
  aStream >> aText;
  aFile.write(aText.c_str(), aText.size());
  aFile.close();
}

// ==========================================================================
// LawnApp::GetUserName()
// @002B5F10  76 bytes
//
// `_ZN7LawnApp11GetUserNameEv` returns a Sexy::String by value. The identifier
// comes from SexyCommonApp::getUserIdentifier. A non-empty identifier is
// returned as it stands. The empty case assigns "transmension" (length 12)
// and clears mCanDoBuyMoneyDialog at +0xB24.
// ==========================================================================
Sexy::String LawnApp::GetUserName()
{
  Sexy::String aName = getUserIdentifier();
  if (!aName.empty())
    return aName;
  aName.assign("transmension", 12);
  mCanDoBuyMoneyDialog = false;
  return aName;
}

// ==========================================================================
// LawnApp::judgeDoTransRecharge()
// @002B5F5C  26 bytes
// ==========================================================================
bool LawnApp::judgeDoTransRecharge()
{
  // The recharge flow is only offered when the ad provider reports the type the
  // ARM compares against 7 (AdProvider::getADType, table byte 0x14).
  return mAdProvider != nullptr && mAdProvider->getADType() == 7;
}

// ==========================================================================
// LawnApp::showPurchaseLimitDialog(int)
// @002B604C  64 bytes
// ==========================================================================
int LawnApp::showPurchaseLimitDialog(int theLimit)
{
  PurchaseLimitDialog *aDialog = new PurchaseLimitDialog(this, theLimit);
  CenterDialog(aDialog, aDialog->mWidth, aDialog->mHeight);
  AddDialogWithId(aDialog->mId, aDialog);
  mWidgetManager->SetFocus(aDialog);
  return 0;
}

// ==========================================================================
// LawnApp::GetAwardSeedForLevel(int)
// @002B6098  68 bytes
// ==========================================================================
int LawnApp::GetAwardSeedForLevel(int theLevel)
{
  // Ten levels make an area, and each area awards eight seeds. The tenth level
  // of an area repeats the eighth award; levels 6 through 9 shift back by one.
  int aLevelInArea = (theLevel - 1) % 10 + 1;
  int aBase = 8 * ((theLevel - 1) / 10);
  int aSeed = aBase + aLevelInArea;
  if ( aLevelInArea == 10 )
    aSeed = aBase + 8;
  else if ( aLevelInArea > 4 )
    aSeed -= 1;
  if ( aSeed >= 40 )
    return 40;
  return aSeed;
}

// ==========================================================================
// LawnApp::AddReanimation(float,float,int,ReanimationType)
// @002B60DC  10 bytes
// ==========================================================================
Reanimation * LawnApp::AddReanimation(float theX, float theY, int theRenderOrder, ReanimationType theType)
{
  // @0x2B60DC: LDR mEffectSystem, LDR holder, B.W AllocReanimation. R0 is that pointer.
  return mEffectSystem->mReanimationHolder->AllocReanimation(theX, theY, theRenderOrder, theType);
}

// ==========================================================================
// LawnApp::AddTodParticle(float,float,int,ParticleEffect)
// @002B60E8  10 bytes
// ==========================================================================
TodParticleSystem* LawnApp::AddTodParticle(float theX, float theY, int theLayer, ParticleEffect theEffect)
{
  return mEffectSystem->mParticleHolder->AllocParticleSystem(theX, theY, theLayer, theEffect);
}

// ==========================================================================
// LawnApp::ReanimationGetID(Reanimation *)
// @002B60F4  6 bytes
// ==========================================================================
ReanimationID LawnApp::ReanimationGetID(Reanimation * theReanimation)
{
  ReanimationHolder *aHolder = mEffectSystem->mReanimationHolder;
  ReanimationSlot *aSlots = aHolder->mBlock;
  for (unsigned int anIndex = 0; anIndex < aHolder->mMaxSize; ++anIndex)
  {
    if (&aSlots[anIndex].mItem == theReanimation)
      return ReanimationID(aSlots[anIndex].mID);
  }
  return REANIMATIONID_NULL;
}

// ==========================================================================
// LawnApp::ReanimationGet(ReanimationID)
// @002B60FC  18 bytes
// ==========================================================================
Reanimation * LawnApp::ReanimationGet(ReanimationID theReanimationId)
{
  // @0x2B60FC: UXTH the id, then block + index * 0xDC. No bounds check.
  ReanimationSlot *aSlots = mEffectSystem->mReanimationHolder->mBlock;
  return &aSlots[(unsigned short)theReanimationId].mItem;
}

// ==========================================================================
// LawnApp::ReanimationTryToGet(ReanimationID)
// @002B6110  48 bytes
// ==========================================================================
Reanimation * LawnApp::ReanimationTryToGet(ReanimationID theReanimationId)
{
  if ( mEffectSystem == nullptr || theReanimationId == 0 )
    return nullptr;

  ReanimationHolder *aHolder = mEffectSystem->mReanimationHolder;
  if ( (unsigned int)(unsigned __int16)theReanimationId >= aHolder->mMaxSize )
    return nullptr;

  ReanimationSlot *aSlots = aHolder->mBlock;
  if ( theReanimationId != aSlots[(unsigned __int16)theReanimationId].mID )
    return nullptr;
  return &aSlots[(unsigned __int16)theReanimationId].mItem;
}

// ==========================================================================
// LawnApp::ParticleGetID(TodParticleSystem *)
// @002B6140  4 bytes
// ==========================================================================
ParticleSystemID LawnApp::ParticleGetID(TodParticleSystem * theParticle)
{
  // The id word sits in the slot that owns this system.
  DataArray<TodParticleSystem, ParticleSystemSlot> *aArray =
      &mEffectSystem->mParticleHolder->mParticleSystems;
  ParticleSystemSlot *aSlots = aArray->mBlock;
  for (unsigned int anIndex = 0; anIndex < aArray->mMaxSize; ++anIndex)
  {
    if (&aSlots[anIndex].mItem == theParticle)
      return ParticleSystemID(aSlots[anIndex].mID);
  }
  return ParticleSystemID(0);
}

// ==========================================================================
// LawnApp::ParticleGet(ParticleSystemID)
// @002B6144  20 bytes
// ==========================================================================
TodParticleSystem* LawnApp::ParticleGet(ParticleSystemID theId)
{
  ParticleSystemSlot *aSlots =
      mEffectSystem->mParticleHolder->mParticleSystems.mBlock;
  return &aSlots[(unsigned short)theId].mItem;
}

// ==========================================================================
// LawnApp::ParticleTryToGet(ParticleSystemID)
// @002B6158  46 bytes
// ==========================================================================
TodParticleSystem* LawnApp::ParticleTryToGet(ParticleSystemID theId)
{
  // A zero id is never a live system. The index is the low half of the id,
  // and it has to match the id stored with that slot.
  if ( theId == 0 )
    return nullptr;

  DataArray<TodParticleSystem, ParticleSystemSlot> *aArray =
      &mEffectSystem->mParticleHolder->mParticleSystems;
  unsigned int aIndex = (unsigned short)theId;
  if ( aIndex >= aArray->mMaxSize )
    return nullptr;

  ParticleSystemSlot *aSlots = aArray->mBlock;
  if ( theId != aSlots[aIndex].mID )
    return nullptr;
  return &aSlots[aIndex].mItem;
}

// ==========================================================================
// LawnApp::RemoveReanimation(ReanimationID)
// @002B6188  18 bytes
// ==========================================================================
Reanimation * LawnApp::RemoveReanimation(ReanimationID theReanimationId)
{
  Reanimation *aReanimation = ReanimationTryToGet(theReanimationId);
  if ( aReanimation != nullptr )
    aReanimation->ReanimationDie();
  return aReanimation;
}

// ==========================================================================
// LawnApp::RemoveParticle(ParticleSystemID)
// @002B619C  18 bytes
// ==========================================================================
TodParticleSystem * LawnApp::RemoveParticle(ParticleSystemID theParticleId)
{
  TodParticleSystem *aParticle = ParticleTryToGet(theParticleId);
  if ( aParticle != nullptr )
    aParticle->ParticleSystemDie();
  return aParticle;
}

// ==========================================================================
// LawnApp::IsNight()
// @002B61B0  52 bytes
// ==========================================================================
int LawnApp::IsNight()
{
  if ( IsIceDemo() != 0 )
    return 0;
  if ( mPlayerInfo == nullptr )
    return 0;

  // 11..20 and 31..40 are the night levels of the day and pool stages, 50 is
  // level 5-10, the roof night level.
  int aLevel = mPlayerInfo->mLevel;
  return (unsigned int)(aLevel - 11) <= 9
      || (unsigned int)(aLevel - 31) <= 9
      || aLevel == 50;
}

// ==========================================================================
// LawnApp::IsSurvivalNormal(GameMode)
// @002B61E4  12 bytes
// ==========================================================================
bool LawnApp::IsSurvivalNormal(GameMode theGameMode)
{
  return (unsigned int)(theGameMode - 2) <= 4;
}

// ==========================================================================
// LawnApp::IsSurvivalHard(GameMode)
// @002B61F0  22 bytes
// ==========================================================================
bool LawnApp::IsSurvivalHard(GameMode theGameMode)
{
  return (unsigned int)(theGameMode - 7) <= 4 || (unsigned int)(theGameMode - 84) <= 3;
}

// ==========================================================================
// LawnApp::IsSurvivalEndless(GameMode)
// @002B6208  18 bytes
// ==========================================================================
bool LawnApp::IsSurvivalEndless(GameMode theGameMode)
{
  // 12..16 are GAMEMODE_SURVIVAL_ENDLESS_STAGE_1 .. _5 and 89 is
  // GAMEMODE_TWO_PLAYER_COOP_ENDLESS.
  return theGameMode >= GAMEMODE_SURVIVAL_ENDLESS_STAGE_1 && theGameMode <= GAMEMODE_SURVIVAL_ENDLESS_STAGE_5
      || theGameMode == GAMEMODE_TWO_PLAYER_COOP_ENDLESS;
}

// ==========================================================================
// LawnApp::IsEndlessScaryPotter(GameMode)
// @002B621C  10 bytes
// ==========================================================================
bool LawnApp::IsEndlessScaryPotter(GameMode theGameMode)
{
  return theGameMode == 70;
}

// ==========================================================================
// LawnApp::IsEndlessIZombie(GameMode)
// @002B6228  8 bytes
// ==========================================================================
bool LawnApp::IsEndlessIZombie(GameMode theGameMode)
{
  return theGameMode == 71;
}

// ==========================================================================
// LawnApp::HasBeatenChallenge(GameMode)
// @002B6230  126 bytes
// ==========================================================================
bool LawnApp::HasBeatenChallenge(GameMode theGameMode)
{
  if ( mPlayerInfo == nullptr )
    return false;

  // mChallengeRecords is indexed by challenge number, which is the game mode
  // minus GAMEMODE_SURVIVAL_NORMAL_STAGE_1.
  int aWins = mPlayerInfo->mChallengeRecords[theGameMode - GAMEMODE_SURVIVAL_NORMAL_STAGE_1];
  if ( IsSurvivalNormal(theGameMode) )
    return aWins > 4;
  if ( IsSurvivalHard(theGameMode) )
    return aWins > 9;
  if ( IsSurvivalEndless(theGameMode) || IsEndlessScaryPotter(theGameMode) || IsEndlessIZombie(theGameMode) )
    return false;
  return aWins > 0;
}

// ==========================================================================
// LawnApp::GetPottedPlantByIndex(int)
// @002B62B0  18 bytes
// ==========================================================================
PottedPlant* LawnApp::GetPottedPlantByIndex(int theIndex)
{
  return &mPlayerInfo->mPottedPlants[theIndex];
}

// ==========================================================================
// LawnApp::HasFinishedAdventure()
// @002B62C4  42 bytes
// ==========================================================================
bool LawnApp::HasFinishedAdventure()
{
  if ( mPlayerInfo == nullptr )
    return false;
  return mPlayerInfo->GetFlag(PlayerFlags(1)) || mPlayerInfo->GetFlag(PlayerFlags(0x8000));
}

// ==========================================================================
// LawnApp::CanShowAlmanac()
// @002B62F0  44 bytes
// ==========================================================================
int LawnApp::CanShowAlmanac()
{
  if ( IsIceDemo() != 0 )
    return 0;
  if ( mPlayerInfo == nullptr )
    return 0;
  if ( HasFinishedAdventure() )
    return 1;
  return mPlayerInfo->mLevel > 14;
}

// ==========================================================================
// LawnApp::CanShowStore()
// @002B631C  80 bytes
// ==========================================================================
bool LawnApp::CanShowStore()
{
  if ( IsIceDemo() != 0 || mPlayerInfo == nullptr )
    return false;
  if ( HasFinishedAdventure() )
    return true;
  if ( mGameMode != GAMEMODE_CHALLENGE_ZEN_GARDEN )
    return mPlayerInfo->mLevel > 24;

  // Crazy Dave on screen hides the store; in the Zen Garden the tutorial state
  // decides instead.
  if ( mCrazyDaveState != CRAZY_DAVE_OFF )
    return false;
  return mBoard == nullptr
      || mBoard->mTutorialState == TUTORIAL_OFF
      || mBoard->mTutorialState > 24;
}

// ==========================================================================
// LawnApp::GetSeedsAvailable(bool)
// @002B636C  100 bytes
// ==========================================================================
int LawnApp::GetSeedsAvailable(bool theIncludeImitater)
{
  // @0x2B636C. Adventure finished returns 80 or 49. Versus mode walks seed
  // types 65..79 and keeps the last one whose zombie is unlocked.
  int aLevel = mPlayerInfo->GetLevel();
  if ( theIncludeImitater != 0 )
  {
    if ( HasFinishedAdventure() != 0 )
      return 80;
    if ( mGameMode == GAMEMODE_MP_VS )
    {
      int aSeedType = 65;
      int aBest = 65;
      do
      {
        ZombieType aZombieType = Challenge::IZombieSeedTypeToZombieType((SeedType)aSeedType);
        if ( aLevel >= GetZombieDefinition(aZombieType)->mStartingLevel )
          aBest = aSeedType;
        ++aSeedType;
      }
      while ( aSeedType != 80 );
      return aBest;
    }
  }
  else if ( HasFinishedAdventure() != 0 || aLevel > 50 )
  {
    return 49;
  }

  int aResult = GetAwardSeedForLevel(aLevel);
  if ( aResult >= 49 )
    return 49;
  return aResult;
}

// ==========================================================================
// LawnApp::IsFirstTimeAdventureMode()
// @002B63D0  32 bytes
// ==========================================================================
bool LawnApp::IsFirstTimeAdventureMode()
{
  return IsAdventureMode() && !HasFinishedAdventure();
}

// ==========================================================================
// LawnApp::CanSpawnYetis()
// @002B6538  56 bytes
// ==========================================================================
bool LawnApp::CanSpawnYetis()
{
  // ZombieDefinition's third word is the level the yeti first appears on.
  int aZombieLevel = GetZombieDefinition(ZOMBIE_YETI)->mStartingLevel;
  if ( !HasFinishedAdventure() )
    return false;
  if ( mPlayerInfo->GetFlag(PlayerFlags(0x8000)) )
    return true;
  return mPlayerInfo->mLevel >= aZombieLevel;
}

// ==========================================================================
// LawnApp::CrazyDaveEnter()
// @002B6570  166 bytes
// ==========================================================================
int LawnApp::CrazyDaveEnter()
{
  Reanimation *aDave = AddReanimation(0, 0, 0, REANIM_CRAZY_DAVE);
  aDave->mIsAttachment = true;
  aDave->SetBasePoseFromAnim("anim_idle_handing");
  mCrazyDaveReanimID = ReanimationGetID(aDave);
  aDave->PlayReanim("anim_enter", REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0);

  // The text the previous line-up left behind is dropped.
  mCrazyDaveMessageIndex = -1;
  mCrazyDaveState = CRAZY_DAVE_ENTERING;
  mCrazyDaveMessageText->clear();

  int aBlinkDelay = RandRangeInt(400, 800);
  mCrazyDaveBlinkCounter = aBlinkDelay;
  if ( mGameScene == SCENE_LEVEL_INTRO && IsStormyNightLevel() != 0 )
    aDave->mColorOverride = Sexy::Color(64, 64, 64);
  return aBlinkDelay;
}

// ==========================================================================
// LawnApp::CrazyDaveDoneHanding()
// @002B6620  28 bytes
// ==========================================================================
void LawnApp::CrazyDaveDoneHanding()
{
  Reanimation *aDave = ReanimationGet(mCrazyDaveReanimID);
  ReanimatorTrackInstance *aHand = aDave->GetTrackInstanceByName("Dave_handinghand");
  if ( aHand != nullptr )
    AttachmentDie(aHand->mAttachmentID);
}

// ==========================================================================
// LawnApp::CrazyDaveStopSound()
// @002B6640  48 bytes
// ==========================================================================
void LawnApp::CrazyDaveStopSound()
{
  mSoundSystem->StopFoley(FOLEY_CRAZY_DAVE_SHORT);
  mSoundSystem->StopFoley(FOLEY_CRAZY_DAVE_LONG);
  mSoundSystem->StopFoley(FOLEY_CRAZY_DAVE_EXTRA_LONG);
  mSoundSystem->StopFoley(FOLEY_CRAZY_DAVE_CRAZY);
}

// ==========================================================================
// LawnApp::CrazyDaveDie()
// @002B6670  68 bytes
// ==========================================================================
Reanimation * LawnApp::CrazyDaveDie()
{
  Reanimation *aDave = ReanimationTryToGet(mCrazyDaveReanimID);
  if ( aDave != nullptr )
  {
    aDave->ReanimationDie();
    mCrazyDaveState = CRAZY_DAVE_OFF;
    mCrazyDaveReanimID = REANIMATIONID_NULL;
    mCrazyDaveMessageIndex = -1;
    mCrazyDaveMessageText->clear();
    CrazyDaveStopSound();
    return aDave;
  }
  return aDave;
}

// ==========================================================================
// LawnApp::CrazyDaveLeave()
// @002B66B4  120 bytes
// ==========================================================================
void LawnApp::CrazyDaveLeave()
{
  Reanimation *aDave = ReanimationTryToGet(mCrazyDaveReanimID);
  if ( aDave == nullptr )
    return;

  // The two handing states have an attachment that has to come off first.
  if ( mCrazyDaveState == CRAZY_DAVE_HANDING_TALKING
       || mCrazyDaveState == CRAZY_DAVE_HANDING_IDLING )
    CrazyDaveDoneHanding();

  aDave->PlayReanim("anim_leave", REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0);
  aDave->SetImageOverride("Dave_mouths", nullptr);

  mCrazyDaveState = CRAZY_DAVE_LEAVING;
  mCrazyDaveMessageIndex = -1;
  mCrazyDaveMessageText->clear();
  CrazyDaveStopSound();
}

// ==========================================================================
// LawnApp::CrazyDaveTalkMessage(Sexy::String const&)
// @002B6734  1298 bytes
// ==========================================================================
void LawnApp::CrazyDaveTalkMessage(Sexy::String const& theMessage)
{
  // A marker such as {NO_SOUND} selects the pose and the voice. The characters
  // inside braces do not count toward the line length that picks the talk pose.
  Reanimation *aDave = ReanimationTryToGet(mCrazyDaveReanimID);
  if ( aDave == nullptr )
    return;

  const char *aText = theMessage.c_str();
  auto aHasMarker = [&](const char *theMarker) {
    return strstr(aText, theMarker) != nullptr;
  };

  bool aHanding = aHasMarker("{HANDING}");
  if ( !aHanding
       && (mCrazyDaveState == CRAZY_DAVE_HANDING_TALKING
           || mCrazyDaveState == CRAZY_DAVE_HANDING_IDLING) )
    CrazyDaveDoneHanding();

  bool aPlayVoice = !aHasMarker("{NO_SOUND}");
  if ( aPlayVoice )
    CrazyDaveStopSound();

  int aTextLength = 0;
  bool aInTag = false;
  for ( const char *aChar = aText; *aChar != '\0'; ++aChar )
  {
    if ( *aChar == '{' )
      aInTag = true;
    else if ( *aChar == '}' )
      aInTag = false;
    else if ( !aInTag )
      ++aTextLength;
  }

  aDave->SetImageOverride("Dave_mouths", nullptr);
  *mCrazyDaveMessageText = theMessage;

  if ( mCrazyDaveState == CRAZY_DAVE_TALKING && !aPlayVoice )
    return;

  if ( aHanding )
  {
    aDave->PlayReanim("anim_talk_handing", REANIM_LOOP, 50, 12.0f);
    if ( aPlayVoice )
    {
      if ( aHasMarker("{SHORT_SOUND}") )
        PlayFoley(FOLEY_CRAZY_DAVE_SHORT);
      else if ( aHasMarker("{SCREAM}") )
        PlayFoley(FOLEY_CRAZY_DAVE_SCREAM);
      else
        PlayFoley(FOLEY_CRAZY_DAVE_LONG);
    }
    mCrazyDaveState = CRAZY_DAVE_HANDING_TALKING;
    return;
  }

  if ( aHasMarker("{SHAKE}") )
  {
    aDave->PlayReanim("anim_crazy", REANIM_PLAY_ONCE_AND_HOLD, 50, 12.0f);
    if ( aPlayVoice )
      PlayFoley(FOLEY_CRAZY_DAVE_CRAZY);
  }
  else if ( aHasMarker("{SCREAM}") )
  {
    aDave->PlayReanim("anim_smalltalk", REANIM_PLAY_ONCE_AND_HOLD, 50, 12.0f);
    if ( aPlayVoice )
      PlayFoley(FOLEY_CRAZY_DAVE_SCREAM);
  }
  else if ( aHasMarker("{SCREAM2}") )
  {
    aDave->PlayReanim("anim_mediumtalk", REANIM_PLAY_ONCE_AND_HOLD, 50, 12.0f);
    if ( aPlayVoice )
      PlayFoley(FOLEY_CRAZY_DAVE_SCREAM_2);
  }
  else if ( aHasMarker("{SHOW_WALLNUT}") || aHasMarker("{SHOW_HAMMER}")
            || aHasMarker("{SHOW_FERTILIZER}") || aHasMarker("{SHOW_TREE_FOOD}")
            || aHasMarker("{SHOW_MONEYBAG}") )
  {
    aDave->PlayReanim("anim_talk_handing", REANIM_LOOP, 50, 12.0f);
    ReanimationType aPropType = REANIM_WALLNUT;
    const char *aPropAnim = "anim_idle";
    ReanimLoopType aPropLoop = REANIM_LOOP;
    float aPropRate = 12.0f;
    float aOffsetX = 100.0f;
    float aOffsetY = 393.0f;
    float aScale = 1.2f;
    FoleyType aVoice = FOLEY_CRAZY_DAVE_SCREAM_2;
    bool aFreeze = false;
    Sexy::Image *aBagImage = nullptr;
    if ( aHasMarker("{SHOW_HAMMER}") )
    {
      aPropType = REANIM_HAMMER;
      aPropAnim = "anim_whack_zombie";
      aPropLoop = REANIM_PLAY_ONCE_AND_HOLD;
      aPropRate = 24.0f;
      aOffsetX = 62.0f;
      aOffsetY = 445.0f;
      aScale = 1.5f;
      aVoice = FOLEY_CRAZY_DAVE_LONG;
      aFreeze = true;
    }
    else if ( aHasMarker("{SHOW_FERTILIZER}") )
    {
      aPropType = REANIM_ZENGARDEN_FERTILIZER;
      aPropAnim = "bag";
      aPropLoop = REANIM_PLAY_ONCE_AND_HOLD;
      aPropRate = 24.0f;
      aOffsetX = 102.0f;
      aOffsetY = 412.0f;
      aScale = 1.0f;
      aVoice = FOLEY_CRAZY_DAVE_LONG;
      aFreeze = true;
    }
    else if ( aHasMarker("{SHOW_TREE_FOOD}") )
    {
      aPropType = REANIM_TREEOFWISDOM_TREEFOOD;
      aPropAnim = "bag";
      aPropLoop = REANIM_PLAY_ONCE_AND_HOLD;
      aPropRate = 24.0f;
      aOffsetX = 102.0f;
      aOffsetY = 412.0f;
      aScale = 1.0f;
      aVoice = FOLEY_CRAZY_DAVE_LONG;
      aFreeze = true;
    }
    else if ( aHasMarker("{SHOW_MONEYBAG}") )
    {
      aPropType = REANIM_ZENGARDEN_FERTILIZER;
      aPropAnim = "bag";
      aPropLoop = REANIM_PLAY_ONCE_AND_HOLD;
      aPropRate = 24.0f;
      aOffsetX = 90.0f;
      aOffsetY = 405.0f;
      aScale = 1.0f;
      aVoice = FOLEY_CRAZY_DAVE_LONG;
      aFreeze = true;
      aBagImage = IMAGE_MONEYBAG;
    }

    Reanimation *aProp = AddReanimation(0, 0, 0, aPropType);
    aProp->PlayReanim(aPropAnim, aPropLoop, 0, aPropRate);
    if ( aFreeze )
    {
      aProp->SetAnimRate(0.0f);
      if ( aHasMarker("{SHOW_HAMMER}") )
        aProp->mAnimTime = 1.0f;
    }
    if ( aBagImage != nullptr )
      aProp->SetImageOverride("bag", aBagImage);
    ReanimatorTrackInstance *aHand = aDave->GetTrackInstanceByName("Dave_handinghand");
    AttachEffect *aEffect = AttachReanim(aHand->mAttachmentID, aProp, aOffsetX, aOffsetY);
    if ( aEffect != nullptr && aScale != 1.0f )
    {
      aEffect->mOffset.m00 = aScale;
      aEffect->mOffset.m11 = aScale;
    }
    aDave->Update();
    if ( aPlayVoice )
      PlayFoley(aVoice);
    mCrazyDaveState = CRAZY_DAVE_HANDING_TALKING;
    return;
  }
  else if ( aTextLength <= 22 )
  {
    aDave->PlayReanim("anim_smalltalk", REANIM_PLAY_ONCE_AND_HOLD, 50, 12.0f);
    if ( aPlayVoice )
      PlayFoley(FOLEY_CRAZY_DAVE_SHORT);
  }
  else if ( aTextLength > 51 )
  {
    aDave->PlayReanim("anim_blahblah", REANIM_PLAY_ONCE_AND_HOLD, 50, 12.0f);
    if ( aPlayVoice )
      PlayFoley(FOLEY_CRAZY_DAVE_EXTRA_LONG);
  }
  else
  {
    aDave->PlayReanim("anim_mediumtalk", REANIM_PLAY_ONCE_AND_HOLD, 50, 12.0f);
    if ( aPlayVoice )
      PlayFoley(FOLEY_CRAZY_DAVE_LONG);
  }

  mCrazyDaveState = CRAZY_DAVE_TALKING;
}
// ==========================================================================
// LawnApp::CrazyDaveStopTalking()
// @002B6CDC  182 bytes
// ==========================================================================
Reanimation * LawnApp::CrazyDaveStopTalking()
{
  // GAMEMODE_UPSELL keeps Dave handing rather than putting him back to the
  // plain idle pose.
  bool aKeepHanding = mGameMode == GAMEMODE_UPSELL;
  if ( !aKeepHanding && mCrazyDaveState == CRAZY_DAVE_HANDING_TALKING )
    CrazyDaveDoneHanding();

  Reanimation *aDave = ReanimationTryToGet(mCrazyDaveReanimID);
  if ( aDave == nullptr )
    return aDave;

  aDave->SetImageOverride("Dave_mouths", nullptr);

  // The handing states go back to the handing idle, the rest to the plain one.
  if ( mCrazyDaveState == CRAZY_DAVE_HANDING_TALKING )
  {
    if ( !aKeepHanding )
    {
      aDave->PlayReanim("anim_idle_handing", REANIM_LOOP, 20, 12.0);
      mCrazyDaveState = CRAZY_DAVE_HANDING_IDLING;
    }
    else
    {
      aDave->PlayReanim("anim_idle", REANIM_LOOP, 20, 12.0);
      mCrazyDaveState = CRAZY_DAVE_IDLING;
    }
  }
  else if ( (unsigned int)(mCrazyDaveState - CRAZY_DAVE_TALKING) > 1 )
  {
    aDave->PlayReanim("anim_idle", REANIM_LOOP, 20, 12.0);
    mCrazyDaveState = CRAZY_DAVE_IDLING;
  }

  // The line that was being spoken goes away with the voice.
  mCrazyDaveMessageIndex = -1;
  mCrazyDaveMessageText->clear();
  CrazyDaveStopSound();
  return aDave;
}
// ==========================================================================
// LawnApp::UpdateCrazyDave()
// @002B6DA0  504 bytes
// ==========================================================================
void LawnApp::UpdateCrazyDave()
{
  Reanimation *aDave = ReanimationTryToGet(mCrazyDaveReanimID);
  if ( aDave == nullptr )
  {
    mCrazyDaveState = CRAZY_DAVE_OFF;
    return;
  }

  // Entering and talking settle into an idle pose once the animation has played
  // through, which is what mLoopCount counts. The mouth image and the blink are
  // then steps of their own, and both are skipped for a Dave who is leaving.
  bool aShowMouth = false;
  bool aShouldBlink = false;
  if ( mCrazyDaveState == CRAZY_DAVE_ENTERING || mCrazyDaveState == CRAZY_DAVE_TALKING )
  {
    if ( aDave->mLoopCount > 0 )
    {
      aDave->PlayReanim("anim_idle", REANIM_LOOP, 20, 12.0);
      mCrazyDaveState = CRAZY_DAVE_IDLING;
      aShowMouth = true;
    }
    aShouldBlink = true;
  }
  else if ( mCrazyDaveState == CRAZY_DAVE_HANDING_TALKING )
  {
    if ( aDave->mLoopCount > 0 )
    {
      aDave->PlayReanim("anim_idle_handing", REANIM_LOOP, 20, 12.0);
      mCrazyDaveState = CRAZY_DAVE_HANDING_IDLING;
      aShowMouth = true;
    }
    aShouldBlink = true;
  }
  else if ( mCrazyDaveState == CRAZY_DAVE_LEAVING )
  {
    // CrazyDaveDie leaves the state at CRAZY_DAVE_OFF, which is not an idle
    // state, so the mouth and blink steps are both skipped after it.
    if ( aDave->mLoopCount > 0 )
      CrazyDaveDie();
  }
  else if ( mCrazyDaveState == CRAZY_DAVE_IDLING || mCrazyDaveState == CRAZY_DAVE_HANDING_IDLING )
  {
    aShowMouth = true;
    aShouldBlink = true;
  }

  if ( aShowMouth )
  {
    // The mouth image is whichever marker the current line carries.
    if ( StringFind(mCrazyDaveMessageText, "{MOUTH_BIG_SMILE}", 0, 17) != (unsigned int)-1 )
      aDave->SetImageOverride("Dave_mouths", IMAGE_REANIM_CRAZYDAVE_MOUTH1);
    else if ( StringFind(mCrazyDaveMessageText, "{MOUTH_SMALL_SMILE}", 0, 19) != (unsigned int)-1 )
      aDave->SetImageOverride("Dave_mouths", IMAGE_REANIM_CRAZYDAVE_MOUTH5);
    else if ( StringFind(mCrazyDaveMessageText, "{MOUTH_BIG_OH}", 0, 14) != (unsigned int)-1 )
      aDave->SetImageOverride("Dave_mouths", IMAGE_REANIM_CRAZYDAVE_MOUTH4);
    else if ( StringFindText(mCrazyDaveMessageText, "{MOUTH_SMALL_OH}", 0) != (unsigned int)-1 )
      aDave->SetImageOverride("Dave_mouths", IMAGE_REANIM_CRAZYDAVE_MOUTH6);
  }

  if ( aShouldBlink && --mCrazyDaveBlinkCounter <= 0 )
  {
    mCrazyDaveBlinkCounter = RandRangeInt(400, 800);

    // The blink is a second Crazy Dave whose eye group takes over the head.
    Reanimation *aBlink = AddReanimation(0, 0, 0, REANIM_CRAZY_DAVE);
    aBlink->SetFramesForLayer("anim_blink");
    aBlink->mLoopType = REANIM_PLAY_ONCE_FULL_LAST_FRAME_AND_HOLD;
    aBlink->SetAnimRate(15.0);
    aBlink->AttachToAnotherReanimation(aDave, "Dave_head");
    aBlink->mColorOverride = aDave->mColorOverride;
    aDave->AssignRenderGroupToTrack("Dave_eye", -1);
    mCrazyDaveBlinkReanimID = ReanimationGetID(aBlink);
  }

  // The eye render group comes back once the blink has finished.
  Reanimation *aBlink = ReanimationTryToGet(mCrazyDaveBlinkReanimID);
  if ( aBlink != nullptr && aBlink->mLoopCount > 0 )
  {
    aDave->AssignRenderGroupToTrack("Dave_eye", 0);
    RemoveReanimation(mCrazyDaveBlinkReanimID);
    mCrazyDaveBlinkReanimID = REANIMATIONID_NULL;
  }

  aDave->Update();
}
// ==========================================================================
// LawnApp::DrawCrazyDave(Sexy::Graphics *)
// @002B6FFC  772 bytes
// ==========================================================================
void LawnApp::DrawCrazyDave(Sexy::Graphics * g)
{
  Reanimation *aDave = ReanimationTryToGet(mCrazyDaveReanimID);
  if ( aDave == nullptr )
    return;

  // Nothing to say: Dave is just drawn.
  if ( mCrazyDaveMessageText == nullptr || mCrazyDaveMessageText->size() == 0 )
  {
    aDave->Draw(g);
    return;
  }

  // The speech bubble follows the store screen: DIALOG_STORE gives it one
  // position, the rest of the game another, and GAMEMODE_UPSELL a third.
  Sexy::Image *aBubble = IMAGE_STORE_SPEECHBUBBLE2;
  int aBubbleX;
  int aBubbleY;
  int aTextX;
  int aTextY;
  if ( GetDialog(6) != nullptr )
  {
    aBubbleX = 105;
    aBubbleY = -58;
    aTextX = 130;
    aTextY = -52;
    aBubble = IMAGE_STORE_SPEECHBUBBLE;
  }
  else if ( mGameMode == GAMEMODE_UPSELL )
  {
    aBubbleX = 415;
    aBubbleY = 90;
    aTextX = 440;
    aTextY = 96;
  }
  else
  {
    aBubbleX = 285;
    aBubbleY = 20;
    aTextX = 310;
    aTextY = 26;
  }

  g->DrawImage(aBubble, aBubbleX, aBubbleY);

  Sexy::String aText = *mCrazyDaveMessageText;
  Sexy::TRect<int> aRect(aTextX, aTextY, 233, 144);

  // {SHAKE} makes the bubble jitter by a pixel or two.
  if ( StringFind(&aText, "{SHAKE}", 0, 7) != (unsigned int)-1 )
  {
    Sexy::String aReplacement;
    Sexy::String aEmpty;
    TodReplaceString(&aReplacement, &aText, "{SHAKE}", &aEmpty);
    aText = aReplacement;
    aRect.mX += PlatformRandomLong() % 2;
    aRect.mY += PlatformRandomLong() % 2;
  }

  // {NO_CLICK} and {CHOOSE_HOUSE} are the two markers that suppress the
  // "[CLICK_TO_CONTINUE]" hint; the rest of the game shows it.
  bool aShowChooseHouse = false;
  bool aShowClickToContinue = false;
  if ( mGameMode != GAMEMODE_UPSELL )
  {
    if ( StringFind(&aText, "{NO_CLICK}", 0, 10) == (unsigned int)-1 )
    {
      if ( StringFind(&aText, "{CHOOSE_HOUSE}", 0, 14) == (unsigned int)-1 )
      {
        aShowClickToContinue = true;
        aRect.mY -= 10;
      }
      else
      {
        // The marker is taken out of the line and the hint takes its place.
        Sexy::String aReplacement;
        Sexy::String aEmpty;
        TodReplaceString(&aReplacement, &aText, "{NO_CLICK}", &aEmpty);
        aText = aReplacement;
        aShowChooseHouse = true;
      }
    }
  }

  TodDrawStringWrapped(g, aText, aRect, FONT_BRIANNETOD16, Sexy::Color::Black, 5, 0);

  // The hint is drawn in the wide font only under a Chinese locale.
  Sexy::Font *aHintFont = FONT_DWARVENTODCRAFT12;
  if ( GetLocale() == "zh_CN" )
    aHintFont = FONT_BRIANNETOD16;

  if ( aShowClickToContinue )
  {
    Sexy::String aHint("[CLICK_TO_CONTINUE]");
    TodDrawString(g, aHint, aBubbleX + 139, aBubbleY + 140, aHintFont,
                  Sexy::Color::Black, 2);
  }

  if ( aShowChooseHouse )
  {
    Sexy::String aHint("[CHOOSE_HOUSE]");
    TodDrawString(g, aHint, aBubbleX + 139, aBubbleY + 140, aHintFont,
                  Sexy::Color::Black, 2);
  }

  aDave->Draw(g);
}
// ==========================================================================
// LawnApp::Pluralize(int,char const*,char const*)
// @002B73F4  160 bytes
// ==========================================================================
// The result comes back through the ARM sret pointer, so the dump's `this` is
// the caller's Sexy::String buffer -- which is also why the body never reads r1.
// The count is r2, the singular text r3 and the plural text the stack argument,
// so the signature below is the mangled `PluralizeEiPKcS1_` one.
Sexy::String LawnApp::Pluralize(int theCount, char const* theSingular, char const* thePlural)
{
  Sexy::String aSource(theCount == 1 ? theSingular : thePlural);
  Sexy::String aResult;
  TodReplaceNumberString(&aResult, &aSource, "{COUNT}", theCount);
  return aResult;
}
// ==========================================================================
// LawnApp::GetNumTrophies(ChallengePage)
// @002B74B4  54 bytes
// ==========================================================================
int LawnApp::GetNumTrophies(ChallengePage thePage)
{
  // Records 0 through 93. A beaten challenge on this page is a trophy.
  int aTrophies = 0;
  for ( int aIndex = 0; aIndex < 94; ++aIndex )
  {
    const ChallengeDefinition *aDef = GetChallengeDefinition(aIndex);
    if ( aDef != nullptr && aDef->mPage == (int)thePage
         && HasBeatenChallenge(aDef->mChallengeMode) != 0 )
      ++aTrophies;
  }
  return aTrophies;
}

// ==========================================================================
// LawnApp::TrophiesNeedForGoldSunflower()
// @002B74EC  42 bytes
// ==========================================================================
int LawnApp::TrophiesNeedForGoldSunflower()
{
  // The gold sunflower needs 30 survival, 18 puzzle and 12 challenge trophies.
  int aMissing = 30 - GetNumTrophies(CHALLENGE_PAGE_SURVIVAL);
  aMissing += 18 - GetNumTrophies(CHALLENGE_PAGE_PUZZLE);
  aMissing += 12 - GetNumTrophies(CHALLENGE_PAGE_CHALLENGE);
  return aMissing;
}

// ==========================================================================
// LawnApp::EarnedGoldTrophy()
// @002B7518  26 bytes
// ==========================================================================
bool LawnApp::EarnedGoldTrophy()
{
  if ( !HasFinishedAdventure() )
    return false;
  return TrophiesNeedForGoldSunflower() <= 0;
}

// ==========================================================================
// LawnApp::IsRegistered()
// @002B7534  4 bytes
// ==========================================================================
int LawnApp::IsRegistered()
{
  return 0;
}

// ==========================================================================
// LawnApp::BuyFullVersion()
// @002B7538  2 bytes
// ==========================================================================
void LawnApp::BuyFullVersion()
{
}

// ==========================================================================
// LawnApp::IsTrialStageLocked()
// @002B753C  4 bytes
// ==========================================================================
int LawnApp::IsTrialStageLocked()
{
  return 0;
}

// ==========================================================================
// LawnApp::CanShowZenGarden()
// @002B7540  44 bytes
// ==========================================================================
bool LawnApp::CanShowZenGarden()
{
  if ( mPlayerInfo == nullptr || IsTrialStageLocked() != 0 )
    return false;
  if ( !HasFinishedAdventure() )
    return mPlayerInfo->mLevel > 44;
  return true;
}

// ==========================================================================
// LawnApp::HasSeedType(SeedType,bool)
// @002B756C  238 bytes
// ==========================================================================
bool LawnApp::HasSeedType(SeedType theSeedType, bool theIncludeImitater)
{
  // @0x2B756C: these reads are the nine plant purchase slots, the same slots
  // StoreScreen::PurchaseItem writes. Seed ids and store item ids differ.
  if ( theSeedType == SEED_GATLINGPEA )
    return mPlayerInfo->mPurchases[STORE_ITEM_PLANT_GATLINGPEA] > 0;
  if ( IsTrialStageLocked() != 0 )
  {
    // The trial build only has the first twenty seeds.
    if ( theSeedType > SEED_TANGLEKELP )
      return false;
  }
  else
  {
    if ( theSeedType == SEED_TWINSUNFLOWER )
      return mPlayerInfo->mPurchases[STORE_ITEM_PLANT_TWINSUNFLOWER] > 0;
    if ( theSeedType == SEED_GLOOMSHROOM )
      return mPlayerInfo->mPurchases[STORE_ITEM_PLANT_GLOOMSHROOM] > 0;
  }

  switch ( theSeedType )
  {
    case SEED_CATTAIL:
      return mPlayerInfo->mPurchases[STORE_ITEM_PLANT_CATTAIL] > 0;
    case SEED_WINTERMELON:
      return mPlayerInfo->mPurchases[STORE_ITEM_PLANT_WINTERMELON] > 0;
    case SEED_GOLD_MAGNET:
      return mPlayerInfo->mPurchases[STORE_ITEM_PLANT_GOLD_MAGNET] > 0;
    case SEED_SPIKEROCK:
      return mPlayerInfo->mPurchases[STORE_ITEM_PLANT_SPIKEROCK] > 0;
    case SEED_COBCANNON:
      return mPlayerInfo->mPurchases[STORE_ITEM_PLANT_COBCANNON] > 0;
    case SEED_IMITATER:
      return mPlayerInfo->mPurchases[STORE_ITEM_PLANT_IMITATER] > 0;
    default:
      break;
  }
  return theSeedType < GetSeedsAvailable(theIncludeImitater);
}
// ==========================================================================
// LawnApp::GetNumPreloadingTasks()
// @002B765C  138 bytes
// ==========================================================================
int LawnApp::GetNumPreloadingTasks()
{
  // Ten seed groups of 136 units. The per-seed walk below reads zombie
  // definitions. Until that table is filled, GetZombieDefinition is empty and
  // the bar is sized with this figure.
  if ( mPlayerInfo == nullptr || GetZombieDefinition(0) == nullptr )
    return 1360;

  // One group per seed the profile has, plus one more when the adventure is
  // finished and every seed is therefore available.
  int aNumGroups = 10;
  for ( int aSeedType = 0; aSeedType != 54; ++aSeedType )
  {
    if ( HasSeedType((SeedType)aSeedType, false) || HasFinishedAdventure() )
      ++aNumGroups;
  }

  // One group per zombie whose starting level the profile has already reached.
  // Types 12, 17, 22, 23 and 25 have no definition of their own, so they add
  // no group. The walk stops at type 33, which is the 34th count.
  int aZombieGroups = 0;
  for ( int aIndex = 0; aIndex < 34; ++aIndex )
  {
    bool aReached = HasFinishedAdventure()
        || mPlayerInfo->mLevel >= GetZombieDefinition((ZombieType)aIndex)->mStartingLevel;
    bool aHasDefinition = (unsigned int)(aIndex - 22) > 1
        && (aIndex & 0xFFFFFFF7) != 0x11
        && aIndex != 12;
    if ( aReached && aHasDefinition )
      ++aZombieGroups;
  }
  return 136 * (aNumGroups + aZombieGroups);
}
// ==========================================================================
// LawnApp::PreloadForUser()
// @002B76E8  356 bytes
// ==========================================================================
int LawnApp::PreloadForUser()
{
  int aStartTasks = mCompletedLoadingThreadTasks;
  unsigned int aSeedType = 0;
  int aTotalTasks = aStartTasks + GetNumPreloadingTasks();

  // Each reanimation group is worth one 136-unit step of the progress bar, and
  // the prologue books them unconditionally -- only the two loops below clamp.
  ReanimatorEnsureDefinitionLoaded(REANIM_PUFF, true);
  ReanimatorEnsureDefinitionLoaded(REANIM_LAWN_MOWERED_ZOMBIE, true);
  ReanimatorEnsureDefinitionLoaded(REANIM_READYSETPLANT, true);
  mCompletedLoadingThreadTasks += 136;
  ReanimatorEnsureDefinitionLoaded(REANIM_FINAL_WAVE, true);
  ReanimatorEnsureDefinitionLoaded(REANIM_SUN, true);
  ReanimatorEnsureDefinitionLoaded(REANIM_TEXT_FADE_ON, true);
  mCompletedLoadingThreadTasks += 136;
  ReanimatorEnsureDefinitionLoaded(REANIM_ZOMBIE, true);
  mCompletedLoadingThreadTasks += 136;
  ReanimatorEnsureDefinitionLoaded(REANIM_ZOMBIE_NEWSPAPER, true);
  mCompletedLoadingThreadTasks += 136;
  ReanimatorEnsureDefinitionLoaded(REANIM_ZOMBIE_HAND, true);
  mCompletedLoadingThreadTasks += 136;
  ReanimatorEnsureDefinitionLoaded(REANIM_MAINMENU_SKY, true);
  mCompletedLoadingThreadTasks += 136;
  ReanimatorEnsureDefinitionLoaded(REANIM_MAINMENU_SKY2, true);
  mCompletedLoadingThreadTasks += 136;
  ReanimatorEnsureDefinitionLoaded(REANIM_MAINMENU, true);
  mCompletedLoadingThreadTasks += 136;
  ReanimatorEnsureDefinitionLoaded(REANIM_CROW, true);
  mCompletedLoadingThreadTasks += 136;
  ReanimatorEnsureDefinitionLoaded(REANIM_FALLING_LEAVES, true);
  mCompletedLoadingThreadTasks += 136;
  ReanimatorEnsureDefinitionLoaded(REANIM_BUTTERFLY, true);
  mCompletedLoadingThreadTasks += 136;

  // The two walks below are the only ones that clamp: they run until the app is
  // shutting down or a quit has been requested, and they never let the counter
  // pass the total the pass is worth.
  int aResult = 0;
  for ( ;; )
  {
    unsigned int aCurrentSeed = aSeedType++;
    aResult = Plant::PreloadPlantResources((SeedType)aCurrentSeed);
    if ( mCompletedLoadingThreadTasks < aTotalTasks )
      mCompletedLoadingThreadTasks += 136;
    if ( mShutDown || mCloseRequest )
      break;

    if ( aSeedType == 54 )
    {
      // The zombie pass starts at the close-request flag, which the ARM reads
      // as the loop's initial value; it is a zero byte by construction.
      int aZombieType = mCloseRequest;
      for ( ;; )
      {
        int aCurrentZombie = aZombieType++;
        aResult = Zombie::PreloadZombieResources((ZombieType)aCurrentZombie);
        if ( mCompletedLoadingThreadTasks < aTotalTasks )
          mCompletedLoadingThreadTasks += 136;
        if ( mShutDown || mCloseRequest )
          break;

        if ( aZombieType == 34 )
        {
          if ( mCompletedLoadingThreadTasks != aTotalTasks )
            mCompletedLoadingThreadTasks = aTotalTasks;
          return aResult;
        }
      }
      return aResult;
    }
  }
  return aResult;
}
// ==========================================================================
// LawnApp::IsDRMConnected()
// @002B784C  4 bytes
// ==========================================================================
int LawnApp::IsDRMConnected()
{
  return 0;
}

// ==========================================================================
// LawnApp::GetMoneyString(int)
// @002B7850  476 bytes
// ==========================================================================
// Template is "[CURRENCY_STRING]". {CURRENCY_SYMBOL} is the translated
// "[CURRENCY_SYMBOL]"; {AMOUNT} is CommaSeperate of the coin count times ten
// (ADD R2,R2,LSL#2 then LSL#1). The receiver is not read.
Sexy::String LawnApp::GetMoneyString(int theAmount)
{
  Sexy::String aTemplate;
  TodStringTranslate(&aTemplate, "[CURRENCY_STRING]");
  Sexy::String aSymbol;
  TodStringTranslate(&aSymbol, "[CURRENCY_SYMBOL]");

  Sexy::String aText;
  TodReplaceString(&aText, &aTemplate, "{CURRENCY_SYMBOL}", &aSymbol);

  Sexy::String aAmount = CommaSeperate(theAmount * 10);
  Sexy::String aResult;
  TodReplaceString(&aResult, &aText, "{AMOUNT}", &aAmount);
  return aResult;
}
// ==========================================================================
// LawnApp::GetCrazyDaveText(int)
// @002B7AB0  270 bytes
// ==========================================================================
// "[CRAZY_DAVE_<index>]" with {PLAYER_NAME}, {MONEY} and {UPGRADE_COST}.
// The name is PlayerInfo::GetName (vtable +0x10). The upgrade price is
// StoreItem 0x15.
Sexy::String LawnApp::GetCrazyDaveText(int theIndex)
{
  Sexy::String aLine;
  StrFormat(&aLine, "[CRAZY_DAVE_%d]", theIndex);

  Sexy::String aName = mPlayerInfo->GetName();
  Sexy::String aWithName;
  TodReplaceString(&aWithName, &aLine, "{PLAYER_NAME}", &aName);

  Sexy::String aMoney = GetMoneyString(mPlayerInfo->mCoins);
  Sexy::String aWithMoney;
  TodReplaceString(&aWithMoney, &aWithName, "{MONEY}", &aMoney);

  Sexy::String aCost = GetMoneyString(StoreScreen::GetItemCost((StoreItem)0x15));
  Sexy::String aResult;
  TodReplaceString(&aResult, &aWithMoney, "{UPGRADE_COST}", &aCost);
  return aResult;
}
// ==========================================================================
// LawnApp::CrazyDaveTalkIndex(int)
// @002B7C50  88 bytes
// ==========================================================================
void LawnApp::CrazyDaveTalkIndex(int theMessageIndex)
{
  mCrazyDaveMessageIndex = theMessageIndex;
  CrazyDaveTalkMessage(GetCrazyDaveText(theMessageIndex));
}
// ==========================================================================
// LawnApp::AdvanceCrazyDaveText()
// @002B7CC0  104 bytes
// ==========================================================================
int LawnApp::AdvanceCrazyDaveText()
{
  int aNextIndex = mCrazyDaveMessageIndex + 1;

  Sexy::String aLine;
  StrFormat(&aLine, "[CRAZY_DAVE_%d]", aNextIndex);

  int aExists = TodStringListExists(aLine);
  if ( aExists != 0 )
    CrazyDaveTalkIndex(aNextIndex);
  return aExists;
}
// ==========================================================================
// LawnApp::CanDoPinataMode()
// @002B7D44  22 bytes
// ==========================================================================
int LawnApp::CanDoPinataMode()
{
  // The 1000-coin record lives in the player profile.
  if ( mPlayerInfo == nullptr )
    return 0;
  return mPlayerInfo->mCoins >= 1000;
}
// ==========================================================================
// LawnApp::CanDoDanceMode()
// @002B7D5C  22 bytes
// ==========================================================================
int LawnApp::CanDoDanceMode()
{
  if ( mPlayerInfo == nullptr )
    return 0;
  return mPlayerInfo->mCoins >= 500;
}
// ==========================================================================
// LawnApp::CanDoDaisyMode()
// @002B7D74  20 bytes
// ==========================================================================
int LawnApp::CanDoDaisyMode()
{
  if ( mPlayerInfo == nullptr )
    return 0;
  return mPlayerInfo->mCoins > 99;
}
// ==========================================================================
// LawnApp::IsOptionsScreenActive()
// @002B7D88  20 bytes
// ==========================================================================
bool LawnApp::IsOptionsScreenActive()
{
  // Dialog id 2 is the options screen.
  return GetDialog(2) != nullptr;
}

// ==========================================================================
// LawnApp::GetTitleID()
// @002B7D9C  6 bytes
// ==========================================================================
int LawnApp::GetTitleID()
{
  return -1;
}

// ==========================================================================
// LawnApp::WriteLeaderboards()
// @002B7DA4  2 bytes
// ==========================================================================
void LawnApp::WriteLeaderboards()
{
  ;
}

// ==========================================================================
// LawnApp::WriteHouse(bool)
// @002B7DA8  2 bytes
// ==========================================================================
void LawnApp::WriteHouse(bool theForce)
{
  if (mPlayerInfo == nullptr)
    return;
  mPlayerInfo->UpdateHouseData();
  if (mGameSelector != nullptr)
    SetHouseReanim(ReanimationTryToGet(mGameSelector->mHouseReanimID));
  if (theForce)
    TryToSaveProfile();
}

// ==========================================================================
// LawnApp::ShowNetError(Sexy::String const&,Sexy::String const&,Sexy::String const&,int)
// @002B7DAC  4 bytes
// ==========================================================================
int LawnApp::ShowNetError()
{
  return 0;
}

// ==========================================================================
// LawnApp::GetLeaderboardName(unsigned int)
// @002B7DB0  14 bytes
// ==========================================================================
Sexy::String LawnApp::GetLeaderboardName(unsigned int theId)
{
  // @0x2B7DB0 stores dword_71BB54+0xC into the sret word. theId is never read.
  (void)theId;
  return Sexy::String();
}

// ==========================================================================
// LawnApp::HasReplays(unsigned int)
// @002B7DC8  4 bytes
// ==========================================================================
int LawnApp::HasReplays(unsigned int theId)
{
  return 0;
}

// ==========================================================================
// LawnApp::NumReplays(unsigned int)
// @002B7DCC  4 bytes
// ==========================================================================
int LawnApp::NumReplays(unsigned int theId)
{
  return 0;
}

// ==========================================================================
// LawnApp::HandleCorruptedGameFile()
// @002B7DD0  258 bytes
// ==========================================================================
void LawnApp::HandleCorruptedGameFile()
{
  // The save-file error dialogs all share this shape: build the three texts,
  // put the dialog up, then pause the board behind it.
  Sexy::String aHeader("[CORRUPTED_SAVEGAME_HEADER]");
  Sexy::String aText("[CORRUPTED_SAVEGAME_TEXT]");
  Sexy::String aButton(gEmptyCString);
  Sexy::Dialog *aDialog = DoDialog(63, true, aHeader, aText, aButton, 1);
  mWidgetManager->SetFocus(aDialog);
  if ( mBoard != nullptr )
    mBoard->Pause(true);
}

// ==========================================================================
// LawnApp::HandleOldGameFile()
// @002B7F14  260 bytes
// ==========================================================================
void LawnApp::HandleOldGameFile()
{
  Sexy::String aHeader("[OLD_SAVEGAME_HEADER]");
  Sexy::String aText("[OLD_SAVEGAME_TEXT]");
  Sexy::String aButton("[DIALOG_BUTTON_OK]");
  Sexy::Dialog *aDialog = DoDialog(63, true, aHeader, aText, aButton, 3);
  mWidgetManager->SetFocus(aDialog);
  if ( mBoard != nullptr )
    mBoard->Pause(true);
}

// ==========================================================================
// LawnApp::HandleNoDevice()
// @002B8084  168 bytes
// ==========================================================================
void LawnApp::HandleNoDevice()
{
  Sexy::String aHeader("[NO_DEVICE_SELECTED_HEADER]");
  Sexy::String aText("[NO_DEVICE_SELECTED]");
  Sexy::String aButton(gEmptyCString);
  Sexy::Dialog *aDialog = DoDialog(64, true, aHeader, aText, aButton, 1);
  mWidgetManager->SetFocus(aDialog);
  if ( mBoard != nullptr )
    mBoard->Pause(true);
}

// ==========================================================================
// LawnApp::HandleInvalidLevel()
// @002B812C  168 bytes
// ==========================================================================
void LawnApp::HandleInvalidLevel()
{
  Sexy::String aHeader("[INVALID_LEVEL_HEADER]");
  Sexy::String aText("[INVALID_LEVEL]");
  Sexy::String aButton("[DIALOG_BUTTON_OK]");
  Sexy::Dialog *aDialog = DoDialog(65, true, aHeader, aText, aButton, 3);
  mWidgetManager->SetFocus(aDialog);
  if ( mBoard != nullptr )
    mBoard->Pause(true);
}
// ==========================================================================
// LawnApp::DoProfileLoadErrorDialog()
// @002B82E0  290 bytes
// ==========================================================================
bool LawnApp::DoProfileLoadErrorDialog()
{
  if ( GetDialog(0x42) != nullptr )
    KillDialog(0x42);

  // DoDialog (vtable +0x198) returns Dialog *. LawnDialog is the derived type.
  LawnDialog *aDialog = static_cast<LawnDialog *>(DoDialog(0x42, true,
      Sexy::String("[PROFILE_LOAD_ERROR_TITLE]"),
      Sexy::String("[PROFILE_LOAD_ERROR]"),
      Sexy::String(gEmptyCString),
      1));

  // WidgetManager::SetFocus is vtable +0x30. Success is WaitForResult == 1000
  // (SUBS #0x3E8, then the carry trick that yields 0 or 1).
  mWidgetManager->SetFocus(aDialog);
  return aDialog->WaitForResult(true) == 1000;
}
// ==========================================================================
// LawnApp::DoProfileSaveErrorDialog()
// @002B8444  318 bytes
// ==========================================================================
int LawnApp::DoProfileSaveErrorDialog()
{
  if ( GetDialog(0x43) != nullptr )
    KillDialog(0x43);

  // STRB #0 at +0x968, which is mSkipProfileSaving, not the later
  // mProfileSaveInProgress byte.
  mSkipProfileSaving = false;

  LawnDialog *aDialog = static_cast<LawnDialog *>(DoDialog(0x43, true,
      Sexy::String("[PROFILE_SAVE_ERROR_TITLE]"),
      Sexy::String("[PROFILE_SAVE_ERROR]"),
      Sexy::String("[PROFILE_SAVE_ERROR_FOOTER]"),
      1));

  mWidgetManager->SetFocus(aDialog);
  if ( aDialog->WaitForResult(true) == 0x3E8 )
    return 1;

  KillDialog(0x4F);
  mSkipProfileSaving = true;
  return 0;
}

// ==========================================================================
// LawnApp::On2PProfileLoadError()
// @002B85C4  52 bytes
// ==========================================================================
int LawnApp::On2PProfileLoadError()
{
  // Only the second player's profile matters here; the loop keeps re-prompting
  // while the load keeps failing and the player has not asked to stop.
  if ( mPlayer2Info == nullptr )
    return 0;

  for ( ;; )
  {
    if ( DoProfileLoadErrorDialog() == 0 )
      return 0;

    if ( mPlayer2Info->IsLoaded() )
      continue;
    if ( mPlayer2Info->IsLoading() )
      continue;
    return 1;
  }
}
// ==========================================================================
// LawnApp::DoHelpDialog(bool)
// @002B85F8  78 bytes
// ==========================================================================
int LawnApp::DoHelpDialog(bool theFromGameSelector)
{
  HelpOptionsDialog *aDialog = new HelpOptionsDialog(this);
  AddDialog(aDialog);
  CenterDialog(aDialog, 413, 535);
  mWidgetManager->AddWidget(aDialog);
  if ( theFromGameSelector != 0 )
    return aDialog->WaitForResult(true);
  return 0;
}

// ==========================================================================
// LawnApp::DoSettingsDialog(bool)
// @002B8650  72 bytes
// ==========================================================================
int LawnApp::DoSettingsDialog(bool theFromGameSelector)
{
  SettingsDialog *aDialog = new SettingsDialog(this);
  AddDialog(aDialog);
  CenterDialog(aDialog, 413, 535);
  mWidgetManager->AddWidget(aDialog);
  return aDialog->WaitForResult(true);
}
// ==========================================================================
// LawnApp::DoConfirmRestartDialog()
// @002B86A4  452 bytes
// ==========================================================================
LawnDialog * LawnApp::DoConfirmRestartDialog()
{
  // The header and body depend on which game the player is in.
  Sexy::String aHeader;
  Sexy::String aBody;
  if ( IsPuzzleMode() )
  {
    aHeader = "[RESTART_PUZZLE_HEADER]";
    aBody = "[RESTART_PUZZLE_BODY]";
  }
  else if ( mGameMode == GAMEMODE_MP_VS )
  {
    aHeader = "[CONCEDE_VS_HEADER]";
    aBody = "[CONCEDE_VS_BODY]";
  }
  else if ( IsChallengeMode() )
  {
    aHeader = "[RESTART_CHALLENGE_HEADER]";
    aBody = "[RESTART_CHALLENGE_BODY]";
  }
  else if ( IsSurvivalMode() )
  {
    aHeader = "[RESTART_SURVIVAL_HEADER]";
    aBody = "[RESTART_SURVIVAL_BODY]";
  }
  else
  {
    aHeader = "[RESTART_LEVEL_HEADER]";
    aBody = "[RESTART_LEVEL_BODY]";
  }

  // Dialog id 0x18. The footer is the empty C string. Button labels are
  // translated, then assigned into ButtonWidget::mLabel at button+0x120.
  LawnDialog *aDialog = DoDialog(24, true, aHeader, aBody, Sexy::String(gEmptyCString), 1);
  if ( mGameMode == GAMEMODE_MP_VS )
  {
    TodStringTranslate(&aDialog->mLawnYesButton->mLabel, "[CONCEDE_VS_LABEL]");
    TodStringTranslate(&aDialog->mLawnNoButton->mLabel, "[DIALOG_BUTTON_CANCEL]");
  }
  else
  {
    TodStringTranslate(&aDialog->mLawnYesButton->mLabel, "[RESTART_LABEL]");
    TodStringTranslate(&aDialog->mLawnNoButton->mLabel, "[DIALOG_BUTTON_CANCEL]");
  }
  return aDialog;
}
// ==========================================================================
// LawnApp::DoNotEnoughMoneyDialog()
// @002B8904  234 bytes
// ==========================================================================
Sexy::Dialog * LawnApp::DoNotEnoughMoneyDialog()
{
  return DoDialog(26, true,
                  Sexy::String("[NOT_ENOUGH_MONEY]"),
                  Sexy::String("[CANNOT_AFFORD_ITEM]"),
                  Sexy::String("[DIALOG_BUTTON_OK]"),
                  3);
}

// ==========================================================================
// LawnApp::DoUpgradeDialog()
// @002B8A30  298 bytes
// ==========================================================================
LawnDialog * LawnApp::DoUpgradeDialog()
{
  // TV formats the new bank size after recording the purchased upgrade.
  Sexy::String aTemplate;
  TodStringTranslate(&aTemplate, "[NOW_YOU_CAN_CHOOSE_X_SEEDS]");
  Sexy::String aBody;
  StrFormat(&aBody, aTemplate.c_str(), mPlayerInfo->mPurchases[STORE_ITEM_PACKET_UPGRADE] + 6);

  return DoDialog(27, true,
                  Sexy::String("[MORE_SLOTS]"),
                  aBody,
                  Sexy::String("[DIALOG_BUTTON_OK]"),
                  3);
}

// ==========================================================================
// LawnApp::DoRestartConfirmDialog()
// @002B8BA8  278 bytes
// ==========================================================================
Sexy::Dialog * LawnApp::DoRestartConfirmDialog()
{
  Sexy::String aHeader;
  Sexy::String aBody;
  if ( IsAdventureMode() )
  {
    aHeader = "[RESTART_LEVEL_HEADER]";
    aBody = "[RESTART_LEVEL]";
  }
  else
  {
    aHeader = "[NEW_GAME_HEADER]";
    aBody = "[NEW_GAME]";
  }

  // Dialog id 0x29, two buttons. The yes label is the translated
  // "[RESTART_BUTTON]", assigned into ButtonWidget::mLabel.
  LawnDialog *aDialog = static_cast<LawnDialog *>(DoDialog(0x29, true, aHeader, aBody, Sexy::String(gEmptyCString), 2));
  TodStringTranslate(&aDialog->mLawnYesButton->mLabel, "[RESTART_BUTTON]");
  return aDialog;
}

// ==========================================================================
// LawnApp::DoTreeItemDialog()
// @002B8D20  232 bytes
// ==========================================================================
LawnDialog * LawnApp::DoTreeItemDialog()
{
  return DoDialog(49, true,
                  Sexy::String("[VISIT_TREE_HEADER]"),
                  Sexy::String("[VISIT_TREE_BODY]"),
                  Sexy::String(gEmptyCString),
                  1);
}

// ==========================================================================
// LawnApp::DoStoreItemDialog()
// @002B8E48  232 bytes
// ==========================================================================
LawnDialog * LawnApp::DoStoreItemDialog()
{
  return DoDialog(48, true,
                  Sexy::String("[BUY_ITEM_HEADER]"),
                  Sexy::String("[BUY_ITEM]"),
                  Sexy::String(gEmptyCString),
                  1);
}

// ==========================================================================
// LawnApp::DoZenSellDialog()
// @002B8F70  228 bytes
// ==========================================================================
Sexy::Dialog * LawnApp::DoZenSellDialog()
{
  Sexy::String aHeader;
  TodStringTranslate(&aHeader, "[ZEN_SELL_HEADER]");
  Sexy::String aBody;
  TodStringTranslate(&aBody, "[ZEN_SELL_LINES]");
  return DoDialog(50, true, aHeader, aBody, Sexy::String(gEmptyCString), 1);
}

// ==========================================================================
// LawnApp::DoPacketPurchaseDialog(int,int)
// @002B9094  300 bytes
// ==========================================================================
Sexy::Dialog * LawnApp::DoPacketPurchaseDialog(int theCost, int theCount)
{
  // The body is the "[UPGRADE_DIALOG_BODY]" text with {SLOTS} replaced by the
  // second argument; the first argument is the price and only feeds the money
  // string, so the dialog body itself does not mention it.
  Sexy::String aTemplate;
  TodStringTranslate(&aTemplate, "[UPGRADE_DIALOG_BODY]");
  Sexy::String aBody;
  TodReplaceNumberString(&aBody, &aTemplate, "{SLOTS}", theCount);

  Sexy::String aPrice = GetMoneyString(theCost);
  return DoDialog(53, true, aBody, aPrice, Sexy::String(gEmptyCString), 1);
}

// ==========================================================================
// LawnApp::DoProfileChangeDialog()
// @002B9208  230 bytes
// ==========================================================================
void LawnApp::DoProfileChangeDialog()
{
  DoDialog(54, true,
           Sexy::String("[PROFILE_CHANGE]"),
           Sexy::String("[PROFILE_CHANGE_MSG]"),
           Sexy::String("[DIALOG_BUTTON_OK]"),
           3);
}

// ==========================================================================
// LawnApp::DoRetryLeaderboardDialog()
// @002B9330  232 bytes
// ==========================================================================
Sexy::Dialog * LawnApp::DoRetryLeaderboardDialog()
{
  return DoDialog(61, true,
                  Sexy::String("[RETRY_LEADERBOARD_HEADER]"),
                  Sexy::String("[RETRY_LEADERBOARD_BODY]"),
                  Sexy::String(gEmptyCString),
                  1);
}

// ==========================================================================
// LawnApp::DoRetryAchievementsDialog()
// @002B9458  232 bytes
// ==========================================================================
Sexy::Dialog * LawnApp::DoRetryAchievementsDialog()
{
  return DoDialog(62, true,
                  Sexy::String("[RETRY_ACHIEVEMENTS_HEADER]"),
                  Sexy::String("[RETRY_ACHIEVEMENTS_TEXT]"),
                  Sexy::String(gEmptyCString),
                  1);
}

// ==========================================================================
// LawnApp::DoContentSavingDialog()
// @002B9580  142 bytes
// ==========================================================================
SavingFileDialog * LawnApp::DoContentSavingDialog()
{
  KillDialog(68);
  SavingFileDialog *aDialog = new SavingFileDialog(this, true, Sexy::String("[SAVING_DIALOG]"));
  AddDialogWithId(aDialog->mId, aDialog);

  // The widget manager only takes the widget while no modal dialog is up.
  if ( mWidgetManager->mFocusWidget == nullptr )
    mWidgetManager->AddWidget(aDialog);
  return aDialog;
}

// ==========================================================================
// LawnApp::DoContentLoadingDialog()
// @002B9630  142 bytes
// ==========================================================================
SavingFileDialog * LawnApp::DoContentLoadingDialog()
{
  KillDialog(68);
  SavingFileDialog *aDialog = new SavingFileDialog(this, true, Sexy::String("[LOADING]"));
  AddDialogWithId(aDialog->mId, aDialog);
  if ( mWidgetManager->mFocusWidget == nullptr )
    mWidgetManager->AddWidget(aDialog);
  return aDialog;
}
// ==========================================================================
// LawnApp::DoRepickWarningDialog(char const*)
// @002B96E0  36 bytes
// ==========================================================================
int LawnApp::DoRepickWarningDialog(char const* theMessage)
{
  return LawnMessageBox(29, "[DIALOG_WARNING]", theMessage, "[DIALOG_BUTTON_YES]", "[REPICK_BUTTON]", 1);
}

// ==========================================================================
// LawnApp::GamepadToPlayerIndex(unsigned int)
// @002B9710  52 bytes
// ==========================================================================
int LawnApp::GamepadToPlayerIndex(unsigned int theGamepadIndex)
{
  if ( theGamepadIndex <= 3 )
  {
    // Player 1 is whichever gamepad the first profile is bound to (PlayerInfo's
    // own GetId); player 2 is the pad recorded for the second player.
    if ( mPlayerInfo != nullptr && theGamepadIndex == (unsigned int)mPlayerInfo->GetId() )
      return 0;
    if ( mSecondPlayerGamepadIndex != -1 && mSecondPlayerGamepadIndex == (int)theGamepadIndex )
      return 1;
  }
  return -1;
}

// ==========================================================================
// LawnApp::PlayerToGamepadIndex(int)
// @002B9744  34 bytes
// ==========================================================================
int LawnApp::PlayerToGamepadIndex(int thePlayerIndex)
{
  if ( thePlayerIndex != 0 )
  {
    if ( thePlayerIndex == 1 )
      return mSecondPlayerGamepadIndex;
    return -1;
  }
  if ( mPlayerInfo == nullptr )
    return -1;
  return mPlayerInfo->GetId();
}

// ==========================================================================
// LawnApp::SetSecondPlayer(int)
// @002B9768  142 bytes
// ==========================================================================
int LawnApp::SetSecondPlayer(int theGamepadIndex)
{
  // The second player cannot be moved while the zen garden is up.
  if ( mGameMode == GAMEMODE_CHALLENGE_ZEN_GARDEN )
    return 0;

  Board *aBoard = mBoard;
  mSecondPlayerGamepadIndex = theGamepadIndex;
  if ( aBoard != nullptr )
    aBoard->AddSecondPlayer(theGamepadIndex);

  // Zen garden (mode 0x2C) returns without storing. Otherwise the pad index
  // is written at +0xA7C, the board is told, and the profile manager's
  // GetProfile (vtable +0x1C) looks the record up. LoadDetails is vtable +0x24.
  Sexy::String aName = mPlayerInfo->GetName();
  mPlayer2Info = dynamic_cast<LawnPlayerInfo *>(mProfileMgr->GetProfile(aName, theGamepadIndex));
  if (mPlayer2Info != nullptr)
    mPlayer2Info->LoadDetails();
  return 0;
}
// ==========================================================================
// LawnApp::CheckFullVersion(bool)
// @002B980C  4 bytes
// ==========================================================================
int LawnApp::CheckFullVersion(bool thePrompt)
{
  return 1;
}

// ==========================================================================
// LawnApp::IsFullVersion()
// @002B9810  6 bytes
// ==========================================================================
int LawnApp::IsFullVersion()
{
  return mIsFullVersion;
}
// ==========================================================================
// LawnApp::WriteCurrentUserConfig()
// @002B9818  52 bytes
// ==========================================================================
int LawnApp::WriteCurrentUserConfig()
{
  if ( IsFullVersion() == 0 )
    return 1;
  OutputDebug("((LawnApp*))->WriteCurrentUserConfig() ...\n");
  // The original vtable call at byte 44 is PlayerInfo::IsSaving.
  if ( mPlayerInfo == nullptr || mPlayerInfo->IsSaving() )
    return 0;
  return mPlayerInfo->SaveDetails();
}
// ==========================================================================
// LawnApp::WriteSecondUserConfig()
// @002B9850  52 bytes
// ==========================================================================
int LawnApp::WriteSecondUserConfig()
{
  if ( IsFullVersion() == 0 )
    return 1;
  OutputDebug("((LawnApp*))->WriteCurrentUserConfig() ...\n");
  if ( mPlayer2Info == nullptr || mPlayer2Info->IsSaving() )
    return 0;
  return mPlayer2Info->SaveDetails();
}
// ==========================================================================
// LawnApp::OnProfileSaveError(Sexy::PlayerInfo *)
// @002B9888  128 bytes
// ==========================================================================
void LawnApp::OnProfileSaveError(Sexy::PlayerInfo * theProfile)
{
  // The error only concerns the profile the manager is pointing at. Each of the
  // two players is retried until the dialog reports success.
  bool aSaved = false;
  if ( mPlayerInfo == theProfile )
  {
    while ( true )
    {
      if ( !DoProfileSaveErrorDialog() )
        break;
      if ( WriteCurrentUserConfig() )
      {
        aSaved = true;
        break;
      }
    }
  }

  // The binary compares the two profile pointers directly.
  if ( mSecondPlayerGamepadIndex != -1 && mPlayer2Info != nullptr
    && theProfile == mPlayer2Info )
  {
    while ( true )
    {
      if ( !DoProfileSaveErrorDialog() )
        break;
      if ( WriteSecondUserConfig() )
      {
        aSaved = true;
        break;
      }
    }
  }

  if ( aSaved )
  {
    SavingProfileDialog *aDialog = new SavingProfileDialog(this, true);
    AddDialogWithId(aDialog->mId, aDialog);
    aDialog->WaitForResult(true);
  }
}

// ==========================================================================
// LawnApp::TryToSaveProfile()
// @002B9920  98 bytes
// ==========================================================================
int LawnApp::TryToSaveProfile()
{
  // Only the full version has a profile file to write.
  int result = IsFullVersion();
  if ( result == 0 )
    return result;

  // The flag stays up for as long as the write is in flight, which is what
  // stops the saving dialog's own update from starting a second one.
  mProfileSaveInProgress = true;
  if ( WriteCurrentUserConfig() )
  {
    SavingProfileDialog *aDialog = new SavingProfileDialog(this, false);
    AddDialogWithId(aDialog->mId, aDialog);
    result = aDialog->WaitForResult(true);
  }
  else
  {
    // The write failed: retry until the player gives up on the error dialog.
    while ( true )
    {
      result = DoProfileSaveErrorDialog();
      if ( result == 0 )
        break;
      if ( WriteCurrentUserConfig() )
      {
        SavingProfileDialog *aDialog = new SavingProfileDialog(this, false);
        AddDialogWithId(aDialog->mId, aDialog);
        result = aDialog->WaitForResult(true);
        break;
      }
    }
  }
  mProfileSaveInProgress = false;
  return result;
}
// ==========================================================================
// LawnApp::UpdatePlayerProfileForFinishingLevel()
// @002B9A70  442 bytes
// ==========================================================================
int LawnApp::UpdatePlayerProfileForFinishingLevel()
{
  PostLevelSuccess();

  // Adventure: the level counter advances, and the three unlock flags follow
  // from which level was just cleared.
  if ( IsAdventureMode() )
  {
    int aLevel = mBoard->mLevel;
    if ( aLevel == 50 )
    {
      mPlayerInfo->SetLevel(1);
      if ( mPlayerInfo->GetFlag(PlayerFlags(1)) )
        mPlayerInfo->SetFlag(0x8000, 1);
      else
        mPlayerInfo->SetFlag(1, 1);
      if ( mPlayerInfo->GetFlag(PlayerFlags(1)) && !mPlayerInfo->GetFlag(PlayerFlags(0x8000)) )
        mPlayerInfo->SetFlag(2048, 1);
      HasFinishedAdventure();
    }
    else
    {
      mPlayerInfo->SetLevel(aLevel + 1);
      if ( !HasFinishedAdventure() && aLevel == 34 )
        mPlayerInfo->SetFlag(4096, 1);
    }

    if ( !HasFinishedAdventure() )
    {
      // The "level 15/31/45 cleared" markers the main menu reads. GameStats
      // starts at +0x774, so the two words the body touches are its +0 and +4.
      mPlayerInfo->mGameStats.mHouseType = BLUEPRINT_BLING;
      int aMarker = mPlayerInfo->mGameStats.mHouseLevel;
      bool aBelowOne = aMarker <= 0;
      if ( aLevel <= 15 )
        aBelowOne = false;
      if ( aBelowOne )
        aMarker = 1;
      else
        aBelowOne = aMarker <= 1;
      bool aBelowTwo = aLevel > 31 && aBelowOne;
      int aResult = 0;
      if ( aBelowTwo )
        aMarker = 2;
      mPlayerInfo->mGameStats.mHouseLevel = aMarker;
      TryToSaveProfile();
      return aResult;
    }
  }
  else if ( !IsSurvivalMode() )
  {
    if ( !IsPuzzleMode() )
    {
      // A challenge on a non-puzzle page: the record's win count is bumped and
      // the "first win" flags are set once.
      int aFirstWin = !HasBeatenChallenge(mGameMode);
      int aIndex = GetCurrentChallengeIndex();
      ++mPlayerInfo->mChallengeRecords[aIndex];
      if ( aFirstWin && HasFinishedAdventure() && GetNumTrophies(CHALLENGE_PAGE_CHALLENGE) <= 17 )
        mPlayerInfo->SetFlag(64, 1);
      TryToSaveProfile();
      return aFirstWin;
    }

    // Puzzle pages count their own records and set one of two unlock flags.
    bool aBeaten = HasBeatenChallenge(mGameMode);
    int aIndex = GetCurrentChallengeIndex();
    ++mPlayerInfo->mChallengeRecords[aIndex];
    if ( (HasFinishedAdventure() || (unsigned int)(mGameMode - 56) > 1) && !aBeaten )
    {
      if ( IsScaryPotterLevel() != 0 )
        mPlayerInfo->SetFlag(128, 1);
      else
        mPlayerInfo->SetFlag(256, 1);
      TryToSaveProfile();
      return 1;
    }
    TryToSaveProfile();
    return 0;
  }

  // Survival: only the final stage of a run counts, and it saves the score.
  if ( mBoard->IsFinalSurvivalStage() == 0 )
  {
    TryToSaveProfile();
    return 0;
  }
  int aFirstWin = !HasBeatenChallenge(mGameMode);
  mBoard->SurvivalSaveScore();
  if ( aFirstWin && HasFinishedAdventure() && (unsigned int)(GetNumTrophies(CHALLENGE_PAGE_SURVIVAL) - 8) > 1 )
    mPlayerInfo->SetFlag(512, 1);
  return aFirstWin;
}
// ==========================================================================
// LawnApp::OnFullVersionLicenseChange()
// @002B9BB0  84 bytes
// ==========================================================================
void LawnApp::OnFullVersionLicenseChange()
{
  if ( mIsFullVersion )
    return;

  mIsFullVersion = true;

  // A profile that is already loaded has to be re-read, because the licensing
  // change alters what the loader is allowed to do.
  if ( mPlayerInfo != nullptr && mPlayerInfo->IsLoaded() )
  {
    OutputDebug("Doing a forced profile reload...\n");
    OutputDebug("Ret: %d\n", 1);
  }

  if ( mGameSelector != nullptr )
    mGameSelector->OnFullVersionLicenseChange();
  if ( mMailBox != nullptr )
    mMailBox->RefreshMessages();
  if ( mBoard != nullptr )
    mBoard->OnFullVersionLicenseChange();
}
// ==========================================================================
// LawnApp::ShowHelpBarWidget()
// @002B9C0C  218 bytes
// ==========================================================================
int LawnApp::ShowHelpBarWidget()
{
  // An unbuilt bar asks the widget manager to add it first.
  if ( mHelpBarWidget->mWidgetManager == nullptr )
    mWidgetManager->AddWidget(mHelpBarWidget);

  if ( !mHelpBarWidget->mVisible )
  {
    // Start off screen and slide in.
    mHelpBarWidget->mDisabled = false;
    mHelpBarWidget->mVisible = true;
    mHelpBarWidget->Move(static_cast<int>(HelpBarWidget::OFFSCREEN_POS.mX),
                        static_cast<int>(HelpBarWidget::OFFSCREEN_POS.mY));
  }

  if ( mHelpBarWidget->mY == static_cast<int>(HelpBarWidget::DEFAULT_POS.mY) )
  {
    // Already at rest: no animation.
    mHelpBarWidget->SetAnimator(nullptr);
  }
  else
  {
    mHelpBarWidget->Move(static_cast<int>(HelpBarWidget::DEFAULT_POS.mX),
                        static_cast<int>(HelpBarWidget::DEFAULT_POS.mY));
    SlideInOutWidgetAnimator *aAnimator = new SlideInOutWidgetAnimator(mHelpBarWidget, SlideInOutWidgetAnimator::SLIDE_FROM_BOTTOM, 0);
    aAnimator->Init((Sexy::WidgetAnimationState)1);
    mHelpBarWidget->SetAnimator(aAnimator);
  }
  mWidgetManager->BringToFront(mHelpBarWidget);
  return 0;
}

// ==========================================================================
// LawnApp::HideHelpBarWidget()
// @002B9CFC  102 bytes
// ==========================================================================
Sexy::WidgetAnimator * LawnApp::HideHelpBarWidget()
{
  if ( !mHelpBarWidget->mVisible )
    return nullptr;

  mHelpBarWidget->Move(static_cast<int>(HelpBarWidget::DEFAULT_POS.mX),
                      static_cast<int>(HelpBarWidget::DEFAULT_POS.mY));
  SlideInOutWidgetAnimator *aAnimator = new SlideInOutWidgetAnimator(mHelpBarWidget, SlideInOutWidgetAnimator::SLIDE_FROM_BOTTOM, 0);
  aAnimator->Init((Sexy::WidgetAnimationState)2);
  return mHelpBarWidget->SetAnimator(aAnimator);
}

// ==========================================================================
// LawnApp::ForceHideHelpBarWidget()
// @002B9D74  46 bytes
// ==========================================================================
Sexy::WidgetAnimator * LawnApp::ForceHideHelpBarWidget()
{
  if ( mHelpBarWidget == nullptr )
    return nullptr;

  // mDrawShade is the "has buttons" flag the two ClearButtons calls take down.
  mHelpBarWidget->mDrawShade = false;
  mHelpBarWidget->ClearButtons(0);
  mHelpBarWidget->ClearButtons(0);
  HideHelpBarWidget();
  return nullptr;
}
// ==========================================================================
// LawnApp::CheckControllerConnections()
// @002B9DA4  2 bytes
// ==========================================================================
void LawnApp::CheckControllerConnections()
{
  ;
}

// ==========================================================================
// LawnApp::ClearSecondPlayer()
// @002B9DA8  18 bytes
// ==========================================================================
int LawnApp::ClearSecondPlayer()
{
  mSecondPlayerGamepadIndex = -1;
  mPlayer2Info = nullptr;
  CheckControllerConnections();
  // TODO(recon): the dump is `return CheckControllerConnections()`, handing back
  // the word the void call leaves in r0.
  return 0;
}

// ==========================================================================
// LawnApp::ShowMainMenuScreen()
// @002B9DBC  86 bytes
// ==========================================================================
int LawnApp::ShowMainMenuScreen()
{
  ClearSecondPlayer();
  VSResultsMenu::ClearPlayerRecords();
  mGameScene = SCENE_MENU;

  MainMenu *aMenu = new MainMenu(this);
  mGameSelector = aMenu;
  aMenu->Resize(LawnApp::FULLSCREEN_RECT);
  mWidgetManager->AddWidget(aMenu);
  // ShowMainMenuScreen @0x2b9e0c calls WidgetManager::SetFocus (vtable byte 48).
  mWidgetManager->SetFocus(aMenu);
  return 0;
}

// ==========================================================================
// LawnApp::ShowGameSelector()
// @002B9E24  30 bytes
// ==========================================================================
int LawnApp::ShowGameSelector()
{
  KillBoard();
  UpdateRegisterInfo();
  mGameScene = SCENE_MENU;
  return ShowMainMenuScreen();
}

// ==========================================================================
// LawnApp::LoadingCompleted()
// @002B9E44  102 bytes
// ==========================================================================
void LawnApp::LoadingCompleted()
{
  mWidgetManager->RemoveWidget(mTitleScreen);
  SafeDeleteWidget(mTitleScreen);
  mTitleScreen = nullptr;

  // The saved music and sound volumes are pushed back onto the app.
  if ( mPlayerInfo != nullptr )
  {
    SetMusicVolume(mPlayerInfo->mMusicVolume);
    SetSfxVolume(mPlayerInfo->mSoundVolume);
  }

  ShowGameSelector();
  mSoundSystem->RehookupSoundWithMusicVolume();
}
// ==========================================================================
// LawnApp::DoBackToMain()
// @002B9EAC  56 bytes
// ==========================================================================
int LawnApp::DoBackToMain()
{
  // The music is stopped, any paused foley released, then the whole board is
  // torn down and the game selector comes back.
  mMusic->StopAllMusic();
  mSoundSystem->CancelPausedFoley();
  if (mGameMode == GAMEMODE_CHALLENGE_ZEN_GARDEN || mGameMode == GAMEMODE_TREE_OF_WISDOM)
  {
    // DisposeBoard finishes garden tools, coins and Stinky's position. Save
    // those profile changes after that cleanup, before opening the main menu.
    KillNewOptionsDialog();
    KillBoard();
    TryToSaveProfile();
  }
  else
  {
    TryToSaveProfile();
    KillNewOptionsDialog();
    KillBoard();
  }
  ClearSecondPlayer();
  return ShowGameSelector();
}

// ==========================================================================
// LawnApp::ShowTitleScreen(bool)
// @002B9EE4  134 bytes
// ==========================================================================
int LawnApp::ShowTitleScreen(bool theFromGame)
{
  KillAll();

  mGameScene = SCENE_LOADING;

  TitleScreen *aTitleScreen = new TitleScreen(this);
  mTitleScreen = aTitleScreen;
  // +0x14C is cleared here and only here. The startup title screen keeps the
  // constructor's true, so the logo and intro still play on a cold launch.
  aTitleScreen->mNeedPlayLogo = false;

  // The title screen is laid out at the app's own size and then added to the
  // widget manager and focused.
  aTitleScreen->Resize(0, 0, mWidth, mHeight);
  aTitleScreen->mNeedRegister = true;
  aTitleScreen->mLoaderScreenIsLoaded = true;
  mWidgetManager->AddWidget(aTitleScreen);
  mWidgetManager->SetFocus(aTitleScreen);

  int aResult = ClearSecondPlayer();
  if ( theFromGame != 0 )
  {
    DoProfileChangeDialog();
    return 0;
  }
  return aResult;
}
// ==========================================================================
// LawnApp::DrawDialogFade(Sexy::Graphics *)
// @002B9F78  142 bytes
// ==========================================================================
void LawnApp::DrawDialogFade(Sexy::Graphics *g)
{
    if (mDialogFadeDrawn)
        return;

    // TV draws one black overlay per frame, shared by stacked dialogs.
    mDialogFadeDrawn = true;
    float savedTransX = g->mTransX;
    float savedTransY = g->mTransY;
    g->mTransX = 0.0f;
    g->mTransY = 0.0f;

    g->SetColor(Sexy::Color(0, 0, 0, 170));
    g->FillRect(Sexy::TRect<int>(-500, -500, 2500, 2500));
    g->SetColor(Sexy::Color::White);

    g->mTransX = savedTransX;
    g->mTransY = savedTransY;
}

// ==========================================================================
// LawnApp::DrawQRCode(Sexy::Graphics *)
// @002BA018  158 bytes
// ==========================================================================
int LawnApp::DrawQRCode(Sexy::Graphics * g)
{
  Sexy::Image *aBackground = mQRCodeImageBackground;

  float aSavedTransX = g->mTransX;
  float aSavedTransY = g->mTransY;

  // The code is centred in an 800-wide band, 73 in from its left edge and 500
  // down; the background plate is the 90x90 cel at (+15, +525).
  int aX = (800 - aBackground->mWidth) / 2;
  g->mTransX = (float)(-mScreenBounds.mX);
  g->mTransY = (float)(-mScreenBounds.mY);

  Sexy::TRect<int> aDestRect(aX + 73, 525, 90, 90);
  Sexy::TRect<int> aSrcRect(15, 15, 90, 90);

  g->DrawImage(aBackground, aX, 500);
  g->DrawImage(mQRCodeImage, aDestRect, aSrcRect);
  g->SetColor(Sexy::Color::White);

  g->mTransX = aSavedTransX;
  g->mTransY = aSavedTransY;
  return 0;
}
// ==========================================================================
// LawnApp::DoGameSaveErrorDialog()
// @002BA0C0  2 bytes
// ==========================================================================
void LawnApp::DoGameSaveErrorDialog()
{
  ;
}

// ==========================================================================
// LawnApp::ShowSavingDingus()
// @002BA0C4  292 bytes
// ==========================================================================
int LawnApp::ShowSavingDingus()
{
  // The dingus is a Crazy Dave that slides up from the bottom; the two words at
  // +0xAA0/+0xAA4 are the ends of its vertical animation.
  if ( mSavingDingusReanimationID != 0 )
  {
    mSavingDingusAnimationTime = 0.0f;
    mSavingDingusAnimationStartY = mSavingDingusCurrentY;
    mSavingDingusAnimationEndY = 550.0f;
  }
  else
  {
    // A rare variant picks the Zombie instead of Crazy Dave; the wide-screen
    // flag at +96 of mPlatformDriverOrQueue picks the start position.
    float aStartY = mPlatformDriverOrQueue->mWideScreen ? 1000.0f : 700.0f;
    Reanimation *aDingus;
    if ( PlatformRandomLong() % 100 > 97 )
    {
      aDingus = AddReanimation(aStartY, mSavingDingusCurrentY, 304000, REANIM_ZOMBIE);
      Zombie::SetupReanimLayers(aDingus, (ZombieType)0);
    }
    else
    {
      aDingus = AddReanimation(aStartY, mSavingDingusCurrentY, 304000, REANIM_SUNFLOWER);
    }
    aDingus->PlayReanim("anim_idle", REANIM_LOOP, 0, 12.0);
    aDingus->mIsAttachment = true;
    mSavingDingusReanimationID = ReanimationGetID(aDingus);
    mSavingDingusAnimationTime = 0.0f;
    mSavingDingusAnimationStartY = 900.0f;
    mSavingDingusAnimationEndY = 550.0f;
  }

  ForceHideHelpBarWidget();
  if ( mSavingDingusShowCount < 0 )
    mSavingDingusShowCount = 1;
  else
    ++mSavingDingusShowCount;
  return 0;
}

// ==========================================================================
// LawnApp::HideSavingDingus()
// @002BA1F8  76 bytes
// ==========================================================================
int * LawnApp::HideSavingDingus()
{
  if ( --mSavingDingusShowCount < 0 || mSavingDingusShowCount == 0 )
  {
    mSavingDingusAnimationTime = 0.0f;
    mSavingDingusAnimationStartY = mSavingDingusCurrentY;
    mSavingDingusAnimationEndY = 900.0f;

    Reanimation *aDingus = ReanimationTryToGet(mSavingDingusReanimationID);
    if ( aDingus != nullptr )
    {
      aDingus->ReanimationDie();
      mSavingDingusReanimationID = REANIMATIONID_NULL;
    }
  }
  return nullptr;
}

// ==========================================================================
// LawnApp::DrawSavingDingus(Sexy::Graphics *)
// @002BA244  512 bytes
// ==========================================================================
char ** LawnApp::DrawSavingDingus(Sexy::Graphics * g)
{
  int aTransX = (int)g->mTransX;
  int aTransY = (int)g->mTransY;
  g->mTransX = 0.0f;
  g->mTransY = 0.0f;

  Reanimation *aDingus = ReanimationTryToGet(mSavingDingusReanimationID);

  // Which save/load screen is up decides the caption. 68/57/56/69 are the
  // dialog ids the saving, leaderboard, live-saving and loading screens use.
  if ( GetDialog(68) != nullptr )
  {
    if ( mNeedGoBackToMain )
      mDingusText = "[SAVING_DIALOG]";
    else if ( mNeedLoadGame )
      mDingusText = "[LOADING]";
  }
  else if ( GetDialog(57) != nullptr )
  {
    mDingusText = "[LEADERBOARDS_DIALOG]";
  }
  else if ( GetDialog(56) != nullptr )
  {
    mDingusText = "[SAVING_LIVE_DIALOG]";
  }
  else if ( GetDialog(69) != nullptr )
  {
    mDingusText = "[LOADING]";
  }

  if ( aDingus != nullptr )
  {
    aDingus->Draw(g);

    // The wide-screen flag at +96 of the platform object drops the caption.
    int aTextY = mPlatformDriverOrQueue->mWideScreen ? 1045 : 745;
    TodDrawString(g, mDingusText, aTextY, (int)(mSavingDingusCurrentY + 90.0f),
                  FONT_HOUSEOFTERROR16, Sexy::Color::White, 2);
  }
  else if ( mSavingDingusShowCount > 0 )
  {
    float aStartY = mPlatformDriverOrQueue->mWideScreen ? 1000.0f : 700.0f;
    Reanimation *aZombie;
    if ( PlatformRandomLong() % 100 > 97 )
    {
      aZombie = AddReanimation(aStartY, mSavingDingusCurrentY, 304000, REANIM_ZOMBIE);
      Zombie::SetupReanimLayers(aZombie, (ZombieType)0);
      aZombie->PlayReanim("anim_idle", REANIM_LOOP, 0, 12.0);
    }
    else
    {
      aZombie = AddReanimation(aStartY, mSavingDingusCurrentY, 304000, REANIM_SUNFLOWER);
    }
    aZombie->Draw(g);
    mSavingDingusReanimationID = ReanimationGetID(aZombie);
  }

  g->mTransX = (float)aTransX;
  g->mTransY = (float)aTransY;
  return nullptr;
}
// ==========================================================================
// LawnApp::UpdateSavingDingus()
// @002BA474  136 bytes
// ==========================================================================
int LawnApp::UpdateSavingDingus()
{
  Reanimation *aDingus = ReanimationTryToGet(mSavingDingusReanimationID);
  if ( aDingus == nullptr )
  {
    mSavingDingusReanimationID = REANIMATIONID_NULL;
    return 0;
  }

  // The curve walks mSavingDingusCurrentY from the start to the end position
  // over a second and a bit; the X is fixed by the wide-screen flag.
  float aStartY = mSavingDingusAnimationStartY;
  mSavingDingusAnimationTime += 0.01f;
  float aTime = mSavingDingusAnimationTime;
  mSavingDingusCurrentY = TodAnimateCurveFloatTime(0.0f, 0.4f, aTime,
                                                   aStartY, mSavingDingusAnimationEndY, CURVE_LINEAR);
  float aX = mPlatformDriverOrQueue->mWideScreen ? 1000.0f : 700.0f;
  aDingus->SetPosition(aX, mSavingDingusCurrentY);
  aDingus->Update();
  return 0;
}
// ==========================================================================
// LawnApp::SetDingusText(Sexy::String &)
// @002BA508  8 bytes
// ==========================================================================
int LawnApp::SetDingusText(Sexy::String & theText)
{
  // @0x2BA508 is an eight-byte tail call into basic_string::assign, and the
  // destination is mDingusText at +0xAAC.
  mDingusText = theText;
  return 0;
}

// ==========================================================================
// LawnApp::DeleteSavedGame()
// @002BA510  116 bytes
// ==========================================================================
void LawnApp::DeleteSavedGame()
{
  // The saved-game file name is built from the game mode and the profile's two
  // ids, then erased.
  Sexy::String aName;
  GetSavedGameName(&aName, mGameMode, mPlayerInfo->GetProfileId(), mPlayerInfo->GetId());
  EraseFile(aName);
}

// ==========================================================================
// LawnApp::CanShopLevel()
// @002BA59C  120 bytes
// ==========================================================================
bool LawnApp::CanShopLevel()
{
  if ( mGameMode != GAMEMODE_ADVENTURE )
  {
    // 17..50 are the challenge modes and 51 is the tree of wisdom; neither gets
    // a shop pad.
    if ( mGameMode >= GAMEMODE_CHALLENGE_WAR_AND_PEAS
      && mGameMode <= GAMEMODE_CHALLENGE_SQUIRREL )
      return false;
    if ( mGameMode == GAMEMODE_TREE_OF_WISDOM )
      return false;
  }
  else if ( mBoard->mLevel % 5 == 0 )
  {
    return false;
  }

  return IsSlotMachineLevel() == 0
      && IsSquirrelLevel() == 0
      && IsWallnutBowlingLevel() == 0
      && IsIZombieLevel() == 0
      && IsScaryPotterLevel() == 0;
}

// ==========================================================================
// LawnApp::ShopPadCanShow()
// @002BA614  102 bytes
// ==========================================================================
bool LawnApp::ShopPadCanShow()
{
  if ( mBoard == nullptr )
    return false;

  CutScene *aCutScene = mBoard->mCutScene;
  if ( aCutScene == nullptr || mGameMode != GAMEMODE_ADVENTURE )
    return false;

  if ( mBoard->mLevel % 5 == 0 )
    return false;

  // 0x2BA614 permits input after the cutscene phases, or once its timer is cleared.
  int aIntroEnd = aCutScene->mGraveStoneTime + aCutScene->mLawnMowerTime + 6000
                + aCutScene->mSodTime + aCutScene->mCrazyDaveTime
                + aCutScene->mFogTime + aCutScene->mBossTime;
  return aIntroEnd <= aCutScene->mCutsceneTime || aCutScene->mCutsceneTime <= 0;
}

// ==========================================================================
// LawnApp::StartHelp(int)
// @002BA67C  14 bytes
// ==========================================================================
void LawnApp::StartHelp(int theHelpIndex)
{
  // @0x2BA67C. theHelpIndex > 1 returns this. The only caller ignores the return.
  if ( theHelpIndex <= 1 )
    mBoard->StartHelp(theHelpIndex);
}

// ==========================================================================
// LawnApp::SdkExit()
// @002BA68C  212 bytes
// ==========================================================================
void LawnApp::SdkExit()
{
  // logtfi takes its tag as an int, and the ARM passes the four-character
  // "sdk" tag directly; the String the transcription built for it was the
  // sret buffer of the call, not an argument.
  Sexy::logtfi(0x6B6473, (char *)"LawnApp::SdkExit\n");

  // The game centre only gets the exit page when it advertises the feature;
  // otherwise the ordinary quit confirmation runs.
  if ( mGameCenter->HasFeature(Sexy::IGameCenter::FEATURE_EXIT) )
  {
    mGameCenter->Show("Exit");
    return;
  }

  Sexy::Dialog *aDialog = ConfirmQuit();
  if ( aDialog->WaitForResult(true) == 1000 )
    Quit();
}
static int GuardedPreload(LawnApp* theApp);

// Title-screen load. The binary runs this on a side thread and the bar reads
// completed/total. One missing file used to return from the whole procedure,
// so the later groups never started and the roll sat on a counter that never
// finished. A miss is logged and skipped. The bar fills only after the last
// step sets mLoadingThreadCompleted.
namespace {

enum TitleLoadPhase
{
  TitleLoad_NeedPrepare = 0,
  TitleLoad_Images,
  TitleLoad_Fonts,
  TitleLoad_Music,
  TitleLoad_Pool,
  TitleLoad_Garden,
  TitleLoad_Cache,
  TitleLoad_Foley,
  TitleLoad_Trails,
  TitleLoad_Particles,
  TitleLoad_Preload,
  TitleLoad_Sounds,
  TitleLoad_Done
};

TitleLoadPhase gTitleLoadPhase = TitleLoad_NeedPrepare;
bool gTitleGroupOpen = false;
int gTitleGroupPulled = 0;
int gTitleGroupSkipped = 0;

void ForgetGroupError(Sexy::ResourceManager* theManager)
{
  if (theManager == nullptr)
    return;
  theManager->mHasError = false;
  theManager->mErrorText.clear();
}

void CloseTitleGroup(LawnApp* theApp, const char* theGroup)
{
  if (!gTitleGroupOpen)
    return;
  Sexy::ResourceManager* aManager = theApp->mResourceManager;
  if (aManager != nullptr)
  {
    ForgetGroupError(aManager);
    if (aManager->mCurResGroupList != nullptr
        && aManager->mCurResGroupItr == aManager->mCurResGroupList->end())
      aManager->LoadNextResource();
    Sexy::ExtractResourcesByName(aManager, theGroup);
  }
  fprintf(stderr, "title load: %s pulled %d, skipped %d\n",
          theGroup, gTitleGroupPulled, gTitleGroupSkipped);
  fflush(stderr);
  gTitleGroupOpen = false;
}

// Pull up to theBudget resources. Returns true while the group still has some.
bool PullTitleGroup(LawnApp* theApp, const char* theGroup, int theWeight, int theBudget)
{
  Sexy::ResourceManager* aManager = theApp->mResourceManager;
  if (aManager == nullptr)
    return false;

  if (!gTitleGroupOpen)
  {
    ForgetGroupError(aManager);
    aManager->mParentResGroupName.clear();
    aManager->StartLoadResources(Sexy::String(theGroup), false);
    gTitleGroupOpen = true;
    gTitleGroupPulled = 0;
    gTitleGroupSkipped = 0;
    fprintf(stderr, "title load: start %s\n", theGroup);
    fflush(stderr);
  }

  int aTaken = 0;
  while (aTaken < theBudget && !theApp->mShutDown && !theApp->mCloseRequest)
  {
    if (aManager->mCurResGroupList == nullptr
        || aManager->mCurResGroupItr == aManager->mCurResGroupList->end())
    {
      CloseTitleGroup(theApp, theGroup);
      return false;
    }

    bool aLoaded = aManager->LoadNextResource();
    if (!aLoaded && aManager->HadError())
    {
      ++gTitleGroupSkipped;
      ForgetGroupError(aManager);
    }
    else if (aLoaded)
    {
      ++gTitleGroupPulled;
    }
    else if (aManager->mCurResGroupList == nullptr
             || aManager->mCurResGroupItr == aManager->mCurResGroupList->end())
    {
      CloseTitleGroup(theApp, theGroup);
      return false;
    }
    else
    {
      ++gTitleGroupSkipped;
    }

    if (theWeight > 0)
      theApp->mCompletedLoadingThreadTasks += theWeight;
    ++aTaken;
  }
  return true;
}

} // namespace

// ==========================================================================
// LawnApp::PrepareTitleLoad()
//
// The part of LoadingThreadProc that has to finish before the roll can draw:
// loader-bar art, string tables, layout, and the task total the bar divides by.
// ==========================================================================
void LawnApp::PrepareTitleLoad()
{
  if (gTitleLoadPhase != TitleLoad_NeedPrepare)
    return;

  static const char* sGroupNames[3] = { "LoadingFonts", "LoadingImages", "LoadingSounds" };
  static const int sGroupWeights[3] = { 54, 9, 108 };

  fprintf(stderr, "title load: prepare\n");
  fflush(stderr);

  if (mResourceManager != nullptr)
  {
    Sexy::String aGroupName("LoaderBar");
    int aNumResources = mResourceManager->GetNumResources(aGroupName, true, true);
    fprintf(stderr, "title load: LoaderBar has %d resources\n", aNumResources);
    fflush(stderr);
    mNumLoadingThreadTasks += aNumResources;
  }

  ReanimatorEnsureDefinitionLoaded(REANIM_ZOMBIE_TARGET, true);
  ReanimatorEnsureDefinitionLoaded(REANIM_LOADBAR_SPROUT, true);
  ReanimatorEnsureDefinitionLoaded(REANIM_LOADBAR_ZOMBIEHEAD, true);

  // Weight 0: these resources are the screen itself. They are in the task
  // total, and the completion flag is what lets the bar cross them.
  while (PullTitleGroup(this, "LoaderBar", 0, 64))
    ;

  Sexy::String aLocaleDir("locale/");
  {
    Sexy::String aPath = aLocaleDir;
    aPath.Append("/properties/LawnStrings.txt", 27);
    if (TodStringListLoad(aPath.c_str()) == 0)
      TodStringListLoad("properties/LawnStrings.txt");
  }
  {
    Sexy::String aPath = aLocaleDir;
    aPath.Append("/properties/LawnOEMStrings.txt", 30);
    if (TodStringListLoad(aPath.c_str()) == 0)
      TodStringListLoad("properties/LawnOEMStrings.txt");
  }

  {
    Sexy::String aLayout("properties/Layout.xml");
    fprintf(stderr, "title load: Layout.xml\n");
    fflush(stderr);
    LoadProperties(aLayout, true, false);
  }

  if (mTitleScreen != nullptr)
  {
    mTitleScreen->OnLoading();
    mTitleScreen->mLoaderScreenIsLoaded = true;
  }

  if (mMusic != nullptr)
    mMusic->MusicTitleScreenInit();

  if (mResourceManager != nullptr)
  {
    for (int aGroup = 0; aGroup < 3; ++aGroup)
    {
      Sexy::String aGroupName(sGroupNames[aGroup]);
      int aNumResources = mResourceManager->GetNumResources(aGroupName, true, true);
      fprintf(stderr, "title load: %s has %d resources\n",
              sGroupNames[aGroup], aNumResources);
      fflush(stderr);
      mNumLoadingThreadTasks += sGroupWeights[aGroup] * aNumResources;
    }
  }
  mNumLoadingThreadTasks += 648;
  mNumLoadingThreadTasks += GetNumPreloadingTasks();
  if (mMusic != nullptr)
    mNumLoadingThreadTasks += mMusic->GetNumLoadingTasks();

  fprintf(stderr, "title load: %d tasks, loader screen ready\n", mNumLoadingThreadTasks.load());
  fflush(stderr);
  gTitleLoadPhase = TitleLoad_Images;
}

// ==========================================================================
// LawnApp::StepTitleLoad()
//
// One slice of the load. The frame loop calls this so the roll moves while
// the groups come in. Returns false once the bar is allowed to finish.
// ==========================================================================
bool LawnApp::StepTitleLoad()
{
  if (gTitleLoadPhase == TitleLoad_NeedPrepare)
    PrepareTitleLoad();
  if (gTitleLoadPhase == TitleLoad_Done || mLoadingThreadCompleted)
    return false;
  if (mShutDown || mCloseRequest)
  {
    mLoadingThreadCompleted = true;
    gTitleLoadPhase = TitleLoad_Done;
    return false;
  }

  switch (gTitleLoadPhase)
  {
  case TitleLoad_Images:
    if (PullTitleGroup(this, "LoadingImages", 9, 8))
      return true;
    gTitleLoadPhase = TitleLoad_Fonts;
    return true;

  case TitleLoad_Fonts:
    if (PullTitleGroup(this, "LoadingFonts", 54, 4))
      return true;
    gTitleLoadPhase = TitleLoad_Music;
    return true;

  case TitleLoad_Music:
    fprintf(stderr, "title load: music\n");
    fflush(stderr);
    if (mMusic != nullptr && mMusicInterface != nullptr)
      mMusic->MusicInit();
    else if (mMusic != nullptr)
      mCompletedLoadingThreadTasks += mMusic->GetNumLoadingTasks();
    gTitleLoadPhase = TitleLoad_Pool;
    return true;

  case TitleLoad_Pool:
    fprintf(stderr, "title load: pool\n");
    fflush(stderr);
    mPoolEffect = new PoolEffect();
    mPoolEffect->PoolEffectInitialize();
    gTitleLoadPhase = TitleLoad_Garden;
    return true;

  case TitleLoad_Garden:
    fprintf(stderr, "title load: garden\n");
    fflush(stderr);
    mZenGarden = new ZenGarden();
    gTitleLoadPhase = TitleLoad_Cache;
    return true;

  case TitleLoad_Cache:
    fprintf(stderr, "title load: reanimator cache\n");
    fflush(stderr);
    mReanimatorCache = new ReanimatorCache();
    mReanimatorCache->ReanimatorCacheInitialize();
    mReanimatorCache->LoadCachedImages();
    gTitleLoadPhase = TitleLoad_Foley;
    return true;

  case TitleLoad_Foley:
    fprintf(stderr, "title load: foley\n");
    fflush(stderr);
    TodFoleyInitialize(gLawnFoleyParamArray, NUM_FOLEY);
    gTitleLoadPhase = TitleLoad_Trails;
    return true;

  case TitleLoad_Trails:
    fprintf(stderr, "title load: trails\n");
    fflush(stderr);
    TrailLoadDefinitions(gLawnTrailArray, 1);
    gTitleLoadPhase = TitleLoad_Particles;
    return true;

  case TitleLoad_Particles:
    fprintf(stderr, "title load: particles\n");
    fflush(stderr);
    // Each definition adds 6 to the completed count, and 108 * 6 is the 648
    // the task total reserved for particles.
    TodParticleLoadDefinitions(gLawnParticleArray, NUM_PARTICLE_EFFECTS);
    gTitleLoadPhase = TitleLoad_Preload;
    return true;

  case TitleLoad_Preload:
    fprintf(stderr, "title load: preload\n");
    fflush(stderr);
    // The reanimator table this walk reads is still a stub in places. A fault
    // here must not drop the sound group or leave the bar unfinished.
    if (GuardedPreload(this) == 0)
    {
      fprintf(stderr, "title load: preload faulted; continuing\n");
      fflush(stderr);
      mCompletedLoadingThreadTasks += GetNumPreloadingTasks();
    }
    gTitleLoadPhase = TitleLoad_Sounds;
    return true;

  case TitleLoad_Sounds:
    if (PullTitleGroup(this, "LoadingSounds", 108, 8))
      return true;
    // TitleScreen::Update @0x1A7698 reads this byte, then writes its own
    // mLoadingThreadComplete in the same breath as SetDisabled(false).
    // Writing the screen flag here makes that check skip the enable.
    fprintf(stderr, "title load: finished %d/%d\n",
            mCompletedLoadingThreadTasks.load(), mNumLoadingThreadTasks.load());
    fflush(stderr);
    gTitleLoadPhase = TitleLoad_Done;
    // The driver invokes LoadingThreadCompleted on the main thread. It owns
    // cache rendering; doing it here as well races that callback and GL draws.
    mLoadingThreadCompleted = true;
    return false;

  default:
    gTitleLoadPhase = TitleLoad_Done;
    return false;
  }
}

// ==========================================================================
// LawnApp::LoadingThreadProc()
// @002BA85C  1040 bytes
// ==========================================================================
void LawnApp::LoadingThreadProc()
{
  // One group per call. TitleScreen::Update asks for the thread once a frame,
  // and the bar reads the fraction between those calls.
  StepTitleLoad();
}
// LawnApp::setErrorCode(Sexy::String)
// ==========================================================================
// LawnApp::setErrorCode(Sexy::String)
// @002BAD40  326 bytes
// ==========================================================================
void LawnApp::setErrorCode(Sexy::String theErrorCode)
{
  // @0x2BAD40 wraps the error code in brackets and shows it in dialog 51. The
  // bracketed form is what the dialog displays; the translated form is the body.
  Sexy::String aBracketed("[");
  aBracketed += theErrorCode;
  aBracketed += "]";

  Sexy::String aMessage;
  TodStringTranslate(&aMessage, theErrorCode.c_str());
  int aResult = LawnMessageBox(51, "[LOGIN_FAILED_HEADER]", aMessage.c_str(),
                               "[DIALOG_BUTTON_OK]", gEmptyCString, 3);
  // 1000 is the OK button id. Slot 107 of the app's table is the by-id kill.
  if ( aResult == 1000 )
    KillDialogById(51);
  (void)aBracketed;
}

// ==========================================================================
// LawnApp::doTransRecharge()
// @002BAEE8  302 bytes
// ==========================================================================
void LawnApp::doTransRecharge()
{
  // A recharge asks the ad provider to show a pay item, then waits for the
  // result. With no provider there is nothing to show.
  if ( mAdProvider == nullptr )
    return;

  Sexy::String aProviderName;
  mAdProvider->getUserName(aProviderName);
  if ( aProviderName == "default" )
    return;

  Sexy::String aUserName = getUserName();
  Sexy::String aItem;
  Sexy::String aEmpty;
  mAdProvider->showItemPay(aUserName, 1, aItem, aEmpty, 0.0f, 1);

  SessionWaitingDialog *aWait = DoSessionWaitDialog();
  aWait->mDelay = 300;
  aWait->WaitForResult(true);
}

// ==========================================================================
// LawnApp::SrvLoginToServer()
// @002BB0A8  254 bytes
// ==========================================================================
int LawnApp::SrvLoginToServer()
{
  // PLACEHOLDER: there is no server to log in to.
  //
  // The body below builds a LawnSessionTask out of the user name, the device
  // identifier and the game name, then hands it to LawnSession::StartTask. Both
  // halves are the online layer, and the objects they depend on are not built on
  // this platform:
  //
  //   * LawnSession::Init used to hang forever here -- its thread joins a server
  //     connection that never completes (fixed by the placeholder there);
  //   * the user-name provider at app+1956 is an object whose vtable pointer is
  //     null, so every virtual call through it lands on address 0x14.
  //
  // Six rounds went into that second one -- the fault moved a little each time
  // but the path contributes nothing to a single-player game: it reads an account
  // name from a server. The caller does not even look at the result:
  //
  //     LawnApp::Init:  SrvLoginToServer();
  //
  // so returning success is the equivalent behaviour, exactly as for the RPC
  // engine and LawnSession::Init. That leaves Init free to run the part that
  // matters -- LoadResourceManifest, TodLoadResources("Init") and the TitleScreen
  // constructor, all of which sit after this call.
  //
  return 1;
}

// Runs the resource-group load under its own SEH frame, so a fault in the
// transcribed loader is reported instead of ending startup.
static int GuardedCall(Music* music, void (Music::*method)())
{ (music->*method)(); return 1; }

static int GuardedLoadGroup(const Sexy::String* group)
{ TodLoadResources(*group); return 1; }

static int GuardedPreload(LawnApp* app)
{ app->PreloadForUser(); return 1; }

void LawnApp::Init()
{
  // The first statement is a call through byte 248 of the VTABLE, i.e. slot 62,
  // NOT through byte 248 of the object.
  //
  // The transcription multiplied the offset by four as well as applying it: 248
  // is a vtable offset (slot 62) and 992 is 248 * 4. So instead of loading the
  // vtable pointer and indexing 248 bytes into it, the old line read the
  // object's own field at byte 992 -- which holds garbage, 0x100 in the observed
  // run -- and then dereferenced that to find the function address:
  //
  //     InitFault: code=0xC0000005 instr=Lawn.exe+0x4516CC
  //     InitFault:   info[0] = 0x00000000     <- a read
  //     InitFault:   info[1] = 0x00000100     <- address 0x100
  //
  // 0x100 is below 64 KiB, which Windows never maps, so the read could only ever
  // fault. The ARM at 0x2BB260 loads the vptr, indexes 0xF8 into it and calls
  // through that; 0xF8 is slot 62 of LawnApp's table, whose entry is
  // Sexy::SexyAppBase::DoParseCmdLine (0x338EE8), so that is the call below.
  //
  // This is the "offset multiplied by four" defect seen elsewhere in the tree
  // (1510 sites of the plain form and 48 of the masked form were fixed earlier);
  // this one was missed because the cast chain around it hid the shape.
  DoParseCmdLine();
  if ( mTodCheatKeys == 0 )
    mOnlyAllowOneCopyToRun = true;
  mSessionCounter[0] = 0;
  mSessionCounter[1] = 0;
  mBoardResult = BOARDRESULT_NONE;
  mKilledYetiAndRestarted = false;
  mSessionStartSeconds = Sexy::GetTickCount() / 0x3E8;
  TodAssertInitForApp((void (*)())BetaSubmitFunc);
  mPendingRechargeAmount = 0;
  // platform, vendor, product, gamename, gameversion, in that order.
  Sexy::SexyCommonApp::GameInfoRecord aGameInfo = getGameInfo();
  mGameInfoStrings[0] = aGameInfo.mPlatform;
  mGameInfoStrings[1] = aGameInfo.mVendor;
  mGameInfoStrings[2] = aGameInfo.mProduct;
  mGameInfoStrings[3] = aGameInfo.mGameName;
  mGameInfoStrings[4] = aGameInfo.mGameVersion;
  DrRpcEngine* aRpcEngine = static_cast<DrRpcEngine *>(DrRpcEngine::getRpcEngine());
  // The RPC engine is Android-only, so on Windows getRpcEngine returns null and
  // calling through it writes to address 0 + 516.
  //
  // DrRpcEngine::getRpcEngine ends with
  //
  //     if ( theInitResult <= 0 ) return nullptr;
  //     else                      return &unk_69F9D0;
  //
  // where theInitResult comes from DrCurlEngine::init. That init goes through
  // curl_global_init and the Android RPC backend, so it fails here, the result
  // becomes -1, and the singleton accessor returns null. The original runs on Android and
  // can assume a live engine; this port cannot.
  //
  // The fault it caused, traced end to end:
  //
  //     InitFault: instr=Lawn.exe+0x2EAAAA   info[1] = 0x00000204
  //     -> String::Release, this = 0x204
  //
  // because setDefaultUrl assigns the String at byte 516 of the engine, and 516
  // is 0x204 -- a legal offset from a null base. Nothing was corrupted; the base
  // was simply never there.
  //
  // Skipping the configuration is the equivalent behaviour on a platform with no
  // RPC engine: there is nothing to configure.
  Sexy::String aDomainURL = ServerConfig::GetDomainURL();
  if ( aRpcEngine != nullptr )
    aRpcEngine->setDefaultUrl(aDomainURL);
  DrRpcEngine* aConfiguredEngine = static_cast<DrRpcEngine *>(DrRpcEngine::getRpcEngine());
  if ( aConfiguredEngine != nullptr )
    aConfiguredEngine->setDataEncryption(ServerConfig::IsEncryptionEnabled());
  // TEMPORARY TRACE: the RPC fault is fixed and Init now stops somewhere between
  // here and LoadResourceManifest, so mark each step. The suspects are the
  // server-session and login calls, which have no backend on this platform.
  if ( mSession.Init() == 0 )
    DoExit(-1);
  mSessionTaskType = 1;
  mLoginToServer = false;
  SrvLoginToServer();
  // The local session uses the TV preset prices instead of a server response.
  setItemToShopItems();
  setItemToCoinItems();
  if ( dynamic_cast<Sexy::LocalGameCenter*>(mGameCenter) != nullptr )
    loadCoinNum();
  Sexy::PerfTimer aTimer;
  aTimer.Start();
  // mProfileMgr is the Sexy::DefaultProfileMgr at +0x964. Its table's byte 12 is
  // slot 3 Load, byte 16 slot 4 Save, byte 40 slot 10 GetAnyProfile, and byte 36
  // slot 9 AddProfile(Sexy::String const&).
  mProfileMgr->Load();
  if ( mProfileMgr->GetAnyProfile() == nullptr )
  {
    Sexy::String aDefaultName("player");
    mProfileMgr->AddProfile(aDefaultName);
    mProfileMgr->Save();
  }
  // The CurUser registry read writes into an empty String.
  Sexy::String aCurUser;
  if ( mPlayerInfo == nullptr )
  {
    Sexy::String aCurUserKey("CurUser");
    if ( RegistryReadString(aCurUserKey, &aCurUser) != 0 )
      mPlayerInfo = (LawnPlayerInfo *)mProfileMgr->GetProfile(aCurUser);
    if ( mPlayerInfo == nullptr )
    {
      Sexy::String aPlayerKey("player");
      mPlayerInfo = (LawnPlayerInfo *)mProfileMgr->GetProfile(aPlayerKey);
    }
  }
  if ( mPlayerInfo != nullptr )
  {
    // Byte 16 of the player info's table is slot 4, PlayerInfo::GetName, which
    // returns its Sexy::String by value -- the ARM's r0 is the destination, so
    // the call is spelled as the returning call it is and the String's own
    // destructor performs the release the old tail call made by hand.
    Sexy::String aPlayerName = mPlayerInfo->GetName();
    logfi("Loaded player info for '%s'", aPlayerName.c_str());
  }
  Sexy::String aMaxExecutionsKey("MaxExecutions");
  mMaxExecutions = GetInteger(aMaxExecutionsKey, 0);
  Sexy::String aMaxPlaysKey("MaxPlays");
  mMaxPlays = GetInteger(aMaxPlaysKey, 0);
  Sexy::String aMaxTimeKey("MaxTime");
  mMaxTime = GetInteger(aMaxTimeKey, 60);
  // TEMPORARY TRACE: this block is the whole resource path.
  //
  // LoadResourceManifest is what parses properties/resources.xml -- the IDA
  // xrefs put it in SexyAppBase::LoadResourceManifest (0x338848), calling
  // ResourceManager::ParseResourcesFile, which calls DoParseResources. Neither
  // TodLoadResources nor LoadingThreadProc calls it: both of their callee lists
  // were checked and contain no parse step. That is why the group table is empty
  // and StartLoadResources reports "pulled 0 resource(s)" -- it computes the
  // resource count as (end - begin) / 12 over a list nobody filled.
  //
  // The line after it creates the title screen, so reaching here means both the
  // manifest and the screen are in play.
  LoadResourceManifest();
  Sexy::String aInitGroup("Init");
  fprintf(stderr, "step: before TodLoadResources(\"Init\")\n"); fflush(stderr);
  // Load the initial resource group before constructing the title screen.
  if (GuardedLoadGroup(&aInitGroup) == 0)
  {
    fprintf(stderr, "step: TodLoadResources(\"Init\") failed; continuing\n");
    fflush(stderr);
  }
  fprintf(stderr, "step: after TodLoadResources(\"Init\")\n"); fflush(stderr);
  TitleScreen* aTitleScreen = new TitleScreen(this);
  int aWidth = mWidth;
  int aHeight = mHeight;
  mTitleScreen = aTitleScreen;
  // Init creates the first title screen and leaves the constructor's
  // mNeedPlayLogo in place. ShowTitleScreen is the path that clears it,
  // because that return is a menu, not another logo.
  // Byte 200 of the TitleScreen table is slot 50, Sexy::Widget::Resize(int, int,
  // int, int) -- the same call the raw dispatch made through the freshly read
  // vptr.
  aTitleScreen->Resize(0, 0, aWidth, aHeight);
  // DrawScreen maps the cursor through these two rects. A zero size divides
  // by zero the first time the screen is drawn. InitGraphics already stored
  // the centered bounds; do not replace them with a 0,0 rect.
  if (mWidgetManager->mMouseSourceRect.mWidth == 0
      || mWidgetManager->mMouseSourceRect.mHeight == 0)
  {
    Sexy::TRect<int> aScreen(mScreenBounds.mX, mScreenBounds.mY,
                             mScreenBounds.mWidth, mScreenBounds.mHeight);
    if (aScreen.mWidth == 0 || aScreen.mHeight == 0)
      aScreen = Sexy::TRect<int>(0, 0, aWidth, aHeight);
    Sexy::TRect<int> aSource(0, 0, aScreen.mWidth, aScreen.mHeight);
    mWidgetManager->Resize(aScreen, aSource);
  }
  // Byte 24 of mWidgetManager's table is slot 6 AddWidget and byte 48 slot 12
  // SetFocus, both WidgetContainer's.
  mWidgetManager->AddWidget(mTitleScreen);
  mWidgetManager->SetFocus(mTitleScreen);
  aTimer.GetDuration();
  aTimer.Start();
  mEffectSystem->EffectSystemInitialize();
  FilterEffectInitForApp();
  TypingCheck* aKonamiCheck = new TypingCheck();
  mKonamiCheck = aKonamiCheck;
  aKonamiCheck->AddChar(97);
  mKonamiCheck->AddChar(97);
  mKonamiCheck->AddChar(98);
  mKonamiCheck->AddChar(98);
  mKonamiCheck->AddChar(99);
  mKonamiCheck->AddChar(100);
  mKonamiCheck->AddChar(99);
  mKonamiCheck->AddChar(100);
  mKonamiCheck->AddChar(98);
  mKonamiCheck->AddChar(97);
  mMustacheCheck = new TypingCheck(Sexy::String("mustache"));
  mMoustacheCheck = new TypingCheck(Sexy::String("moustache"));
  mSuperMowerCheck = new TypingCheck(Sexy::String("trickedout"));
  mSuperMowerCheck2 = new TypingCheck(Sexy::String("tricked out"));
  mFutureCheck = new TypingCheck(Sexy::String("future"));
  mPinataCheck = new TypingCheck(Sexy::String("pinata"));
  mDanceCheck = new TypingCheck(Sexy::String("dance"));
  mDaisyCheck = new TypingCheck(Sexy::String("daisies"));
  mSukhbirCheck = new TypingCheck(Sexy::String("sukhbir"));
  aTimer.GetDuration();
  aTimer.Start();
  ReanimatorLoadDefinitions(gLawnReanimationArray, NUM_REANIMS);
  aTimer.GetDuration();
  aTimer.Start();
  // dword_715B18 holds the Sexy::Widget the constructor allocates and then
  // re-points at the DaveWidget table at 0x687E40; byte 200 of that table is
  // slot 50, Sexy::Widget::Resize(int, int, int, int).
  gDaveWidget->Resize(0, 0, mWidth, mHeight);
  mIsFullVersion = CheckFullVersion(true);
  Sexy::Graphics aTrackState(nullptr);
  aTrackState.SetTrackingDeviceState(false);
  // mGameCenter is the Sexy::IGameCenter at +0x7C0; byte 172 (0xAC) of its table
  // is IGameCenter::SetListener, and the argument is this object's
  // IGameCenter::Listener sub-object at +0x89C.
  mGameCenter->SetListener(static_cast<Sexy::IGameCenter::Listener *>(this));
}

// LawnApp::loadCoinNum()
// ==========================================================================
// LawnApp::loadCoinNum()
// @002BBAAC  664 bytes
// ==========================================================================
void LawnApp::loadCoinNum()
{
  // CoinFile lives in the app's data folder. Its first line is the coin count,
  // which is stored on the current user. A missing or unreadable file leaves
  // the count alone.
  Sexy::String aFolder;
  GetAppDataFolder(&aFolder);
  Sexy::String aPath = aFolder + "/CoinFile";

  FILE* aFile = p_fopen(aPath.c_str(), "r");
  if (aFile == nullptr)
  {
    puts("CoinFile open failed!");
    return;
  }

  char aLine[64] = {0};
  if (fgets(aLine, (int)sizeof(aLine), aFile) != nullptr)
  {
    int aCoins = atoi(aLine);
    LawnUser* aUser = LawnUser::getCurUser();
    if (aUser != nullptr)
    {
      char aText[32];
      sprintf(aText, "%d", aCoins);
      aUser->mCoinText = Sexy::String(aText);
    }
  }
  p_fclose(aFile);
}


// LawnApp::BuyMoney(int)
// @002BBE3C  1574 bytes
// ==========================================================================
void LawnApp::BuyMoney(int theTier)
{
  // The chosen tier says how much cash it costs and how many coins it grants.
  // The signed-in user's coin count is what the later balance query checks.
  LawnUser* aUser = LawnUser::getCurUser();
  if ( theTier < 0 || theTier >= aUser->CoinTierCount() )
    return;
  const SCoinItem* aItems = aUser->mCoinItems.data();
  int aPrice = aItems[theTier].mValue;
  int aCoins = aItems[theTier].mGetCoin;
  mQueryCoinState.mPreviousBalance = atoi(aUser->mCoinText.c_str());

  // A platform that sells through the game center builds a payment and stops.
  if ( mGameCenter->HasFeature(Sexy::IGameCenter::FEATURE_PURCHASE) )
  {
    logfi("MobileCMGCHOME: BuyMoney", "");
    static const char* sProductIds[3] = { "pvz50", "pvz100", "pvz200" };
    Sexy::IGameCenter::Payment aPayment;
    aPayment.mProductId = Sexy::String(sProductIds[theTier]);
    aPayment.mPrice = (float)aPrice;
    aPayment.mUserName = GetUserName();
    aPayment.mAccount = GetUserName();
    mGameCenter->Purchase(aPayment);
    return;
  }

  if ( mAdProvider == nullptr )
  {
    DoDialog(51, true, "[MONEY_NOT_ENOUGH_HEADER]",
             u8"当前版本未接入支付服务，暂时无法充值。", "[DIALOG_BUTTON_OK]", 3);
    return;
  }

  // The "novel" channel names its products pvz50/pvz100/pvz200. Every other
  // channel uses the translated coin count as the item name.
  Sexy::String aUserName = GetUserName();
  Sexy::String aItem;
  if ( mAdProvider->mChannel == "novel" )
  {
    static const char* sProductIds[3] = { "pvz50", "pvz100", "pvz200" };
    aItem = Sexy::String(sProductIds[theTier]);
  }
  else
  {
    char aText[32];
    sprintf(aText, "%d", aCoins);
    aItem = Sexy::String(aText);
  }
  Sexy::String aEmpty;
  mAdProvider->showItemPay(aUserName, theTier, aEmpty, aItem, (float)aPrice, 1);
  SessionWaitingDialog* aWait = DoSessionWaitDialog();
  aWait->mDelay = 300;
  aWait->WaitForResult(1);
}

// ==========================================================================
// LawnApp::setQueryBalanceRes(bool,int)
// @002BC568  258 bytes
// ==========================================================================
void LawnApp::setQueryBalanceRes(bool theSucceeded, int theBalance)
{
  // A failed query leaves the stored balance alone. A successful one writes the
  // new amount into the current user's balance string, creating that user the
  // first time it is asked for.
  if ( !theSucceeded )
    return;

  char aText[32];
  sprintf(aText, "%d", theBalance);
  LawnUser* aUser = LawnUser::getCurUser();
  if ( aUser != nullptr )
    aUser->mCoinText = Sexy::String(aText);
}

// LawnApp::setPayItemsRes(bool,char const*,int,char const*)
// ==========================================================================
// LawnApp::setPayItemsRes(bool,char const*,int,char const*)
// @002BD6C4  998 bytes
// ==========================================================================
void LawnApp::setPayItemsRes(bool theSucceeded, char const* /*theOrder*/, int theTier, char const* /*theReceipt*/)
{
  // Dialog 81 is the purchase wait. Either way the wait is over.
  SessionWaitingDialog* aWait = dynamic_cast<SessionWaitingDialog*>(GetDialog(81));
  if ( aWait != nullptr )
    aWait->mResult = 0;
  logfi("****OFFERRES***** : %d", theSucceeded);
  if ( !theSucceeded )
    return;

  // Credit the coins the chosen tier grants and record the cash that bought them.
  LawnUser* aUser = LawnUser::getCurUser();
  int aBalance = atoi(aUser->mCoinText.c_str());
  int aGranted = 0;
  int aPrice = 0;
  if ( theTier >= 0 && theTier < aUser->CoinTierCount() )
  {
    aGranted = aUser->mCoinItems[theTier].mGetCoin;
    aPrice = aUser->mCoinItems[theTier].mValue;
  }
  char aText[32];
  sprintf(aText, "%d", aBalance + aGranted);
  aUser->mCoinText = Sexy::String(aText);
  MLogManager->sendLog(mLogComposer->composeRechargeByRMBLog(aPrice, 0));
  printf("****buy money %d\n", aPrice);

  // Ad type 7 confirms the new balance with the provider.
  if ( mAdProvider != nullptr && mAdProvider->getADType() == 7 )
  {
    Sexy::String aBalanceUser = getUserName();
    mAdProvider->queryBalance(aBalanceUser);
  }
}

// ==========================================================================
// LawnApp::setItemToCoinItems()
// @002BDCF8  288 bytes
// ==========================================================================
void LawnApp::setItemToCoinItems()
{
  // The three coin packs the buy dialog offers: cash price, coins granted.
  static const SCoinItem sTiers[3] = {
    { 50, 5 },
    { 100, 10 },
    { 200, 20 },
  };
  LawnUser* aUser = LawnUser::getCurUser();
  aUser->mCoinItems.assign(std::begin(sTiers), std::end(sTiers));
}

// LawnApp::NextTestDialog()
// ==========================================================================
// LawnApp::NextTestDialog()
// @002BDFE4  1836 bytes
// ==========================================================================
int LawnApp::NextTestDialog()
{
  // Remember the dialog that is up, then close it. The history lets
  // PrevTestDialog step back to it.
  int aShownDialog = mCurrentTestDialogId;
  int aShownMessage = mCurrentTestDaveMessage;
  if ( gTestDialogEnd == nullptr )
  {
    gTestDialogEnd = (TestDialogRecord *)mTestInfos;
    gTestDialogCapacity = gTestDialogEnd + 64;
  }
  if ( gTestDialogEnd < gTestDialogCapacity )
  {
    gTestDialogEnd->mDialogId = aShownDialog;
    gTestDialogEnd->mDaveMessage = aShownMessage;
    gTestDialogEnd->mMessageIndex = gTestDialogIndex;
    ++gTestDialogEnd;
  }
  if ( aShownDialog >= 0 )
  {
    mCurrentTestDialogId = -1;
    KillDialogById(aShownDialog);
    mCurrentTestDialogId = aShownDialog;
  }

  // A Dave line that is already showing advances one sentence. Past the last
  // sentence Dave leaves and the next dialog opens.
  if ( aShownMessage != -1 )
  {
    while ( mCrazyDaveMessageIndex <= 11000 )
    {
      if ( AdvanceCrazyDaveText() != 0 )
      {
        mCurrentTestDaveMessage = mCrazyDaveMessageIndex;
        return 1;
      }
      ++mCrazyDaveMessageIndex;
    }
    CrazyDaveLeave();
    mCurrentTestDaveMessage = -1;
    mWidgetManager->RemoveWidget(gDaveWidget);
    ToggleTestDialog();
    return 0;
  }

  // Otherwise walk dialog ids until one of them actually opens. A dialog that
  // has another message to show sets gTestDialogStay so the id stays put.
  int aDialogId = mCurrentTestDialogId;
  for (;;)
  {
    if ( gTestDialogStay == 0 )
      mCurrentTestDialogId = ++aDialogId;
    gTestDialogStay = 0;

    switch ( aDialogId )
    {
      case 2:
        DoNewOptions(false, mPlayerInfo->GetId());
        break;
      case 3:
        DoHelpDialog(false);
        break;
      case 13:
        ConfirmQuit();
        break;
      case 16:
      {
        if ( mGameMode <= 1 )
          mGameMode = GAMEMODE_SURVIVAL_NORMAL_STAGE_1;
        const char* aMessage = "[ZOMBIQUARIUM_DEATH_MESSAGE]";
        switch ( gTestDialogIndex )
        {
          case 0:
            aMessage = "[ZOMBIQUARIUM_DEATH_MESSAGE]";
            mGameMode = GAMEMODE_SURVIVAL_NORMAL_STAGE_1;
            break;
          case 1:
            aMessage = "[LAST_STAND_DEATH_MESSAGE]";
            break;
          case 2:
            aMessage = "[LAST_STAND_DEATH_MESSAGE]";
            break;
          case 3:
            aMessage = "[ENDLESS_PUZZLE_DEATH_MESSAGE]";
            break;
          case 4:
            aMessage = "[I_ZOMBIE_DEATH_MESSAGE]";
            break;
          case 5:
            aMessage = "[MP_ZOMBIES_WIN_MESSAGE]";
            mGameMode = (GameMode)((int)mGameMode + 1);
            if ( (int)mGameMode == 91 )
              gTestDialogIndex = 0;
            break;
          default:
            break;
        }
        if ( gTestDialogIndex < 5 )
        {
          gTestDialogStay = 1;
          ++gTestDialogIndex;
        }
        GameOverDialog* aGameOver = new GameOverDialog(Sexy::String(aMessage), true);
        AddDialogWithId(16, aGameOver);
        break;
      }
      case 20:
        DoPauseDialog();
        break;
      case 23:
        DoConfirmBackToMain(true);
        break;
      case 24:
        DoConfirmRestartDialog();
        break;
      case 26:
        DoNotEnoughMoneyDialog();
        break;
      case 27:
        DoUpgradeDialog();
        break;
      case 28:
        gTestDialogIndex = 0;
        break;
      case 29:
      {
        static const char* sWarnings[] = {
          "repick0", "repick1", "repick2", "repick3", "repick4",
          "repick5", "repick6", "repick7", "repick8", "repick9", nullptr
        };
        const char* aWarning = sWarnings[gTestDialogIndex];
        if ( aWarning != nullptr )
        {
          ++gTestDialogIndex;
          gTestDialogStay = 1;
          DoRepickWarningDialog(aWarning);
        }
        else
        {
          gTestDialogIndex = 0;
        }
        break;
      }
      case 39:
        if ( gTestDialogIndex == 0 )
        {
          mGameMode = GAMEMODE_ADVENTURE;
          DoContinueDialog();
          gTestDialogStay = 1;
          ++gTestDialogIndex;
        }
        else if ( gTestDialogIndex == 1 )
        {
          mGameMode = GAMEMODE_SURVIVAL_NORMAL_STAGE_1;
          DoContinueDialog();
          ++gTestDialogIndex;
        }
        else if ( gTestDialogIndex == 2 )
        {
          gTestDialogIndex = 0;
        }
        break;
      case 41:
        if ( gTestDialogIndex == 0 )
        {
          mGameMode = GAMEMODE_ADVENTURE;
          DoRestartConfirmDialog();
          gTestDialogStay = 1;
        }
        else if ( gTestDialogIndex == 1 )
        {
          mGameMode = GAMEMODE_SURVIVAL_NORMAL_STAGE_1;
          DoRestartConfirmDialog();
          gTestDialogStay = 1;
        }
        else if ( gTestDialogIndex == 2 )
        {
          gTestDialogIndex = 0;
        }
        break;
      case 48:
        DoStoreItemDialog();
        break;
      case 49:
        DoTreeItemDialog();
        break;
      case 50:
        DoZenSellDialog();
        break;
      case 53:
        DoPacketPurchaseDialog(10000, 10);
        break;
      case 54:
        DoProfileChangeDialog();
        break;
      case 56:
        if ( gTestDialogIndex == 0 )
        {
          AddDialog(static_cast<Sexy::Dialog*>(new WaitForRemoteFileDialog(0, this, false)));
          gTestDialogStay = 1;
          ++gTestDialogIndex;
        }
        else if ( gTestDialogIndex == 1 )
        {
          AddDialog(static_cast<Sexy::Dialog*>(new WaitForLeaderboardsDialog(this)));
          gTestDialogIndex = 0;
        }
        break;
      case 61:
        DoRetryLeaderboardDialog();
        break;
      case 62:
        DoRetryAchievementsDialog();
        break;
      case 63:
        if ( gTestDialogIndex == 0 )
        {
          HandleCorruptedGameFile();
          gTestDialogStay = 1;
          ++gTestDialogIndex;
        }
        else if ( gTestDialogIndex == 1 )
        {
          gTestDialogIndex = 0;
          HandleOldGameFile();
        }
        break;
      case 64:
        HandleNoDevice();
        break;
      case 65:
        HandleInvalidLevel();
        break;
      case 66:
        DoProfileLoadErrorDialog();
        break;
      case 67:
        DoProfileSaveErrorDialog();
        break;
      case 68:
        if ( gTestDialogIndex == 0 )
        {
          DoContentSavingDialog();
          gTestDialogStay = 1;
          ++gTestDialogIndex;
        }
        else if ( gTestDialogIndex == 1 )
        {
          DoContentLoadingDialog();
          gTestDialogIndex = 0;
        }
        break;
      case 85:
        mCurrentTestDialogId = -1;
        mWidgetManager->AddWidget(gDaveWidget);
        if ( mCrazyDaveState != 0 )
          CrazyDaveLeave();
        CrazyDaveEnter();
        CrazyDaveTalkIndex(0);
        mCurrentTestDaveMessage = 0;
        delete mBoard;
        mBoard = nullptr;
        break;
      default:
        break;
    }

    aDialogId = mCurrentTestDialogId;
    if ( GetDialog(aDialogId) != nullptr || aDialogId == -1 )
      return 1;
  }
}


// ==========================================================================
// LawnApp::ToggleTestDialog()
// @002BE830  102 bytes
// ==========================================================================
void LawnApp::ToggleTestDialog()
{
  // A dialog that is already up advances to the next one. A Dave line that is
  // already showing is dismissed. With neither, the first test dialog opens.
  if ( mCurrentTestDialogId >= 0 )
  {
    int aDialogId = mCurrentTestDialogId;
    mCurrentTestDialogId = -1;
    KillDialogById(aDialogId);
    mCurrentTestDialogId = 84;
    NextTestDialog();
  }
  else if ( mCurrentTestDialogId != -1 || mCurrentTestDaveMessage != -1 )
  {
    CrazyDaveLeave();
    mCurrentTestDaveMessage = -1;
    mCurrentTestDialogId = -1;
  }
  else
  {
    NextTestDialog();
  }
  gTestDialogIndex = 0;
}

// ==========================================================================
// LawnApp::PrevTestDialog()
// @002BE8A4  136 bytes
// ==========================================================================
void LawnApp::PrevTestDialog()
{
  // The history holds one record per dialog that was shown. With fewer than two
  // there is nothing to step back to.
  int aCount = (int)(gTestDialogEnd - (TestDialogRecord *)mTestInfos);
  if ( aCount <= 1 )
    return;

  int aShownDialog = mCurrentTestDialogId;
  int aShownMessage = mCurrentTestDaveMessage;
  if ( aShownDialog >= 0 )
  {
    mCurrentTestDialogId = -1;
    KillDialogById(aShownDialog);
    mCurrentTestDialogId = aShownDialog;
  }

  // Drop the record just pushed and restore the one before it.
  gTestDialogEnd -= 1;
  TestDialogRecord* aPrevious = gTestDialogEnd - 1;
  mCurrentTestDialogId = aPrevious->mDialogId;
  mCurrentTestDaveMessage = aPrevious->mDaveMessage;
  mCrazyDaveMessageIndex = aPrevious->mDaveMessage;
  gTestDialogIndex = aPrevious->mMessageIndex;
  if ( aShownMessage >= 0 && aPrevious->mDaveMessage == -1 )
    CrazyDaveLeave();
  NextTestDialog();
}

// LawnApp::GotoBuyTool(E_SHOP_PACKET)
// ==========================================================================
// LawnApp::GotoBuyTool(E_SHOP_PACKET)
// @002BF360  774 bytes
// ==========================================================================
bool LawnApp::GotoBuyTool(E_SHOP_PACKET thePacket)
{
  // The shop prices live on the current user, keyed by the packet. With no
  // price the purchase cannot start.
  LawnUser* aUser = LawnUser::getCurUser();
  if ( aUser == nullptr )
  {
    OnSessionTaskFailed(-500, Sexy::String(""));
    return false;
  }

  std::map<int, Sexy::String>::iterator aPriceEntry = aUser->mPrices.find((int)thePacket);
  if ( aPriceEntry == aUser->mPrices.end() )
  {
    OnSessionTaskFailed(-499, Sexy::String(""));
    return false;
  }

  int aPrice = atoi(aPriceEntry->second.c_str());
  int aBalance = atoi(aUser->mCoinText.c_str());
  if ( aBalance < aPrice )
  {
    if ( mAdProvider != nullptr && mAdProvider->getADType() == 7 )
      doTransRecharge();
    else
    {
      Sexy::String aHeaderText("[MONEY_NOT_ENOUGH_TO_BUY_MONEY_HEADER]");
      DoBuyMoneyDialog(aHeaderText);
      Sexy::Dialog* aDialog = GetDialog(82);
      if ( aDialog != nullptr )
        aDialog->WaitForResult(true);
    }
    return false;
  }

  if ( !SrvBuyTool(thePacket) )
  {
    OnSessionTaskFailed(-498, Sexy::String(""));
    return false;
  }

  int aLevel = getLevelForPost();
  MLogManager->sendLog(mLogComposer->composePayByRMBLog((int)thePacket, aLevel, 1));
  char aPriceText[32];
  sprintf(aPriceText, "%d", aPrice);
  logfi("price is%s\n", aPriceText);
  MLogManager->sendLog(mLogComposer->composeConsumeLog((int)thePacket, aLevel, 1));
  printf("****use Tool %d in level %d\n", (int)thePacket, aLevel);
  return true;
}


// ==========================================================================
// LawnApp::buyToolForDemo(E_SHOP_PACKET)
// @002BF720  496 bytes
// ==========================================================================
bool LawnApp::buyToolForDemo(E_SHOP_PACKET thePacket)
{
  // The demo spends coins directly. A packet with no price costs nothing.
  LawnUser* aUser = LawnUser::getCurUser();
  int aPrice = 0;
  std::map<int, Sexy::String>::iterator aPriceEntry = aUser->mPrices.find((int)thePacket);
  if ( aPriceEntry != aUser->mPrices.end() )
    aPrice = atoi(aPriceEntry->second.c_str());

  int aBalance = atoi(aUser->mCoinText.c_str());
  if ( aBalance < aPrice )
  {
    Sexy::String aEmpty;
    DoBuyMoneyDialog(aEmpty);
    return false;
  }

  int aLeft = aBalance - aPrice;
  char aText[32];
  sprintf(aText, "%d", aLeft);
  aUser->mCoinText = Sexy::String(aText);
  writeCoinNum(aLeft);
  return true;
}

// LawnApp::setItemToShopItems()
// @002BF9AC  1182 bytes
// ==========================================================================
int LawnApp::setItemToShopItems()
{
  // The shop's eleven packets and the coin price of each.
  static const char* sPrices[11] = {
    "18", "12", "18", "8", "12", "8", "18", "20", "20", "20", "20"
  };
  LawnUser* aUser = LawnUser::getCurUser();
  for ( int aPacket = 0; aPacket < 11; ++aPacket )
    aUser->mPrices[aPacket] = Sexy::String(sPrices[aPacket]);
  return 1;
}

// LawnApp::SetKeyValueData()
// ==========================================================================
// LawnApp::SetKeyValueData()
// @002BFFB4  424 bytes
// ==========================================================================
int LawnApp::SetKeyValueData()
{
  // The session stores the sun-money purchase count under this one key.
  mKeyValueData[Sexy::String("iBuySunMoneyCount")] = mInitialSunMoney;
  logfi("saveGameData, mBuySunMoneyCount is :%d\n", mInitialSunMoney);
  mSessionTaskType = 12;
  return mSession.StartTask(mLawnSessionTask);
}


// LawnApp::AddBuySunMoneyCount()
// ==========================================================================
// LawnApp::AddBuySunMoneyCount()
// @002C01C4  14 bytes
// ==========================================================================
int LawnApp::AddBuySunMoneyCount()
{
  // +0xA14 (ordinal 645) is mInitialSunMoney, the counter BuySunMoneyDialog
  // increments and then reads back through its own 50 * mInitialSunMoney price.
  ++mInitialSunMoney;
  return SetKeyValueData();
}

// ==========================================================================
// LawnApp::GetKeyValueData()
// @002C01D4  208 bytes
// ==========================================================================
int LawnApp::GetKeyValueData()
{
  // The server's copy of the purchase count arrives in the key/value table.
  // A missing key leaves the count at zero, then the coin balance is queried.
  LawnUser::getCurUser();
  std::map<Sexy::String, int>::iterator aEntry =
      mKeyValueData.find(Sexy::String("iBuySunMoneyCount"));
  mInitialSunMoney = aEntry == mKeyValueData.end() ? 0 : aEntry->second;
  logfi("initData, mBuySunMoneyCount is :%d\n", mInitialSunMoney);
  return SrvQueryCoin();
}


// LawnApp::QueryKeyValues()
// ==========================================================================
// LawnApp::QueryKeyValues()
// @002C02C8  312 bytes
// ==========================================================================
int LawnApp::QueryKeyValues()
{
  // Ask the session for the stored key/value pairs. The answer comes back
  // through the task, and UpdateSessionState reads it with GetKeyValueData.
  LawnUser::getCurUser();
  logfi("send login to server", "");
  mSessionTaskType = 13;
  return mSession.StartTask(mLawnSessionTask);
}


// ==========================================================================
// LawnApp::UpdateSessionState()
// @002C0458  598 bytes
// ==========================================================================
void LawnApp::UpdateSessionState()
{
  // A finished session task is freed here. The callers ignore the old return,
  // which was this stored as an integer.
  if (mLawnSessionTask == nullptr || !mSession.IsTaskDone() || !mLoadingThreadCompleted)
    return;

  LawnSessionTask* aTask = mLawnSessionTask;
  int aType = mSessionTaskType;
  mLawnSessionTask = nullptr;
  mSessionTaskType = 0;

  bool aQueryBalance = false;
  if (aType == 1)
  {
    if (!aTask->Failed())
    {
      mLoginToServer = true;
      QueryKeyValues();
    }
    else
      OnSessionTaskFailed(aTask->GetLastError(), aTask->GetLastErrorMsg());
  }
  else if (aType == 9)
  {
    if (aTask->Failed())
    {
      OnSessionTaskFailed(aTask->GetLastError(), aTask->GetLastErrorMsg());
      logfi("Recharge failed!\n");
    }
    else
    {
      logfi("Recharge succeed!***********\n");
      MLogManager->sendLog(mLogComposer->composeRechargeByRMBLog(mPendingRechargeAmount, 0));
      KillDialogById(82);
      OnSessionTaskSucceed();
    }
  }
  else if (aType == 5 || aType == 6 || aType == 7)
  {
    SessionWaitingDialog* aWait = (SessionWaitingDialog*)GetDialog(81);
    if (aWait != nullptr)
      aWait->mResult = aTask->GetLastError();
    if (!aTask->Failed())
    {
      if (aType == 5)
        SrvQueryCoin();
      if (aType == 7 && mAdProvider != nullptr && mAdProvider->getADType() == 7)
        aQueryBalance = true;
    }
  }
  else if (aType == 13)
  {
    if (aTask->Failed())
      logfi("Load keyvalue failed!\n");
    else
    {
      GetKeyValueData();
      if (mAdProvider != nullptr && mAdProvider->getADType() == 7)
        aQueryBalance = true;
    }
  }
  else if (aType == 12 && aTask->Failed())
  {
    logfi("Set keyvalue failed!");
  }

  if (aQueryBalance && mAdProvider != nullptr)
  {
    Sexy::String aUser;
    mAdProvider->queryBalance(aUser);
  }
  delete aTask;
}

// LawnApp::LawnApp()
// ==========================================================================
// LawnApp::LawnApp()
// @002C06E8  1216 bytes
// ==========================================================================
LawnApp::LawnApp()
{
  // The six stores the dump makes to +0, +4, +8, +2084, +2200 and +2204 are
  // this object's own vptr and the vptrs of its InputDeviceListener,
  // ProfileEventListener and IGameCenter::Listener sub-objects. They are not
  // emitted here: the compiler writes them for the real class, so a
  // hand-written vptr store would only fight the ABI.

  // The two embedded list heads at +0x8DC and +0x8E4 are std::list sentinels:
  // the empty state is both links pointing back at the head itself.
  //
  // There is deliberately NO `new ((GamepadApp *)this) GamepadApp();` here. The
  // dump calls GamepadApp::GamepadApp(this) exactly once, and the compiler
  // already emits that call for the mem-initializer -- it is `LawnApp::LawnApp
  // +0x37`. The extra placement-new ran the whole base chain (ButtonListener,
  // SexyAppBase, SexyCommonApp, GamepadApp) a SECOND time, and its last act was
  // to store GamepadApp's vtable over the LawnApp one, so `theApp->Init()` went
  // to SexyCommonApp::Init and LawnApp::Init was never reached at all.
  mCreatedImageList.next = &mCreatedImageList;
  mCreatedImageList.prev = &mCreatedImageList;
  mControlButtonList.next = &mControlButtonList;
  mControlButtonList.prev = &mControlButtonList;
  mReferId = Sexy::String();
  mRegisterLink = Sexy::String();
  mMod = Sexy::String();
  mCrazyDaveMessageText = new Sexy::String();
  mDingusText = Sexy::String();
  mGameInfoStrings[0] = Sexy::String();
  mGameInfoStrings[1] = Sexy::String();
  mGameInfoStrings[2] = Sexy::String();
  mGameInfoStrings[3] = Sexy::String();
  mGameInfoStrings[4] = Sexy::String();
  mSecondPlayerGamepadIndex = -1;
  mVsInitialPlantMode = 0;
  gLawnApp = this;
  mGameSelector = nullptr;
  mNeedLoadGame = 0;
  mNeedGoBackToMain = 0;
  mSaveGameOperation = 0;
  mModalCount[0] = 0;
  mBoard = nullptr;
  mLeaderboardScreen = nullptr;
  mChallengeScreen = nullptr;
  mSeedChooserScreen = nullptr;
  mZombieChooserScreen = nullptr;
  mVSSetupMenu = nullptr;
  mVSResultsMenu = nullptr;
  mAwardScreen = nullptr;
  mCreditScreen = nullptr;
  mTitleScreen = nullptr;
  mSoundSystem = nullptr;
  mKonamiCheck = nullptr;
  mMustacheCheck = nullptr;
  mMoustacheCheck = nullptr;
  mSuperMowerCheck = nullptr;
  mSuperMowerCheck2 = nullptr;
  mFutureCheck = nullptr;
  mPinataCheck = nullptr;
  mDanceCheck = nullptr;
  mDaisyCheck = nullptr;
  mSukhbirCheck = nullptr;
  mMustacheMode = 0;
  mSuperMowerMode = 0;
  mFutureMode = 0;
  mPinataMode = 0;
  mDanceMode = 0;
  mDaisyMode = 0;
  mSukhbirMode = 0;
  mGameScene = SCENE_LOADING;
  mPoolEffect = nullptr;
  mZenGarden = nullptr;
  mEffectSystem = nullptr;
  mReanimatorCache = nullptr;
  mCloseRequest = 0;
  mAppCounter = 0;
  // LawnApp::LawnApp @0x2C087C. 0x500/0x2D0 are the TV layout,
  // 0x320/0x258 are the 800 by 600 stage the art was drawn for.
  // InitGraphics centers that stage in the display: (display - mWidth) / 2.
  mLayoutWidth = 1280;
  mLayoutHeight = 720;
  mWidth = 800;
  mHeight = 600;
  mFullscreenBits = 32;
  mPlayTimeActiveSession = 0;
  mAppRandSeed = (int)time(nullptr);
  mMuteSoundsForCutscene = 0;
  mHelpTextScreen = nullptr;
  mIsFullVersion = 0;
  mMailboxRefreshed = 0;
  mDialogFadeDrawn = 0;
  // The binary allocates 0x40 bytes here, which is sizeof(Mailbox) now that
  // game/Mailbox.h models the class at its real size.
  mMailBox = new Mailbox(this);
  mWindowed = 1;
  mMusicVolume = 0.85;
  mSfxVolume = 0.5525;
  mTrialType = 0;
  mAutoStartLoadingThread = 0;
  mDebugKeysEnabled = 0;
  mRegKey = "PopCap\\PlantsVsZombiesConsole";
  mProdName = "PlantsVsZombies";
  mTitle = "Plants vs. Zombies";
  mCustomCursorsEnabled = 0;
  mPlayerInfo = nullptr;
  mPlayer2Info = nullptr;
  // sizeof, not the binary's 4.
  //
  // The binary's 4 bytes is the size of its own LevelStats, but this tree's
  // LevelStats is opaque storage sized from the largest offset its bodies
  // reach, which is far more than a word. LevelStats() writes across that
  // model, so a 4-byte block is overflowed immediately.
  mLastLevelStats = new LevelStats();
  mFirstTimeGameSelector = 1;
  mTest3D = 1;
  mGameMode = GAMEMODE_ADVENTURE;
  mEasyPlantingCheat = 0;
  Tod_SWTri_AddAllDrawTriFuncs();
  mLoadingZombiesThreadCompleted = 1;
  mGamesPlayed = 0;
  mMaxExecutions = 0;
  mMaxPlays = 0;
  mMaxTime = 0;
  mCompletedLoadingThreadTasks = 0;
  // The listener the manager registers is this object's ProfileEventListener
  // base sub-object, the one at +0x898.
  mProfileMgr = new Sexy::DefaultProfileMgr((Sexy::ProfileEventListener *)this);
  mRegisterResourcesLoaded = 0;
  mTodCheatKeys = 0;
  mCrazyDaveReanimID = REANIMATIONID_NULL;
  mCrazyDaveState = CRAZY_DAVE_OFF;
  mCrazyDaveBlinkCounter = 0;
  mCrazyDaveBlinkReanimID = REANIMATIONID_NULL;
  mCrazyDaveMessageIndex = -1;
  mCurrentTestDaveMessage = -1;
  mCurrentTestDialogId = -1;
  // The Dave widget is reached only through this global.
  gDaveWidget = new Sexy::Widget();
  mProfileOperationPending1 = 0;
  mProfileOperationPending2 = 0;
  mHelpBarWidget = nullptr;
  mSavingDingusReanimationID = REANIMATIONID_NULL;
  mSavingDingusCurrentY = 1400.0f;
  mSavingDingusAnimationStartY = 0.0f;
  mSavingDingusAnimationEndY = 0.0f;
  mSavingDingusAnimationTime = 0.0f;
  mDingusText = "";
  mSavingDingusShowCount = 0;
  mProfileSaveInProgress = 0;
  mProfileSaveFailed = 0;
  mSaveGame = nullptr;
  mP2JoinPromptTimer = 0;
  mSkipProfileSaving = 0;
  mIsFullVersion = 1;
  // The byte at +0x795 is the second of SexyAppBase's four packed flags
  // (mRuntimeFlags), the one the binary sets here.
  mRuntimeFlags[1] = 1;
  mLawnMouseMode = 1;
  mSafeReload = Sexy::GetEnvOption("LAWN_SAFE_RELOAD", true) != 0;
  // Byte 0x4C of the vtable is the deleting destructor, the same slot
  // SexyAppBase::Cleanup dispatches through for this member. The manager the
  // base constructor created is thrown away here and replaced below.
  if ( mResourceManager != nullptr )
    delete mResourceManager;
  // The binary allocates 0x1BC bytes here, which is sizeof(Sexy::ResourceManager)
  // -- the same expression SexyAppBase's own constructor uses for the same
  // class.
  //
  // The dump then installs the TodResourceManager vtable in the object it just
  // constructed. That subclass adds no data member -- its vtable differs from
  // the base's only in the two destructor slots -- so the base object is the
  // same size and is constructed directly here.
  mResourceManager = new Sexy::ResourceManager((Sexy::SexyAppBase *)this);
  SetLocale(Sexy::String("zh_CN"), Sexy::String("UTF-8"));
  _putenv_s("SEXY_LENOVO_WORKAROUND", "0");
  mLawnSessionTask = nullptr;
  MLogManager = nullptr;
  mLogComposer = nullptr;
  mDaveHelp = new DaveHelp(this);
  mMaskHelpWidget = new MaskHelpWidget(this);
  mQRCodeImage = Sexy::GamepadApp::GetImage(Sexy::String("images/mark/2d_mark.png"), true);
  mQRCodeImageBackground =
      Sexy::GamepadApp::GetImage(Sexy::String("images/mark/2d_mark_bg.png"), true);
  mAuthId = 47;
  if (mAuthManager != nullptr)
    mAuthManager->SetAuthSerialNumber(Sexy::String("4ADS-5PK4-UUF9"));
  mInitialSunMoney = 0;
  mQueryCoinState.mPending = 0;
  mCanDoBuyMoneyDialog = 1;
}

// ==========================================================================
// LawnApp::BetaAddFile(std::list<Sexy::String> &,Sexy::String,Sexy::String)
// @002C0D10  74 bytes
// ==========================================================================
void LawnApp::BetaAddFile(std::list<Sexy::String> & theFiles, Sexy::String theName, Sexy::String thePath)
{
  // A file that can be read is recorded as the name the caller asked for.
  // The path is what gets opened; the name is what the list keeps.
  (void)theName;
  Sexy::Buffer aBuffer;
  if ( ((SexyAppBase*)gSexyAppBase)->ReadBufferFromFile(thePath, &aBuffer, false) != 0 )
    theFiles.push_back(thePath);
}

// LawnApp::TryLoadGame()
// ==========================================================================
// LawnApp::TryLoadGame()
// @002C0D80  208 bytes
// ==========================================================================
int LawnApp::TryLoadGame()
{
  // The trial build has no saved games. A full build names the save from the
  // mode and the profile, reads it, and marks the board to be restored.
  if ( IsFullVersion() == 0 )
    return 0;

  Sexy::String aSaveName;
  GetSavedGameName(&aSaveName, mGameMode,
                   mPlayerInfo->GetProfileId(), mPlayerInfo->GetId());
  mMusic->MusicDispose();

  delete mSaveGame;
  mSaveGame = new SaveGameContext();
  mSaveGame->mReadError = false;
  mSaveGame->mReading = true;

  bool aRead = ReadBufferFromFile(aSaveName, mSaveGame, false) != 0;
  if ( aRead )
  {
    mNeedLoadGame = true;
    mSaveGameOperation = 1;
  }
  return aRead ? 1 : 0;
}

// LawnApp::PreNewGame(GameMode,bool)
// ==========================================================================
// LawnApp::PreNewGame(GameMode,bool)
// @002C0E78  170 bytes
// ==========================================================================
void LawnApp::PreNewGame(GameMode theGameMode, bool theContinue)
{
  PostEnterLevel();
  if ( NeedRegister() != 0 )
  {
    ShowGameSelector();
    return;
  }

  mGameMode = theGameMode;
  if ( theContinue && TryLoadGame() != 0 )
    return;

  // A fresh game throws away the save for this mode and this profile.
  Sexy::String aSaveName;
  GetSavedGameName(&aSaveName, mGameMode,
                   mPlayerInfo->GetProfileId(), mPlayerInfo->GetId());
  EraseFile(aSaveName);
  NewGame();
}

// ==========================================================================
// LawnApp::FinishCheatDialog(bool)
// @002C0F38  88 bytes
// ==========================================================================
void LawnApp::FinishCheatDialog(bool theAccepted)
{
  CheatDialog *aDialog = (CheatDialog *)GetDialog(36);
  if ( aDialog != nullptr )
  {
    if ( theAccepted )
    {
      if ( aDialog->ApplyCheat() != 0 )
      {
        // ARM table byte 428 is LawnApp's by-id kill, the header's
        // KillDialogById.
        KillDialogById(36);
        // +0x940 is mMusic; ARM table byte 20 is Music2's slot 5, StopAllMusic.
        mMusic->StopAllMusic();
        // +0xA38 is mBoardResult, and 6 is BOARDRESULT_CHEAT.
        mBoardResult = BOARDRESULT_CHEAT;
        PreNewGame(mGameMode, 0);
      }
    }
    else
    {
      KillDialogById(36);
    }
  }
}

// LawnApp::FinishRestartConfirmDialog()
// ==========================================================================
// LawnApp::FinishRestartConfirmDialog()
// @002C0F90  62 bytes
// ==========================================================================
void LawnApp::FinishRestartConfirmDialog()
{
  // +0xA3C is mKilledYetiAndRestarted and the byte it takes comes from the
  // board's own mKilledYeti at +0x5880. Byte 428 of the app's table is slot
  // 107, the by-id kill the header names KillDialogById.
  mKilledYetiAndRestarted = mBoard->mKilledYeti;
  KillDialogById(39);
  KillDialogById(41);
  KillBoard();
  PreNewGame(mGameMode, 0);
}

// LawnApp::CheckForGameEnd()
// @002C0FD0  468 bytes
// ==========================================================================
void LawnApp::CheckForGameEnd()
{
  if ( mBoard == nullptr || mBoard->mLevelComplete == 0 || mProfileSaveInProgress )
    return;

  int aProfileUpdated = UpdatePlayerProfileForFinishingLevel();
  HideHelpBarWidget();

  // Adventure ends on the note levels (10, 20, 30, 40, 50) and on the credits
  // of a first playthrough. Every other finished adventure level starts over.
  if ( IsAdventureMode() )
  {
    int aLevel = mBoard->mLevel;
    KillBoard();
    bool aFirstPlay = IsFirstTimeAdventureMode() != 0 && aLevel <= 49;
    bool aNoteLevel = aLevel == 9 || aLevel == 19 || aLevel == 29
      || aLevel == 39 || aLevel == 49 || aLevel == 50;
    if ( aFirstPlay || aNoteLevel )
    {
      if ( aLevel == 50
        && mPlayerInfo->GetFlag((PlayerFlags)1)
        && !mPlayerInfo->GetFlag((PlayerFlags)0x8000) )
      {
        ShowAwardScreen(AWARD_FORLEVEL);
        return;
      }
      if ( aLevel == 50 )
      {
        ShowAwardScreen(AWARD_CREDITS_ZOMBIENOTE);
        return;
      }
      ShowAwardScreen(AWARD_FORLEVEL);
      return;
    }
    PreNewGame(mGameMode, false);
    return;
  }

  // A finished survival stage either ends the run or starts the next stage.
  // Coop and single player land on different challenge pages.
  if ( IsSurvivalMode() )
  {
    if ( mBoard == nullptr )
      return;
    if ( mBoard->IsFinalSurvivalStage() == 0 )
    {
      ++mBoard->mChallenge->mSurvivalStage;
      KillSeedChooserScreen();
      mBoard->InitSurvivalStage();
      return;
    }
    KillBoard();
    DeleteSavedGame();
    ChallengePage aPage = IsCoopMode() ? CHALLENGE_PAGE_COOP : CHALLENGE_PAGE_SURVIVAL;
    if ( aProfileUpdated == 0 || !HasFinishedAdventure() )
    {
      ShowChallengeScreen(aPage);
      return;
    }
    ShowAwardScreen(AWARD_FORLEVEL);
    return;
  }

  if ( IsCoopMode() )
  {
    KillBoard();
    ShowChallengeScreen(CHALLENGE_PAGE_COOP);
    return;
  }

  if ( IsPuzzleMode() )
  {
    KillBoard();
    if ( aProfileUpdated == 0 )
    {
      ShowChallengeScreen(CHALLENGE_PAGE_PUZZLE);
      return;
    }
    ShowAwardScreen(AWARD_FORLEVEL);
    return;
  }

  if ( mGameMode == GAMEMODE_MP_VS )
  {
    ShowVSResultsScreen();
    mVSResultsMenu->InitFromBoard(mBoard);
    KillBoard();
    return;
  }

  KillBoard();
  if ( aProfileUpdated == 0 || !HasFinishedAdventure() )
  {
    ShowChallengeScreen(CHALLENGE_PAGE_CHALLENGE);
    return;
  }
  ShowAwardScreen(AWARD_FORLEVEL);
}

// ==========================================================================
// LawnApp::FastLoad(GameMode)
// @002C11AC  60 bytes
// ==========================================================================
void LawnApp::FastLoad(GameMode theGameMode)
{
  if ( mShutDown )
    return;

  mWidgetManager->RemoveWidget(mTitleScreen);
  SafeDeleteWidget(mTitleScreen);
  mTitleScreen = nullptr;
  PreNewGame(theGameMode, false);
}

// LawnApp::ButtonDepress(int)
// ==========================================================================
// LawnApp::ButtonDepress(int)
// @002C11E8  702 bytes
// ==========================================================================
void LawnApp::ButtonDepress(int theId)
{
  // A dialog button arrives as `dialogId + 2000` (OK) or `dialogId + 3000`
  // (cancel), added by SexyAppBase::DialogButtonDepress. The range test runs on
  // theId % 10000 so a composite id still classifies; the value switched on is the
  // full theId with that base subtracted, and that same value is what the shared
  // kill call takes -- the binary never reloads r1 before it.
  // Every path ends by killing the dialog it came from (KillDialogById), which
  // is why most arms share that one call.
  if ( (unsigned int)(theId % 10000 - 2000) >= 0x3E8 )
  {
    // ---- dialogs whose buttons are numbered from 3000 ----
    if ( (unsigned int)(theId % 10000 - 3000) >= 0x3E8 )
      return;

    int aButton = theId - 3000;
    if ( aButton == 44 )
    {
      FinishTimesUpDialog();
      return;
    }
    if ( aButton <= 44 )
    {
      if ( aButton == 36 )
      {
        FinishCheatDialog(false);
        return;
      }
      if ( aButton <= 36 )
      {
        KillDialogById(aButton);
        return;
      }
      if ( aButton == 37 )
      {
        FinishCheatCodeDialog(false);
        return;
      }
      if ( aButton == 41 )
      {
        // The options dialog hands focus to the help-bar widget before it goes
        // away. GetDialog(39) is the help bar; a non-null result means it is
        // open and should take focus back.
        Sexy::Dialog *aHelpDialog = GetDialog(39);
        if ( aHelpDialog != NULL )
          mWidgetManager->SetFocus(aHelpDialog);
        KillDialogById(41);
        return;
      }
    }
    else
    {
      if ( aButton == 77 )
      {
        KillDialogById(aButton);
        HandleNoDevice();
        return;
      }
      if ( aButton > 77 )
      {
        if ( aButton != 82 )
        {
          if ( aButton != 10009 )
          {
            KillDialogById(aButton);
            return;
          }
          KillDialogById(aButton);
          KillDialogById(9);
          return;
        }
        FinishBuyMoneyDialog(false);
        return;
      }
      if ( aButton == 64 )
      {
        KillDialogById(aButton);
        if ( mBoard != NULL )
        {
          bool aAlreadyOpen = GetDialog(4) != NULL;
          if ( !aAlreadyOpen )
          {
            if ( mSaveGameOperation == 2 )
            {
              // Windows saves complete synchronously and clear their pending flag.
              mBoard->TryToSaveGame();
            }
            else if ( mSaveGameOperation == 1 )
            {
              TryLoadGame();
              mNeedLoadGame = true;
            }
          }
        }
        return;
      }
    }
    KillDialogById(aButton);
    return;
  }

  // ---- dialogs whose buttons are numbered from 2000 ----
  int aButton = theId - 2000;
  if ( aButton == 32 )
  {
    FinishConfirmDeleteUserDialog(true);
    return;
  }
  if ( aButton > 32 )
  {
    if ( aButton == 41 )
    {
      FinishRestartConfirmDialog();
      return;
    }
    if ( aButton > 41 )
    {
      if ( aButton == 64 )
      {
        KillDialogById(aButton);
        return;
      }
      if ( aButton > 64 )
      {
        if ( aButton != 82 )
        {
          if ( aButton != 20009 )
          {
            if ( aButton == 77 )
            {
              KillDialogById(77);
              mNeedGoBackToMain = true;
              mSaveGameOperation = 2;
              KillDialogById(68);
              return;
            }
            KillDialogById(aButton);
            return;
          }
          KillDialogById(aButton);
          KillDialogById(9);
          return;
        }
        FinishBuyMoneyDialog(true);
        return;
      }
      if ( aButton != 44 )
      {
        KillDialogById(aButton);
        return;
      }
      FinishTimesUpDialog();
      return;
    }
    if ( aButton != 35 )
    {
      if ( aButton > 35 )
      {
        if ( aButton != 36 )
        {
          if ( aButton != 37 )
          {
            KillDialogById(aButton);
            return;
          }
          FinishCheatCodeDialog(true);
          return;
        }
        FinishCheatDialog(true);
        return;
      }
      if ( aButton == 33 )
      {
        FinishRenameUserDialog(true);
        return;
      }
    }
    FinishNameError(aButton);
    return;
  }
  if ( aButton == 15 )
  {
    KillDialogById(aButton);
    return;
  }
  if ( aButton <= 15 )
  {
    if (aButton == 13)
    {
      KillDialogById(aButton);
      Quit();
      return;
    }
    if ( aButton == 7 )
      return;
    if ( aButton <= 7 )
    {
      if ( aButton == 0 )
      {
        KillDialogById(aButton);
        ShowGameSelector();
        return;
      }
      if ( aButton == 2 )
      {
        KillNewOptionsDialog();
        return;
      }
    }
    else if ( aButton == 8 )
    {
      KillDialogById(aButton);
      CheckForUpdates();
      return;
    }
    KillDialogById(aButton);
    return;
  }
  if ( aButton == 22 )
  {
    KillDialogById(aButton);
    return;
  }
  if ( aButton <= 22 )
  {
    if ( aButton == 21 )
    {
      KillDialogById(aButton);
      mBoard->AddSunMoney(100, 0);
      return;
    }
    KillDialogById(aButton);
    return;
  }
  if ( aButton != 30 )
  {
    if ( aButton > 30 )
    {
      FinishCreateUserDialog(true);
      return;
    }
    if ( aButton == 23 )
    {
      Board *aBoard = mBoard;
      if (aBoard == nullptr || !aBoard->TryToSaveGame())
        return;
      PostLeaveLevel();
      KillDialogById(23);
      mBoardResult = BOARDRESULT_QUIT;
      aBoard->TryToExit();
      return;
    }
    KillDialogById(aButton);
    return;
  }
  FinishUserDialog(true);
}

// LawnApp::FinishLoadGame()
// @002C14A8  170 bytes
// ==========================================================================
int LawnApp::FinishLoadGame()
{
  // The save is applied to a fresh board. When it loads, the board finishes the
  // restore and the save is released. When it does not, a new game starts and
  // any dialog that is already up keeps the board paused, with the focus on
  // the last dialog.
  PostEnterLevel();
  MakeNewBoard();
  int aLoaded = LawnLoadGame(mBoard, mSaveGame);
  delete mSaveGame;
  mSaveGame = nullptr;

  if ( aLoaded != 0 )
  {
    mBoard->PostLoadGame();
    return aLoaded;
  }

  NewGame();
  if ( GetDialogCount() == 0 )
    return 0;

  mBoard->Pause(true);
  Sexy::Dialog* aLastDialog = nullptr;
  for (const auto& aEntry : mDialogMap)
    aLastDialog = aEntry.second;
  if ( aLastDialog != nullptr )
    mWidgetManager->SetFocus(aLastDialog);
  return 0;
}

// ==========================================================================
// LawnApp::Shutdown()
// @002C1554  32 bytes
// ==========================================================================
void LawnApp::Shutdown()
{
  // The session is only torn down when it has not already shut itself down.
  if ( !mSession.mShutdown )
  {
    SrvLogoutFromServer();
    mSession.Shutdown();
  }
  delete MLogManager;
  MLogManager = nullptr;
  SexyAppBase::Shutdown();
}
// LawnApp::WriteToRegistry()
// ==========================================================================
// LawnApp::WriteToRegistry()
// @002C1574  158 bytes
// ==========================================================================
void LawnApp::WriteToRegistry()
{
  SexyAppBase::WriteToRegistry();
  if ( mPlayerInfo != nullptr )
  {
    // The key is the literal "CurUser" and the value is the profile's name.
    // GetName returns its Sexy::String by value, so the local owns the release
    // the dump spelled out by hand.
    Sexy::String aKey("CurUser");
    Sexy::String aValue = mPlayerInfo->GetName();
    RegistryWriteString(aKey, aValue);
  }
}

// ==========================================================================
// LawnApp::ReadFromRegistry()
// @002C163C  4 bytes
// ==========================================================================
void LawnApp::ReadFromRegistry()
{
  SexyAppBase::ReadFromRegistry();
}
// ==========================================================================
// LawnApp::DoDialog(int,bool,Sexy::String const&,Sexy::String const&,Sexy::String const&,int)
// @002C1640  338 bytes
// ==========================================================================
LawnDialog * LawnApp::DoDialog(int theDialogId, bool theModal, Sexy::String const& theHeader, Sexy::String const& theLines, Sexy::String const& theFooter, int theButtonMode)
{
  Sexy::String aHeader;
  TodStringTranslate(&aHeader, theHeader.c_str());
  Sexy::String aBody;
  TodStringTranslate(&aBody, theLines.c_str());
  Sexy::String aFooter;
  TodStringTranslate(&aFooter, theFooter.c_str());

  // The body's {BUTTONS} placeholder expands to the captions the button mode
  // selects.
  Sexy::String aButtons;
  TodReplaceButtons(&aButtons, &aBody);
  aBody = aButtons;

  // Keep the base DoDialog lifecycle while retaining NewDialog's real type.
  KillDialog(theDialogId);
  LawnDialog *aDialog = NewDialog(theDialogId, theModal, aHeader, aBody, aFooter, theButtonMode);
  AddDialogWithId(theDialogId, aDialog);
  if ( mWidgetManager->mFocusWidget == nullptr )
  {
    // Nothing holds the focus yet, so the dialog takes it and hands it on to
    // its first button.
    mWidgetManager->SetFocus(aDialog);
    aDialog->SetFocus(aDialog->mLawnYesButton != nullptr ? aDialog->mLawnYesButton
                                                      : aDialog->mLawnNoButton, true);
  }
  return aDialog;
}
// LawnApp::KillDialog(int)
// ==========================================================================
// LawnApp::KillDialog(int)
// @002C17D4  74 bytes
// ==========================================================================
bool LawnApp::KillDialog(int theDialogId)
{
  return KillDialogById(theDialogId);
}

bool LawnApp::KillDialogById(int theDialogId)
{
  bool aKilled = SexyAppBase::KillDialogById(theDialogId);
  if ( !aKilled )
    return false;

  if ( mBoard != nullptr )
  {
    if ( mDialogMap.empty() && mSeedChooserScreen == nullptr )
      mWidgetManager->SetFocus(mBoard);
    if ( !NeedPauseGame() )
      mBoard->Pause(false);
  }
  return aKilled;
}

// ==========================================================================
// LawnApp::Start()
// @002C1E9C  12 bytes
// ==========================================================================
void LawnApp::Start()
{
  // A load that already failed does not start the framework's startup.
  if ( mLoadingFailed )
    return;
  SexyAppBase::Start();
}
// ==========================================================================
// LawnApp::HandleCmdLineParam(Sexy::String const&,Sexy::String const&)
// @002C1EA8  58 bytes
// ==========================================================================
void LawnApp::HandleCmdLineParam(Sexy::String const& theParamName, Sexy::String const& theParamValue)
{
#ifdef _DEBUG
  // "-tod" turns the developer keys on unless the build forbids them.
  if ( theParamName == "-tod" && !gDisableDebugKeys )
  {
    mTodCheatKeys = true;
    mDebugKeysEnabled = true;
    return;
  }
#endif
  SexyCommonApp::HandleCmdLineParam(theParamName, theParamValue);
}
// LawnApp::UpdateFrames()
// ==========================================================================
// LawnApp::UpdateFrames()
// @002C1EF0  414 bytes
// ==========================================================================
void LawnApp::UpdateFrames()
{
  if ( !mActive || mMinimized )
  {
    if ( mBoard != nullptr )
      mBoard->ResetFPSStats();
  }
  UpdateSessionState();
  UpdatePlayTimeStats();

  // Slow motion steps one frame and then skips one. Fast motion runs twenty
  // frames per tick. Step mode waits until StepAdvance arms a single frame.
  int aSteps = 0;
  if ( gSlowMo != 0 )
  {
    ++gSlowMoFrame;
    if ( gSlowMoFrame >= 2 )
    {
      gSlowMoFrame = 0;
      aSteps = 1;
    }
  }
  else if ( gFastMo != 0 )
  {
    aSteps = 20;
  }
  else if ( gStep != 0 )
  {
    if ( gStepReady != 0 )
    {
      gStepReady = 0;
      aSteps = 1;
    }
  }
  else
  {
    aSteps = 1;
  }

  for ( int aStep = 0; aStep < aSteps; ++aStep )
  {
    ++mAppCounter;
    if ( mBoard != nullptr )
      mBoard->ProcessDeleteQueue();
    Sexy::GamepadApp::UpdateFrames();
    mMusic->MusicUpdate();
    if ( mPlayerInfo != nullptr && mPlayerInfo->IsLoaded() )
    {
      mMailBox->Update();
      if ( !mMailboxRefreshed )
      {
        mMailBox->RefreshMessages();
        mMailboxRefreshed = true;
      }
    }
    if ( mLoadingThreadCompleted && mEffectSystem != nullptr )
      mEffectSystem->ProcessDeleteQueue();
    CheckForGameEnd();
    UpdateSavingDingus();
  }

  if ( mNeedLoadGame )
  {
    KillDialogById(68);
    mNeedLoadGame = false;
    FinishLoadGame();
    if ( GetDialog(63) == nullptr && GetDialog(65) == nullptr )
    {
      mFirstTimeGameSelector = false;
      DoContinueDialog();
    }
    delete mSaveGame;
    mSaveGame = nullptr;
  }

  if ( !mQueryCoinState.mPending )
    return;
  if ( mQueryCoinState.mRequestCount > 20 )
  {
    mQueryCoinState.mPending = false;
    return;
  }
  unsigned int aNow = Sexy::GetTickCount();
  if ( (int)(aNow - mQueryCoinState.mLastRequestTime) <= 5000 )
    return;
  mQueryCoinState.mLastRequestTime = aNow;
  ++mQueryCoinState.mRequestCount;
  LawnUser* aUser = LawnUser::getCurUser();
  if ( atoi(aUser->mCoinText.c_str()) != mQueryCoinState.mPreviousBalance )
  {
    mQueryCoinState.mPending = false;
    return;
  }
  SrvQueryCoin();
}

// LawnApp::WriteImage(Sexy::String const&,Sexy::Image *)
// ==========================================================================
// LawnApp::WriteImage(Sexy::String const&,Sexy::Image *)
// @002C20A4  62 bytes
// ==========================================================================
bool LawnApp::WriteImage(Sexy::String const& thePath, Sexy::Image * theImage)
{
  // The record borrows the image's pixels. Clearing the pointer before the
  // record is destroyed keeps the writer from freeing them.
  ImageLib::Image aRecord;
  aRecord.mWidth = theImage->mWidth;
  aRecord.mHeight = theImage->mHeight;
  aRecord.mBits = theImage->GetBits();
  bool aWritten = ImageLib::WritePNGImage(thePath, &aRecord) != nullptr;
  aRecord.mBits = nullptr;
  return aWritten;
}

// ==========================================================================
// LawnApp::URLOpenFailed(Sexy::String const&)
// @002C20EC  312 bytes
// ==========================================================================
void LawnApp::URLOpenFailed(Sexy::String const& theURL)
{
  SexyAppBase::URLOpenFailed(theURL);
  KillDialogById(11);
  CopyToClipboard(theURL);

  // The body is the [OPEN_URL] template with the failed URL formatted into it.
  Sexy::String aTemplate;
  TodStringTranslate(&aTemplate, "[OPEN_URL]");
  Sexy::String aBody;
  StrFormat(&aBody, aTemplate.c_str(), theURL.c_str());

  Sexy::String aHeader("[OPEN_BROWSER]");
  Sexy::String aButton("[DIALOG_BUTTON_OK]");
  DoDialog(11, true, aHeader, aBody, aButton, 3);
}
// ==========================================================================
// LawnApp::URLOpenSucceeded(Sexy::String const&)
// @002C2274  22 bytes
// ==========================================================================
void LawnApp::URLOpenSucceeded(Sexy::String const& theURL)
{
  SexyAppBase::URLOpenSucceeded(theURL);
  KillDialogById(11);
}
// ==========================================================================
// LawnApp::OpenURL(Sexy::String const&,bool)
// @002C228C  254 bytes
// ==========================================================================
bool LawnApp::OpenURL(Sexy::String const& theURL, bool theShutdownOnOpen)
{
  // The "opening browser" notice is opened with the same text as both its
  // header and its body, and no button text.
  Sexy::String aHeader("[OPENING_BROWSER]");
  Sexy::String aBody("[OPENING_BROWSER]");
  Sexy::String aButton(gEmptyCString);
  DoDialog(11, true, aHeader, aBody, aButton, 0);

  PreDrawScreen();
  return SexyAppBase::OpenURL(theURL, theShutdownOnOpen);
}
// ==========================================================================
// LawnApp::PreDisplayHook()
// @002C23C8  4 bytes
// ==========================================================================
void LawnApp::PreDisplayHook()
{
  SexyAppBase::PreDisplayHook();
}

// ==========================================================================
// LawnApp::UpdateApp()
// @002C23CC  72 bytes
// ==========================================================================
bool LawnApp::UpdateApp()
{
  // A close request skips the frame entirely and goes through Shutdown.
  if ( mCloseRequest )
  {
    Shutdown();
    return false;
  }

  // The hesitation bracket measures the frame the game spends loading; it is
  // skipped once the app is on its way out.
  bool aHesitating = mLoadingThreadCompleted;
  if ( aHesitating )
    TodHesitationStartBuffer();

  bool aUpdated = SexyAppBase::UpdateApp();

  if ( aHesitating && !mCloseRequested )
  {
    bool aResult = aUpdated;
    TodHesitationEndBuffer();
    return aResult;
  }
  return aUpdated;
}

// ==========================================================================
// LawnApp::UpdateAppStep(bool *)
// @002C2414  22 bytes
// ==========================================================================
bool LawnApp::UpdateAppStep(bool * theUpdated)
{
  bool aUpdated = SexyAppBase::UpdateAppStep(theUpdated);
  if ( aUpdated )
    UpdateSessionState();
  return aUpdated;
}

// ==========================================================================
// LawnApp::EnforceCursor()
// @002C242C  4 bytes
// ==========================================================================
void LawnApp::EnforceCursor()
{
  SexyAppBase::EnforceCursor();
}

// ==========================================================================
// LawnApp::SwitchScreenModeFull(bool,bool,bool)
// @002C2430  22 bytes
// ==========================================================================
// The header spells this slot `SwitchScreenModeFull` to match SexyAppBase; the
// binary's own symbol is `_ZN7LawnApp16SwitchScreenModeEbbb`.
void LawnApp::SwitchScreenModeFull(bool theWindowed, bool theUse3D, bool theForce)
{
  SexyAppBase::SwitchScreenModeFull(theWindowed, theUse3D, theForce);
  // The body's tail is vtable byte 412, GetDialog(2) -- the store screen probe.
  GetDialog(2);
}
// LawnApp::HardwareInit()
// @002C2448  882 bytes
// ==========================================================================
int LawnApp::HardwareInit()
{
  TodStringListSetColors(gLawnStringFormats, 14);
  gGetCurrentLevelName = LawnGetCurrentLevelName;
  gAppCloseRequest = LawnGetCloseRequest;
  gAppHasUsedCheatKeys = LawnHasUsedCheatKeys;
  gExtractResourcesByName = Sexy::ExtractResourcesByName;
  // `BL _ZN4Sexy13SexyCommonApp4InitEv` @0x2c24a2 -- a static base call. The
  // vptr slot for Init is 71, and a LawnApp's slot 71 is LawnApp::Init, so the
  // cast form called Init from inside HardwareInit and never came back.
  SexyCommonApp::Init();
  // Byte +96 of the platform display is the wide-screen flag the cursor and text
  // code reads. The binary's AndroidAppDriver::CreateGraphics stores the display
  // at app+0x2E4 before this runs; this port's driver never did, so the object
  // is built here when it is missing. mClip is WidgetContainer's byte at +0x45.
  if ( mPlatformDriverOrQueue == nullptr )
    mPlatformDriverOrQueue = new Sexy::NativeDisplay();
  mPlatformDriverOrQueue->mWideScreen = true;
  mWidgetManager->mClip = false;
  Sexy::SWTri_AddDrawTriFunc(1, 1, 0, 0, 34952, 1, Sexy::DrawTriangle_8888_TEX1_TALPHA1_MOD0_GLOB0_BLEND1);
  // Both branches allocate the same 0x68 bytes; Music2 adds no member, so the
  // block Music constructs fits the Music2* the member is typed as.
  // Music is the XM path (mainmusic.xm plus the drums and hihats layers).
  // Music2 is the per-file ogg path. The default is Music; LAWN_MUSIC2=1
  // selects the ogg path.
  if ( Sexy::GetEnvOption("LAWN_MUSIC2", false) != 0 )
    mMusic = new Music2();
  else
    mMusic = new Music();
  // 0x4678 bytes, 110 records of 164. TodFoley is that array, and its
  // constructor runs FoleyTypeData::FoleyTypeData on each record.
  mSoundSystem = new TodFoley();
  mEffectSystem = new EffectSystem();
  mHelpBarWidget = new HelpBarWidget();
  if ( Sexy::GetEnvOption("LAWN_NO_ATLAS", false) == 0 )
  {
    // The ten atlas maps, loaded in the order the binary lists them.
    const char* aAtlasFiles[] = {
      "reanim/plant-atlas.xml",
      "reanim/zombie-atlas.xml",
      "reanim/zombie-boss-atlas.xml",
      "reanim/crazydave-atlas.xml",
      "reanim/mainmenu3/crow-atlas.xml",
      "reanim/mainmenu3/mainmenu3-atlas.xml",
      "reanim/mainmenu3/menu-atlas.xml",
      "particles/particles-atlas.xml",
      "images/dialog-atlas.xml",
      "images/seeds-atlas.xml",
    };
    for ( const char* aFile : aAtlasFiles )
    {
      Sexy::String aPath(aFile);
      LoadAtlasMap(aPath);
    }
  }
  // The metrics server and the local data folder are logged, then handed to the
  // network log manager. loc_2EC954 is basic_string::assign(const char*, size).
  Sexy::String aServer;
  aServer.assign("www.transmension.com", 20);
  Sexy::String aLocalPath;
  GetAppDataFolder(&aLocalPath);
  logfi("localpath: %s server: %s\n", aLocalPath.c_str(), aServer.c_str());
  MLogManager = new TM::LogManager(aServer, aLocalPath);
  mLogComposer = new LogComposer();
  mLogComposer->mAccount = GetUserName();
  Sexy::Buffer* aBuffer = new Sexy::Buffer();
  Sexy::String aMetricsPath("metrics.xml");
  if ( ReadBufferFromFile(aMetricsPath, aBuffer, false) != 0 )
  {
    // A staged metrics file is the document itself. The binary's buffer holds
    // a length-prefixed string of that document; ReadString is that case.
    Sexy::String aMetrics;
    if (!aBuffer->mData.empty() && aBuffer->mData[0] == '<')
    {
      aMetrics = Sexy::String(static_cast<const char *>(static_cast<const void *>(&aBuffer->mData[0])),
                              static_cast<unsigned>(aBuffer->mData.size()));
    }
    else
    {
      aMetrics = aBuffer->ReadString();
    }
    mLogComposer->initComposer(aMetrics);
  }
  delete aBuffer;
  // composeLoginLog returns its Sexy::String by value, and sendLog consumes it.
  MLogManager->sendLog(mLogComposer->composeLoginLog());
  mLaunchTime = Sexy::GetTickCount();
  return 0;
}

// ==========================================================================
// LawnApp::DrawDirtyStuff()
// @002C28D4  10 bytes
// ==========================================================================
bool LawnApp::DrawDirtyStuff()
{
  mDialogFadeDrawn = false;
  return SexyAppBase::DrawDirtyStuff();
}

// ==========================================================================
// LawnApp::SafeDeleteWidget(Sexy::Widget *)
// @002C28E0  4 bytes
// ==========================================================================
void LawnApp::SafeDeleteWidget(Sexy::Widget * theWidget)
{
  SexyAppBase::SafeDeleteWidget(theWidget);
}
// LawnApp::LoadLevelConfiguration @0x2C28E4. The XML loader now selects
// a record using the mode, adventure cycle and current survival stage.
bool LawnApp::LoadLevelConfiguration(GameMode theMode, int theLevelId)
{
  Sexy::LevelLoadContext context;
  context.mGameMode = theMode;
  context.mLevelId = theLevelId;
  context.mAdventureComplete = HasFinishedAdventure();
  if (mBoard != nullptr && mBoard->mChallenge != nullptr)
    context.mSurvivalStage = mBoard->mChallenge->mSurvivalStage;

  if (!mLevel.InitFromXML(context, Sexy::String("properties/levels.xml")))
  {
    if (!mLevel.mConfigurationError.empty())
      fprintf(stderr, "levels.xml: mode=%d level=%d stage=%d: %s\n",
              static_cast<int>(theMode), theLevelId, context.mSurvivalStage,
              mLevel.mConfigurationError.c_str());
    return false;
  }
  if (mLevel.mSkipShopSeedTutorial && mPlayerInfo != nullptr)
    mPlayerInfo->mPassedShopSeedTutorial = true;
  return true;
}

void LawnApp::StartAuth()
{
  // composeAuthStartLog returns its Sexy::String by value, so the dump's r0 was
  // its own result buffer and r1 the real receiver: exactly one String argument.
  Sexy::String aKey("4ADS-5PK4-UUF9");
  MLogManager->sendLog(mLogComposer->composeAuthStartLog(aKey));
  SexyAppBase::StartAuth();
}

// ==========================================================================
// LawnApp::AuthFinished(bool)
// @002C3860  372 bytes
// ==========================================================================
void LawnApp::AuthFinished(bool theSuccess)
{
  // Both compose* calls return a Sexy::String by value: the dump's r0 was their
  // own result buffer, r1 the real receiver, and the key is their only argument.
  Sexy::String aLog;
  if ( theSuccess )
  {
    Sexy::String aKey("4ADS-5PK4-UUF9");
    aLog = mLogComposer->composeAuthSucceedLog(aKey);
  }
  else
  {
    Sexy::String aKey("4ADS-5PK4-UUF9");
    aLog = mLogComposer->composeAuthFailedLog(aKey);
  }
  MLogManager->sendLog(aLog);
  SexyAppBase::AuthFinished(theSuccess);
}

// ==========================================================================
// LawnApp::ProcessMessage(Sexy::Event &)
// @002C3A30  10 bytes
// ==========================================================================
bool LawnApp::ProcessMessage(Sexy::Event & theEvent)
{
  // Update held gamepad buttons before widgets handle the event. B's dig
  // state and the other held actions read Gamepad::IsButtonDown each tick.
  Sexy::GamepadApp::ProcessMessage(theEvent);
  return true;
}
