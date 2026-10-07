// ReanimatorCache -- reconstructed from libGameMain.so.

#include "ReanimatorCache.h"
#include "Graphics.h"
#include "Image.h"
#include "LawnApp.h"
#include "MemoryImage.h"
#include "Reanimation.h"
#include "ReconTypes.h"
#include "SexyAppBase.h"
#include "Zombie.h"

ReanimatorCache::~ReanimatorCache()
{
  ReanimatorCacheDispose();
}

// ==========================================================================
// ReanimatorCache::UpdateReanimationForVariation(Reanimation *,DrawVariation)
// @001A4F88  286 bytes
// ==========================================================================
void ReanimatorCache::UpdateReanimationForVariation(Reanimation * theReanim, DrawVariation theVariation)
{
  // The receiver is the Reanimation, not the cache: the binary takes the cache
  // in r0 and never reads it -- 0x1A4F8C onwards only touches r1 (the
  // Reanimation) and r2 (the variation).
  if (theVariation == VARIATION_NORMAL)
    return;

  // Variations 2..12 are the eleven marigold petal colors, in table order.
  int aVariationIndex = theVariation - VARIATION_MARIGOLD_WHITE;
  if (theVariation >= VARIATION_MARIGOLD_WHITE && theVariation <= VARIATION_MARIGOLD_LIGHT_GREEN)
  {
    Sexy::Color aColors[11] = {
      Sexy::Color(255, 255, 255),
      Sexy::Color(230, 30, 195),
      Sexy::Color(250, 125, 5),
      Sexy::Color(255, 145, 215),
      Sexy::Color(160, 255, 245),
      Sexy::Color(230, 30, 30),
      Sexy::Color(5, 130, 255),
      Sexy::Color(195, 55, 235),
      Sexy::Color(235, 210, 255),
      Sexy::Color(255, 245, 55),
      Sexy::Color(180, 255, 105),
    };
    // A missing track comes back as index -1. Indexing the instance array
    // with that writes off the end, which is the cache fault on a plant
    // that has no Marigold_petals track.
    int aPetalTrack = theReanim->FindTrackIndex("Marigold_petals");
    if (aPetalTrack < 0)
      return;
    theReanim->mTrackInstances[aPetalTrack].mTrackColor = aColors[aVariationIndex];
  }
  else
  {
    switch (theVariation)
    {
      case VARIATION_IMITATER:
        theReanim->mFilterEffect = FILTER_EFFECT_WASHED_OUT;
        break;
      case VARIATION_IMITATER_LESS:
        theReanim->mFilterEffect = FILTER_EFFECT_LESS_WASHED_OUT;
        break;
      case VARIATION_ZEN_GARDEN:
        theReanim->SetFramesForLayer("anim_zengarden");
        break;
      case VARIATION_ZEN_GARDEN_WATER:
        theReanim->SetFramesForLayer("anim_waterplants");
        break;
      case VARIATION_AQUARIUM:
        theReanim->SetFramesForLayer("anim_idle_aquarium");
        break;
      case VARIATION_BIG_IDLE:
        theReanim->SetFramesForLayer("anim_bigidle");
        break;
      case VARIATION_SPROUT_NO_FLOWER:
        theReanim->SetFramesForLayer("anim_idle_noflower");
        break;
      default:
        break;
    }
  }
}

// ==========================================================================
// ReanimatorCache::GetPlantImageSize(SeedType,int &,int &,int &,int &)
// @001A50C0  80 bytes
// ==========================================================================
void ReanimatorCache::GetPlantImageSize(SeedType theSeedType, int & theOffsetX, int & theOffsetY, int & theWidth, int & theHeight)
{
  theOffsetX = -20;
  theOffsetY = -20;
  theWidth = 120;
  theHeight = 120;
  if (theSeedType == SEED_TALLNUT)
  {
    theOffsetY = -40;
    theHeight += 40;
  }
  else if (theSeedType == SEED_MELONPULT || theSeedType == SEED_WINTERMELON)
  {
    theOffsetX = -40;
    theWidth += 40;
  }
  else if (theSeedType == SEED_COBCANNON)
  {
    theWidth += 80;
  }
}

