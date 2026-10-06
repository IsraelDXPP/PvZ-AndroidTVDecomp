// Projectile -- transcribed from libGameMain.so, then rewritten against the
// reconstructed headers (stage 2).
//
// The bodies below are the IDA decompilation with the register plumbing removed:
// `this`-relative byte offsets are the named members from Projectile.h, GameObject.h
// and the other base headers, and the hand-written vtable slot calls are real
// virtual calls. The ARM sret shape is restored where the binary returns a
// Sexy::Rect by value (GetProjectileRect and friends).

#include "Projectile.h"
#include "DataArrayMethods.h"
#include "game/Board.h"
#include "CutScene.h"
#include "GameObject.h"
#include "Graphics.h"
#include "GridItem.h"
#include "Image.h"
#include "LawnApp.h"
#include "Plant.h"
#include "Reanimation.h"
#include "ReconTypes.h"
#include "SexyTransform2D.h"
#include "Zombie.h"
#include <cstring>
#include <cmath>

namespace
{
// Sexy\ReconTypes.h still models ProjectileType, GridItemType, ParticleEffect,
// PlantPriority and PlantWeapon as one-int surrogates rather than real enums, so
// their enumerators are not in scope. The names and values below are the ones the
// authoritative ConstEnums.h gives and the ones this binary compares against.
// They are plain ints: the surrogates convert both ways, so they work as switch
// labels and as the enum-typed arguments these bodies pass.



// The two GetTopPlantAt priorities FindCollisionTargetPlant passes are written
// out at the call site instead of being named here. Sexy\ReconTypes.h declares
// `enum TopPlant` with the STOCK PvZ order, where TOPPLANT_CATAPULT_ORDER is 1,
// but this binary loads 3: the ARM at 0x1932E6/0x1932E8 picks 0 or 3, and the
// authoritative libGameMain115.h has CatapultOrder = 3 (DiggingOrder = 1 and
// BungeeOrder = 2 come first). Naming them in this file would collide with that
// enum and make every use C2872.


// RenderLayer::RENDER_LAYER_TOP, which the two particle spawns pass as an already
// folded render order (400000 + row * 100 + order with row and order both zero).
} // namespace

// ==========================================================================
// Projectile::~Projectile()
// @00192DFC  32 bytes
// ==========================================================================
Projectile::~Projectile()
{
  AttachmentDie(mAttachmentID);
}

// ==========================================================================
// Projectile::CantHitHighGround()
// @00192E44  40 bytes
// ==========================================================================
int Projectile::CantHitHighGround()
{
  // Only the pea-like projectiles are stopped by the high ground: the dump
  // rejects everything except type <= 1, PUFF, FIREBALL and STAR.
  if (mMotionType == MOTION_BACKWARDS || mMotionType == MOTION_HOMING)
    return 0;
  if (mProjectileType > PROJECTILE_SNOWPEA && mProjectileType != PROJECTILE_PUFF
      && mProjectileType != PROJECTILE_FIREBALL && mProjectileType != PROJECTILE_STAR)
    return 0;

  return !mOnHighGround;
}

// ==========================================================================
// Projectile::IsSplashDamage(Zombie *)
// @00192E6C  56 bytes
// ==========================================================================
int Projectile::IsSplashDamage(Zombie *theZombie)
{
  if (mProjectileType != PROJECTILE_FIREBALL)
    return mProjectileType == PROJECTILE_MELON || mProjectileType == PROJECTILE_WINTERMELON;

  // A fireball is splash only against a target that is not fire resistant.
  if (theZombie == nullptr)
    return 1;
  if (theZombie->IsFireResistant() != 0)
    return 0;
  return mProjectileType == PROJECTILE_MELON || mProjectileType == PROJECTILE_WINTERMELON
      || mProjectileType == PROJECTILE_FIREBALL;
}

// ==========================================================================
// Projectile::IsSplashDamage(GridItem *)
// @00192EA4  24 bytes
// ==========================================================================
bool Projectile::IsSplashDamage(GridItem *theGridItem)
{
  return mProjectileType == PROJECTILE_MELON || mProjectileType == PROJECTILE_WINTERMELON
      || mProjectileType == PROJECTILE_FIREBALL;
}

// ==========================================================================
// Projectile::GetDamageFlags(Zombie *)
// @00192EBC  74 bytes
// ==========================================================================
int Projectile::GetDamageFlags(Zombie *theZombie)
{
  int aResult;

  if (IsSplashDamage(theZombie) != 0)
  {
    aResult = 2;
  }
  else if (mMotionType == MOTION_LOBBED || mMotionType == MOTION_BACKWARDS)
  {
    aResult = 1;
  }
  else
  {
    aResult = 0;
    if (mMotionType == MOTION_STAR)
      aResult = mVelX < 0.0;
  }
  if ((mProjectileType & ~4) == PROJECTILE_SNOWPEA)
    return aResult | 4;
  return aResult;
}

// ==========================================================================
// Projectile::PlayImpactSound(Zombie *)
// @00192F08  146 bytes
// ==========================================================================
void Projectile::PlayImpactSound(Zombie *theZombie)
{
  // `aPlaySplatSound` is the dump's v6; the kernel, butter, melon and the two
  // "ignite" branches clear it.
  bool aPlaySplatSound;

  if (mProjectileType == PROJECTILE_KERNEL)
  {
    mApp->PlayFoley(FOLEY_KERNEL_SPLAT);
    return;
  }
  if (mProjectileType == PROJECTILE_BUTTER)
  {
    mApp->PlayFoley(FOLEY_BUTTER);
    aPlaySplatSound = false;
  }
  else
  {
    if (mProjectileType == PROJECTILE_FIREBALL)
    {
      if (IsSplashDamage(theZombie) != 0)
      {
        mApp->PlayFoley(FOLEY_IGNITE);
        return;
      }
    }
    if (mProjectileType == PROJECTILE_MELON || mProjectileType == PROJECTILE_WINTERMELON)
    {
      mApp->PlayFoley(FOLEY_MELONIMPACT);
      aPlaySplatSound = false;
    }
    else
    {
      aPlaySplatSound = true;
    }
  }

  if (theZombie != nullptr)
  {
    // This binary's PlayImpactSound only knows the four original helm types: the
    // authoritative source also tests HELMTYPE_GIGA_FOOTBALL and
    // HELMTYPE_CROSSING_GUARD, so the dump's `helmType - 3 <= 1` (types 3 and 4)
    // is kept as written rather than widened.
    if (theZombie->mHelmType == HELMTYPE_PAIL)
    {
      mApp->PlayFoley(FOLEY_SHIELD_HIT);
      return;
    }
    if (theZombie->mHelmType == HELMTYPE_TRAFFIC_CONE || (unsigned int)(theZombie->mHelmType - HELMTYPE_FOOTBALL) <= 1)
      mApp->PlayFoley(FOLEY_PLASTIC_HIT);
  }
  if (aPlaySplatSound)
    mApp->PlayFoley(FOLEY_SPLAT);
}

