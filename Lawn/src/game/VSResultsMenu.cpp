// VSResultsMenu -- reconstructed from libGameMain.so.
//
// The bodies below are the binary's own code, rewritten in ordinary C++:
//
//   * every `this + N` word access is the named member of VSResultsMenu.h;
//   * the hand-written vtable-slot calls are real calls -- `FindWidget` for the
//     byte-36 slot, `SetFocus(widget, true)` for byte 228, `Exit` for byte 496
//     (MenuWidget::Exit is what runs OnExit);
//   * the constructor no longer stores recon vtable addresses (the vptr is the
//     compiler's) and initialises the base through the initialiser list;
//   * the libstdc++ COW-string refcount dance the dump spells out at every
//     temporary (`v - 12`, `v - 4`, `__ldrex`/`__strex`, `operator delete`) is
//     gone: `Sexy::String` owns its storage and the compiler emits the same
//     release;
//   * `msPlayerRecords` is the 2x5 table the binary keeps at
//     _ZN13VSResultsMenu15msPlayerRecordsE (0x69E8DC), so the `dword_69E8xx`
//     globals that alias into it are gone.
//
// Signature corrections against the binary, all mirrored in VSResultsMenu.h:
// AddedToManager/Update/Draw/DrawOverlay/GotFocus/OnExit/ButtonDepress all
// return void (they override void slots), KeyDown returns int, and
// ClearPlayerRecords is static -- its only call site
// (LawnApp::ShowMainMenuScreen @0x2B9DC6) passes no receiver.

#include "VSResultsMenu.h"
#include "game/Board.h"
#include "DefaultProfileMgr.h"
#include "Font.h"
#include "Gamepad.h"
#include "GamepadControls.h"
#include "Graphics.h"
#include "Image.h"
#include "ImageWidget.h"
#include "LawnApp.h"
#include "LawnMenuWidget.h"
#include "MenuWidget.h"
#include "PlayerInfo.h"
#include "TodParticleSystem.h"
#include "Widget.h"
#include "WidgetManager.h"


// ==========================================================================
// VSResultsMenu::AddedToManager(Sexy::WidgetManager *)
// @00190C68  30 bytes
// ==========================================================================
void VSResultsMenu::AddedToManager(Sexy::WidgetManager * theWidgetManager)
{
  MenuWidget::AddedToManager(theWidgetManager);
  ((LawnApp *)gLawnApp)->mMusic->MakeSureMusicIsPlaying(MUSIC_TUNE_CHOOSE_YOUR_SEEDS);
}

// ==========================================================================
// VSResultsMenu::KeyDown(Sexy::KeyCode)
// @00190C90  12 bytes
// ==========================================================================
int VSResultsMenu::KeyDown(Sexy::KeyCode theKey)
{
  if ( theKey == Sexy::KEYCODE_ESCAPE )
    return 1;
  return Widget::KeyDown(theKey);
}

// ==========================================================================
// VSResultsMenu::~VSResultsMenu()
// @00190C9C  34 bytes
// ==========================================================================
// The binary's body stores the two vptrs and calls the base destructor; with a
// real base chain the compiler emits both, so nothing is left to write.
VSResultsMenu::~VSResultsMenu()
{
}

// ==========================================================================
// VSResultsMenu::GotFocus()
// @00190CEC  62 bytes
// ==========================================================================
void VSResultsMenu::GotFocus()
{
  MenuWidget::GotFocus();
  Sexy::Widget *aPlayAgain = FindWidget(VSResultsMenu_Play_Again);
  if ( mFocusedChildWidget == nullptr && aPlayAgain != nullptr )
    SetFocus(aPlayAgain, true);
  ((LawnApp *)gLawnApp)->ForceHideHelpBarWidget();
}

