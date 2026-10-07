// LawnMower -- reconstructed from libGameMain.so.
//
// A lawn mower is a SyncObject (0x10 bytes), not a GameObject: it has no drawing
// virtuals and is positioned by an explicit mRenderOrder. Every offset in the
// authoritative header PvZ\Lawn\Board\LawnMower.h is shifted by +0x10 here,
// which is exactly the SyncObject base the PC version does not have.
//
// Return values. Most of these methods are `void` in the binary -- the mangled
// names carry no return type -- and the transcription gave each one the type of
// whatever the last call happened to leave in r0. Where every caller discards
// that value the declaration was corrected to `void`. The three that still
// return a pointer or an int (`Die`, `EnableSuperMower`, `LawnMowerInitialize`)
// are called from other translation units that assign the result, so they keep
// their declared type and return nullptr where the register holds nothing real.

#include "LawnMower.h"
#include "DataArrayMethods.h"
#include "RawObject.h"
#include "game/Board.h"
#include "Challenge.h"
#include "CutScene.h"
#include "Graphics.h"
#include "Image.h"
#include "LawnApp.h"
#include "Reanimation.h"
#include "ReanimatorCache.h"
#include "TodFoley.h"
#include "Zombie.h"
#include <cstring>

namespace
{
// ReanimationType, ParticleEffect, RenderLayer and CoinMotion are still one-int
// surrogate structs in Sexy\ReconTypes.h, so their enumerators have no names in
// this tree yet. The values below are the ones from the authoritative
// PvZ\Lawn\Common\ConstEnums.h; naming them here keeps the call sites readable
// and makes the swap to real enumerators a one-line change each.
const ReanimationType kReanimLawnMower      = REANIM_LAWNMOWER;
const ReanimationType kReanimSplash         = REANIM_SPLASH;
const ReanimationType kReanimPoolCleaner    = REANIM_POOL_CLEANER;
const ReanimationType kReanimRoofCleaner    = REANIM_ROOF_CLEANER;
const ParticleEffect  kParticlePlantingPool = ParticleEffect(23);
const RenderLayer     kRenderLayerLawnMower = RENDER_LAYER_LAWN_MOWER;
const CoinMotion      kCoinMotionFromPlant  = COIN_MOTION_FROM_PLANT;
// Index into LawnPlayerInfo::mPurchases (PvZ\Lawn\Common\ConstEnums.h StoreItem).
const int             kStoreItemPoolCleaner = 22;
}

// ==========================================================================
// LawnMower::UpdatePool()
// @0017C6BC  490 bytes
// ==========================================================================
void LawnMower::UpdatePool()
{
  // The splash/water animation only runs while the mower is over the pool.
  const bool aOverPool = mPosX > 26.0f && mPosX < 660.0f;

  Reanimation *aReanim = (Reanimation *)mApp->ReanimationGet(mReanimID);

  if ( aOverPool && mMowerHeight == MOWER_HEIGHT_LAND )
  {
    // Just reached the water: splash once, then start sinking.
    Reanimation *aSplash = (Reanimation *)mApp->AddReanimation(
        mPosX, mPosY + 25.0f, mRenderOrder + 1, kReanimSplash);
    aSplash->OverrideScale(1.2f, 0.8f);
    mApp->AddTodParticle(mPosX + 50.0f, mPosY + 42.0f, mRenderOrder + 1, kParticlePlantingPool);
    mApp->PlayFoley(FOLEY_ZOMBIESPLASH);
    mMowerHeight = MOWER_HEIGHT_DOWN_TO_POOL;
    return;
  }

  if ( mMowerHeight == MOWER_HEIGHT_DOWN_TO_POOL )
  {
    mAltitude -= 2.0f;
    if ( mAltitude > -28.0f )
      return;                                   // still sinking

    mMowerHeight = MOWER_HEIGHT_IN_POOL;
    mAltitude = 0.0f;
    aReanim->PlayReanim("anim_water", REANIM_LOOP, 0, 0.0f);
  }
  else if ( mMowerHeight == MOWER_HEIGHT_UP_TO_LAND )
  {
    mAltitude += 2.0f;
    if ( mAltitude < 0.0f )
      return;                                   // still rising

    mAltitude = 0.0f;
    mMowerHeight = MOWER_HEIGHT_LAND;
    return;
  }
  else if ( mMowerHeight == MOWER_HEIGHT_IN_POOL )
  {
    if ( !aOverPool )
    {
      // Left the water: splash again on the way back up to the lawn.
      mMowerHeight = MOWER_HEIGHT_UP_TO_LAND;
      mAltitude = -28.0f;
      Reanimation *aLand = (Reanimation *)mApp->AddReanimation(
          mPosX, mPosY + 25.0f, mRenderOrder + 1, kReanimSplash);
      aLand->OverrideScale(1.2f, 0.8f);
      mApp->AddTodParticle(mPosX + 50.0f, mPosY + 42.0f, mRenderOrder + 1, kParticlePlantingPool);
      mApp->PlayFoley(FOLEY_PLANT_WATER);
      aReanim->PlayReanim("anim_land", REANIM_LOOP, 0, 0.0f);
    }
  }
  else
  {
    return;                                     // MOWER_HEIGHT_LAND, nothing to do
  }

  // In the water: keep the looping water animation going once the one-shot has
  // finished its hold.
  if ( aReanim->mLoopType == REANIM_PLAY_ONCE_AND_HOLD && aReanim->mLoopCount > 0 )
    aReanim->PlayReanim("anim_water", REANIM_LOOP, 10, 35.0f);
}