// ==========================================================================
// Projectile::DrawShadow(Sexy::Graphics *)
// @00192F9C  426 bytes
// ==========================================================================
void Projectile::DrawShadow(Sexy::Graphics *theGraphics)
{
  if (mApp->mGameMode == GAMEMODE_CHALLENGE_HEAVY_WEAPON)
    return;

  float aOffsetX = mPosX - (float)mX;
  float aOffsetY = mPosY - (float)mY;
  int aGridX = mBoard->PixelToGridXKeepOnBoard(mX, mY);
  if (mOnHighGround)
  {
    if (mBoard->mGridSquareType[aGridX][mRow] != GRIDSQUARE_HIGH_GROUND)
      aOffsetY += 30.0;
  }
  else if (mBoard->mGridSquareType[aGridX][mRow] == GRIDSQUARE_HIGH_GROUND)
  {
    aOffsetY -= 30.0;
  }

  float aScale = 1.0;
  float aStretch = 1.0;

  switch (mProjectileType)
  {
    case PROJECTILE_PEA:
    case PROJECTILE_ZOMBIE_PEA:
      aOffsetX += 3.0;
      break;
    case PROJECTILE_SNOWPEA:
      aOffsetX -= 1.0;
      aStretch = 1.3;
      break;
    case PROJECTILE_STAR:
      aOffsetX += 7.0;
      break;
    case PROJECTILE_CABBAGE:
    case PROJECTILE_KERNEL:
    case PROJECTILE_BUTTER:
    case PROJECTILE_MELON:
    case PROJECTILE_WINTERMELON:
      aOffsetX += 3.0;
      aOffsetY += 10.0;
      aStretch = 1.6;
      break;
    case PROJECTILE_PUFF:
      return;
    case PROJECTILE_COBBIG:
      aScale = 3.0;
      aOffsetX += 57.0;
      break;
    case PROJECTILE_FIREBALL:
      aStretch = 1.4;
      break;
    default:
      break;
  }

  // A lobbed projectile's shadow shrinks with its height: -mPosZ clamped to
  // [0, 200] scales the vertical stretch by 200 / (clamp + 200).
  if (mMotionType == MOTION_LOBBED)
  {
    float aHeight = -mPosZ;
    if (aHeight > 200.0)
      aHeight = 200.0;
    if (aHeight < 0.0)
      aHeight = 0.0;
    aStretch *= 200.0 / (aHeight + 200.0);
  }

  // The shadow sits at mShadowY, which trails the projectile's own Y.
  TodDrawImageCelScaledF(theGraphics, IMAGE_PEA_SHADOWS, aOffsetX,
                         (mShadowY - mPosY) + aOffsetY, mBoard->StageIsNight(), 0,
                         aStretch * aScale, aStretch);
}

// ==========================================================================
// Projectile::Die()
// @00193168  52 bytes
// ==========================================================================
void Projectile::Die()
{
  mDead = true;
  // The puff and the snow pea cross-fade their attachment out instead of
  // dropping it. Both helpers are void; callers of Die ignore any result.
  if (mProjectileType == PROJECTILE_PUFF || mProjectileType == PROJECTILE_SNOWPEA)
  {
    AttachmentCrossFade(mAttachmentID, "FadeOut");
    AttachmentDetach(mAttachmentID);
    return;
  }
  AttachmentDie(mAttachmentID);
}

// ==========================================================================
// Projectile::GetProjectileDef()
// @001931A0  20 bytes
// ==========================================================================
const ProjectileDefinition * Projectile::GetProjectileDef()
{
  return &gProjectileDefinition[mProjectileType];
}

// ==========================================================================
// Projectile::GetProjectileRect()
// @001931BC  184 bytes
//
// ARM returns the 16-byte Sexy::Rect through the buffer in r0, so the dump's
// `this` IS that buffer: the four stores the dump writes at +0x00..+0x0C are the
// returned rectangle, not members of the object -- they land on the vptr.
// Returning by value is the real shape; the call sites outside this file still
// carry the merged spelling.
// ==========================================================================
Sexy::Rect Projectile::GetProjectileRect()
{
  switch (mProjectileType)
  {
    case PROJECTILE_COBBIG:
      return Sexy::Rect(mX + mWidth / 2 - 115, mY + mHeight / 2 - 115, 230, 230);
    case PROJECTILE_MELON:
    case PROJECTILE_WINTERMELON:
      return Sexy::Rect(mX + 20, mY, 60, mHeight);
    case PROJECTILE_FIREBALL:
      return Sexy::Rect(mX, mY, mWidth - 10, mHeight);
    case PROJECTILE_SPIKE:
      return Sexy::Rect(mX - 25, mY, mWidth + 25, mHeight);
    case PROJECTILE_PEA:
    case PROJECTILE_SNOWPEA:
    case PROJECTILE_ZOMBIE_PEA:
      return Sexy::Rect(mX - 15, mY, mWidth + 15, mHeight);
    default:
      // This binary has no zombie-pole case; everything else is the raw bounds.
      return Sexy::Rect(mX, mY, mWidth, mHeight);
  }
}

// ==========================================================================
// ==========================================================================
// Projectile::FindCollisionTargetPlant()
// @00193274  130 bytes
// ==========================================================================
Plant * Projectile::FindCollisionTargetPlant()
{
  Sexy::Rect aProjectileRect = GetProjectileRect();
  Plant * aPlant = nullptr;

  while (mBoard->IteratePlants(aPlant) != 0)
  {
    if (aPlant->mRow != mRow)
      continue;

    if (mProjectileType == PROJECTILE_ZOMBIE_PEA)
    {
      // A zombie pea never hits the low-profile plants. The dump tests the seed
      // type against {4, 8, 9, 21, 46, 16}, which are SUNSHROOM, SPIKEWEED,
      // SPIKEROCK, LILYPAD, FLOWERPOT and PUFFSHROOM.
      SeedType aSeedType = aPlant->mSeedType;
      if (aSeedType == SEED_SUNSHROOM || aSeedType == SEED_SPIKEWEED || aSeedType == SEED_SPIKEROCK
          || aSeedType == SEED_LILYPAD || aSeedType == SEED_FLOWERPOT || aSeedType == SEED_PUFFSHROOM)
        continue;
    }

    Sexy::Rect aPlantRect = aPlant->GetPlantRect();
    if (GetRectOverlap(aProjectileRect, aPlantRect) > 8)
    {
      // A zombie pea takes whatever the zombies would eat first; every other
      // projectile prefers the plant a catapult would target. The dump's 0/3 are
      // TOPPLANT_EATING_ORDER and TOPPLANT_CATAPULT_ORDER -- see the note on the
      // local constants above for why 3 is not Sexy\ReconTypes.h's value.
      PlantPriority aPriority = (PlantPriority)((mProjectileType == PROJECTILE_ZOMBIE_PEA) ? 0 : 3);
      return mBoard->GetTopPlantAt(aPlant->mPlantCol, aPlant->mRow, aPriority);
    }
  }
  return nullptr;
}

// ==========================================================================
// Projectile::FindCollisionTargetGridItem()
// @001932F8  308 bytes
// ==========================================================================
GridItem * Projectile::FindCollisionTargetGridItem()
{
  Sexy::Rect aProjectileRect = GetProjectileRect();
  GridItem * aBestItem = nullptr;
  GridItem * aGridItem = nullptr;

  while (mBoard->IterateGridItems(aGridItem) != 0)
  {
    if (aGridItem->mGridItemType == GRIDITEM_GRAVESTONE)
    {
      if (mRow != aGridItem->mGridY)
        continue;

      int aX = mBoard->GridToPixelX(aGridItem->mGridX, aGridItem->mGridY);
      int aY = mBoard->GridToPixelY(aGridItem->mGridX, aGridItem->mGridY);
      int aWidth = (IMAGE_TOMBSTONES)->GetCelWidth();
      int aHeight = (IMAGE_TOMBSTONES)->GetCelHeight();
      Sexy::Rect aGraveRect(aX, aY, aWidth, aHeight);
      if (GetRectOverlap(aProjectileRect, aGraveRect) <= 12)
        continue;

      // A gravestone always takes over from a VS target; against another
      // gravestone only the further-left one wins.
      if (aBestItem == nullptr || aBestItem->mGridItemType == GRIDITEM_MP_TARGET_ZOMBIE)
      {
        aBestItem = aGridItem;
        continue;
      }
      if (aGridItem->mGridX < aBestItem->mGridX)
        aBestItem = aGridItem;
    }
    else if (aGridItem->mGridItemType == GRIDITEM_MP_TARGET_ZOMBIE && aBestItem == nullptr
             && aGridItem->mGridItemCounter > 0)
    {
      if (mRow != aGridItem->mGridY)
        continue;

      int aCenterX = mBoard->GridToPixelX(aGridItem->mGridX, aGridItem->mGridY)
                   + mBoard->GridCellWidth(aGridItem->mGridX, aGridItem->mGridY) / 2;
      int aY = mBoard->GridToPixelY(aGridItem->mGridX, aGridItem->mGridY);
      Sexy::Image * aTargetImage = IMAGE_MP_TARGET;
      Sexy::Rect aTargetRect(aCenterX, aY, aTargetImage->GetCelWidth(), aTargetImage->GetCelHeight());
      if (GetRectOverlap(aProjectileRect, aTargetRect) > 12)
        aBestItem = aGridItem;
    }
  }
  return aBestItem;
}

