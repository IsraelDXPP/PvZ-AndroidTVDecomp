// GridItem -- reconstructed from libGameMain.so.
//
// Inherits `SyncObject` (0x10); the member table comes from the authoritative header
// PvZ\Lawn\Board\GridItem.h. That header's comments mix ordinals with byte values,
// so every offset below has been converted to a byte offset.
//
// Size **0x12C**: the stride of `DataArray<GridItem>` is 304(0x130) in Board.cpp:6592
// (`mGridItems`), minus the 4 bytes of `DataArrayItem::mID`.
// The tail of the layout agrees with that: the last member `unkMems2[3]` sits at
// +0x120 and ends exactly at +0x12C.
//
// vtable order (two destructor slots only):
//   [  0] 0x13c5a0  GridItem::~GridItem
//   [  1] 0x13c5ec  GridItem::~GridItem

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_GRIDITEM_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_GRIDITEM_H

#include "ReconTypes.h"
// The bodies need the complete types, so these includes stay for now (stage C
// narrows them to forward declarations).
#include "Graphics.h"
#include "Image.h"
#include "Reanimation.h"
#include "SharedImageRef.h"
#include "TodParticleSystem.h"
#include "Zombie.h"

// Board and LawnApp are stored only as pointers (`Board *mBoard`, `LawnApp *mApp`),
// and Challenge, ReanimatorCache and ZenGarden are not named in this file at all,
// so all five includes were pure cycle. The GridItem.h <-> ReanimatorCache.h edge
// mattered twice over: ReanimatorCache.h includes Zombie.h and Zombie.h used to
// include this header, so the triplet closed a three-header cycle.
// Sexy\ReconTypes.h already declares LawnApp.
class Board;
class Challenge;
class ReanimatorCache;
class ZenGarden;
class LawnApp;

// One frame of the motion trail (defined in GridItem.h in the authoritative header).
// 12 bytes.
class MotionTrailFrame
{
public:
    float mPosX;
    float mPosY;
    float mAnimTime;
};

class GridItem : public SyncObject
{
public:
    LawnApp *mApp; // +0x10
    Board *mBoard; // +0x14
    GridItemType mGridItemType; // +0x18
    GridItemState mGridItemState; // +0x1C
    int mGridX; // +0x20
    int mGridY; // +0x24
    int mGridItemCounter; // +0x28
    int mRenderOrder; // +0x2C
    bool mDead; // +0x30
    float mPosX; // +0x34
    float mPosY; // +0x38
    float mGoalX; // +0x3C
    float mGoalY; // +0x40
    ReanimationID mGridItemReanimID; // +0x44
    ParticleSystemID mGridItemParticleID; // +0x48
    ZombieType mZombieType; // +0x4C
    SeedType mSeedType; // +0x50
    ScaryPotType mScaryPotType; // +0x54
    bool mHighlighted; // +0x58
    int mTransparentCounter; // +0x5C
    int mSunCount; // +0x60
    MotionTrailFrame mMotionTrailFrames[12]; // +0x64 ~ +0xF3
    int mMotionTrailCount; // +0xF4
    bool mIsVSGraveStone; // +0xF8
    bool mIsSpecialGrave; // +0xF9
    int mMoundLevel; // +0xFC
    int mSummonIndex; // +0x100
    int mSummonCounter; // +0x104
    int mLaunchCounter; // +0x108
    int mLaunchRate; // +0x10C
    int mGraveJustGotShotCounter; // +0x110
    int mVSGraveStoneHealth; // +0x114
    int mVSTargetZombieHealth; // +0x118
    int mVSTargetZombieDieCounter; // +0x11C
    // +0x120 is a SharedImageRef, not three anonymous words: the constructor
    // placement-news one there (`SharedImageRef::SharedImageRef()` at 0x13DA90
    // with r0 = this + 0x120), and DrawMPTarget assigns the GetSharedImage result
    // to it before handing the image to the reanimation's "target" override. It is
    // 12 bytes, exactly the space `unkMems2[3]` used to occupy, and it sits
    // OUTSIDE the 260-byte sync block that starts at mGridItemType.
    Sexy::SharedImageRef mTargetImageOverride; // +0x120
    TodParticleSystem *GridItemDie();
    float *DrawGridItemOverlay(Sexy::Graphics *theGraphics);
    TodParticleSystem *UpdateBrain();
    void DrawIZombieBrain(Sexy::Graphics *theGraphics);
    void DrawGraveStone(Sexy::Graphics *theGraphics);
    void DrawMPTarget(Sexy::Graphics *theGraphics);
    void DrawStinky(Sexy::Graphics *theGraphics);
    int DrawCrater(Sexy::Graphics *theGraphics);
    int DrawLadder(Sexy::Graphics *theGraphics);
    void AddGraveStoneParticles();
    void OpenPortal();
    TodParticleSystem *ClosePortal();
    TodParticleSystem *UpdatePortal();
    void DrawSquirrel(Sexy::Graphics *theGraphics);
    Zombie *RakeFindZombie();
    Zombie *UpdateRake();
    int UpdateBurialMound();
    int GetMoundUpgradeCost();
    int TakeDamage(int theDamage, unsigned int theFlags);
    // Returns the rect BY VALUE. ARM passes a struct return in r0, so IDA read the
    // signature as `GridItem *GetItemRect(GridItem *this, _DWORD *a2)` with `this`
    // being the destination buffer; the transcription merged the two and wrote the
    // four rect words at object offset 0, over the vptr.
    Sexy::Rect GetItemRect();
    GridItem();
    int UpdateScaryPot();
    int Update();
    int DrawScaryPot(Sexy::Graphics *theGraphics);
    TodParticleSystem *DrawGridItem(Sexy::Graphics *theGraphics);
};

struct GridItemSlot
{
    GridItem mItem;
    unsigned int mID;
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_GRIDITEM_H