// ==========================================================================
// LawnMower::Draw(Sexy::Graphics *)
// @0017C8C0  508 bytes
// ==========================================================================
void LawnMower::Draw(Sexy::Graphics *theGraphics)
{
  if ( !mVisible )
    return;

  // The shadow is skipped while the mower is between heights (sinking or rising).
  if ( mMowerHeight == MOWER_HEIGHT_LAND && mMowerState != MOWER_SQUISHED )
  {
    const bool aIsNight = mBoard->StageIsNight() != 0;
    const LawnMowerType aType = mMowerType;
    float aShadowX = mPosX - 7.0f;
    float aShadowY = (mPosY + 47.0f) - mAltitude;
    float aScaleY;

    if ( aType == LAWNMOWER_POOL )
    {
      aScaleY = 1.0f;
      aShadowX -= 17.0f;
      aShadowY -= 8.0f;
    }
    else if ( aType == LAWNMOWER_ROOF )
    {
      aShadowX -= 9.0f;
      aScaleY = 1.2f;
      if ( mMowerState != MOWER_TRIGGERED )
        aShadowY -= 36.0f;
    }
    else
    {
      aScaleY = 1.0f;
    }

    Sexy::Image *aShadowImage = (Sexy::Image *)(aIsNight ? IMAGE_PLANTSHADOW2 : IMAGE_PLANTSHADOW);
    TodDrawImageCelCenterScaledF(theGraphics, aShadowImage, aShadowX, aShadowY, 0, 1.0f, aScaleY);
  }

  theGraphics->PushState();

  const float aAltitude = mAltitude;
  const LawnMowerType aType = mMowerType;
  float aDrawX = (theGraphics->mTransX + mPosX) + 6.0f;
  float aDrawY = (mPosY + theGraphics->mTransY) - aAltitude;
  theGraphics->mTransX = aDrawX;
  theGraphics->mTransY = aDrawY;

  LawnMowerState aState = mMowerState;
  bool aDrawReanim = false;

  if ( aType == LAWNMOWER_POOL )
  {
    if ( aState != MOWER_TRIGGERED )
      aDrawX = 33.0f;

    if ( aState == MOWER_TRIGGERED )
    {
      aDrawY -= 7.0f;
      theGraphics->mTransX = aDrawX - 10.0f;
    }
    else
    {
      aDrawY -= aDrawX;
    }
    theGraphics->mTransY = aDrawY;

    const bool aSinkingOrRising =
        mMowerHeight == MOWER_HEIGHT_DOWN_TO_POOL || mMowerHeight == MOWER_HEIGHT_UP_TO_LAND;
    if ( aSinkingOrRising )
    {
      theGraphics->SetClipRect(-50, -50, 150, (int)(aAltitude + 132.0f));
      aState = mMowerState;
    }
  }
  else if ( aType == LAWNMOWER_ROOF )
  {
    if ( aState == MOWER_TRIGGERED )
    {
      theGraphics->mTransY = aDrawY - 4.0f;
      theGraphics->mTransX = aDrawX - 10.0f;
      aDrawReanim = true;
    }
    else
    {
      theGraphics->mTransY = aDrawY - 40.0f;
    }
  }

  // Anything that is not mid-charge or squashed is drawn from the cache instead
  // of from a live reanimation.
  if ( !aDrawReanim && aState != MOWER_TRIGGERED && aState != MOWER_SQUISHED )
  {
    LawnMowerType aCacheType = mMowerType;
    if ( aCacheType == LAWNMOWER_LAWN && mBoard->mSuperMowerMode )
      aCacheType = LAWNMOWER_SUPER_MOWER;

    mApp->mReanimatorCache->DrawCachedMower(theGraphics, 0.0f, 19.0f, aCacheType);
    theGraphics->PopState();
  return;
  }

  Reanimation *aReanim = (Reanimation *)mApp->ReanimationGet(mReanimID);
  aReanim->Draw(theGraphics);
  theGraphics->PopState();
  return;
}