// ==========================================================================
// ReanimatorCache::MakeBlankCanvasImage(int,int)
// @001A5110  36 bytes
// ==========================================================================
Sexy::MemoryImage * ReanimatorCache::MakeBlankCanvasImage(int theWidth, int theHeight)
{
  // The dump's `operator new(0xE8)` plus a placement constructor is one `new`;
  // the slot it then calls through the vtable at +240 (0x1A512A) is
  // MemoryImage::Create.
  Sexy::MemoryImage *aImage = new Sexy::MemoryImage();
  aImage->Create(theWidth, theHeight);
  return aImage;
}

// ==========================================================================
// ReanimatorCache::ReanimatorCacheInitialize()
// @001A5140  72 bytes
// ==========================================================================
void ReanimatorCache::ReanimatorCacheInitialize()
{
  mApp = gLawnApp;
  for (int i = 0; i < NUM_SEED_TYPES; i++)
    mPlantImages[i] = nullptr;
  for (int i = 0; i < NUM_MOWER_TYPES; i++)
    mLawnMowers[i] = nullptr;
  for (int i = 0; i < NUM_CACHED_ZOMBIE_TYPES; i++)
    mZombieImages[i] = nullptr;
}

// ==========================================================================
// ReanimatorCache::ReanimatorCacheDispose()
// @001A5190  128 bytes
// ==========================================================================
void ReanimatorCache::ReanimatorCacheDispose()
{
  for (Sexy::Image *&aImage : mPlantImages)
  {
    delete aImage;
    aImage = nullptr;
  }

  for (const ReanimCacheImageVariation &aVariation : mImageVariationList)
    delete aVariation.mImage;
  mImageVariationList.clear();

  for (Sexy::Image *&aImage : mLawnMowers)
  {
    delete aImage;
    aImage = nullptr;
  }
  for (Sexy::Image *&aImage : mZombieImages)
  {
    delete aImage;
    aImage = nullptr;
  }
}

// ==========================================================================
// ReanimatorCache::DrawCachedPlant(Sexy::Graphics *,float,float,SeedType,DrawVariation)
// @001A5210  286 bytes
// ==========================================================================
void ReanimatorCache::DrawCachedPlant(Sexy::Graphics * theGraphics, float theX, float theY, SeedType theSeedType, DrawVariation theVariation)
{
  Sexy::Image *aPlantImage;

  // VARIATION_IMITATER_LESS (16), VARIATION_IMITATER (1) and VARIATION_NORMAL
  // (0) all come straight out of mPlantImages.
  if (theVariation == VARIATION_IMITATER_LESS || theVariation <= VARIATION_IMITATER)
  {
    aPlantImage = mPlantImages[theSeedType];
    if (aPlantImage == nullptr)
    {
      OutputDebug("(!!) ReanimatorCache uninitialized plant variation (%d).\n", (int)theSeedType);
      return;
    }
  }
  else
  {
    aPlantImage = nullptr;
    for (const ReanimCacheImageVariation &aVariation : mImageVariationList)
    {
      if (aVariation.mSeedType == theSeedType && aVariation.mDrawVariation == theVariation)
      {
        aPlantImage = aVariation.mImage;
        break;
      }
    }
    if (aPlantImage == nullptr)
    {
      OutputDebug("(!!) ReanimatorCache uninitialized plant variation (%d)-(%d).\n",
                  (int)theSeedType, (int)theVariation);
      return;
    }
  }

  int aOffsetX, aOffsetY, aWidth, aHeight;
  GetPlantImageSize(theSeedType, aOffsetX, aOffsetY, aWidth, aHeight);

  if (theVariation == VARIATION_IMITATER)
    aPlantImage = FilterEffectGetImage(aPlantImage, FILTER_EFFECT_WASHED_OUT);
  else if (theVariation == VARIATION_IMITATER_LESS)
    aPlantImage = FilterEffectGetImage(aPlantImage, FILTER_EFFECT_LESS_WASHED_OUT);

  // Unscaled and not 3D: blit at the natural size. The binary only takes this
  // path when the app is NOT 3D accelerated AND both scales are exactly 1.
  // Is3DAccelerated comes from SexyAppBase, which LawnApp reaches through
  // GamepadApp -> SexyCommonApp; the TV original does not override it.
  if (mApp->Is3DAccelerated() == 0 && theGraphics->mScaleX == 1.0 && theGraphics->mScaleY == 1.0)
  {
    theGraphics->DrawImage(aPlantImage, (int)(theX + (float)aOffsetX), (int)(theY + (float)aOffsetY));
    return;
  }

  TodDrawImageScaledF(theGraphics, aPlantImage,
                      theX + (float)(theGraphics->mScaleX * (float)aOffsetX),
                      theY + (float)(theGraphics->mScaleY * (float)aOffsetY),
                      theGraphics->mScaleX,
                      theGraphics->mScaleY);
}

