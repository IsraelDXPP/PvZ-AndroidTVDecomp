// ReanimatorCache -- reconstructed from libGameMain.so.
//
// Plain class: no base class and no virtual function (the transcription's header
// has no vtable comment block, and the recon bodies never store a vptr).
//
// LAYOUT -- corrected against the binary. The older symbol dump this header used
// to carry put mPlantImages at +0x004, twelve bytes too early and twelve bytes
// short at the end: its `int *mImageVariationList` is the mod's
// `TodList<ReanimCacheImageVariation>`, which is four words, not one. Two
// independent loops in the binary fix every offset, and they agree word for word
// with the mod's own header PvZ\Lawn\System\ReanimationLawn.h:51-55:
//   * ReanimatorCacheInitialize @0x1A5140 writes gSexyAppBase to +0x188, then
//     clears +0x010..+0x0E7 (the loop `STR R1,[R2,#0x10]` with R3 = 0..0xD4 step
//     4, i.e. 54 words = SeedType::NUM_SEED_TYPES), then +0x0E8/+0x0EC/+0x0F0/
//     +0x0F4 (LawnMowerType::NUM_MOWER_TYPES = 4), then +0x0F8..+0x187 (R3 =
//     0..0x8C, 36 words = ZombieType::NUM_CACHED_ZOMBIE_TYPES);
//   * ReanimatorCacheDispose @0x1A5190 walks those same three ranges and, in
//     between, the linked list the four leading words describe.
// The word at +0x188 is therefore mApp, and the object is 99 words = 0x18C.
//
// TODO(recon): ReanimatorCacheInitialize does NOT clear the list header itself
// (the decompiler's `HIDWORD(this) = 0` is a misread of `MOV R1, R2`, which is
// only register shuffling); whatever constructs the object owns that, and this
// tree has no ReanimatorCache constructor yet.

#pragma once

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept as-is so other files' transitive dependencies do not change.
#include "Graphics.h"
#include "Reanimation.h"
#include "SexyAppBase.h"

// Zombie is not named anywhere in this file, so the include was pure cycle:
// ReanimatorCache.h -> Zombie.h -> GridItem.h -> ReanimatorCache.h was a
// three-header cycle, and Zombie.h is one of the most widely included headers in
// the tree, so the edge was expensive as well as wrong.
class Zombie;

// Forward declaration only: LawnApp.h includes this header, so pulling it in
// here would make the two mutually recursive.
class LawnApp;

namespace Sexy { class MemoryImage; }

// One cached plant variation. The mod's own source keeps the value
// (ReanimCacheImageVariation) apart from the node that holds it
// (TodListNode<ReanimCacheImageVariation>), but the binary only ever sees the
// node, and its size is the 0x14 the pool is created with:
// `FindGlobalAllocator(20)` in MakeCachedPlantFrame. The search loop there reads
// +0x00 (seed), +0x04 (variation), +0x08 (image) and +0x0C (next); the Dispose
// walk also reads +0x10 (prev).
struct ReanimCacheImageVariation
{
    SeedType mSeedType;                // +0x00
    DrawVariation mDrawVariation;      // +0x04
    Sexy::MemoryImage *mImage;         // +0x08
    ReanimCacheImageVariation *mNext;  // +0x0C
    ReanimCacheImageVariation *mPrev;  // +0x10
    // 0x14 bytes.
};

// The pool the nodes come from. MakeCachedPlantFrame reaches all five of its
// words through the list's allocator pointer.
#include "TodAllocator.h"
typedef TodAllocator ReanimCacheAllocator;

// TodList<ReanimCacheImageVariation>: head, tail, size, allocator. The binary
// inlines RemoveHead over these four words in ReanimatorCacheDispose.
struct ReanimCacheImageVariationList
{
    ReanimCacheImageVariation *mHead;   // +0x00
    ReanimCacheImageVariation *mTail;   // +0x04
    int mSize;                          // +0x08
    ReanimCacheAllocator *mpAllocator;  // +0x0C
    // 0x10 bytes.
};

class ReanimatorCache
{
public:
    ReanimCacheImageVariationList mImageVariationList; // +0x000 .. +0x00F
    Sexy::MemoryImage *mPlantImages[54];   // +0x010 .. +0x0E7
    Sexy::MemoryImage *mLawnMowers[4];     // +0x0E8 .. +0x0F7
    Sexy::MemoryImage *mZombieImages[36];  // +0x0F8 .. +0x187
    LawnApp *mApp;                         // +0x188
    // Size 0x18C (99 words).

    void UpdateReanimationForVariation(Reanimation *theReanim, DrawVariation theVariation);
    int GetPlantImageSize(SeedType theSeedType, int &theOffsetX, int &theOffsetY, int &theWidth, int &theHeight);
    Sexy::MemoryImage *MakeBlankCanvasImage(int theWidth, int theHeight);
    ReanimatorCache *ReanimatorCacheInitialize();
    int ReanimatorCacheDispose();
    void DrawCachedPlant(Sexy::Graphics *theGraphics, float theX, float theY, SeedType theSeedType, DrawVariation theVariation);
    int DrawCachedMower(Sexy::Graphics *theGraphics, float theX, float theY, LawnMowerType theMowerType);
    int DrawCachedZombie(Sexy::Graphics *theGraphics, float theX, float theY, ZombieType theZombieType);
    void LoadCachedImages();
    void SaveCachedImages();
    void DrawReanimatorFrame(Sexy::Graphics *theGraphics, float theX, float theY, ReanimationType theType, char const *theTrackName, DrawVariation theVariation);
    Sexy::Image *MakeCachedMowerFrame(LawnMowerType theMowerType);
    Sexy::Image *MakeCachedPlantFrame(SeedType theSeedType, DrawVariation theVariation);
    Sexy::Image *MakeCachedZombieFrame(ZombieType theZombieType);
};

// Layout check: 0x10 (list) + 216 (plants) + 16 (mowers) + 144 (zombies) + 4 (mApp).