// ==========================================================================
// LawnMower::StartMower()
// @0017CAE8  192 bytes
// ==========================================================================
void LawnMower::StartMower()
{
  if ( mMowerState == MOWER_TRIGGERED )
    return;

  // In versus mode the mower pays out sun as it starts.
  if ( mApp->mGameMode == GAMEMODE_MP_VS
    && (Challenge::gVSResourseDropMode & 0xFFFFFFFD) == 1
    && Challenge::gVSResourceDropCount > 0 )
  {
    for ( int i = 0; i < Challenge::gVSResourceDropCount; ++i )
      mBoard->AddCoin(0, mBoard->GridToPixelY(0, mRow), COIN_SUN, kCoinMotionFromPlant);
  }

  Reanimation *aReanim = (Reanimation *)mApp->ReanimationGet(mReanimID);
  if ( mMowerType == LAWNMOWER_POOL )
  {
    aReanim->SetAnimRate(35.0f);
    mApp->PlayFoley(FOLEY_POOL_CLEANER);
  }
  else
  {
    aReanim->SetAnimRate(70.0f);
    mApp->PlayFoley(FOLEY_LAWNMOWER);
  }

  mBoard->mWaveRowGotLawnMowered[mRow] = mBoard->mCurrentWave;
  ++mBoard->mTriggeredLawnMowers;
  mMowerState = MOWER_TRIGGERED;
}

// ==========================================================================
// LawnMower::MowZombie(Zombie *)
// @0017CBB4  136 bytes
// ==========================================================================
int LawnMower::MowZombie(Zombie *theZombie)
{
  if ( mMowerState == MOWER_READY )
  {
    StartMower();
    mChompCounter = 25;
  }
  else if ( mMowerState == MOWER_TRIGGERED )
  {
    mChompCounter = 50;
  }

  if ( mMowerType == LAWNMOWER_POOL )
  {
    mApp->PlayFoley(FOLEY_SHOOP);

    Reanimation *aReanim = (Reanimation *)mApp->ReanimationGet(mReanimID);
    if ( mMowerHeight == MOWER_HEIGHT_IN_POOL )
      aReanim->PlayReanim("anim_suck", REANIM_PLAY_ONCE_AND_HOLD, 10, 35.0f);
    else
      aReanim->PlayReanim("anim_landsuck", REANIM_PLAY_ONCE_AND_HOLD, 10, 35.0f);

    {
      theZombie->DieWithLoot();
      return 0;
    }
  }

  mApp->PlayFoley(FOLEY_SPLAT);
  theZombie->MowDown();
  return 0;
}

// ==========================================================================
// LawnMower::GetLawnMowerAttackRect()
// @0017CC44  40 bytes
// ==========================================================================
Rect LawnMower::GetLawnMowerAttackRect()
{
  // The rect is returned by value: ARM passes its destination in r0, which is why
  // the transcription wrote these four stores at object offset 0, over the vptr.
  Rect aRect;
  aRect.mX = (int)mPosX;
  aRect.mY = (int)mPosY;
  aRect.mWidth = 50;
  aRect.mHeight = 80;
  return aRect;
}

// ==========================================================================
// LawnMower::SquishMower()
// @0017CC6C  74 bytes
// ==========================================================================
void LawnMower::SquishMower()
{
  Reanimation *aReanim = (Reanimation *)mApp->ReanimationGet(mReanimID);
  aReanim->OverrideScale(0.85f, 0.22f);
  aReanim->SetPosition(-11.0f, 65.0f);

  mMowerState = MOWER_SQUISHED;
  mSquishedCounter = 500;

  mApp->PlayFoley(FOLEY_SQUISH);
}

// ==========================================================================
// LawnMower::EnableSuperMower(bool)
// @0017CCB8  30 bytes
// ==========================================================================
void LawnMower::EnableSuperMower(bool theEnable)
{
  (void)theEnable;
  if ( mMowerType == LAWNMOWER_LAWN )
  {
    Reanimation *aReanim = (Reanimation *)mApp->ReanimationGet(mReanimID);
    aReanim->SetFramesForLayer("anim_tricked");
  }
}

