// GridItem -- reconstructed from libGameMain.so.
//
// A grid item is a SyncObject (0x10 bytes), not a GameObject: it owns its own
// position and render order and draws itself through one of the DrawXxx helpers
// below. Every offset in the authoritative header PvZ\Lawn\Board\GridItem.h is
// shifted by +0x10 here, which is exactly the SyncObject base the PC version
// does not have.
//
// Return values. Several of these methods are `void` in the binary -- the mangled
// names carry no return type -- and the transcription gave each one the type of
// whatever the last call happened to leave in r0. The declaration is kept and the
// body returns the value that register really holds where that value is
// meaningful, and nullptr/0 where it is not.
//
// The one exception is GetItemRect: it returns a Rect BY VALUE, so ARM passes the
// destination in r0 and the decompiler's `this` is that destination. The
// transcription merged the two and wrote the rect fields at object offset 0 --
// over the vptr. The definition and every call site now use the by-value form.

#include "GridItem.h"
#include "DataArrayMethods.h"
#include "game/Board.h"
#include "Challenge.h"
#include "Graphics.h"
#include "Image.h"
#include "LawnApp.h"
#include "MessageWidget.h"
#include "Plant.h"
#include "Reanimation.h"
#include "ReanimatorCache.h"
#include "SharedImageRef.h"
#include "TodParticleSystem.h"
#include "WidgetManager.h"
#include "ZenGarden.h"
#include "Zombie.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace
{
// GridItemType, GridItemState, ReanimationType and ParticleEffect are still
// one-int surrogate structs in Sexy\ReconTypes.h, so their enumerators have no
// names in this tree yet. The values below are the ones from the authoritative
// PvZ\Lawn\Common\ConstEnums.h; naming them here keeps the call sites readable
// and makes the swap to real enumerators a one-line change each.
//
// GRIDITEM_CRATER, GRIDITEM_LADDER, GRIDITEM_SCARY_POT,
// GRIDITEM_STATE_GRAVESTONE_SPECIAL, GRIDITEM_STATE_PORTAL_CLOSED,
// GRIDITEM_STATE_SCARY_POT_QUESTION and COIN_MOTION_FROM_GRAVE_STONE are NOT
// repeated here: Sexy\ReconTypes.h:3166-3175 and :3153 now name them at
// namespace scope with these same values, and a second definition in the same
// translation unit is C2872.
// Grid item and grave-particle names are enumerators of GridItemType,
// GridItemState, and ParticleEffect.

// AdviceType entry. Sexy\ReconFreeFunctions.h declares an `extern _DWORD` stub
// for this name, but the binary holds no such variable: it is an enumerator, and
// DrawIZombieBrain @0x13C840 compares the board's mHelpIndex against the literal
// 57 (`v4[5571] == 57`, where v4 is mBoard and 5571 words = +0x5710). The value
// is spelled locally for the same reason the grid-item constants above are.
}

// ==========================================================================
// GridItem::GridItemDie()
// @0013C600  334 bytes
// ==========================================================================
TodParticleSystem * GridItem::GridItemDie()
{
  mDead = true;

  // In versus mode a dying grid item pays out the resources the mode is set to
  // drop, and the last target zombie ends the level.
  if ( mApp->mGameMode == GAMEMODE_MP_VS )
  {
    if ( mGridItemType == GRIDITEM_GRAVESTONE )
    {
      if ( (unsigned int)(Challenge::gVSResourseDropMode - 2) <= 1 && Challenge::gVSResourceDropCount > 0 )
      {
        for ( int i = 0; i < Challenge::gVSResourceDropCount; ++i )
        {
          const int aX = mBoard->GridToPixelX(mGridX, mGridY);
          const int aY = mBoard->GridToPixelY(mGridX, mGridY);
          mBoard->AddCoin(aX, aY, COIN_VS_ZOMBIE_BRAIN, COIN_MOTION_FROM_GRAVE_STONE);
        }
      }
    }
    else if ( mGridItemType == GRIDITEM_MP_TARGET_ZOMBIE )
    {
      if ( (Challenge::gVSResourseDropMode & 0xFFFFFFFD) == 1 )
      {
        if ( Challenge::gVSResourceDropCount > 0 )
        {
          for ( int i = 0; i < Challenge::gVSResourceDropCount; ++i )
          {
            const int aX = mBoard->GridToPixelX(mGridX, mGridY);
            const int aY = mBoard->GridToPixelY(mGridX, mGridY);
            mBoard->AddCoin(aX, aY, COIN_VS_ZOMBIE_BRAIN, COIN_MOTION_FROM_GRAVE_STONE);
          }
        }
      }
      else if ( mApp->mGameScene == SCENE_PLAYING && !mBoard->mLevelAwardSpawned )
      {
        bool aPlantsWon = false;
        if ( Challenge::gVSWinMode == 1 )
          aPlantsWon = mBoard->GetMPTargetCount() <= 2;
        if ( !aPlantsWon )
          aPlantsWon = mBoard->GetMPTargetCount() == 0;

        if ( aPlantsWon )
        {
          mBoard->FreezeEffectsForCutscene(true);
          mBoard->PlantsWon(this);
          mGridItemReanimID = REANIMATIONID_NULL;
        }
      }
    }
  }

  Reanimation *aReanim = (Reanimation *)mApp->ReanimationTryToGet(mGridItemReanimID);
  if ( aReanim != nullptr )
  {
    aReanim->ReanimationDie();
    mGridItemReanimID = REANIMATIONID_NULL;
  }

  TodParticleSystem *aParticle = mApp->ParticleTryToGet(mGridItemParticleID);
  if ( aParticle != nullptr )
  {
    aParticle->ParticleSystemDie();
    mGridItemParticleID = ParticleSystemID(0);
  }
  return aParticle;
}

