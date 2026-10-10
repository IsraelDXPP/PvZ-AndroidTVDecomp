// ReanimatorCache -- reconstructed from libGameMain.so.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_REANIMATORCACHE_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_REANIMATORCACHE_H

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept as-is so other files' transitive dependencies do not change.
#include "Graphics.h"
#include "Reanimation.h"
#include "SexyAppBase.h"
#include <vector>

// Zombie is not named anywhere in this file, so the include was pure cycle:
// ReanimatorCache.h -> Zombie.h -> GridItem.h -> ReanimatorCache.h was a
// three-header cycle, and Zombie.h is one of the most widely included headers in
// the tree, so the edge was expensive as well as wrong.
class Zombie;

// Forward declaration only: LawnApp.h includes this header, so pulling it in
// here would make the two mutually recursive.
class LawnApp;

namespace Sexy
{
class MemoryImage;
}

struct ReanimCacheImageVariation
{
    SeedType mSeedType;
    DrawVariation mDrawVariation;
    Sexy::Image *mImage;
};

class ReanimatorCache
{
public:
    std::vector<ReanimCacheImageVariation> mImageVariationList;
    Sexy::Image *mPlantImages[NUM_SEED_TYPES]{};
    Sexy::Image *mLawnMowers[NUM_MOWER_TYPES]{};
    Sexy::Image *mZombieImages[NUM_CACHED_ZOMBIE_TYPES]{};
    LawnApp *mApp = nullptr;

    ~ReanimatorCache();
    void UpdateReanimationForVariation(Reanimation *theReanim, DrawVariation theVariation);
    void GetPlantImageSize(SeedType theSeedType, int &theOffsetX, int &theOffsetY, int &theWidth, int &theHeight);
    Sexy::MemoryImage *MakeBlankCanvasImage(int theWidth, int theHeight);
    void ReanimatorCacheInitialize();
    void ReanimatorCacheDispose();
    void DrawCachedPlant(
        Sexy::Graphics *theGraphics, float theX, float theY, SeedType theSeedType, DrawVariation theVariation);
    void DrawCachedMower(Sexy::Graphics *theGraphics, float theX, float theY, LawnMowerType theMowerType);
    void DrawCachedZombie(Sexy::Graphics *theGraphics, float theX, float theY, ZombieType theZombieType);
    void LoadCachedImages();
    void SaveCachedImages();
    void DrawReanimatorFrame(Sexy::Graphics *theGraphics, float theX, float theY, ReanimationType theType,
        char const *theTrackName, DrawVariation theVariation);
    Sexy::Image *MakeCachedMowerFrame(LawnMowerType theMowerType);
    Sexy::Image *MakeCachedPlantFrame(SeedType theSeedType, DrawVariation theVariation);
    Sexy::Image *MakeCachedZombieFrame(ZombieType theZombieType);
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_REANIMATORCACHE_H
