// EffectSystem -- transcribed from libGameMain.so
// Rewritten by hand in stage 2; see build/stage2-brief.md.

#include "EffectSystem.h"

#include "Attachment.h"
#include "AttachmentHolder.h"
#include "Reanimation.h"
#include "ReanimationHolder.h"
#include "TodParticle.h"
#include "TodParticleEmitter.h"
#include "TodParticleHolder.h"
#include "TodParticleSystem.h"
#include "Trail.h"
#include "TrailHolder.h"
#include <cstring>

namespace {

// Every holder in this file is a `DataArray<T>` expanded in place, and the
// template's element is `{ T mItem; uint32_t mID; }` with the key in the id's
// high half and the slot index in its low half
// (PvZ\TodLib\Common\DataArray.h:28-50). The slot types are modelled next to
// their users, the way Game\TrailHolder.cpp models DataArray<Trail>; each one is
// exactly the stride its holder allocates with.

} // namespace

// ==========================================================================
// EffectSystem::EffectSystem()
// @00205070  12 bytes
// ==========================================================================
EffectSystem::EffectSystem()
{
  mParticleHolder = nullptr;
  mTrailHolder = nullptr;
  mReanimationHolder = nullptr;
  mAttachmentHolder = nullptr;
}

// ==========================================================================
// EffectSystem::~EffectSystem()
// @0020507C  2 bytes
// ==========================================================================
// The dump's body is a bare return; the four holders are released by
// EffectSystemDispose, which every owner calls first.
EffectSystem::~EffectSystem()
{
}

// ==========================================================================
// EffectSystem::EffectSystemInitialize()
// @00205080  98 bytes
// ==========================================================================
// The dump allocates each holder with `operator new(size)` followed by a
// placement-new of the same type; `new TodParticleHolder()` is what that pair
// spells. The sizes it passes are the classes' real ones (0x7C, 0x1C, 0x1C,
// 0x1C), which their headers assert.
int EffectSystem::EffectSystemInitialize()
{
  gEffectSystem = this;

  mParticleHolder = new TodParticleHolder();
  mTrailHolder = new TrailHolder();
  mReanimationHolder = new ReanimationHolder();
  mAttachmentHolder = new AttachmentHolder();

  mParticleHolder->InitializeHolder();
  mTrailHolder->InitializeHolder();
  mReanimationHolder->InitializeHolder();
  return mAttachmentHolder->InitializeHolder();
}

// ==========================================================================
// EffectSystem::EffectSystemDispose()
// @002050FC  130 bytes
// ==========================================================================
// Each block is `if (mX != nullptr) { mX->DisposeHolder(); delete mX; mX = nullptr; }`;
// the dump's explicit `~TX()` plus `operator delete` pair is the delete
// expression spelled out, and the second null test inside it is the compiler's
// own guard on the reloaded pointer.
void EffectSystem::EffectSystemDispose()
{
  if (mParticleHolder != nullptr)
  {
    mParticleHolder->DisposeHolder();
    delete mParticleHolder;
    mParticleHolder = nullptr;
  }
  if (mTrailHolder != nullptr)
  {
    mTrailHolder->DisposeHolder();
    delete mTrailHolder;
    mTrailHolder = nullptr;
  }
  if (mReanimationHolder != nullptr)
  {
    mReanimationHolder->DisposeHolder();
    delete mReanimationHolder;
    mReanimationHolder = nullptr;
  }
  if (mAttachmentHolder != nullptr)
  {
    mAttachmentHolder->DisposeHolder();
    delete mAttachmentHolder;
    mAttachmentHolder = nullptr;
  }
  gEffectSystem = nullptr;
}