// ==========================================================================
// LawnMower::LawnMowerInitialize(int)
// @0017CCDC  314 bytes
// ==========================================================================
void LawnMower::LawnMowerInitialize(int theRow)
{
  mApp = (LawnApp *)gSexyAppBase;
  mRow = theRow;
  mBoard = mApp->mBoard;
  mRenderOrder = Board::MakeRenderOrder(kRenderLayerLawnMower, theRow, 0);
  mPosX = -40.0f;
  mPosY = mBoard->GetPosYBasedOnRow(-120.0f, theRow) + 23.0f;
  mLastPortalX = -1;
  mDead = false;
  mChompCounter = 0;
  mRollingInCounter = 0;
  mSquishedCounter = 0;
  mMowerState = MOWER_READY;
  mVisible = true;

  ReanimationType aReanimType;
  if ( mBoard->StageHasRoof() )
  {
    mMowerType = LAWNMOWER_ROOF;
    aReanimType = kReanimRoofCleaner;
  }
  else if ( mBoard->mPlantRow[mRow] == 2 && mApp->mPlayerInfo->mPurchases[kStoreItemPoolCleaner] != 0 )
  {
    mMowerType = LAWNMOWER_POOL;
    aReanimType = kReanimPoolCleaner;
  }
  else
  {
    mMowerType = LAWNMOWER_LAWN;
    aReanimType = kReanimLawnMower;
  }

  Reanimation *aReanim = (Reanimation *)mApp->AddReanimation(0.0f, 18.0f, mRenderOrder, aReanimType);
  aReanim->mLoopType = REANIM_LOOP;
  aReanim->SetAnimRate(0.0f);
  aReanim->mIsAttachment = true;
  aReanim->OverrideScale(0.85f, 0.85f);

  mReanimID = mApp->ReanimationGetID(aReanim);

  if ( mMowerType == LAWNMOWER_POOL )
  {
    aReanim->OverrideScale(0.8f, 0.8f);
    aReanim->SetFramesForLayer("anim_land");
    aReanim->SetTruncateDisappearingFrames(nullptr, false);
  }
  else if ( mMowerType == LAWNMOWER_LAWN )
  {
    aReanim->SetFramesForLayer("anim_normal");
  }

  if ( mBoard->mSuperMowerMode )
    EnableSuperMower(true);
}

// ==========================================================================
// LawnMower::LawnMower()
// @0017CE28  76 bytes
// ==========================================================================
LawnMower::LawnMower()
{
  // The mower registers its whole state area mPosX..mLastPortalX (+0x18..+0x53,
  // 60 bytes) as one sync block, then zero-fills exactly that range. The dump
  // renders the registration as a mislabelled `_M_insert_aux` on an empty vector,
  // which is what push_back does.
  SyncBlockInfo aBlock = MakeSyncBlock(mPosX, mLastPortalX);
  mSyncBlocks.push_back(aBlock);
  memset(aBlock.mData, 0, aBlock.mSize);
}

// ==========================================================================
// LawnMower::Die()
// @0017CE94  170 bytes
// ==========================================================================
void LawnMower::Die()
{
  mDead = true;
  mApp->RemoveReanimation(mReanimID);

  if ( mBoard->mBonusLawnMowersRemaining > 0 && mBoard->HasLevelAwardDropped() == 0 )
  {
    // A bonus mower replaces this one. DataArrayAlloc, inlined: mFreeListHead is
    // a slot index, and while a slot is free its id word holds the index of the
    // next free slot; the list is empty exactly when the head has caught up with
    // the used prefix.
    unsigned int aIndex = mBoard->mLawnMowers.mFreeListHead;
    LawnMowerSlot *aSlots = mBoard->mLawnMowers.mBlock;
    LawnMowerSlot *aSlot = aSlots + aIndex;

    unsigned int aNextFree;
    if ( aIndex == mBoard->mLawnMowers.mMaxUsedCount )
    {
      aNextFree = aIndex + 1;
      mBoard->mLawnMowers.mMaxUsedCount = aNextFree;
    }
    else
    {
      aNextFree = aSlot->mID;
    }
    mBoard->mLawnMowers.mFreeListHead = aNextFree;

    // Clear the item, then stamp the new id: `key << 16 | index`, with the key
    // skipping zero on wrap.
    memset(&aSlot->mItem, 0, sizeof(LawnMower));
    unsigned int aKey = mBoard->mLawnMowers.mNextKey;
    aSlot->mID = aIndex | (aKey << 16);
    mBoard->mLawnMowers.mNextKey = (aKey == 0xFFFF) ? 1 : aKey + 1;
    ++mBoard->mLawnMowers.mSize;

    // The slot is raw block storage Board owns, so the mower is constructed in
    // place rather than heap-allocated.
    ConstructRaw<LawnMower>(&aSlot->mItem);
    aSlot->mItem.LawnMowerInitialize(mRow);
    aSlot->mItem.mMowerState = MOWER_ROLLING_IN;

    --mBoard->mBonusLawnMowersRemaining;
  }
}