// ==========================================================================
// Projectile::PeaAboutToHitTorchwood()
// @00193438  170 bytes
// ==========================================================================
int Projectile::PeaAboutToHitTorchwood()
{
  if (mMotionType != MOTION_STRAIGHT)
    return 0;
  if (mProjectileType > PROJECTILE_SNOWPEA)
    return 0;

  Plant * aPlant = nullptr;
  while (mBoard->IteratePlants(aPlant) != 0)
  {
    if (aPlant->mSeedType == SEED_TORCHWOOD && aPlant->mRow == mRow && aPlant->NotOnGround() == 0
        && mHitTorchwoodGridX != aPlant->mPlantCol)
    {
      Sexy::Rect aAttackRect = aPlant->GetPlantAttackRect(WEAPON_PRIMARY);
      Sexy::Rect aProjectileRect = GetProjectileRect();
      aProjectileRect.mX += 40;
      if (GetRectOverlap(aAttackRect, aProjectileRect) > 10)
        return 1;
    }
  }
  return 0;
}

// ==========================================================================
// Projectile::FindCollisionTarget()
// @001934F0  212 bytes
// ==========================================================================
Zombie * Projectile::FindCollisionTarget()
{
  if (PeaAboutToHitTorchwood() != 0)
    return nullptr;

  int aMinX = 0;
  Sexy::Rect aProjectileRect = GetProjectileRect();
  Zombie * aBestZombie = nullptr;
  Zombie * aZombie = nullptr;

  while (mBoard->IterateZombies(aZombie) != 0)
  {
    // The boss is hit from any row; everything else must share the row.
    if (aZombie->mZombieType != ZOMBIE_BOSS && aZombie->mRow != mRow)
      continue;
    if (aZombie->EffectedByDamage(mDamageRangeFlags) == 0)
      continue;
    // A snorkel zombie that is still walking in the pool is under the water line.
    if (aZombie->mZombiePhase == PHASE_SNORKEL_WALKING_IN_POOL && mPosZ <= 45.0)
      continue;
    // A fresh star does not hit a digger.
    if (mProjectileType == PROJECTILE_STAR && mProjectileAge <= 24 && mVelX >= 0.0
        && aZombie->mZombieType == ZOMBIE_DIGGER)
      continue;

    Sexy::Rect aZombieRect = aZombie->GetZombieRect();
    if (GetRectOverlap(aProjectileRect, aZombieRect) < 0)
      continue;

    // Of all the overlapping zombies, keep the one furthest along the direction of
    // travel.
    if (aBestZombie == nullptr || aZombie->mX < aMinX)
    {
      aMinX = aZombie->mX;
      aBestZombie = aZombie;
    }
  }
  return aBestZombie;
}

// ==========================================================================
// Projectile::IsZombieHitBySplash(Zombie *)
// @001935C8  138 bytes
// ==========================================================================
bool Projectile::IsZombieHitBySplash(Zombie *theZombie)
{
  Sexy::Rect aProjectileRect = GetProjectileRect();
  int aRowDeviation = theZombie->mRow - mRow;
  if (mProjectileType == PROJECTILE_FIREBALL)
    aProjectileRect.mWidth = 100;
  Sexy::Rect aZombieRect = theZombie->GetZombieRect();
  if (theZombie->IsFireResistant() != 0 && mProjectileType == PROJECTILE_FIREBALL)
    return false;
  if (theZombie->mZombieType != ZOMBIE_BOSS)
  {
    if (mProjectileType == PROJECTILE_FIREBALL)
    {
      if (aRowDeviation != 0)
        return false;
    }
    else if ((unsigned int)(aRowDeviation + 1) > 2)
    {
      return false;
    }
  }
  if (theZombie->EffectedByDamage(mDamageRangeFlags) == 0)
    return false;
  return GetRectOverlap(aProjectileRect, aZombieRect) >= 0;
}

// ==========================================================================
// Projectile::IsGridItemHitBySplash(GridItem *)
// @00193654  98 bytes
// ==========================================================================
bool Projectile::IsGridItemHitBySplash(GridItem *theGridItem)
{
  // The dump's two grid-item types {14, 1} are GRIDITEM_MP_TARGET_ZOMBIE and
  // GRIDITEM_GRAVESTONE; everything else is not splashable.
  if (theGridItem == nullptr)
    return false;
  if (theGridItem->mGridItemType != GRIDITEM_MP_TARGET_ZOMBIE && theGridItem->mGridItemType != GRIDITEM_GRAVESTONE)
    return false;

  Sexy::Rect aProjectileRect = GetProjectileRect();
  int aRowDeviation = theGridItem->mGridY - mRow;
  if (mProjectileType == PROJECTILE_FIREBALL)
  {
    aProjectileRect.mWidth = 100;
    if (aRowDeviation != 0)
      return false;
  }
  else if ((unsigned int)(aRowDeviation + 1) > 2)
  {
    return false;
  }
  Sexy::Rect aItemRect = theGridItem->GetItemRect();
  return GetRectOverlap(aProjectileRect, aItemRect) >= 0;
}

// ==========================================================================
// Projectile::DoSplashDamage(Zombie *,GridItem *)
// @001936B8  282 bytes
// ==========================================================================
void Projectile::DoSplashDamage(Zombie *theHitZombie, GridItem *theHitGridItem)
{
  const ProjectileDefinition * aProjectileDef = GetProjectileDef();

  // Count everything the splash reaches, excluding the two primary targets.
  int aTargetCount = 0;
  Zombie * aZombie = nullptr;
  while (mBoard->IterateZombies(aZombie) != 0)
  {
    if (theHitZombie != aZombie && IsZombieHitBySplash(aZombie))
      ++aTargetCount;
  }
  GridItem * aGridItem = nullptr;
  while (mBoard->IterateGridItems(aGridItem) != 0)
  {
    if (theHitGridItem != aGridItem && IsGridItemHitBySplash(aGridItem))
      ++aTargetCount;
  }

  // The main target takes the full damage; everything else shares a budget of
  // seven times the damage (once for a fireball), split evenly.
  int aFullDamage = aProjectileDef->mDamage;
  int aSpreadDamage = aFullDamage / 3;
  int aDamageBudget = (mProjectileType == PROJECTILE_FIREBALL) ? aFullDamage : aFullDamage * 7;
  if (aDamageBudget < aTargetCount * aSpreadDamage)
  {
    int aScaled = aFullDamage * aDamageBudget / (3 * aTargetCount * aSpreadDamage);
    aSpreadDamage = (aScaled < 1) ? 1 : aScaled;
  }

  aZombie = nullptr;
  while (mBoard->IterateZombies(aZombie) != 0)
  {
    if (IsZombieHitBySplash(aZombie))
    {
      unsigned int aDamageFlags = GetDamageFlags(aZombie);
      aZombie->TakeDamage((aZombie == theHitZombie) ? aFullDamage : aSpreadDamage, aDamageFlags);
    }
  }
  aGridItem = nullptr;
  while (mBoard->IterateGridItems(aGridItem) != 0)
  {
    if (IsGridItemHitBySplash(aGridItem) && aGridItem == theHitGridItem)
      aGridItem->TakeDamage(aFullDamage, 0);
  }
}