// ==========================================================================
// EffectSystem::Update()
// @00205188  224 bytes
// ==========================================================================
// Three DataArray<T>::IterateNext passes, one per live array: walk the used
// prefix, skip every slot whose id still carries a key (the high half of mID),
// and update the element unless it is an attachment. Particle systems and
// reanimations both call their own Update; a trail runs its counter only while
// mUnkE4 is clear.
//
// The return type is the transcriber's: the binary leaves the trail block
// pointer in r0 because the last loop happens to hold it there, and no caller
// reads it.
unsigned int EffectSystem::Update()
{
  ParticleSystemSlot *aSystems = mParticleHolder->mParticleSystems.mBlock;
  for (unsigned int i = 0; i < mParticleHolder->mParticleSystems.mMaxUsedCount; ++i)
  {
    if ((aSystems[i].mID & 0xFFFF0000u) == 0)
      continue;
    if (!aSystems[i].mItem.mIsAttachment)
      aSystems[i].mItem.Update();
  }

  ReanimationSlot *aReanimations = mReanimationHolder->mBlock;
  for (unsigned int i = 0; i < mReanimationHolder->mMaxUsedCount; ++i)
  {
    if ((aReanimations[i].mID & 0xFFFF0000u) == 0)
      continue;
    if (!aReanimations[i].mItem.mIsAttachment)
      aReanimations[i].mItem.Update();
  }

  TrailSlot *aTrails = mTrailHolder->mTrails.mBlock;
  for (unsigned int i = 0; i < mTrailHolder->mTrails.mMaxUsedCount; ++i)
  {
    if ((aTrails[i].mID & 0xFFFF0000u) == 0)
      continue;
    if (!aTrails[i].mItem.mIsAttachment)
      aTrails[i].mItem.Update();
  }

  return 0;
}