// ==========================================================================
// LawnMower::Update()
// @0017CF40  606 bytes
// ==========================================================================
void LawnMower::Update()
{
  if ( mMowerState == MOWER_SQUISHED )
  {
    const bool aLastTick = mSquishedCounter == 1;
    --mSquishedCounter;
    if ( mSquishedCounter < 0 || aLastTick )
      Die();
    return;
  }

  if ( mMowerState == MOWER_ROLLING_IN )
  {
    // Roll in from off-screen until the mower reaches its resting position.
    ++mRollingInCounter;
    mPosX = TodAnimateCurveFloat(0, 100, mRollingInCounter, -40.0f, -21.0f, CURVE_EASE_IN_OUT);
    if ( mRollingInCounter == 100 )
      mMowerState = MOWER_READY;
    return;
  }

  const GameScenes aScene = mApp->mGameScene;
  const bool aMowersActive = aScene == SCENE_PLANTS_WON || aScene == SCENE_PLAYING;
  if ( !aMowersActive && mBoard->mCutScene->ShouldRunUpsellBoard() == 0 )
    return;

  Rect aAttackRect = GetLawnMowerAttackRect();

  Zombie *aZombie = nullptr;
  while ( mBoard->IterateZombies(aZombie) != 0 )
  {
    if ( aZombie->mZombieType == ZOMBIE_BOSS
      || aZombie->mRow != mRow
      || aZombie->mZombiePhase == PHASE_ZOMBIE_MOWERED
      || aZombie->IsTangleKelpTarget() != 0
      || aZombie->EffectedByDamage(0x7Fu) == 0 )
    {
      continue;
    }

    Rect aZombieRect = aZombie->GetZombieRect();
    const int aOverlap = GetRectOverlap(aAttackRect, aZombieRect);

    const ZombieType aZombieType = aZombie->mZombieType;
    // A balloon zombie has to be hit deeper before the mower catches it.
    const int aMinOverlap = (aZombieType == ZOMBIE_BALLOON) ? 20 : 0;
    const bool aHeadOn = aZombieType != ZOMBIE_BUNGEE && aZombie->mHasHead;

    if ( aMinOverlap < aOverlap && (mMowerState != MOWER_READY || aHeadOn) )
      MowZombie(aZombie);
  }

  if ( mMowerState != MOWER_TRIGGERED && mMowerState != MOWER_SQUISHED )
    return;

  float aSpeed = 3.33f;
  if ( mMowerType == LAWNMOWER_POOL )
    aSpeed = 2.5f;

  if ( mChompCounter > 0 )
  {
    --mChompCounter;
    aSpeed = TodAnimateCurveFloat(50, 0, mChompCounter, aSpeed, 1.0f, CURVE_BOUNCE_SLOW_MIDDLE);
  }

  const float aNewPosX = aSpeed + mPosX;
  mPosX = aNewPosX;
  mPosY = mBoard->GetPosYBasedOnRow(aNewPosX + 40.0f, mRow) + 23.0f;

  if ( mMowerType == LAWNMOWER_POOL )
    UpdatePool();

  if ( mMowerType == LAWNMOWER_LAWN && mBoard->mPlantRow[mRow] == 2 && mPosX > 50.0f )
  {
    // The mower has driven into the pool: splash, then die.
    Reanimation *aSplash = (Reanimation *)mApp->AddReanimation(
        mPosX, mPosY + 25.0f, mRenderOrder + 1, kReanimSplash);
    aSplash->OverrideScale(1.2f, 0.8f);
    mApp->AddTodParticle(mPosX + 50.0f, mPosY + 67.0f, mRenderOrder + 1, kParticlePlantingPool);
    mApp->PlaySample(SOUND_ZOMBIE_ENTERING_WATER, true);
    mApp->mSoundSystem->StopFoley(FOLEY_LAWNMOWER);
    Die();
  }

  if ( mPosX > 800.0f )
    Die();

  Reanimation *aReanim = (Reanimation *)mApp->ReanimationGet(mReanimID);
  aReanim->Update();
}