// ==========================================================================
// VSResultsMenu::Update()
// @00190D34  226 bytes
// ==========================================================================
void VSResultsMenu::Update()
{
  MenuWidget::Update();

  // The widget shakes while mUpdateCounter counts down; mSmokeCounter is the
  // delay before the winner's smoke starts, and it only runs once the sparkle
  // counter has expired.
  bool aShake = false;
  if ( mSparkleCounter > 0 )
  {
    --mSparkleCounter;
  }
  else if ( ++mSmokeCounter == 50 )
  {
    // The smoke emitter is created for every record that has a winner, and each
    // one overwrites mSmokeParticleID, so only the last survives.
    for ( int i = 0; i < 2; ++i )
    {
      if ( msPlayerRecords[i][0] == -1 )
        continue;
      TodParticleSystem *aSmoke = (TodParticleSystem *)((LawnApp *)gLawnApp)->AddTodParticle(
          144.0f, 159.0f, 0, (ParticleEffect)107);
      mSmokeParticleID = ((LawnApp *)gLawnApp)->ParticleGetID(aSmoke);
    }
    ((LawnApp *)gLawnApp)->PlaySample(SOUND_BIGTHUD, true);
    mUpdateCounter = 24;
    aShake = true;
  }

  if ( !aShake )
  {
    if ( mUpdateCounter > 0 )
    {
      --mUpdateCounter;
      aShake = true;
    }
    else
    {
      mX = 0;
      mY = 0;
    }
  }

  if ( aShake )
  {
    mX = Rand(3) - 1;
    mY = Rand(3) - 1;
  }

  TodParticleSystem *aSparkle = (TodParticleSystem *)((LawnApp *)gLawnApp)->ParticleTryToGet(mSparkleParticleID);
  if ( aSparkle != nullptr )
    aSparkle->Update();
  TodParticleSystem *aSmokeParticle = (TodParticleSystem *)((LawnApp *)gLawnApp)->ParticleTryToGet(mSmokeParticleID);
  if ( aSmokeParticle != nullptr )
    aSmokeParticle->Update();
}

// ==========================================================================
// VSResultsMenu::OnExit()
// @00190E28  58 bytes
// ==========================================================================
void VSResultsMenu::OnExit()
{
  if ( mResultsButtonId == VSResultsMenu_Play_Again )
    ((LawnApp *)gLawnApp)->PreNewGame(GAMEMODE_MP_VS, false);
  else if ( mResultsButtonId == VSResultsMenu_Quit_VS )
    ((LawnApp *)gLawnApp)->ShowMainMenuScreen();
  ((LawnApp *)gLawnApp)->KillVSResultsScreen();
}

// ==========================================================================
// VSResultsMenu::ButtonDepress(int)
// @00190E6C  22 bytes
// ==========================================================================
// The binary tail-calls slot 124, MenuWidget::Exit, which is what runs OnExit.
void VSResultsMenu::ButtonDepress(int theId)
{
  if ( mIsFading )
    return;
  mResultsButtonId = theId;
  Exit();
}

// ==========================================================================
// VSResultsMenu::Draw(Sexy::Graphics *)
// @00190E8C  154 bytes
// ==========================================================================
void VSResultsMenu::Draw(Sexy::Graphics * theGraphics)
{
  MenuWidget::Draw(theGraphics);

  Sexy::String aText;
  TodStringTranslate(&aText, "[VS_RESULTS]");
  Color aColor(220, 220, 220);
  TodDrawString(theGraphics, aText, 400, 50, FONT_HOUSEOFTERROR28, aColor, 2);
  DeferOverlay(0);
}

// ==========================================================================
// VSResultsMenu::VSResultsMenu()
// @00190F44  248 bytes
// ==========================================================================
VSResultsMenu::VSResultsMenu()
  : LawnMenuWidget("menu/VSResultsMenu.txt")
{
  mBackdropReanimID = REANIMATIONID_NULL;
  mCameraOffsetX = 0.0f;
  mCameraOffsetY = 0.0f;
  mSparkleCounter = 250;
  mSmokeCounter = 0;
  mResultsButtonId = -1;
  mTrophyPosX = 0.0f;
  mTrophyPosY = 0.0f;
  mSparkleParticleID = ParticleSystemID(0);
  mSmokeParticleID = ParticleSystemID(0);
  mUpdateCounter = 0;
  mClip = false;
}