// ==========================================================================
// ReanimatorCache::DrawCachedMower(Sexy::Graphics *,float,float,LawnMowerType)
// @001A5338  68 bytes
// ==========================================================================
void ReanimatorCache::DrawCachedMower(Sexy::Graphics * theGraphics, float theX, float theY, LawnMowerType theMowerType)
{
  Sexy::Image *aMowerImage = mLawnMowers[theMowerType];
  if (aMowerImage != nullptr)
  {
    TodDrawImageScaledF(theGraphics, aMowerImage, theX - 20.0, theY, theGraphics->mScaleX, theGraphics->mScaleY);
    return;
  }
  OutputDebug("(!!) ReanimatorCache uninitialized mower variation (%d).\n", (int)theMowerType);
}

// ==========================================================================
// ReanimatorCache::DrawCachedZombie(Sexy::Graphics *,float,float,ZombieType)
// @001A5380  52 bytes
// ==========================================================================
void ReanimatorCache::DrawCachedZombie(Sexy::Graphics * theGraphics, float theX, float theY, ZombieType theZombieType)
{
  Sexy::Image *aZombieImage = mZombieImages[theZombieType];
  if (aZombieImage != nullptr)
  {
    TodDrawImageScaledF(theGraphics, aZombieImage, theX, theY, theGraphics->mScaleX, theGraphics->mScaleY);
    return;
  }
  OutputDebug("(!!) ReanimatorCache uninitialized zombie variation (%d).\n", (int)theZombieType);
}

// ==========================================================================
// ReanimatorCache::LoadCachedImages()
// @001A53B8  708 bytes
// ==========================================================================
void ReanimatorCache::LoadCachedImages()
{
  // The binary builds each name as `aPath + StrFormat(...)`, with aPath living
  // across all three loops (its string destructor is the last one in the
  // function), so the split is the source's, not an artefact of the dump.
  const char *aPath = "images/";

  for (int i = 0; i < NUM_SEED_TYPES; i++)
  {
    if (mPlantImages[i] == nullptr)
    {
      Sexy::String aFileName;
      StrFormat(&aFileName, "seed_cached_%d.png", i);
      mPlantImages[i] = mApp->GetImage(aPath + aFileName, true);
    }
  }

  for (int i = 0; i < NUM_MOWER_TYPES; i++)
  {
    if (mLawnMowers[i] == nullptr)
    {
      Sexy::String aFileName;
      StrFormat(&aFileName, "mower_cached_%d.png", i);
      mLawnMowers[i] = mApp->GetImage(aPath + aFileName, true);
    }
  }

  for (int i = 0; i < NUM_CACHED_ZOMBIE_TYPES; i++)
  {
    if (mZombieImages[i] == nullptr)
    {
      Sexy::String aFileName;
      StrFormat(&aFileName, "zombie_cached_%d.png", i);
      mZombieImages[i] = mApp->GetImage(aPath + aFileName, true);
    }
  }
}

// ==========================================================================
// ReanimatorCache::SaveCachedImages()
// @001A56A0  880 bytes
// ==========================================================================
void ReanimatorCache::SaveCachedImages()
{
  const char *aPath = "images/";

  for (int i = 0; i < NUM_SEED_TYPES; i++)
  {
    if (mPlantImages[i] != nullptr)
    {
      Sexy::String aFileName;
      StrFormat(&aFileName, "seed_cached_%d.png", i);
      Sexy::MemoryImage *aMemoryImage = mPlantImages[i]->AsMemoryImage();
      if (aMemoryImage != nullptr)
        aMemoryImage->WriteToPng(aPath + aFileName);
    }
  }

  for (int i = 0; i < NUM_MOWER_TYPES; i++)
  {
    if (mLawnMowers[i] != nullptr)
    {
      Sexy::String aFileName;
      StrFormat(&aFileName, "mower_cached_%d.png", i);
      Sexy::MemoryImage *aMemoryImage = mLawnMowers[i]->AsMemoryImage();
      if (aMemoryImage != nullptr)
        aMemoryImage->WriteToPng(aPath + aFileName);
    }
  }

  for (int i = 0; i < NUM_CACHED_ZOMBIE_TYPES; i++)
  {
    if (mZombieImages[i] != nullptr)
    {
      Sexy::String aFileName;
      StrFormat(&aFileName, "zombie_cached_%d.png", i);
      Sexy::MemoryImage *aMemoryImage = mZombieImages[i]->AsMemoryImage();
      if (aMemoryImage != nullptr)
        aMemoryImage->WriteToPng(aPath + aFileName);
    }
  }
}