// ==========================================================================
// GridItem::DrawGridItemOverlay(Sexy::Graphics *)
// @0013C760  158 bytes
// ==========================================================================
float * GridItem::DrawGridItemOverlay(Sexy::Graphics *theGraphics)
{
  // While the chocolate cursor is over Stinky and he has not been fed yet, show
  // the speech bubble with the chocolate in it.
  if ( mGridItemType == GRIDITEM_STINKY
    && mBoard->mCursorObject[0]->mCursorType == CURSOR_TYPE_CHOCOLATE )
  {
    if ( !mApp->mZenGarden->IsStinkyHighOnChocolate() )
    {
      theGraphics->DrawImage(IMAGE_PLANTSPEECHBUBBLE,
                    (int)(mPosX + 50.0f), (int)(mPosY - 36.0f));
      TodDrawImageScaledF(theGraphics, IMAGE_CHOCOLATE,
                          mPosX + 63.0f, mPosY - 28.0f, 0.44f, 0.44f);
    }
  }
  return nullptr;
}

// ==========================================================================
// GridItem::UpdateBrain()
// @0013C818  40 bytes
// ==========================================================================
TodParticleSystem * GridItem::UpdateBrain()
{
  // A squished brain fades out and then dies.
  if ( mGridItemState == GRIDITEM_STATE_BRAIN_SQUISHED )
  {
    const bool aLastTick = mGridItemCounter == 1;
    --mGridItemCounter;
    if ( mGridItemCounter < 0 || aLastTick )
      return GridItemDie();
  }

  if ( mTransparentCounter > 0 )
    --mTransparentCounter;
  return nullptr;
}

// ==========================================================================
// GridItem::DrawIZombieBrain(Sexy::Graphics *)
// @0013C840  242 bytes
// ==========================================================================
int GridItem::DrawIZombieBrain(Sexy::Graphics *theGraphics)
{
  if ( mGridItemState == GRIDITEM_STATE_BRAIN_SQUISHED )
    TodDrawImageScaledF(theGraphics, IMAGE_BRAIN, mPosX, mPosY + 20.0f, 1.0f, 0.25f);
    return 0;

  // The brain flashes while the "eat all the brains" hint is on screen.
  if ( mBoard->mAdvice->mDuration > 0 && mBoard->mHelpIndex == ADVICE_I_ZOMBIE_EAT_ALL_BRAINS )
  {
    Sexy::Color aFlashColor;
    GetFlashingColor(&aFlashColor, mBoard->mMainCounter);
    theGraphics->SetColorizeImages(true);
    theGraphics->SetColor(aFlashColor);
  }

  theGraphics->DrawImageF(IMAGE_BRAIN, mPosX, mPosY);

  // The highlight glow ramps up as the brain becomes visible.
  if ( mTransparentCounter > 0 )
  {
    theGraphics->SetDrawMode(Sexy::Graphics::DRAWMODE_ADDITIVE);
    theGraphics->SetColorizeImages(true);

    const int aAlpha = std::min(mTransparentCounter * 3, 255);
    theGraphics->SetColor(Sexy::Color(255, 255, 255, aAlpha));
    theGraphics->DrawImageF(IMAGE_BRAIN, mPosX, mPosY);

    theGraphics->SetDrawMode(Sexy::Graphics::DRAWMODE_NORMAL);
    theGraphics->SetColorizeImages(false);
  }

  theGraphics->SetColorizeImages(false);
  return 0;
}

// ==========================================================================
// GridItem::DrawGraveStone(Sexy::Graphics *)
// @0013C93C  784 bytes
// ==========================================================================
void GridItem::DrawGraveStone(Sexy::Graphics *theGraphics)
{
  if ( mGridItemCounter <= 0 )
    return;

  const int aRiseProgress = TodAnimateCurve(0, 100, mGridItemCounter, 1000, 0, CURVE_EASE_IN_OUT);
  const int aCelLook = mBoard->mGridCelLook[mGridX][mGridY];
  const int aCelOffsetX = mBoard->mGridCelOffset[mGridX][mGridY][0];
  const int aCelOffsetY = mBoard->mGridCelOffset[mGridX][mGridY][1];

  Sexy::Image *aTombstones = IMAGE_TOMBSTONES;
  const int aCelWidth = aTombstones->GetCelWidth();
  const int aCelHeight = aTombstones->GetCelHeight();
  const int aCelCol = aCelLook % 5;

  // Which row of the tombstone strip this grave uses.
  int aCelRow;
  if ( mGridY != 0 )
  {
    if ( mGridItemState == GRIDITEM_STATE_GRAVESTONE_SPECIAL )
      aCelRow = 0;
    else
      aCelRow = (aCelLook % 2 + 2) * aCelHeight;
  }
  else
  {
    aCelRow = aCelHeight;
  }

  // A plant sitting on the grave pushes the stone up as the grave rises.
  int aPlantLift = 0;
  const int aRiseHeight = TodAnimateCurve(0, 1000, aRiseProgress, aCelHeight, 0, CURVE_EASE_IN_OUT);
  const int aMoundHeight = TodAnimateCurve(0, 50, aRiseProgress, 0, 14, CURVE_EASE_IN_OUT);
  const int aDirtHeight = TodAnimateCurve(500, 1000, aRiseProgress, aCelHeight, 0, CURVE_EASE_IN_OUT);

  Plant *aTopPlant = mBoard->GetTopPlantAt(mGridX, mGridY, (PlantPriority)6);
  if ( aTopPlant != nullptr )
  {
    if ( aTopPlant->mState == STATE_GRAVEBUSTER_EATING )
      aPlantLift = (int)TodAnimateCurveFloat(400, 0, aTopPlant->mStateCountdown, 10.0f, 40.0f, CURVE_LINEAR);
  }

  const int aStoneX = aCelOffsetX + mBoard->GridToPixelX(mGridX, mGridY) - 4;
  const int aStoneY = mBoard->GridToPixelY(mGridX, mGridY) + aCelHeight - 9 + aCelOffsetY;

  if ( mApp->mGameMode == GAMEMODE_MP_VS )
  {
    // Versus mode draws the animated stone from its reanimation and clips it to
    // the part that has risen out of the ground.
    Reanimation *aReanim = (Reanimation *)mApp->ReanimationTryToGet(mGridItemReanimID);
    if ( aReanim != nullptr )
    {
      const int aClipHeight = aRiseHeight - aMoundHeight - aPlantLift;
      const int aClipY = aStoneY - aRiseHeight + aPlantLift;
      const TRect<int> aOldClip = theGraphics->mClipRect;

      theGraphics->SetClipRect(TRect<int>(aStoneX, aClipY, 86, aClipHeight));
      aReanim->SetPosition((float)aStoneX, (float)aClipY);
      aReanim->DrawRenderGroup(theGraphics, 0);
      theGraphics->ClearClipRect();

      Sexy::Image *aDirt = IMAGE_VS_STONE_DIRT;
      const int aDirtHeight2 = TodAnimateCurve(500, 1000, aRiseProgress, aDirt->mHeight, 0, CURVE_EASE_IN_OUT);
      theGraphics->DrawImage(aDirt, aStoneX, aStoneY - aDirtHeight, TRect<int>(0, 0, aDirt->mWidth, aDirtHeight2));

      aReanim->DrawRenderGroup(theGraphics, 1);
      theGraphics->mClipRect = aOldClip;
    }

    Sexy::String aHealthText;
    StrFormat(&aHealthText, "%d", mVSGraveStoneHealth);
    theGraphics->DrawString(aHealthText, aStoneX, aStoneY);
    return;
  }

  // Everything except the dirt mound.
  TRect<int> aStoneCel(aCelCol * aCelWidth, aCelRow + aPlantLift, aCelWidth,
                       aRiseHeight - aMoundHeight - aPlantLift);
  TRect<int> aMoundCel(aCelCol * aCelWidth, aCelRow, aCelWidth, aDirtHeight);

  theGraphics->DrawImage(aTombstones, aStoneX, aStoneY - aRiseHeight + aPlantLift, aStoneCel);
  theGraphics->DrawImage(IMAGE_TOMBSTONE_MOUNDS, aStoneX, aStoneY - aDirtHeight, aMoundCel);
}