// ==========================================================================
// Projectile::DoImpact(Zombie *)
// @001937D4  774 bytes
// ==========================================================================
void Projectile::DoImpact(Zombie *theZombie)
{
  PlayImpactSound(theZombie);
  if (IsSplashDamage(theZombie) != 0)
  {
    if (mProjectileType == PROJECTILE_FIREBALL && theZombie != nullptr)
      theZombie->RemoveColdEffects();
    DoSplashDamage(theZombie, nullptr);
  }
  else if (theZombie != nullptr)
  {
    const ProjectileDefinition * aProjectileDef = GetProjectileDef();
    theZombie->TakeDamage(aProjectileDef->mDamage, GetDamageFlags(theZombie));
  }

  // aSpawnX/aSpawnY are the particle spawn point, which trails the projectile by
  // its velocity and its height above the shadow; aRow is the render order slot
  // just past this projectile's own.
  int aRow = mRenderOrder + 1;
  float aSpawnX = mPosX - mVelX;
  float aSpawnY = (mPosY + mPosZ) - mShadowY - mAccZ;

  switch (mProjectileType)
  {
    case PROJECTILE_MELON:
      mApp->AddTodParticle(aSpawnX + 30.0, aSpawnY + 30.0, aRow, PARTICLE_MELONSPLASH);
      Die();
      return;
    case PROJECTILE_WINTERMELON:
      mApp->AddTodParticle(aSpawnX + 30.0, aSpawnY + 30.0, aRow, PARTICLE_WINTERMELON);
      Die();
      return;
    case PROJECTILE_COBBIG:
      mApp->AddTodParticle(mPosX + 80.0, mPosY + 40.0,
                           Board::MakeRenderOrder(RenderLayer(200000), mCobTargetRow, 2),
                           PARTICLE_BLASTMARK);
      mApp->AddTodParticle(mPosX + 80.0, mPosY + 40.0, aRow, PARTICLE_POPCORNSPLASH);
      mApp->PlaySample(SOUND_DOOMSHROOM, true);
      mBoard->ShakeBoard(3, -4);
      Die();
      return;
    default:
      break;
  }

  // Everything else spawns a splat particle whose type and offset depend on the
  // projectile type.
  float aSplatX = mPosX + 12.0;
  float aSplatY = mPosY + 12.0;
  ParticleEffect aSplatType;
  switch (mProjectileType)
  {
    case PROJECTILE_PEA:
      aSplatType = PARTICLE_PEA_SPLAT;
      aSplatX -= 15.0;
      break;
    case PROJECTILE_SNOWPEA:
      aSplatType = PARTICLE_SNOWPEA_SPLAT;
      aSplatX -= 15.0;
      break;
    case PROJECTILE_STAR:
      aSplatType = PARTICLE_STAR_SPLAT;
      break;
    case PROJECTILE_PUFF:
      aSplatType = PARTICLE_PUFF_SPLAT;
      aSplatX -= 20.0;
      break;
    case PROJECTILE_CABBAGE:
      aSplatType = PARTICLE_CABBAGE_SPLAT;
      aSplatY = aSpawnY + 23.0;
      aSplatX = aSpawnX - 38.0;
      break;
    case PROJECTILE_BUTTER:
      aSplatType = PARTICLE_BUTTER_SPLAT;
      aSplatX = aSpawnX - 20.0;
      aSplatY = aSpawnY + 63.0;
      if (theZombie != nullptr)
      {
        theZombie->ApplyButter();
        break;
      }
      mApp->AddTodParticle(aSplatX, aSplatY, aRow, aSplatType);
      Die();
      return;
    case PROJECTILE_FIREBALL:
      if (IsSplashDamage(theZombie) != 0)
      {
        Reanimation * aReanim = mApp->AddReanimation(mPosX + 38.0, mPosY - 20.0, aRow,
                                                     REANIM_JALAPENO_FIRE);
        aReanim->mAnimTime = 0.25;
        aReanim->SetAnimRate(24.0);
        aReanim->OverrideScale(0.7, 0.4);
      }
      Die();
      return;
    default:
      Die();
      return;
  }

  if (theZombie == nullptr)
  {
    mApp->AddTodParticle(aSplatX, aSplatY, aRow, aSplatType);
    Die();
    return;
  }

  // A zombie hit attaches the splat to the zombie instead of the board. The two
  // phases the dump tests are the two in-pool walk phases, whose particle needs a
  // Y nudge.
  float aParticleX = (52.0 - (float)theZombie->mX) + aSplatX;
  float aParticleY = aSplatY - (float)theZombie->mY;
  if (theZombie->mZombiePhase == PHASE_SNORKEL_WALKING_IN_POOL || theZombie->mZombiePhase == PHASE_DOLPHIN_WALKING_IN_POOL)
    aParticleY += 60.0;
  // The nudge depends on the projectile's motion, not on its type.
  if (mMotionType == MOTION_BACKWARDS)
    aParticleX -= 80.0;
  else if (mPosX > (float)(theZombie->mX + 40) && mMotionType != MOTION_LOBBED)
    aParticleX -= 60.0;
  if (aParticleY > 100.0)
    aParticleY = 100.0;
  if (aParticleY < 20.0)
    aParticleY = 20.0;
  theZombie->AddAttachedParticle((int)aParticleX, (int)aParticleY, aSplatType);
  Die();
}

// ==========================================================================
// Projectile::CheckForHighGround()
// @00193B00  170 bytes
// ==========================================================================
void Projectile::CheckForHighGround()
{
  // `aNeedsLowGround` is the dump's v2: everything above SNOWPEA counts as a
  // high-flying shot except FIREBALL, and the "too close to the ground" test
  // re-admits SPIKE and COBBIG. aHeight is how far the shadow trails the shot.
  int aType = mProjectileType;
  bool aNeedsLowGround = (aType == PROJECTILE_FIREBALL) ? false : (aType > PROJECTILE_SNOWPEA);
  float aHeight = mShadowY - mPosY;

  if ((!aNeedsLowGround || aType == PROJECTILE_SPIKE || aType == PROJECTILE_COBBIG) && aHeight < 28.0)
  {
    DoImpact(nullptr);
    return;
  }

  if (aType != PROJECTILE_PUFF)
  {
    if (aType == PROJECTILE_STAR && aHeight < 23.0)
    {
      DoImpact(nullptr);
      return;
    }
    if (CantHitHighGround() == 0)
      return;
  }
  else
  {
    if (aHeight < 0.0)
    {
      DoImpact(nullptr);
      return;
    }
    if (CantHitHighGround() == 0)
      return;
  }

  // The high-ground column is raised, so a shot that reaches it lands on it. The
  // dump reaches the board's grid-square array through the +604 base and a
  // `row + 2 * 3 * gridX` word index, which is mGridSquareType[gridX][row]; the
  // +30 it feeds in is the column test offset.
  int aGridX = mBoard->PixelToGridXKeepOnBoard((int)mPosX + 30, (int)mPosY);
  if (mBoard->mGridSquareType[aGridX][mRow] == GRIDSQUARE_HIGH_GROUND)
    DoImpact(nullptr);
}