// ==========================================================================
// VSResultsMenu::GetPlayerRecord(unsigned int)
// @0019106C  118 bytes
// ==========================================================================
int * VSResultsMenu::GetPlayerRecord(unsigned int theSlot)
{
  // A record whose stored gamepad index already matches is that gamepad's.
  if ( msPlayerRecords[0][4] == (int)theSlot )
    return msPlayerRecords[0];
  if ( msPlayerRecords[1][4] == (int)theSlot )
    return msPlayerRecords[1];

  // Otherwise slot 0 belongs to the gamepad the first player is NOT using, so
  // that a fresh record is only ever handed out for the other one.
  const int aFirstIndex = mPlayerIndices[0];
  const int aOtherIndex = ( (int)theSlot == aFirstIndex ) ? mPlayerIndices[1] : mPlayerIndices[0];
  int aSlot;
  if ( msPlayerRecords[0][4] == -1 || msPlayerRecords[0][4] != aOtherIndex )
  {
    aSlot = 0;
  }
  else
  {
    const int aTakenIndex = ( (int)theSlot == aFirstIndex ) ? mPlayerIndices[1] : aFirstIndex;
    if ( msPlayerRecords[1][4] != -1 && aTakenIndex == msPlayerRecords[1][4] )
      return nullptr;
    aSlot = 1;
  }

  int *aRecord = msPlayerRecords[aSlot];
  aRecord[0] = -1;
  aRecord[1] = 0;
  aRecord[2] = 0;
  aRecord[3] = 0;
  aRecord[4] = (int)theSlot;
  return aRecord;
}

// ==========================================================================
// VSResultsMenu::DrawInfoBox(Sexy::Graphics *,int)
// @001910EC  1360 bytes
// ==========================================================================
void VSResultsMenu::DrawInfoBox(Sexy::Graphics * theGraphics, int thePlayerIndex)
{
  Sexy::ImageWidget *aInfoWidget = dynamic_cast<Sexy::ImageWidget *>(FindWidget(thePlayerIndex + 2));
  if ( aInfoWidget == nullptr )
    return;

  const int aSide = mSides[thePlayerIndex];
  const int aGamepadIndex = mPlayerIndices[aSide];

  theGraphics->PushState();
  theGraphics->Translate(aInfoWidget->mX, aInfoWidget->mY);

  int *aPlayerRecord = GetPlayerRecord(aGamepadIndex);
  Sexy::String aPlayerName = ((LawnApp *)gLawnApp)->mPlayerInfo->GetName();
  Sexy::PlayerInfo *aProfile =
      ((LawnApp *)gLawnApp)->mProfileMgr->GetProfile(aPlayerName, aGamepadIndex);
  if ( aPlayerRecord == nullptr || aProfile == nullptr )
    return;

  const bool aIsZombie = (aSide != 0);
  Sexy::Image *aTrophyBase = IMAGE_MP_TROPHY_BASE;
  Sexy::Image *aPlantTrophy = IMAGE_MP_PLANT_TROPHY;
  Sexy::Image *aZombieTrophy = IMAGE_MP_ZOMBIE_TROPHY;
  aInfoWidget->mImage = aIsZombie ? IMAGE_VS_INFO_BOX_ZOMBIES
                                  : IMAGE_VS_INFO_BOX_PLANTS;
  theGraphics->DrawImage(IMAGE_NO_GAMERPIC, 31, 52);
  theGraphics->DrawImage(aIsZombie ? IMAGE_VS_INFO_BOX_ZOMBIES_OVERLAY
                          : IMAGE_VS_INFO_BOX_PLANTS_OVERLAY, 0, 0);

  // The profile's name is fetched into the same string the label then
  // overwrites, so only the translated "[PLAYER_FMT]" text is drawn.
  Sexy::String aLabel = aProfile->GetName();
  Sexy::String aFormat;
  TodStringTranslate(&aFormat, "[PLAYER_FMT]");
  Sexy::String aText;
  StrFormat(&aText, aFormat.c_str(), thePlayerIndex + 1);
  aLabel = aText;

  theGraphics->SetColor(Sexy::Color::White);
  theGraphics->SetFont(FONT_DWARVENTODCRAFT18);
  theGraphics->DrawString(aLabel, 42, 44);

  aProfile->mWinStreak = aPlayerRecord[3];
  if ( aProfile->mWinStreak > 1 )
  {
    Sexy::String aStreakFormat;
    TodStringTranslate(&aStreakFormat, "[WIN_STREAK_FMT]");
    Sexy::String aStreakText;
    StrFormat(&aStreakText, aStreakFormat.c_str(), aProfile->mWinStreak);
    theGraphics->DrawString(aStreakText, 263, 78);
  }
  // The trophy drops to its resting Y as the smoke counter runs; over the first
  // 50 frames it still comes down from the info box' own position.
  if ( mSmokeCounter > 49 )
    mTrophyPosY = (float)TodAnimateCurve(50, 60, mSmokeCounter, 82, 74, CURVE_BOUNCE);
  else
    mTrophyPosY = (float)TodAnimateCurve(
        0, 50, mSmokeCounter,
        LawnApp::FULLSCREEN_RECT.mY - aInfoWidget->mY - aTrophyBase->mHeight - aPlantTrophy->mHeight,
        82, CURVE_EASE_IN);

  if ( aPlayerRecord[0] != -1 )
  {
    TodParticleSystem *aSmoke = (TodParticleSystem *)((LawnApp *)gLawnApp)->ParticleTryToGet(mSmokeParticleID);
    if ( aSmoke != nullptr )
      aSmoke->Draw(theGraphics);

    Sexy::Image *aTopTrophy = (aPlayerRecord[0] != 0) ? aZombieTrophy : aPlantTrophy;
    theGraphics->DrawImage(aTrophyBase,
                  (int)(mTrophyPosX - (float)(aTrophyBase->mWidth / 2)),
                  (int)mTrophyPosY);
    theGraphics->DrawImage(aTopTrophy,
                  (int)(mTrophyPosX + 2.0f - (float)(aTopTrophy->mWidth / 2)),
                  (int)(mTrophyPosY - 46.0f));

    TodParticleSystem *aSparkle = (TodParticleSystem *)((LawnApp *)gLawnApp)->ParticleTryToGet(mSparkleParticleID);
    if ( aSparkle != nullptr )
    {
      aSparkle->SystemMove(mTrophyPosX, mTrophyPosY);
      aSparkle->Draw(theGraphics);
    }
  }

  // Both columns of small trophies start at the same X.
  const float aTrophyX = (aPlayerRecord[0] == -1) ? 117.0f : 192.0f;

  const int aPlantWins = aPlayerRecord[1];
  if ( aPlantWins > 0 )
  {
    float aX = aTrophyX;
    float aStep = 196.0f / (float)aPlantWins;
    if ( aStep > 52.0f )
      aStep = 52.0f;
    for ( int i = 0; i < aPlantWins; ++i )
    {
      theGraphics->DrawImage(aPlantTrophy, (int)aX, 82, 40, 40);
      aX += aStep;
    }
  }

  const int aZombieWins = aPlayerRecord[2];
  if ( aZombieWins > 0 )
  {
    float aX = aTrophyX;
    float aStep = 196.0f / (float)aZombieWins;
    if ( aStep > 52.0f )
      aStep = 52.0f;
    for ( int i = 0; i < aZombieWins; ++i )
    {
      theGraphics->DrawImage(aZombieTrophy, (int)aX, 124, 40, 40);
      aX += aStep;
    }
  }

  theGraphics->PopState();
}