// ==========================================================================
// ReanimatorCache::DrawReanimatorFrame(Sexy::Graphics *,float,float,ReanimationType,char const*,DrawVariation)
// @001A5A28  166 bytes
// ==========================================================================
void ReanimatorCache::DrawReanimatorFrame(Sexy::Graphics * theGraphics, float theX, float theY, ReanimationType theType, char const* theTrackName, DrawVariation theVariation)
{
  // A throwaway Reanimation renders the single frame the caller asked for.
  Reanimation aReanim;
  aReanim.ReanimationInitializeType(theX, theY, theType);

  if (theTrackName != nullptr && aReanim.TrackExists(theTrackName) != 0)
    aReanim.SetFramesForLayer(theTrackName);

  // The dump splits these two over a shared LABEL_4; a cornpult never gets the
  // slowed-down rate.
  if (theType == REANIM_KERNELPULT)
    aReanim.AssignRenderGroupToTrack("Cornpult_butter", -1);
  else if (theType == REANIM_SUNFLOWER)
    aReanim.mAnimRate = 0.15f;

  aReanim.AssignRenderGroupToTrack("anim_waterline", -1);

  // A colorized Graphics hands its color over as the override.
  if (theGraphics->mColorizeImages != 0)
    aReanim.mColorOverride = theGraphics->mColor;

  aReanim.OverrideScale(theGraphics->mScaleX, theGraphics->mScaleY);
  UpdateReanimationForVariation(&aReanim, theVariation);
  // A cached frame is drawn before any Update. Plants without an
  // anim_waterline track still need their selected pose calculated.
  aReanim.mDirtyFlags |= 5;
  aReanim.Draw(theGraphics);
}

// ==========================================================================
// ReanimatorCache::MakeCachedMowerFrame(LawnMowerType)
// @001A5AE0  202 bytes
// ==========================================================================
Sexy::Image * ReanimatorCache::MakeCachedMowerFrame(LawnMowerType theMowerType)
{
  if (mLawnMowers[theMowerType] != nullptr)
    return mLawnMowers[theMowerType];

  Sexy::MemoryImage *aBlankCanvasImage = MakeBlankCanvasImage(90, 100);
  Sexy::Graphics aGraphics(aBlankCanvasImage);
  aGraphics.SetLinearBlend(true);

  switch (theMowerType)
  {
    case LAWNMOWER_LAWN:
      aGraphics.mScaleX = 0.85f;
      aGraphics.mScaleY = 0.85f;
      DrawReanimatorFrame(&aGraphics, 10.0f, 0.0f, REANIM_LAWNMOWER, "anim_normal", VARIATION_NORMAL);
      break;
    case LAWNMOWER_POOL:
      aGraphics.mScaleX = 0.8f;
      aGraphics.mScaleY = 0.8f;
      DrawReanimatorFrame(&aGraphics, 10.0f, 25.0f, REANIM_POOL_CLEANER, nullptr, VARIATION_NORMAL);
      break;
    case LAWNMOWER_ROOF:
      aGraphics.mScaleX = 0.85f;
      aGraphics.mScaleY = 0.85f;
      DrawReanimatorFrame(&aGraphics, 10.0f, 0.0f, REANIM_ROOF_CLEANER, nullptr, VARIATION_NORMAL);
      break;
    case LAWNMOWER_SUPER_MOWER:
      aGraphics.mScaleX = 0.85f;
      aGraphics.mScaleY = 0.85f;
      DrawReanimatorFrame(&aGraphics, 10.0f, 0.0f, REANIM_LAWNMOWER, "anim_tricked", VARIATION_NORMAL);
      break;
    default:
      break;
  }

  mLawnMowers[theMowerType] = aBlankCanvasImage;
  return aBlankCanvasImage;
}