// ==========================================================================
// GridItem::DrawMPTarget(Sexy::Graphics *)
// @0013CC74  1004 bytes
// ==========================================================================
void GridItem::DrawMPTarget(Sexy::Graphics *theGraphics)
{
  // The target zombie's reanimation gets a damage-dependent image override.
  const int aDamageStage = (int)(4.0f - (float)mVSTargetZombieHealth * 0.02f);

  Reanimation *aReanim = (Reanimation *)mApp->ReanimationTryToGet(mGridItemReanimID);
  if ( aReanim == nullptr )
    return;

  // GetSharedImage hands the SharedImageRef back BY VALUE now (SexyAppBase.h:401
  // declares it so; the transcription's pointer return and the deref that used to
  // be written here were the sret shape -- ARM passes the 12-byte destination in
  // r0). SharedImageRef's own `operator*` yields the owned Image, not the ref.
  const char *aImageName;
  switch ( aDamageStage )
  {
    case 0:  aImageName = "reanim/zombie_target"; break;
    case 1:  aImageName = "reanim/zombie_target_damage01"; break;
    case 2:  aImageName = "reanim/zombie_target_damage02"; break;
    case 3:
    case 4:  aImageName = "reanim/zombie_target_damage03"; break;
    default: aImageName = "reanim/zombie_target"; break;
  }

  Sexy::String aFileName(aImageName);
  Sexy::String aVariant;
  mTargetImageOverride = mApp->GetSharedImage(aFileName, aVariant, nullptr, true, false);

  aReanim->SetImageOverride("target", (Sexy::Image *)mTargetImageOverride);
  aReanim->Draw(theGraphics);
}

// ==========================================================================
// GridItem::DrawStinky(Sexy::Graphics *)
// @0013D0B8  308 bytes
// ==========================================================================
void GridItem::DrawStinky(Sexy::Graphics *theGraphics)
{
  Reanimation *aReanim = (Reanimation *)mApp->ReanimationTryToGet(mGridItemReanimID);
  if ( aReanim == nullptr )
    return;

  // Stinky's motion trail: every other recorded frame is drawn as a translucent
  // ghost offset by how far he has moved since it was recorded.
  const float aSavedAnimTime = aReanim->mAnimTime;
  for ( int i = mMotionTrailCount - 2; i >= 0; --i )
  {
    if ( (i & 1) == 0 )
      continue;

    const MotionTrailFrame &aFrame = mMotionTrailFrames[i];
    const float aOffsetX = aFrame.mPosX - mPosX;
    const float aOffsetY = aFrame.mPosY - mPosY;

    const int aAlpha = TodAnimateCurve(0, 11, i, 64, 16, CURVE_LINEAR);
    theGraphics->SetColor(Sexy::Color(255, 255, 255, aAlpha));
    theGraphics->SetColorizeImages(true);

    const float aSavedTransX = theGraphics->mTransX;
    const float aSavedTransY = theGraphics->mTransY;
    aReanim->mAnimTime = aFrame.mAnimTime;
    theGraphics->mTransX = aSavedTransX + aOffsetX;
    theGraphics->mTransY = aSavedTransY + aOffsetY;
    aReanim->Draw(theGraphics);
    theGraphics->SetColorizeImages(false);
    theGraphics->mTransX = aSavedTransX;
    theGraphics->mTransY = aSavedTransY;
  }

  aReanim->mAnimTime = aSavedAnimTime;

  // While the chocolate cursor is over him, Stinky is drawn with an additive
  // highlight so he reads as the thing the cursor is for.
  if ( mGridItemType == GRIDITEM_STINKY && mHighlighted )
  {
    aReanim->mEnableExtraAdditiveDraw = true;
    aReanim->mExtraAdditiveColor = Sexy::Color(255, 255, 255, 196);
  }

  aReanim->Draw(theGraphics);
  aReanim->mEnableExtraAdditiveDraw = false;
}