// ==========================================================================
// EffectSystem::EffectSystemFreeAll()
// @0020636C  746 bytes
// ==========================================================================
// Three DataArray<T>::DataArrayFreeAll passes over the particle holder's arrays
// and one each for trails, reanimations and attachments. A pass walks the used
// prefix, destructs every slot whose id still carries a key and threads the
// freed slot onto mFreeListHead, then clears mMaxUsedCount and mFreeListHead
// (PvZ\TodLib\Common\DataArray.h:79-95).
void EffectSystem::EffectSystemFreeAll()
{
  ParticleSystemSlot *aSystems = mParticleHolder->mParticleSystems.mBlock;
  for (unsigned int i = 0; i < mParticleHolder->mParticleSystems.mMaxUsedCount; ++i)
  {
    if ((aSystems[i].mID & 0xFFFF0000u) == 0)
      continue;

    aSystems[i].mItem.~TodParticleSystem();
    unsigned int anIndex = aSystems[i].mID & 0xFFFFu;
    aSystems[i].mID = mParticleHolder->mParticleSystems.mFreeListHead;
    mParticleHolder->mParticleSystems.mFreeListHead = anIndex;
    --mParticleHolder->mParticleSystems.mSize;
  }
  mParticleHolder->mParticleSystems.mMaxUsedCount = 0;
  mParticleHolder->mParticleSystems.mFreeListHead = 0;

  ParticleEmitterSlot *aEmitters = mParticleHolder->mEmitters.mBlock;
  for (unsigned int i = 0; i < mParticleHolder->mEmitters.mMaxUsedCount; ++i)
  {
    if ((aEmitters[i].mID & 0xFFFF0000u) == 0)
      continue;

    // Return each live particle node to the emitter's allocator. The next live
    // node is the second pointer. The free-list head is the first word.
    TodAllocator *aAllocator = aEmitters[i].mItem.mParticleList.mpAllocator;
    for (void *aNode = aEmitters[i].mItem.mParticleList.mHead; aNode != nullptr; )
    {
      void *aNext = nullptr;
      std::memcpy(&aNext, (char *)aNode + sizeof(void *), sizeof(aNext));
      TodAllocatorWriteLink(aNode, aAllocator->mFreeList);
      --aAllocator->mTotalItems;
      aAllocator->mFreeList = aNode;
      aNode = aNext;
    }
    aEmitters[i].mItem.mParticleList.mSize = 0;
    aEmitters[i].mItem.mParticleList.mHead = nullptr;
    aEmitters[i].mItem.mParticleList.mTail = nullptr;
    aEmitters[i].mItem.~TodParticleEmitter();

    unsigned int anIndex = aEmitters[i].mID & 0xFFFFu;
    aEmitters[i].mID = mParticleHolder->mEmitters.mFreeListHead;
    mParticleHolder->mEmitters.mFreeListHead = anIndex;
    --mParticleHolder->mEmitters.mSize;
  }
  mParticleHolder->mEmitters.mMaxUsedCount = 0;
  mParticleHolder->mEmitters.mFreeListHead = 0;

  ParticleSlot *aParticles = mParticleHolder->mParticles.mBlock;
  for (unsigned int i = 0; i < mParticleHolder->mParticles.mMaxUsedCount; ++i)
  {
    if ((aParticles[i].mID & 0xFFFF0000u) == 0)
      continue;

    // The particle record's destructor is the SyncObject base destructor: the
    // only store beyond it is the vptr and the release of mSyncBlocks.
    aParticles[i].mItem.~TodParticle();

    unsigned int anIndex = aParticles[i].mID & 0xFFFFu;
    aParticles[i].mID = mParticleHolder->mParticles.mFreeListHead;
    mParticleHolder->mParticles.mFreeListHead = anIndex;
    --mParticleHolder->mParticles.mSize;
  }
  mParticleHolder->mParticles.mMaxUsedCount = 0;
  mParticleHolder->mParticles.mFreeListHead = 0;

  // TodAllocator::FreeAll for the particle holder's two node pools, inlined.
  // Each pool's mBlockList (+0x58 and +0x6C) is a chain of `operator new[]`
  // blocks whose leading word links the previous block, so the walk follows it
  // to zero; mFreeList (+0x54 and +0x68) and mTotalItems (+0x60 and +0x74) are
  // cleared alongside. TodParticleHolder::DisposeHolder does the same teardown
  // with the same named members.
  for (void *aBlock = mParticleHolder->mParticleNodes.mBlockList; aBlock != nullptr; )
  {
    void *aNext = *(void **)aBlock;
    operator delete[](aBlock);
    aBlock = aNext;
  }
  mParticleHolder->mParticleNodes.mBlockList = nullptr;

  void *aEmitterBlock = mParticleHolder->mEmitterNodes.mBlockList;
  mParticleHolder->mParticleNodes.mFreeList = nullptr;
  mParticleHolder->mParticleNodes.mTotalItems = 0;
  for (void *aBlock = aEmitterBlock; aBlock != nullptr; )
  {
    void *aNext = *(void **)aBlock;
    operator delete[](aBlock);
    aBlock = aNext;
  }
  mParticleHolder->mEmitterNodes.mBlockList = nullptr;
  mParticleHolder->mEmitterNodes.mFreeList = nullptr;
  mParticleHolder->mEmitterNodes.mTotalItems = 0;

  TrailSlot *aTrails = mTrailHolder->mTrails.mBlock;
  for (unsigned int i = 0; i < mTrailHolder->mTrails.mMaxUsedCount; ++i)
  {
    if ((aTrails[i].mID & 0xFFFF0000u) == 0)
      continue;

    aTrails[i].mItem.~Trail();
    unsigned int anIndex = aTrails[i].mID & 0xFFFFu;
    aTrails[i].mID = mTrailHolder->mTrails.mFreeListHead;
    mTrailHolder->mTrails.mFreeListHead = anIndex;
    --mTrailHolder->mTrails.mSize;
  }
  mTrailHolder->mTrails.mMaxUsedCount = 0;
  mTrailHolder->mTrails.mFreeListHead = 0;
  ReanimationSlot *aReanimations = mReanimationHolder->mBlock;
  for (unsigned int i = 0; i < mReanimationHolder->mMaxUsedCount; ++i)
  {
    if ((aReanimations[i].mID & 0xFFFF0000u) == 0)
      continue;

    aReanimations[i].mItem.~Reanimation();
    unsigned int anIndex = aReanimations[i].mID & 0xFFFFu;
    aReanimations[i].mID = mReanimationHolder->mFreeListHead;
    mReanimationHolder->mFreeListHead = anIndex;
    --mReanimationHolder->mSize;
  }
  mReanimationHolder->mMaxUsedCount = 0;
  mReanimationHolder->mFreeListHead = 0;

  // Attachment is its own record type: mAttachmentID at +0x318 IS the id word,
  // so the block walks as an array of Attachment.
  for (unsigned int i = 0; i < mAttachmentHolder->mMaxUsedCount; ++i)
  {
    Attachment *aRecord = &mAttachmentHolder->mBlock[i];
    if ((aRecord->mAttachmentID & 0xFFFF0000u) == 0)
      continue;

    aRecord->~Attachment();
    unsigned int anIndex = (unsigned int)aRecord->mAttachmentID & 0xFFFFu;
    aRecord->mAttachmentID = (int)mAttachmentHolder->mFreeListHead;
    mAttachmentHolder->mFreeListHead = anIndex;
    --mAttachmentHolder->mSize;
  }
  mAttachmentHolder->mMaxUsedCount = 0;
  mAttachmentHolder->mFreeListHead = 0;
}