// ==========================================================================
// ReanimatorCache::MakeCachedPlantFrame(SeedType,DrawVariation)
// @001A5BBC  986 bytes
// ==========================================================================
Sexy::Image * ReanimatorCache::MakeCachedPlantFrame(SeedType theSeedType, DrawVariation theVariation)
{
  if (theVariation != VARIATION_NORMAL)
  {
    // A non-default variation is only ever cached in the list; the dump returns
    // the stored image without a null test here.
    for (const ReanimCacheImageVariation &aVariation : mImageVariationList)
    {
      if (aVariation.mSeedType == theSeedType && aVariation.mDrawVariation == theVariation)
        return aVariation.mImage;
    }
  }
  else
  {
    Sexy::Image *aCached = mPlantImages[theSeedType];
    if (aCached != nullptr)
      return aCached;
  }

  int aOffsetX, aOffsetY, aWidth, aHeight;
  GetPlantImageSize(theSeedType, aOffsetX, aOffsetY, aWidth, aHeight);
  Sexy::MemoryImage *aBlankCanvasImage = MakeBlankCanvasImage(aWidth, aHeight);

  Sexy::Graphics aGraphics(aBlankCanvasImage);
  aGraphics.SetLinearBlend(true);

  PlantDefinition *aPlantDefinition = GetPlantDefinition(theSeedType);
  ReanimationType aReanimationType = aPlantDefinition->mReanimationType;

  if (theSeedType == SEED_POTATOMINE || theSeedType == SEED_INSTANT_COFFEE)
  {
    // These two are drawn 12 pixels in and at 0.8 scale.
    aOffsetX = (int)((float)aOffsetX - 12.0f);
    aOffsetY = (int)((float)aOffsetY - 12.0f);
    aGraphics.mScaleX = 0.8f;
    aGraphics.mScaleY = 0.8f;
    DrawReanimatorFrame(&aGraphics, (float)-aOffsetX, (float)-aOffsetY, aReanimationType,
                        theSeedType == SEED_POTATOMINE ? "anim_armed" : "anim_idle", theVariation);
  }
  else
  {
    if (theSeedType == SEED_EXPLODE_O_NUT)
    {
      aGraphics.SetColorizeImages(true);
      aGraphics.SetColor(Sexy::Color(255, 64, 64));
    }
    DrawReanimatorFrame(&aGraphics, (float)-aOffsetX, (float)-aOffsetY, aReanimationType,
                        "anim_idle", theVariation);
  }

  // The head layer. Snowpea/Repeater (5/7), Peashooter (0), Leftpeater (53) and
  // Gatlingpea (40) take one extra draw; Splitpea and Threepeater take their
  // own.
  if (theSeedType == SEED_SNOWPEA || theSeedType == SEED_REPEATER || theSeedType == SEED_PEASHOOTER || theSeedType == SEED_LEFTPEATER
      || theSeedType == SEED_GATLINGPEA)
  {
    DrawReanimatorFrame(&aGraphics, (float)-aOffsetX, (float)-aOffsetY, aReanimationType,
                        "anim_head_idle", theVariation);
  }
  else if (theSeedType == SEED_SPLITPEA)
  {
    DrawReanimatorFrame(&aGraphics, (float)-aOffsetX, (float)-aOffsetY, aReanimationType,
                        "anim_head_idle", theVariation);
    DrawReanimatorFrame(&aGraphics, (float)-aOffsetX, (float)-aOffsetY, aReanimationType,
                        "anim_splitpea_idle", theVariation);
  }
  else if (theSeedType == SEED_THREEPEATER)
  {
    DrawReanimatorFrame(&aGraphics, (float)-aOffsetX, (float)-aOffsetY, aReanimationType,
                        "anim_head_idle1", theVariation);
    DrawReanimatorFrame(&aGraphics, (float)-aOffsetX, (float)-aOffsetY, aReanimationType,
                        "anim_head_idle3", theVariation);
    DrawReanimatorFrame(&aGraphics, (float)-aOffsetX, (float)-aOffsetY, aReanimationType,
                        "anim_head_idle2", theVariation);
  }

  if (theVariation == VARIATION_NORMAL)
  {
    mPlantImages[theSeedType] = aBlankCanvasImage;
    return aBlankCanvasImage;
  }

  // TV inserts at the head, and disposal walks from the newest entry.
  mImageVariationList.insert(mImageVariationList.begin(),
      {theSeedType, theVariation, aBlankCanvasImage});
  return aBlankCanvasImage;
}