// ==========================================================================
// GridItem::DrawCrater(Sexy::Graphics *)
// @0013D1EC  424 bytes
// ==========================================================================
int GridItem::DrawCrater(Sexy::Graphics *theGraphics)
{
  float aPosX = (float)mBoard->GridToPixelX(mGridX, mGridY) - 8.0f;
  float aPosY = (float)mBoard->GridToPixelY(mGridX, mGridY) + 40.0f;

  // The crater fades in over its first 25 ticks.
  if ( mGridItemCounter <= 24 )
  {
    const int aAlpha = TodAnimateCurve(25, 0, mGridItemCounter, 255, 0, CURVE_LINEAR);
    theGraphics->SetColor(Sexy::Color(255, 255, 255, aAlpha));
    theGraphics->SetColorizeImages(true);
  }

  // The fading cel is used for the first 9000 ticks.
  const bool aFading = mGridItemCounter <= 8999;
  int aCelCol = aFading ? 1 : 0;
  Sexy::Image *aImage = IMAGE_CRATER;

  if ( mBoard->IsPoolSquare(mGridX, mGridY) )
  {
    aImage = mBoard->StageIsNight() ? IMAGE_CRATER_WATER_NIGHT
                                    : IMAGE_CRATER_WATER_DAY;
    // The crater bobs on the water; the phases come from the grid position and
    // the board's tick counter.
    const float aBob = (float)sin((float)mGridY * 3.1416f
                                + (float)mGridX * 0.7854f
                                + (float)mBoard->mMainCounter * 0.031416f);
    aPosY += 2.0f * aBob;
  }
  else if ( mBoard->StageHasRoof() )
  {
    if ( mGridX > 4 )
    {
      aPosX += 18.0f;
      aPosY -= 9.0f;
      aImage = IMAGE_CRATER_ROOF_CENTER;
    }
    else
    {
      aPosX += 16.0f;
      aPosY -= 16.0f;
      aImage = IMAGE_CRATER_ROOF_LEFT;
    }
  }
  else if ( mBoard->StageIsNight() )
  {
    aCelCol = 1;
    if ( aFading )
      aImage = IMAGE_CRATER_FADING;
  }
  else if ( aFading )
  {
    aCelCol = 0;
    aImage = IMAGE_CRATER_FADING;
  }

  TodDrawImageCelF(theGraphics, aImage, aPosX, aPosY, aCelCol, 0);
  theGraphics->SetColorizeImages(false);
  return 0;
}

// ==========================================================================
// GridItem::DrawLadder(Sexy::Graphics *)
// @0013D3C0  102 bytes
// ==========================================================================
int GridItem::DrawLadder(Sexy::Graphics *theGraphics)
{
  const int aX = mBoard->GridToPixelX(mGridX, mGridY);
  const int aY = mBoard->GridToPixelY(mGridX, mGridY);
  TodDrawImageScaledF(theGraphics, IMAGE_REANIM_ZOMBIE_LADDER_5,
                      (float)aX + 25.0f, (float)aY - 4.0f, 0.8f, 0.8f);
  return 0;
}

// ==========================================================================
// GridItem::AddGraveStoneParticles()
// @0013D430  114 bytes
// ==========================================================================
void GridItem::AddGraveStoneParticles()
{
  // The particle spawn point is offset by the cel offset the grave uses.
  const int aCelOffsetX = mBoard->mGridCelOffset[mGridX][mGridY][0];
  const int aCelOffsetY = mBoard->mGridCelOffset[mGridX][mGridY][1];
  const int aX = aCelOffsetX + mBoard->GridToPixelX(mGridX, mGridY) + 14;
  const int aY = mBoard->GridToPixelY(mGridX, mGridY) + 78 + aCelOffsetY;

  mApp->AddTodParticle((float)aX, (float)aY, mGridItemReanimID + 1, (ParticleEffect)PARTICLE_GRAVE_STONE);
  mApp->PlayFoley(FOLEY_DIRT_RISE);
}

// ==========================================================================
// GridItem::OpenPortal()
// @0013D4A4  206 bytes
// ==========================================================================
void GridItem::OpenPortal()
{
  const int aGridX = mGridX;
  float aPosX = (float)aGridX * 80.0f - 6.0f;
  float aPosY = (float)mBoard->GridToPixelY(0, mGridY) - 65.0f;

  Reanimation *aReanim = (Reanimation *)mApp->ReanimationTryToGet(mGridItemReanimID);
  if ( aReanim != nullptr )
  {
    aReanim->SetPosition((float)aGridX * 80.0f - 6.0f, aPosY);
  }
  else
  {
    ReanimationType aReanimType = REANIM_PORTAL_CIRCLE;
    if ( mGridItemType == GRIDITEM_PORTAL_SQUARE )
    {
      aReanimType = REANIM_PORTAL_SQUARE;
      aPosY += 25.0f;
      aPosX -= 4.0f;
    }

    aReanim = (Reanimation *)mApp->AddReanimation(aPosX, aPosY, 0, aReanimType);
    aReanim->mIsAttachment = true;
    mGridItemReanimID = mApp->ReanimationGetID(aReanim);
  }

  TodParticleSystem *aParticle = mApp->ParticleTryToGet(mGridItemParticleID);
  if ( aParticle != nullptr )
  {
    aParticle->ParticleSystemDie();
    mGridItemParticleID = ParticleSystemID(0);
  }

  aReanim->PlayReanim("anim_appear", REANIM_PLAY_ONCE_AND_HOLD, 0, 12.0f);
  mApp->PlayFoley(FOLEY_PORTAL);
}