// ==========================================================================
// EffectSystem::ProcessDeleteQueue()
// @00206664  410 bytes
// ==========================================================================
// One DataArray<T> pass per holder, freeing only the slots whose element has
// mDead set. Unlike EffectSystemFreeAll the arrays keep their used prefix and
// their free-list head is threaded slot by slot.
void EffectSystem::ProcessDeleteQueue()
{
  ParticleSystemSlot *aSystems = mParticleHolder->mParticleSystems.mBlock;
  for (unsigned int i = 0; i < mParticleHolder->mParticleSystems.mMaxUsedCount; ++i)
  {
    if ((aSystems[i].mID & 0xFFFF0000u) == 0)
      continue;
    if (!aSystems[i].mItem.mDead)
      continue;

    aSystems[i].mItem.~TodParticleSystem();
    unsigned int anIndex = aSystems[i].mID & 0xFFFFu;
    aSystems[i].mID = mParticleHolder->mParticleSystems.mFreeListHead;
    mParticleHolder->mParticleSystems.mFreeListHead = anIndex;
    --mParticleHolder->mParticleSystems.mSize;
  }

  ReanimationSlot *aReanimations = mReanimationHolder->mBlock;
  for (unsigned int i = 0; i < mReanimationHolder->mMaxUsedCount; ++i)
  {
    if ((aReanimations[i].mID & 0xFFFF0000u) == 0)
      continue;
    if (!aReanimations[i].mItem.mDead)
      continue;

    aReanimations[i].mItem.~Reanimation();
    unsigned int anIndex = aReanimations[i].mID & 0xFFFFu;
    aReanimations[i].mID = mReanimationHolder->mFreeListHead;
    mReanimationHolder->mFreeListHead = anIndex;
    --mReanimationHolder->mSize;
  }
  TrailSlot *aTrails = mTrailHolder->mTrails.mBlock;
  for (unsigned int i = 0; i < mTrailHolder->mTrails.mMaxUsedCount; ++i)
  {
    if ((aTrails[i].mID & 0xFFFF0000u) == 0)
      continue;
    if (!aTrails[i].mItem.mDead)
      continue;

    aTrails[i].mItem.~Trail();
    unsigned int anIndex = aTrails[i].mID & 0xFFFFu;
    aTrails[i].mID = mTrailHolder->mTrails.mFreeListHead;
    mTrailHolder->mTrails.mFreeListHead = anIndex;
    --mTrailHolder->mTrails.mSize;
  }

  for (unsigned int i = 0; i < mAttachmentHolder->mMaxUsedCount; ++i)
  {
    Attachment *aRecord = &mAttachmentHolder->mBlock[i];
    if ((aRecord->mAttachmentID & 0xFFFF0000u) == 0)
      continue;
    if (!aRecord->mDead)
      continue;

    aRecord->~Attachment();
    unsigned int anIndex = (unsigned int)aRecord->mAttachmentID & 0xFFFFu;
    aRecord->mAttachmentID = (int)mAttachmentHolder->mFreeListHead;
    mAttachmentHolder->mFreeListHead = anIndex;
    --mAttachmentHolder->mSize;
  }
}