// ==========================================================================
// Projectile::DoImpactGridItem(GridItem *)
// @00193BAC  550 bytes
// ==========================================================================
void Projectile::DoImpactGridItem(GridItem *theGridItem)
{
  PlayImpactSound(nullptr);
  if (IsSplashDamage(theGridItem) != 0)
  {
    DoSplashDamage(nullptr, theGridItem);
  }
  else if (theGridItem != nullptr)
  {
    const ProjectileDefinition * aProjectileDef = GetProjectileDef();
    theGridItem->TakeDamage(aProjectileDef->mDamage, 0);
  }

  // The particle spawn point trails the shot by its velocity and its height above
  // the shadow; aRenderOrder is the slot just past this projectile's own.
  int aRenderOrder = mRenderOrder + 1;
  float aSpawnX = mPosX - mVelX;
  float aSpawnY = (mPosY + mPosZ) - mShadowY;
  float aTrailY = aSpawnY - mAccZ;

  switch (mProjectileType)
  {
    case PROJECTILE_MELON:
      mApp->AddTodParticle(aSpawnX + 30.0, aTrailY + 30.0, aRenderOrder, PARTICLE_MELONSPLASH);
      Die();
      return;
    case PROJECTILE_WINTERMELON:
      mApp->AddTodParticle(aSpawnX + 30.0, aTrailY + 30.0, aRenderOrder, PARTICLE_WINTERMELON);
      Die();
      return;
    case PROJECTILE_COBBIG:
      mApp->AddTodParticle(mPosX + 80.0, mPosY + 40.0,
                           Board::MakeRenderOrder(RenderLayer(200000), mCobTargetRow, 2),
                           PARTICLE_BLASTMARK);
      mApp->AddTodParticle(mPosX + 80.0, mPosY + 40.0, aRenderOrder, PARTICLE_POPCORNSPLASH);
      mApp->PlaySample(SOUND_DOOMSHROOM, true);
      mBoard->ShakeBoard(3, -4);
      Die();
      return;
    default:
      break;
  }

  float aSplatX = mPosX + 12.0;
  float aSplatY = mPosY + 12.0;
  ParticleEffect aSplatType;
  switch (mProjectileType)
  {
    case PROJECTILE_PEA:
      aSplatType = PARTICLE_PEA_SPLAT;
      aSplatX -= 15.0;
      break;
    case PROJECTILE_SNOWPEA:
      aSplatType = PARTICLE_SNOWPEA_SPLAT;
      aSplatX -= 15.0;
      break;
    case PROJECTILE_STAR:
      aSplatType = PARTICLE_STAR_SPLAT;
      break;
    case PROJECTILE_PUFF:
      aSplatType = PARTICLE_PUFF_SPLAT;
      aSplatX -= 20.0;
      break;
    case PROJECTILE_CABBAGE:
      aSplatType = PARTICLE_CABBAGE_SPLAT;
      aSplatY = aTrailY + 23.0;
      aSplatX = aSpawnX - 38.0;
      break;
    case PROJECTILE_BUTTER:
      aSplatType = PARTICLE_BUTTER_SPLAT;
      aSplatX = aSpawnX - 20.0;
      aSplatY = aTrailY + 63.0;
      break;
    case PROJECTILE_FIREBALL:
      if (IsSplashDamage(theGridItem) != 0)
      {
        Reanimation * aReanim = mApp->AddReanimation(mPosX + 38.0, mPosY - 20.0, aRenderOrder,
                                                     REANIM_JALAPENO_FIRE);
        aReanim->mAnimTime = 0.25;
        aReanim->SetAnimRate(24.0);
        aReanim->OverrideScale(0.7, 0.4);
      }
      Die();
      return;
    default:
      Die();
      return;
  }

  mApp->AddTodParticle(aSplatX, aSplatY, aRenderOrder, aSplatType);
  Die();
}

// ==========================================================================
// Projectile::UpdateLobMotion()
// @00193DEC  778 bytes
// ==========================================================================
void Projectile::UpdateLobMotion()
{
  // A cob cannon shell that has fallen past -700 drops onto its target column
  // before the normal lob integration runs.
  float aPosY;
  float aPosZ;
  if (mProjectileType == PROJECTILE_COBBIG && mPosZ < -700.0)
  {
    mAccZ = 8.0;
    mPosX = mCobTargetX;
    mRow = mCobTargetRow;
    int aGridX = mBoard->PixelToGridXKeepOnBoard((int)mCobTargetX, 0);
    aPosY = (float)mBoard->GridToPixelY(aGridX, mCobTargetRow);
    aPosZ = mPosZ;
    mPosY = aPosY;
    mRotation = -1.5708;
    mShadowY = aPosY + 67.0;
  }
  else
  {
    aPosY = mPosY;
    aPosZ = mPosZ;
  }

  int aType = mProjectileType;
  bool aHighGravity = (mApp->mGameMode == GAMEMODE_CHALLENGE_HIGH_GRAVITY);

  // Vertical integration: the velocity picks up the acceleration, and under high
  // gravity it picks it up twice.
  float aVelZ = mVelZ + mAccZ;
  mAccZ = aVelZ;
  if (aHighGravity)
  {
    aVelZ += mVelZ;
    mAccZ = aVelZ;
  }
  float aNewPosY = aPosY + mVelY;
  float aNewPosZ = aVelZ + aPosZ;
  float aNewPosX = mPosX + mVelX;
  mPosY = aNewPosY;
  mPosZ = aNewPosZ;
  mPosX = aNewPosX;
  if (aVelZ < 0.0)
  {
    // Still rising: a basketball and a cob shell cannot land, and no projectile
    // collides during its first 20 ticks.
    if ((aType & 0xFFFFFFFD) == PROJECTILE_BASKETBALL || mProjectileAge > 20)
      return;
  }
  else if (mProjectileAge > 20)
  {
    // Falling: how far below the launch line this type has to sink before it can
    // hit anything.
    float aMinCollisionZ;
    if (aType == PROJECTILE_BUTTER)
      aMinCollisionZ = -32.0;
    else if (aType == PROJECTILE_BASKETBALL)
      aMinCollisionZ = 60.0;
    else if (aType == PROJECTILE_MELON || aType == PROJECTILE_WINTERMELON)
      aMinCollisionZ = -35.0;
    else if (aType == PROJECTILE_CABBAGE || aType == PROJECTILE_KERNEL)
      aMinCollisionZ = -30.0;
    else if (aType == PROJECTILE_COBBIG)
      aMinCollisionZ = -60.0;
    else
      aMinCollisionZ = 0.0;

    // A pool row sits lower, so the shot has that much further to fall.
    if (mBoard->mPlantRow[mRow] == 2)
      aMinCollisionZ += 40.0;
    if (aMinCollisionZ >= aNewPosZ)
      return;
  }
  // Gather the three kinds of target. A basketball and a cob shell only ever hit
  // plants; everything else looks for zombies and, in VS mode, for grid items.
  Plant * aPlantTarget = nullptr;
  Zombie * aZombieTarget = nullptr;
  GridItem * aGridItemTarget = nullptr;
  bool aNoZombieTarget;
  bool aNoGridItemTarget;

  if ((aType & 0xFFFFFFFB) == PROJECTILE_BASKETBALL)
  {
    aPlantTarget = FindCollisionTargetPlant();
    aNoZombieTarget = (aPlantTarget == nullptr);
    aNoGridItemTarget = true;
  }
  else
  {
    aZombieTarget = FindCollisionTarget();
    if (mApp->mGameMode == GAMEMODE_MP_VS)
      aGridItemTarget = FindCollisionTargetGridItem();
    aNoZombieTarget = (aZombieTarget == nullptr);
    aNoGridItemTarget = true;
  }
  // How far the shot may still be above the ground and count as a landing.
  float aGroundZ = (aType == PROJECTILE_COBBIG) ? -40.0 : 80.0;

  if (aNoZombieTarget && aNoGridItemTarget)
  {
    if (aGridItemTarget == nullptr && mPosZ <= aGroundZ)
      return;
  }
  else if (aPlantTarget != nullptr)
  {
    Plant * aUmbrella = mBoard->FindUmbrellaPlant(aPlantTarget->mPlantCol, aPlantTarget->mRow);
    if (aUmbrella != nullptr)
    {
      if (aUmbrella->mState == STATE_UMBRELLA_REFLECTING)
      {
        mApp->PlayFoley(FOLEY_SPLAT);
        // 400001 is Board::MakeRenderOrder(RENDER_LAYER_TOP, 0, 1), folded.
        mApp->AddTodParticle(mPosX + 20.0, mPosY + 20.0, RENDER_LAYER_TOP + 1, PARTICLE_UMBRELLA_REFLECT);
      }
      else if (aUmbrella->mState != STATE_UMBRELLA_TRIGGERED)
      {
        mApp->PlayFoley(FOLEY_UMBRELLA);
        aUmbrella->DoSpecial();
      }
      return;
    }

    const ProjectileDefinition * aProjectileDef = GetProjectileDef();
    int aFlash = aPlantTarget->mEatenFlashCountdown;
    if (aFlash < 25)
      aFlash = 25;
    aPlantTarget->mPlantHealth -= aProjectileDef->mDamage;
    aPlantTarget->mEatenFlashCountdown = aFlash;
    mApp->PlayFoley(FOLEY_SPLAT);
    Die();
    return;
  }
  if (aType == PROJECTILE_COBBIG)
  {
    mBoard->KillAllZombiesInRadius(mRow, (int)(mPosX + 80.0), (int)(mPosY + 40.0), 115, 1, true,
                                   mDamageRangeFlags);
    DoImpact(nullptr);
    return;
  }
  if (aZombieTarget != nullptr)
  {
    DoImpact(aZombieTarget);
    return;
  }
  if (aGridItemTarget != nullptr)
  {
    DoImpactGridItem(aGridItemTarget);
    return;
  }
  if (mPosZ > aGroundZ)
    DoImpact(nullptr);
}