// ==========================================================================
// GridItem::ClosePortal()
// @0013D580  62 bytes
// ==========================================================================
TodParticleSystem * GridItem::ClosePortal()
{
  Reanimation *aReanim = (Reanimation *)mApp->ReanimationTryToGet(mGridItemReanimID);
  if ( aReanim != nullptr )
    aReanim->PlayReanim("anim_dissapear", REANIM_PLAY_ONCE_AND_HOLD, 0, 12.0f);

  TodParticleSystem *aParticle = mApp->ParticleTryToGet(mGridItemParticleID);
  if ( aParticle != nullptr )
  {
    aParticle->ParticleSystemDie();
    mGridItemParticleID = ParticleSystemID(0);
  }

  mGridItemState = GRIDITEM_STATE_PORTAL_CLOSED;
  return aParticle;
}

// ==========================================================================
// GridItem::UpdatePortal()
// @0013D5C4  198 bytes
// ==========================================================================
TodParticleSystem * GridItem::UpdatePortal()
{
  Reanimation *aReanim = (Reanimation *)mApp->ReanimationGet(mGridItemReanimID);

  if ( mGridItemState == GRIDITEM_STATE_PORTAL_CLOSED )
  {
    // The close animation has finished holding its last frame: the portal is gone.
    if ( aReanim->mLoopCount > 0 )
      return GridItemDie();
    return nullptr;
  }

  // The open animation reached its hold frame: start the looping pulse and spawn
  // the portal particle once.
  if ( aReanim->mLoopType == REANIM_PLAY_ONCE_AND_HOLD && aReanim->mLoopCount > 0 )
  {
    aReanim->PlayReanim("anim_pulse", REANIM_LOOP, 0, 12.0f);

    float aParticleX = (float)mGridX * 80.0f + 13.0f;
    const float aGridTop = (float)mBoard->GridToPixelY(0, mGridY);
    float aParticleY = aGridTop - 39.0f;

    int aParticleType = PARTICLE_PORTAL_CIRCLE;
    if ( mGridItemType == GRIDITEM_PORTAL_SQUARE )
    {
      aParticleType = PARTICLE_PORTAL_SQUARE;
      aParticleX -= 8.0f;
      aParticleY += 15.0f;
    }

    TodParticleSystem *aParticle =
        (TodParticleSystem *)mApp->AddTodParticle(aParticleX, aParticleY, 0, (ParticleEffect)aParticleType);
    mGridItemParticleID = mApp->ParticleGetID(aParticle);
    return aParticle;
  }

  return nullptr;
}

// ==========================================================================
// GridItem::DrawSquirrel(Sexy::Graphics *)
// @0013D698  162 bytes
// ==========================================================================
int GridItem::DrawSquirrel(Sexy::Graphics *theGraphics)
{
  // The binary calls both converters for their side effects and returns the
  // animation offset for the current state; the caller places the squirrel.
  mBoard->GridToPixelX(mGridX, mGridY);
  const int aBaseY = mBoard->GridToPixelY(mGridX, mGridY);

  switch ( mGridItemState )
  {
    case GRIDITEM_STATE_SQUIRREL_PEEKING:
      return TodAnimateCurve(50, 0, mGridItemCounter, 0, -40, CURVE_BOUNCE_SLOW_MIDDLE);
    case GRIDITEM_STATE_SQUIRREL_RUNNING_UP:
      return TodAnimateCurve(50, 0, mGridItemCounter, 100, 0, CURVE_EASE_IN);
    case GRIDITEM_STATE_SQUIRREL_RUNNING_DOWN:
      return TodAnimateCurve(50, 0, mGridItemCounter, -100, 0, CURVE_EASE_IN);
    case GRIDITEM_STATE_SQUIRREL_RUNNING_LEFT:
      return TodAnimateCurve(50, 0, mGridItemCounter, 80, 0, CURVE_EASE_IN);
    case GRIDITEM_STATE_SQUIRREL_RUNNING_RIGHT:
      return TodAnimateCurve(50, 0, mGridItemCounter, -80, 0, CURVE_EASE_IN);
    default:
      break;
  }
  return aBaseY;
}

// ==========================================================================
// GridItem::RakeFindZombie()
// @0013D73C  126 bytes
// ==========================================================================
Zombie * GridItem::RakeFindZombie()
{
  const TRect<int> aRakeRect((int)mPosX, (int)mPosY, 63, 80);

  Zombie *aZombie = nullptr;
  while ( mBoard->IterateZombies(aZombie) != 0 )
  {
    if ( aZombie->IsDeadOrDying() == 0
      && aZombie->IsBobsledTeamWithSled() == 0
      && aZombie->mRow == mGridY
      && aZombie->EffectedByDamage(1u) != 0 )
    {
      const Rect aZombieRect = aZombie->GetZombieRect();
      if ( GetRectOverlap(aRakeRect, aZombieRect) >= 0 )
        return aZombie;
    }
  }
  return nullptr;
}

// ==========================================================================
// GridItem::UpdateRake()
// @0013D7BC  138 bytes
// ==========================================================================
Zombie * GridItem::UpdateRake()
{
  if ( mGridItemState == GRIDITEM_STATE_RAKE_ATTRACTING
    || mGridItemState == GRIDITEM_STATE_RAKE_WAITING )
  {
    Zombie *aZombie = RakeFindZombie();
    if ( aZombie != nullptr )
    {
      Reanimation *aReanim = (Reanimation *)mApp->ReanimationGet(mGridItemReanimID);
      aReanim->SetAnimRate(20.0f);
      mGridItemCounter = 200;
      mGridItemState = GRIDITEM_STATE_RAKE_TRIGGERED;
      return (Zombie *)mApp->PlayFoley(FOLEY_SWING);
    }
    return nullptr;
  }

  if ( mGridItemState != GRIDITEM_STATE_RAKE_TRIGGERED )
    return nullptr;

  Reanimation *aReanim = (Reanimation *)mApp->ReanimationGet(mGridItemReanimID);
  if ( aReanim->ShouldTriggerTimedEvent(0.8f) )
  {
    Zombie *aZombie = RakeFindZombie();
    if ( aZombie != nullptr )
    {
      aZombie->TakeDamage(1800, 0);
      mApp->PlayFoley(FOLEY_BONK);
    }
  }

  if ( --mGridItemCounter == 0 )
    return (Zombie *)GridItemDie();
  return nullptr;
}