// ==========================================================================
// VSResultsMenu::DrawOverlay(Sexy::Graphics *)
// @0019169C  86 bytes
// ==========================================================================
void VSResultsMenu::DrawOverlay(Sexy::Graphics * theGraphics)
{
  DrawInfoBox(theGraphics, 0);
  DrawInfoBox(theGraphics, 1);
  const int aAlpha = TodAnimateCurve(180, 0, mSparkleCounter, 255, 0, CURVE_LINEAR);
  theGraphics->SetColor(Color(255, 255, 255, aAlpha));
  theGraphics->FillRect(LawnApp::FULLSCREEN_RECT);
}

// ==========================================================================
// VSResultsMenu::InitFromBoard(Board *)
// @001916FC  608 bytes
// ==========================================================================
void VSResultsMenu::InitFromBoard(Board * theBoard)
{
  GamepadControls *aControls0 = theBoard->mGamepadControls[0];
  GamepadControls *aControls1 = theBoard->mGamepadControls[1];

  mPlayerIndices[0] = aControls0->mGamepadIndex;
  mPlayerIndices[1] = aControls1->mGamepadIndex;
  mSides[0] = (WinSide)aControls0->mIsZombie;
  mSides[1] = (WinSide)aControls1->mIsZombie;

  // Credit each gamepad's own record for the side it played.
  const BoardResult aBoardResult = ((LawnApp *)gLawnApp)->mBoardResult;
  for ( int aSlot = 0; aSlot < 2; ++aSlot )
  {
    int *aRecord = GetPlayerRecord(mPlayerIndices[aSlot]);
    if ( aRecord == nullptr )
      continue;
    if ( aBoardResult == BOARDRESULT_VS_PLANT_WON && mSides[aSlot] == WIN_SIDE_PLANT )
    {
      aRecord[0] = 0;
      ++aRecord[1];
      ++aRecord[3];
    }
    else if ( aBoardResult == BOARDRESULT_VS_ZOMBIE_WON && mSides[aSlot] == WIN_SIDE_ZOMBIE )
    {
      aRecord[0] = 1;
      ++aRecord[2];
      ++aRecord[3];
    }
    else
    {
      aRecord[0] = -1;
      aRecord[3] = 0;
    }
  }

  // The side that won lights up: the plant panels when the plants took the
  // match, the zombie panels otherwise.
  Sexy::ImageWidget *aPlantSide = dynamic_cast<Sexy::ImageWidget *>(FindWidget(VSResultsMenu_Plant_Side));
  Sexy::ImageWidget *aPlantSideFront = dynamic_cast<Sexy::ImageWidget *>(FindWidget(VSResultsMenu_Plant_Side_Front));
  Sexy::ImageWidget *aZombieSide = dynamic_cast<Sexy::ImageWidget *>(FindWidget(VSResultsMenu_Zombie_Side));
  Sexy::ImageWidget *aZombieSideFront = dynamic_cast<Sexy::ImageWidget *>(FindWidget(VSResultsMenu_Zombie_Side_Front));

  const bool aPlantsWon = (aBoardResult == BOARDRESULT_VS_PLANT_WON);
  aPlantSide->mAlpha = aPlantsWon ? 255 : 0;
  aPlantSideFront->mAlpha = aPlantsWon ? 255 : 0;
  aZombieSide->mAlpha = aPlantsWon ? 0 : 255;
  aZombieSideFront->mAlpha = aPlantsWon ? 0 : 255;

  // The trophy hangs under the winner's info box: P1's when the first record
  // took the match, otherwise P2's when the second record exists at all.
  int aWinnerBoxId = 0;
  if ( msPlayerRecords[0][0] != -1 )
    aWinnerBoxId = VSResultsMenu_Info_Box_P1;
  else if ( msPlayerRecords[1][0] != -1 )
    aWinnerBoxId = VSResultsMenu_Info_Box_P2;

  if ( aWinnerBoxId != 0 )
  {
    Sexy::ImageWidget *aWinnerBox = dynamic_cast<Sexy::ImageWidget *>(FindWidget(aWinnerBoxId));
    Sexy::Image *aTrophyBase = IMAGE_MP_TROPHY_BASE;
    mTrophyPosX = (float)(aTrophyBase->mWidth / 2 + 94);
    mTrophyPosY = (float)(LawnApp::FULLSCREEN_RECT.mY - aWinnerBox->mY - aTrophyBase->mHeight
                          - (IMAGE_MP_PLANT_TROPHY)->mHeight);
    TodParticleSystem *aSparkle = (TodParticleSystem *)((LawnApp *)gLawnApp)->AddTodParticle(
        mTrophyPosX, mTrophyPosY, 0, (ParticleEffect)83);
    mSparkleParticleID = ((LawnApp *)gLawnApp)->ParticleGetID(aSparkle);
  }

  OutputDebug("VS Results\n");
  for ( int i = 0; i < 2; ++i )
  {
    const int aGamepadIndex = mPlayerIndices[i];
    int *aRecord = GetPlayerRecord(aGamepadIndex);
    const char *aWinner = (aRecord[0] == -1) ? gEmptyCString : "[WINNER]";
    const char *aSide = (mSides[i] != 0) ? "Zombies" : "Plants";
    OutputDebug("P%d%s: Side(%s), Gamepad(%d), Record( %dPW, %dZW, %dST, %dG )\n",
                i, aWinner, aSide, aGamepadIndex,
                aRecord[1], aRecord[2], aRecord[3], aRecord[4]);
  }
}

// ==========================================================================
// VSResultsMenu::ClearPlayerRecords()
// @00191994  36 bytes
// ==========================================================================
void VSResultsMenu::ClearPlayerRecords()
{
  for ( int aSlot = 0; aSlot < 2; ++aSlot )
  {
    msPlayerRecords[aSlot][0] = -1;  // winning side
    msPlayerRecords[aSlot][1] = 0;   // plant wins
    msPlayerRecords[aSlot][2] = 0;   // zombie wins
    msPlayerRecords[aSlot][3] = 0;   // win streak
    msPlayerRecords[aSlot][4] = -1;  // gamepad index
  }
}