// ==========================================================================
// Projectile::CheckForCollision()
// @00194120  436 bytes
// ==========================================================================
void Projectile::CheckForCollision()
{
  if (mMotionType == MOTION_PUFF && mProjectileAge > 74)
  {
    Die();
    return;
  }
  if (mPosX > 800.0 || mPosX + (float)mWidth < 0.0)
  {
    Die();
    return;
  }
  // A homing shot only ever hits the zombie it was aimed at, and only once the
  // target's own rect covers the shot's Y.
  if (mMotionType == MOTION_HOMING)
  {
    Zombie * aZombie = mBoard->ZombieTryToGet(mTargetZombieID);
    if (aZombie != nullptr && aZombie->EffectedByDamage(mDamageRangeFlags) != 0)
    {
      Sexy::Rect aProjectileRect = GetProjectileRect();
      Sexy::Rect aZombieRect = aZombie->GetZombieRect();
      if (GetRectOverlap(aProjectileRect, aZombieRect) >= 0
          && mPosY > (float)aZombieRect.mY && mPosY < (float)(aZombieRect.mY + aZombieRect.mHeight))
        DoImpact(aZombie);
    }
    return;
  }

  // A star has to stay inside the vertical band; a star and a pea both stop
  // colliding once their shadow trails them by more than 90.
  if (mProjectileType == PROJECTILE_STAR)
  {
    if (mPosY > 600.0 || mPosY < 40.0)
    {
      Die();
      return;
    }
    if (mShadowY - mPosY > 90.0)
      return;
  }
  else if (mProjectileType == PROJECTILE_PEA && mShadowY - mPosY > 90.0)
  {
    return;
  }

  if (mMotionType == MOTION_FLOAT_OVER)
    return;

  // A zombie pea damages plants instead of zombies.
  if (mProjectileType == PROJECTILE_ZOMBIE_PEA)
  {
    Plant * aPlant = FindCollisionTargetPlant();
    if (aPlant == nullptr)
      return;

    const ProjectileDefinition * aProjectileDef = GetProjectileDef();
    int aFlash = aPlant->mEatenFlashCountdown;
    if (aFlash < 25)
      aFlash = 25;
    aPlant->mPlantHealth -= aProjectileDef->mDamage;
    aPlant->mEatenFlashCountdown = aFlash;
    mApp->PlayFoley(FOLEY_SPLAT);
    mApp->AddTodParticle(mPosX - 3.0, mPosY + 17.0, mRenderOrder + 1, PARTICLE_PEA_SPLAT);
    Die();
    return;
  }
  // Everything else only tests for a hit on every other tick.
  if ((mProjectileAge & 1) != 0)
  {
    Zombie * aZombie = FindCollisionTarget();
    if (aZombie != nullptr)
    {
      if (!aZombie->mOnHighGround || CantHitHighGround() == 0)
        DoImpact(aZombie);
    }
    else if (mApp->mGameMode == GAMEMODE_MP_VS)
    {
      GridItem * aGridItem = FindCollisionTargetGridItem();
      if (aGridItem != nullptr)
        DoImpactGridItem(aGridItem);
    }
  }
}

// ==========================================================================
// Projectile::UpdateNormalMotion()
// @001942E4  830 bytes
// ==========================================================================
void Projectile::UpdateNormalMotion()
{
  switch (mMotionType)
  {
    case MOTION_BACKWARDS:
      mPosX -= 3.33;
      break;

    case MOTION_HOMING:
    {
      // A homing projectile steers towards the zombie it was aimed at.
      Zombie * aZombie = mBoard->ZombieTryToGet(mTargetZombieID);
      float aVelX;
      float aVelY;
      if (aZombie != nullptr && aZombie->EffectedByDamage(mDamageRangeFlags) != 0)
      {
        Sexy::Rect aZombieRect = aZombie->GetZombieRect();
        float aLeadX = aZombie->ZombieTargetLeadX(0.0);
        float aPosY = mPosY;
        float aDeltaY = (float)((aZombieRect.mY + aZombieRect.mHeight / 2) - (mHeight / 2)) - aPosY;
        float aDeltaX = aLeadX - (mPosX + (float)(mWidth / 2));
        float aDist = sqrtf(aDeltaX * aDeltaX + aDeltaY * aDeltaY);
        if (aDist != 0.0)
        {
          aDeltaX /= aDist;
          aDeltaY /= aDist;
        }
        float aStep = (float)mProjectileAge * 0.001;
        aVelX = mVelX + aDeltaX * aStep;
        aVelY = mVelY + aDeltaY * aStep;
        float aSpeed = sqrtf(aVelX * aVelX + aVelY * aVelY);
        if (aSpeed != 0.0)
        {
          aVelX /= aSpeed;
          aVelY /= aSpeed;
        }
        aVelX += aVelX;
        aVelY += aVelY;
        mVelX = aVelX;
        mVelY = aVelY;
        mRotation = -atan2(aVelY, aVelX);
      }
      else
      {
        aVelX = mVelX;
        aVelY = mVelY;
      }
      mPosY += aVelY;
      mPosX += aVelX;
      mShadowY += aVelY;
      mRow = mBoard->PixelToGridYKeepOnBoard((int)mPosX, (int)mPosY);
      break;
    }

    case MOTION_STAR:
    {
      float aVelY = mVelY;
      mPosY += aVelY;
      mShadowY += aVelY;
      mPosX += mVelX;
      if (aVelY != 0.0)
        mRow = mBoard->PixelToGridYKeepOnBoard((int)mPosX, (int)mPosY);
      break;
    }

    case MOTION_BEE:
      // A bee drifts up for its first 60 ticks and then flies straight on.
      if (mProjectileAge <= 59)
        mPosY -= 0.5;
      mPosX += 3.33;
      break;

    case MOTION_BEE_BACKWARDS:
      if (mProjectileAge <= 59)
        mPosY -= 0.5;
      mPosX -= 3.33;
      break;

    case MOTION_FLOAT_OVER:
      // MOTION_FLOAT_OVER drifts up over the board under a very small gravity.
      if (mAccZ < 0.0)
      {
        float aAccZ = mAccZ + 0.002;
        if (aAccZ > 0.0)
          aAccZ = 0.0;
        mAccZ = aAccZ;
        mPosY += aAccZ;
        mRotation = (float)((aAccZ * -0.7) + 0.3) * 0.7854;
      }
      mPosX += 0.4;
      break;

    case MOTION_THREEPEATER:
      // The pea arcs over: its vertical component decays while X advances.
      {
        float aVelY = mVelY;
        float aDampedVelY = aVelY * 0.97;
        mPosY += aVelY;
        mShadowY += aDampedVelY;
        mPosX += 3.33;
        mVelY = aDampedVelY;
      }
      break;

    default:
      // MOTION_STRAIGHT, MOTION_LOBBED, MOTION_PUFF: straight ahead.
      mPosX += 3.33;
      break;
  }

  // The heavy-weapon challenge gives every projectile gravity.
  if (mApp->mGameMode == GAMEMODE_CHALLENGE_HEAVY_WEAPON)
  {
    float aGravity = (mMotionType == MOTION_FLOAT_OVER) ? 0.004 : 0.2;
    mAccZ += aGravity;
    mPosY += mAccZ;
  }

  CheckForCollision();
  if (mApp->mGameMode == GAMEMODE_CHALLENGE_HEAVY_WEAPON)
  {
    if (mPosY < 0.0)
      Die();
  }
  else
  {
    CheckForHighGround();
  }
}