// ==========================================================================
// GridItem::UpdateBurialMound()
// @0013D848  28 bytes
// ==========================================================================
int GridItem::UpdateBurialMound()
{
  if ( mSummonCounter > 0 )
    --mSummonCounter;
  ++mGridItemCounter;
  return 0;
}

// ==========================================================================
// GridItem::GetMoundUpgradeCost()
// @0013D864  20 bytes
// ==========================================================================
int GridItem::GetMoundUpgradeCost()
{
  // Each level costs 100 more than the total spent so far.
  const int aNextLevelCost = 100 * mMoundLevel + 100;
  return aNextLevelCost + mSummonIndex * aNextLevelCost;
}

// ==========================================================================
// GridItem::TakeDamage(int,unsigned int)
// @0013D878  426 bytes
// ==========================================================================
int GridItem::TakeDamage(int theDamage, unsigned int theFlags)
{
  if ( mApp->mGameMode != GAMEMODE_MP_VS )
    return 0;

  if ( mGridItemType == GRIDITEM_MP_TARGET_ZOMBIE )
  {
    if ( mDead )
      return 0;

    const int aOldHealth = mVSTargetZombieHealth;
    if ( aOldHealth <= 0 )
      return 0;

    const int aNewHealth = aOldHealth - theDamage;
    mVSTargetZombieHealth = aNewHealth;
    if ( aNewHealth > 0 )
    {
      Reanimation *aReanim = (Reanimation *)mApp->ReanimationGet(mGridItemReanimID);
      aReanim->PlayReanim("anim_hit", REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
      return 0;
    }

    mVSTargetZombieHealth = 0;

    // With more than three targets left the target plays its death animation and
    // only dies once that counter runs out.
    if ( mBoard->GetMPTargetCount() > 3 )
    {
      Reanimation *aReanim = (Reanimation *)mApp->ReanimationGet(mGridItemReanimID);
      aReanim->AssignRenderGroupToTrack("target", -1);
      aReanim->PlayReanim("anim_death2", REANIM_PLAY_ONCE_AND_HOLD, 0, 12.0f);
      mVSTargetZombieDieCounter = 300;
      return 0;
    }

    return (int)GridItemDie();
  }

  if ( mGridItemType != GRIDITEM_GRAVESTONE )
    return 0;

  if ( mDead )
    return 0;

  const int aOldHealth = mVSGraveStoneHealth;
  if ( aOldHealth <= 0 )
    return 0;

  const int aNewHealth = aOldHealth - theDamage;
  mVSGraveStoneHealth = aNewHealth;
  if ( aNewHealth <= 0 )
    return (int)GridItemDie();

  mGraveJustGotShotCounter = std::max(mGraveJustGotShotCounter, 25);

  // The stone has six break stages; the reanimation only switches when the stage
  // the health fraction falls into changes. The subtraction happens on the float,
  // not on the truncated integer.
  const float aMaxHealth = mIsSpecialGrave ? 1000.0f : 350.0f;
  const float aOldFraction = std::clamp((float)aOldHealth / aMaxHealth * 6.0f, 0.0f, 6.0f);
  const float aNewFraction = std::clamp((float)aNewHealth / aMaxHealth * 6.0f, 0.0f, 6.0f);
  const int aOldStage = (int)(6.0f - aOldFraction);
  const int aNewStage = (int)(6.0f - aNewFraction);

  if ( aNewStage != aOldStage )
  {
    Reanimation *aReanim = (Reanimation *)mApp->ReanimationTryToGet(mGridItemReanimID);
    if ( aReanim != nullptr )
    {
      char aAnimName[12];
      sprintf(aAnimName, "anim_break%d", aNewStage);
      aReanim->PlayReanim(aAnimName, REANIM_PLAY_ONCE_AND_HOLD, 4, 10.0f);
      return 0;
    }
  }

  return 0;
}

// ==========================================================================
// GridItem::GetItemRect()
// @0013DA3C  44 bytes
// ==========================================================================
Sexy::Rect GridItem::GetItemRect()
{
  // The rect is returned by value: ARM passes its destination in r0, which is why
  // the transcription wrote these four stores at object offset 0, over the vptr.
  Sexy::Rect aRect;
  aRect.mX = mBoard->GridToPixelX(mGridX, mGridY);
  aRect.mY = mBoard->GridToPixelY(mGridX, mGridY);
  aRect.mWidth = 63;
  aRect.mHeight = 80;
  return aRect;
}

// ==========================================================================
// GridItem::GridItem()
// @0013DA68  254 bytes
// ==========================================================================
GridItem::GridItem()
{
  // The item registers its whole state area mGridItemType..mVSTargetZombieDieCounter
  // (+0x18..+0x11B, 260 bytes) as one sync block, then zero-fills exactly that
  // range. The dump renders the registration as a mislabelled `_M_insert_aux` on
  // an empty vector, which is what push_back does. The transcriber scaled the base
  // offset by 4 on the way out and wrote +96 where the binary uses +0x18.
  SyncBlockInfo aBlock;
  aBlock.mData = &mGridItemType;
  aBlock.mSize = 260;
  mSyncBlocks.push_back(aBlock);
  memset(&mGridItemType, 0, 260);

  mApp = (LawnApp *)gSexyAppBase;
  mBoard = mApp->mBoard;

  // The trail frames and the state words are already zero; these are the
  // non-zero defaults.
  mZombieType = (ZombieType)-1;
  mSeedType = (SeedType)-1;
  mMoundLevel = -1;
  mVSGraveStoneHealth = 350;
  mVSTargetZombieHealth = 200;
  mLaunchRate = 3000;
  mLaunchCounter = RandRangeInt(300, 1500);
}

// ==========================================================================
// GridItem::UpdateScaryPot()
// @0013DCD8  190 bytes
// ==========================================================================
int GridItem::UpdateScaryPot()
{
  // With the Tod cheat keys on, holding SHIFT lights the pot up from anywhere on
  // the board; otherwise a Plantern within one square does it.
  bool aLit = false;
  if ( mApp->mTodCheatKeys )
    aLit = mApp->mWidgetManager->mKeyDown.Get(KEYCODE_SHIFT)->mDown;

  if ( !aLit )
  {
    Plant *aPlant = nullptr;
    while ( mBoard->IteratePlants(aPlant) != 0 )
    {
      if ( aPlant->mSeedType == SEED_PLANTERN && aPlant->NotOnGround() == 0 )
      {
        int aDeltaRow = aPlant->mRow - mGridY;
        int aDeltaCol = aPlant->mPlantCol - mGridX;
        if ( aDeltaRow < 0 )
          aDeltaRow = -aDeltaRow;
        if ( aDeltaCol < 0 )
          aDeltaCol = -aDeltaCol;
        if ( std::max(aDeltaRow, aDeltaCol) <= 1 )
        {
          aLit = true;
          break;
        }
      }
    }
  }

  if ( aLit )
  {
    if ( mTransparentCounter < 50 )
      ++mTransparentCounter;
  }
  else if ( mTransparentCounter > 0 )
  {
    --mTransparentCounter;
  }
  return 0;
}

// ==========================================================================
// GridItem::Update()
// @0013DD98  488 bytes
// ==========================================================================
int GridItem::Update()
{
  Reanimation *aReanim = (Reanimation *)mApp->ReanimationTryToGet(mGridItemReanimID);
  if ( aReanim != nullptr )
    aReanim->Update();

  TodParticleSystem *aParticle = mApp->ParticleTryToGet(mGridItemParticleID);
  if ( aParticle != nullptr )
    aParticle->Update();

  // Each of these re-reads mGridItemType, so an update that changes the type
  // immediately runs the handler for the new one.
  if ( mGridItemType == GRIDITEM_PORTAL_CIRCLE || mGridItemType == GRIDITEM_PORTAL_SQUARE )
    UpdatePortal();
  if ( mGridItemType == GRIDITEM_SCARY_POT )
    UpdateScaryPot();
  if ( mGridItemType == GRIDITEM_RAKE )
    UpdateRake();
  if ( mGridItemType == GRIDITEM_IZOMBIE_BRAIN )
    UpdateBrain();
  if ( mGridItemType == GRIDITEM_MP_BURIAL_MOUND )
    UpdateBurialMound();

  if ( mGridItemType == GRIDITEM_GRAVESTONE )
  {
    const unsigned int aGameMode = (unsigned int)(mApp->mGameMode - GAMEMODE_MP_VS_DEBUG);
    if ( aGameMode > 1 || mApp->mGameScene != SCENE_PLAYING )
      return 0;

    // The grave flashes brighter the closer it is to producing.
    if ( mGraveJustGotShotCounter > 0 )
      --mGraveJustGotShotCounter;
    --mLaunchCounter;

    if ( mLaunchCounter <= 100 )
    {
      const int aFlashCountdown = TodAnimateCurve(100, 0, mLaunchCounter, 0, 100, CURVE_LINEAR);
      mGraveJustGotShotCounter = std::max(mGraveJustGotShotCounter, aFlashCountdown);
    }

    if ( aReanim != nullptr )
    {
      if ( mGraveJustGotShotCounter <= 0 )
      {
        aReanim->mEnableExtraAdditiveDraw = false;
      }
      else
      {
        const int aGrayness = std::min(mGraveJustGotShotCounter * 3, 255);
        const int aGrayness2 = (aGrayness == 255) ? 127 : aGrayness / 2;
        aReanim->mExtraAdditiveColor = Sexy::Color(aGrayness, aGrayness2, aGrayness, 255);
        aReanim->mEnableExtraAdditiveDraw = true;
      }
    }

    if ( mLaunchCounter <= 0 )
    {
      mLaunchCounter = RandRangeInt(mLaunchRate - 150, mLaunchRate);
      const int aX = mBoard->GridToPixelX(mGridX, mGridY);
      const int aY = mBoard->GridToPixelY(mGridX, mGridY);
      mBoard->AddCoin(aX, aY, COIN_VS_ZOMBIE_BRAIN, COIN_MOTION_FROM_GRAVE_STONE);
    }
  }

  if ( mGridItemType == GRIDITEM_MP_TARGET_ZOMBIE )
  {
    if ( mVSTargetZombieDieCounter > 0 )
    {
      if ( --mVSTargetZombieDieCounter == 0 )
        return (int)GridItemDie();
      return 0;
    }

    // Not hit for a while: fall back to the idle animation, but only once the
    // hit animation has played through.
    Reanimation *aTargetReanim = (Reanimation *)mApp->ReanimationTryToGet(mGridItemReanimID);
    if ( aTargetReanim != nullptr && !aTargetReanim->IsAnimPlaying("anim_idle") )
    {
      if ( !aTargetReanim->IsAnimPlaying("anim_hit") || aTargetReanim->mAnimTime == 1.0f )
        aTargetReanim->PlayReanim("anim_idle", REANIM_LOOP, 0, 24.0f);
        return 0;
    }
  }

  return 0;
}

// ==========================================================================
// GridItem::DrawScaryPot(Sexy::Graphics *)
// @0013DF88  710 bytes
// ==========================================================================
int GridItem::DrawScaryPot(Sexy::Graphics *theGraphics)
{
  const int aImageCol = mGridItemState - GRIDITEM_STATE_SCARY_POT_QUESTION;

  const int aXPos = mBoard->GridToPixelX(mGridX, mGridY) - 5;
  const int aYPos = mBoard->GridToPixelY(mGridX, mGridY) - 15;

  TodDrawImageCelCenterScaledF(theGraphics, IMAGE_PLANTSHADOW2,
                               (float)aXPos - 5.0f, (float)aYPos + 72.0f, 0, 1.3f, 1.3f);

  // While the pot is lit up by the x-ray cheat, draw its contents inside it.
  if ( mTransparentCounter > 0 )
  {
    theGraphics->DrawImageCel(IMAGE_SCARY_POT, aXPos, aYPos, aImageCol, 0);
    theGraphics->PushState();

    switch ( mScaryPotType )
    {
      case SCARYPOT_SEED:
        theGraphics->mScaleX = 0.7f;
        theGraphics->mScaleY = 0.7f;
        DrawSeedPacket(theGraphics, (float)aXPos + 23.0f, (float)aYPos + 33.0f,
                       mSeedType, (SeedType)-1, 0.0f, 255, false, false, false, true);
        break;

      case SCARYPOT_ZOMBIE:
      {
        theGraphics->mScaleX = 0.4f;
        theGraphics->mScaleY = 0.4f;

        float aOffsetX = 6.0f;
        float aOffsetY = 19.0f;
        if ( mZombieType == ZOMBIE_FOOTBALL )
        {
          aOffsetX = 1.0f;
          aOffsetY = 16.0f;
        }
        else if ( mZombieType == ZOMBIE_GARGANTUAR )
        {
          aOffsetX = 15.0f;
          aOffsetY = 26.0f;
          theGraphics->mScaleX = 0.3f;
          theGraphics->mScaleY = 0.3f;
        }

        mApp->mReanimatorCache->DrawCachedZombie(theGraphics, aOffsetX + (float)aXPos, aOffsetY + (float)aYPos, mZombieType);
        break;
      }

      case SCARYPOT_SUN:
      {
        const int aSunCount = Challenge::ScaryPotterCountSunInPot(this);

        Reanimation aReanim;
        aReanim.ReanimationInitializeType(0.0f, 0.0f, REANIM_SUN);
        aReanim.OverrideScale(0.5f, 0.5f);

        // The suns are laid out on a small ring inside the pot.
        static const float kSunOffsetX[4] = { 45.0f, 35.0f, 48.0f, 47.0f };
        static const float kSunOffsetY[4] = { 42.0f, 52.0f, 57.0f, 47.0f };
        for ( int i = 0; i < aSunCount; ++i )
        {
          float aOffsetX = 42.0f;
          float aOffsetY = 62.0f;
          if ( i > 0 && i <= 4 )
          {
            aOffsetX = kSunOffsetX[i - 1];
            aOffsetY = kSunOffsetY[i - 1];
          }

          aReanim.SetPosition((float)aXPos + aOffsetX, (float)aYPos + aOffsetY);
          aReanim.Draw(theGraphics);
        }
        break;
      }

      default:
        break;
    }

    const int aAlpha = TodAnimateCurve(0, 50, mTransparentCounter, 255, 58, CURVE_LINEAR);
    theGraphics->SetColorizeImages(true);
    theGraphics->SetColor(Sexy::Color(255, 255, 255, aAlpha));
    theGraphics->PopState();
  }

  theGraphics->DrawImageCel(IMAGE_SCARY_POT, aXPos, aYPos, aImageCol, 1);

  if ( mHighlighted )
  {
    theGraphics->SetDrawMode(Sexy::Graphics::DRAWMODE_ADDITIVE);
    theGraphics->SetColorizeImages(true);
    if ( mTransparentCounter == 0 )
      theGraphics->SetColor(Sexy::Color(255, 255, 255, 196));
    theGraphics->DrawImageCel(IMAGE_SCARY_POT, aXPos, aYPos, aImageCol, 1);
    theGraphics->SetDrawMode(Sexy::Graphics::DRAWMODE_NORMAL);
  }

  theGraphics->SetColorizeImages(false);
  return 0;
}

// ==========================================================================
// GridItem::DrawGridItem(Sexy::Graphics *)
// @0013E280  182 bytes
// ==========================================================================
TodParticleSystem * GridItem::DrawGridItem(Sexy::Graphics *theGraphics)
{
  switch ( (int)mGridItemType )
  {
    case GRIDITEM_GRAVESTONE:
      DrawGraveStone(theGraphics);
      break;
    case GRIDITEM_CRATER:
      DrawCrater(theGraphics);
      break;
    case GRIDITEM_LADDER:
      DrawLadder(theGraphics);
      break;
    case GRIDITEM_BRAIN:
      theGraphics->DrawImageF(IMAGE_BRAIN, mPosX, mPosY);
      break;
    case GRIDITEM_SCARY_POT:
      DrawScaryPot(theGraphics);
      break;
    case GRIDITEM_SQUIRREL:
      DrawSquirrel(theGraphics);
      break;
    case GRIDITEM_STINKY:
      DrawStinky(theGraphics);
      return nullptr;
    case GRIDITEM_IZOMBIE_BRAIN:
      DrawIZombieBrain(theGraphics);
      break;
    case GRIDITEM_MP_TARGET_ZOMBIE:
      DrawMPTarget(theGraphics);
      break;
    default:
      break;
  }

  // DrawGraveStone draws the grave's own reanimation as part of the clipped stone,
  // so the common tail skips it for that type.
  if ( mGridItemType != GRIDITEM_GRAVESTONE )
  {
    Reanimation *aReanim = (Reanimation *)mApp->ReanimationTryToGet(mGridItemReanimID);
    if ( aReanim != nullptr )
      aReanim->Draw(theGraphics);
  }

  // The ARM tail-branches into TodParticleSystem::Draw (0x13E2CC), which is void
  // (TodParticleSystem.h:109-115), so the register it returns is not meaningful.
  // The particle the body fetched is what this declaration can honestly hand back.
  TodParticleSystem *aParticle = mApp->ParticleTryToGet(mGridItemParticleID);
  if ( aParticle != nullptr )
    aParticle->Draw(theGraphics);
  return aParticle;
}