// ==========================================================================
// ReanimatorCache::MakeCachedZombieFrame(ZombieType)
// @001A5FC8  574 bytes
// ==========================================================================
Sexy::Image * ReanimatorCache::MakeCachedZombieFrame(ZombieType theZombieType)
{
  if (mZombieImages[theZombieType] != nullptr)
    return mZombieImages[theZombieType];

  // The zamboni is the one zombie whose frame is twice as wide.
  int aWidth = (theZombieType == ZOMBIE_ZAMBONI) ? 512 : 256;
  Sexy::MemoryImage *aBlankCanvasImage = MakeBlankCanvasImage(aWidth, 256);

  Sexy::Graphics aGraphics(aBlankCanvasImage);
  aGraphics.SetLinearBlend(true);

  // The cached pole vaulter with pole is built from the plain pole vaulter.
  ZombieType aUseZombieType =
      (theZombieType == ZOMBIE_CACHED_POLEVAULTER_WITH_POLE) ? ZOMBIE_POLEVAULTER : theZombieType;

  // +0x04 of the definition is ZombieDefinition::mReanimationType
  // (GetZombieDefinition has no typed declaration anywhere in this tree).
  ZombieDefinition *aZombieDefinition = GetZombieDefinition(aUseZombieType);
  ReanimationType aReanimationType = aZombieDefinition->mReanimationType;

  float aPosX = (theZombieType == ZOMBIE_ZAMBONI) ? 140.0f : 40.0f;

  if (aReanimationType == REANIM_ZOMBIE) // REANIM_ZOMBIE: the plain walking body
  {
    Reanimation aReanim;
    aReanim.ReanimationInitializeType(aPosX, 40.0f, aReanimationType);
    aReanim.SetFramesForLayer("anim_idle");
    Zombie::SetupReanimLayers(&aReanim, aUseZombieType);

    switch (theZombieType)
    {
      case ZOMBIE_DOOR:
        aReanim.AssignRenderGroupToTrack("anim_screendoor", 0);
        break;
      case ZOMBIE_TRASHCAN:
        aReanim.AssignRenderGroupToTrack("anim_screendoor", 0);
        aReanim.SetImageOverride("anim_screendoor", IMAGE_REANIM_ZOMBIE_TRASHCAN1);
        break;
      case ZOMBIE_FLAG:
      {
        Reanimation aFlagReanim;
        aFlagReanim.ReanimationInitializeType(aPosX, 40.0f, REANIM_FLAG);
        aFlagReanim.SetFramesForLayer("Zombie_flag");
        aFlagReanim.Draw(&aGraphics);
        break;
      }
      default:
        break;
    }
    aReanim.Draw(&aGraphics);
  }
  else if (aReanimationType != REANIM_BOSS) // not REANIM_BOSS
  {
    const char *aTrackName = "anim_idle";
    float aPosY = 40.0f;
    if (theZombieType == ZOMBIE_POGO)
      aTrackName = "anim_pogo";
    else if (theZombieType == ZOMBIE_POLEVAULTER)
      aTrackName = "anim_walk";
    else if (theZombieType == ZOMBIE_GARGANTUAR)
      aPosY = 60.0f;

    DrawReanimatorFrame(&aGraphics, aPosX, aPosY, aReanimationType, aTrackName, DrawVariation(0));
  }
  else // REANIM_BOSS: the body and its driver are two reanimations
  {
    Reanimation aReanim;
    aReanim.ReanimationInitializeType(-524.0f, -88.0f, aReanimationType);
    aReanim.SetFramesForLayer("anim_head_idle");

    Reanimation aDriverReanim;
    aDriverReanim.ReanimationInitializeType(46.0f, 22.0f, REANIM_BOSS_DRIVER);
    aDriverReanim.SetFramesForLayer("anim_idle");

    aReanim.Draw(&aGraphics);
    aDriverReanim.Draw(&aGraphics);
    aReanim.AssignRenderGroupToTrack("boss_body1", -1);
    aReanim.AssignRenderGroupToTrack("Boss_neck", -1);
    aReanim.AssignRenderGroupToTrack("Boss_head2", -1);
    aReanim.Draw(&aGraphics);
  }

  mZombieImages[theZombieType] = aBlankCanvasImage;
  return aBlankCanvasImage;
}