// ==========================================================================
// Projectile::UpdateMotion()
// @00194650  188 bytes
// ==========================================================================
void Projectile::UpdateMotion()
{
  if (mAnimTicksPerFrame > 0)
  {
    mAnimCounter = (mAnimCounter + 1) % (mNumFrames * mAnimTicksPerFrame);
    mFrame = mAnimCounter / mAnimTicksPerFrame;
  }

  // The board's ground line follows the row the shot is on, so the Y it gains
  // from a row change has to be handed to the shot (and to its shadow) instead
  // of being baked into the motion itself.
  int aRow = mRow;
  float aOldGroundY = mBoard->GetPosYBasedOnRow(mPosX, aRow);

  if (mMotionType == MOTION_LOBBED)
    UpdateLobMotion();
  else
    UpdateNormalMotion();

  float aNewGroundY = mBoard->GetPosYBasedOnRow(mPosX, aRow);
  float aSlopeHeightChange = (mProjectileType == PROJECTILE_COBBIG) ? 0.0 : aNewGroundY - aOldGroundY;

  float aPosY = mPosY;
  float aPosZ = mPosZ;
  if (mMotionType == MOTION_FLOAT_OVER)
  {
    aPosY += aSlopeHeightChange;
    mPosY = aPosY;
  }
  else if (mMotionType == MOTION_LOBBED)
  {
    aPosY += aSlopeHeightChange;
    aPosZ -= aSlopeHeightChange;
    mPosY = aPosY;
    mPosZ = aPosZ;
  }

  mShadowY += aSlopeHeightChange;
  mY = (int)(aPosY + aPosZ);
  mX = (int)mPosX;
}

// ==========================================================================
// Projectile::Update()
// @00194710  230 bytes
// ==========================================================================
void Projectile::Update()
{
  mProjectileAge++;
  if (mApp->mGameScene != SCENE_PLAYING && !mBoard->mCutScene->ShouldRunUpsellBoard())
    return;

  // The render order is only refreshed once the shot has flown long enough; the
  // ten original projectile types are already at their final order.
  int aTime = 20;
  switch (mProjectileType)
  {
    case PROJECTILE_PEA:
    case PROJECTILE_SNOWPEA:
    case PROJECTILE_CABBAGE:
    case PROJECTILE_MELON:
    case PROJECTILE_WINTERMELON:
    case PROJECTILE_KERNEL:
    case PROJECTILE_COBBIG:
    case PROJECTILE_BUTTER:
    case PROJECTILE_ZOMBIE_PEA:
      aTime = 0;
      break;
    default:
      break;
  }
  if (mProjectileAge > aTime)
    mRenderOrder = Board::MakeRenderOrder(RenderLayer(305000), mRow, 0);

  if (mClickBackoffCounter > 0)
    --mClickBackoffCounter;
  mRotation += mRotationSpeed;

  UpdateMotion();
  if (mProjectileType != PROJECTILE_FIREBALL || mRotation == 0.0)
  {
    AttachmentUpdateAndMove(mAttachmentID, mPosX, mPosZ + mPosY);
  }
  else
  {
    // A spinning fireball carries its attachment along the rotation.
    SexyTransform2D aTransform;
    aTransform.RotateRad(mRotation);
    aTransform.Translate(mPosX, mPosZ + mPosY);
    AttachmentUpdateAndSetMatrix(mAttachmentID, aTransform);
  }
}

// ==========================================================================
// Projectile::ConvertToFireball(int)
// @001947F8  186 bytes
// ==========================================================================
void Projectile::ConvertToFireball(int theGridX)
{
  if (mHitTorchwoodGridX == theGridX)
    return;

  mHitTorchwoodGridX = theGridX;
  mProjectileType = PROJECTILE_FIREBALL;
  mApp->PlayFoley(FOLEY_FIREPEA);

  // A backwards shot has its flame mirrored and offset the other way.
  float aOffsetX = -25.0;
  Reanimation * aReanim = mApp->AddReanimation(0.0, 0.0, 0, REANIM_FIRE_PEA);
  if (mMotionType == MOTION_BACKWARDS)
  {
    aReanim->OverrideScale(-1.0, 1.0);
    aOffsetX += 80.0;
  }

  aReanim->SetPosition(mPosX + aOffsetX, mPosY - 25.0);
  aReanim->mLoopType = REANIM_LOOP;
  aReanim->SetAnimRate(RandRangeFloat(50.0, 80.0));
  AttachReanim(mAttachmentID, aReanim, aOffsetX, -25.0);
}

// ==========================================================================
// Projectile::ProjectileInitialize(int,int,int,int,ProjectileType)
// @001948B8  684 bytes
// ==========================================================================
void Projectile::ProjectileInitialize(int theX, int theY, int theRenderOrder, int theRow, ProjectileType theType)
{
  float aX = (float)theX;
  int aGridX = mBoard->PixelToGridXKeepOnBoard(theX, theY);
  mProjectileType = theType;
  mPosY = (float)theY;
  mPosX = aX;
  mPosZ = 0.0;
  mVelX = 0.0;
  mVelY = 0.0;
  mVelZ = 0.0;
  mAccZ = 0.0;
  float aGroundY = (float)mBoard->GridToPixelY(aGridX, theRow);
  mRow = theRow;
  mCobTargetX = 0.0;
  int aSquareType = mBoard->mGridSquareType[aGridX][theRow];
  mNumFrames = 1;
  mMotionType = MOTION_STRAIGHT;
  mFrame = 0;
  mDamageRangeFlags = 0;
  mDead = false;
  mAttachmentID = 0;
  mCobTargetRow = 0;
  mTargetZombieID = ZOMBIEID_NULL;
  mShadowY = aGroundY + 67.0;
  mOnHighGround = (aSquareType == GRIDSQUARE_HIGH_GROUND);
  mHitTorchwoodGridX = -1;
  mLastPortalX = -1;
  if (mBoard->StageHasRoof() && aX < 480.0)
    mShadowY -= 12.0;

  mWidth = 40;
  mHeight = 40;
  mRenderOrder = theRenderOrder;
  mRotation = 0.0;
  mRotationSpeed = 0.0;
  mProjectileAge = 0;
  mClickBackoffCounter = 0;
  mAnimTicksPerFrame = 0;

  // Cabbage, butter, melon and wintermelon tumble as they fly; the rest pick
  // their spin from the switch below.
  if (mProjectileType == PROJECTILE_CABBAGE || mProjectileType == PROJECTILE_BUTTER)
  {
    mRotation = -0.8796459;   // -7 * pi / 25
    mRotationSpeed = RandRangeFloat(-0.08, -0.02);
  }
  else if (mProjectileType == PROJECTILE_MELON || mProjectileType == PROJECTILE_WINTERMELON)
  {
    mRotation = -1.2566371;   // -2 * pi / 5
    mRotationSpeed = RandRangeFloat(-0.08, -0.02);
  }
  else
  {
    switch (mProjectileType)
    {
      case PROJECTILE_KERNEL:
        mRotationSpeed = RandRangeFloat(-0.2, -0.08);
        break;
      case PROJECTILE_SNOWPEA:
      {
        // 400000 is RENDER_LAYER_TOP, folded at compile time.
        TodParticleSystem* aParticle = mApp->AddTodParticle(mPosX + 8.0, mPosY + 13.0, RENDER_LAYER_TOP,
                                             PARTICLE_SNOWPEA_TRAIL);
        AttachParticle(mAttachmentID, aParticle, 8.0, 13.0);
        break;
      }
      case PROJECTILE_FIREBALL:
        ConvertToFireball(0);
        break;
      case PROJECTILE_COBBIG:
      {
        Sexy::Image * aCobImage = IMAGE_REANIM_COBCANNON_COB;
        mWidth = aCobImage->GetCelWidth();
        mHeight = aCobImage->GetCelHeight();
        mRotation = 1.5707964;   // pi / 2
        break;
      }
      case PROJECTILE_PUFF:
      {
        TodParticleSystem* aParticle = mApp->AddTodParticle(mPosX + 13.0, mPosY + 13.0, RENDER_LAYER_TOP,
                                             PARTICLE_PUFFSHROOM_TRAIL);
        AttachParticle(mAttachmentID, aParticle, 13.0, 13.0);
        break;
      }
      case PROJECTILE_BASKETBALL:
        mRotation = Rand(6.2831855);
        mRotationSpeed = RandRangeFloat(0.05, 0.1);
        break;
      case PROJECTILE_STAR:
        mShadowY += 15.0;
        mRotationSpeed = RandRangeFloat(0.05, 0.1);
        if (Rand(2) == 0)
          mRotationSpeed = -mRotationSpeed;
        break;
      default:
        break;
    }
  }

  mAnimCounter = 0;
  mX = (int)mPosX;
  mY = (int)mPosY;
}

// ==========================================================================
// Projectile::ConvertToPea(int)
// @00194B74  42 bytes
// ==========================================================================
void Projectile::ConvertToPea(int theGridX)
{
  if (mHitTorchwoodGridX == theGridX)
    return;

  AttachmentDie(mAttachmentID);
  mHitTorchwoodGridX = theGridX;
  mProjectileType = PROJECTILE_PEA;
  mApp->PlayFoley(FOLEY_THROW);
}

// ==========================================================================
// Projectile::Projectile()
// @00194BA0  98 bytes
// ==========================================================================
Projectile::Projectile()
{
  // Only clear the derived state. Debug STL bookkeeping enlarges GameObject;
  // subtracting the original base size would overwrite the data array slot ID.
  const unsigned int aStateSize = sizeof(Projectile) - sizeof(GameObject);
  mSyncBlocks.push_back(SyncBlockInfo{&mFrame, aStateSize});
  memset(&mFrame, 0, aStateSize);
}

// ==========================================================================
// Projectile::Draw(Sexy::Graphics *)
// @00194C14  610 bytes
// ==========================================================================
void Projectile::Draw(Sexy::Graphics *theGraphics)
{
  const ProjectileDefinition * aProjectileDef = GetProjectileDef();

  Sexy::Image * aImage = nullptr;
  float aScale = 1.0;
  switch (mProjectileType)
  {
    case PROJECTILE_COBBIG:
      aScale = 0.9;
      aImage = IMAGE_REANIM_COBCANNON_COB;
      break;
    case PROJECTILE_PEA:
    case PROJECTILE_ZOMBIE_PEA:
      aImage = IMAGE_PROJECTILEPEA;
      break;
    case PROJECTILE_SNOWPEA:
      aImage = IMAGE_PROJECTILESNOWPEA;
      break;
    case PROJECTILE_FIREBALL:
      // A fireball is drawn entirely by its attachment, so it has no cel.
      break;
    case PROJECTILE_SPIKE:
      aImage = IMAGE_PROJECTILECACTUS;
      break;
    case PROJECTILE_STAR:
      aImage = IMAGE_PROJECTILE_STAR;
      break;
    case PROJECTILE_PUFF:
      aImage = IMAGE_PUFFSHROOM_PUFF1;
      // A puff inflates over its first 30 ticks.
      aScale = TodAnimateCurveFloat(0, 30, mProjectileAge, 0.3, 1.0, CURVE_LINEAR);
      break;
    case PROJECTILE_BASKETBALL:
      aScale = 1.1;
      aImage = IMAGE_REANIM_ZOMBIE_CATAPULT_BASKETBALL;
      break;
    case PROJECTILE_CABBAGE:
      aImage = IMAGE_REANIM_CABBAGEPULT_CABBAGE;
      break;
    case PROJECTILE_KERNEL:
      aScale = 0.95;
      aImage = IMAGE_REANIM_CORNPULT_KERNAL;
      break;
    case PROJECTILE_BUTTER:
      aScale = 0.8;
      aImage = IMAGE_REANIM_CORNPULT_BUTTER;
      break;
    case PROJECTILE_MELON:
      aImage = IMAGE_REANIM_MELONPULT_MELON;
      break;
    default:
      if (mProjectileType == PROJECTILE_WINTERMELON)
        aImage = IMAGE_REANIM_WINTERMELON_PROJECTILE;
      break;
  }

  if (aImage != nullptr)
  {
    int aCelWidth = aImage->GetCelWidth();
    int aCelHeight = aImage->GetCelHeight();
    TRect<int> aSrcRect(mFrame * aCelWidth, aProjectileDef->mImageRow * aCelHeight,
                        aCelWidth, aCelHeight);
    if (fabsf(mRotation) < 0.000001 && fabsf(aScale - 1.0) < 0.000001)
    {
      // Upright and unscaled: no transform needed, so the frame can be blitted
      // with the mirror flag alone.
      TRect<int> aDestRect(0, 0, aCelWidth, aCelHeight);
      theGraphics->DrawImageMirror(aImage, aDestRect, aSrcRect, mMotionType == MOTION_BEE_BACKWARDS);
    }
    else
    {
      // The board's own origin is already folded into the graphics transform.
      float aCenterX = theGraphics->mTransX + (float)aCelWidth * 0.5;
      float aCenterY = theGraphics->mTransY + mPosZ + (float)mBoard->mY + (float)aCelHeight * 0.5;
      SexyTransform2D aTransform;
      TodScaleRotateTransformMatrix(aTransform, aCenterX, aCenterY, mRotation, aScale, aScale);
      TodBltMatrix(theGraphics, aImage, aTransform, theGraphics->mClipRect, Color::White, theGraphics->mDrawMode, aSrcRect);
    }
  }

  if (mAttachmentID != 0)
  {
    Sexy::Graphics aAttachmentGraphics(*theGraphics);
    MakeParentGraphicsFrame(&aAttachmentGraphics);
    AttachmentDraw(mAttachmentID, &aAttachmentGraphics, 0);
  }
}
