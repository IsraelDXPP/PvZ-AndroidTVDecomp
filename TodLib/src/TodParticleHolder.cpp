#include "TodParticleHolder.h"

#include "RawObject.h"
#include "ReconFreeFunctions.h"
#include "TodParticle.h"
#include "DataArrayMethods.h"
#include "TodParticleDefinition.h"
#include "TodParticleEmitter.h"
#include "TodParticleSystem.h"
#include <cstring>

namespace
{

void FreeListNodes(DataArrayIdNode *theHead, TodAllocator *theAllocator)
{
    for (DataArrayIdNode *aNode = theHead; aNode != nullptr;)
    {
        DataArrayIdNode *aNext = aNode->mNext;
        TodAllocatorWriteLink(aNode, theAllocator->mFreeList);
        --theAllocator->mTotalItems;
        theAllocator->mFreeList = aNode;
        aNode = aNext;
    }
}

void FreeAllocatorBlocks(TodAllocator *theAllocator)
{
    for (void *aBlock = theAllocator->mBlockList; aBlock != nullptr;)
    {
        void *aNext = TodAllocatorReadLink(aBlock);
        operator delete[](aBlock);
        aBlock = aNext;
    }
    theAllocator->mBlockList = nullptr;
    theAllocator->mFreeList = nullptr;
    theAllocator->mTotalItems = 0;
}

void ReleaseEmitterLists(DataArray<TodParticleEmitter, ParticleEmitterSlot> *theArray)
{
    if (theArray->mBlock == nullptr)
        return;

    ParticleEmitterSlot *aSlots = theArray->mBlock;
    for (unsigned int i = 0; i < theArray->mMaxUsedCount; ++i)
    {
        if ((aSlots[i].mID & 0xFFFF0000u) == 0)
            continue;

        TodParticleEmitter *anEmitter = &aSlots[i].mItem;
        FreeListNodes(anEmitter->mParticleList.mHead, anEmitter->mParticleList.mpAllocator);
        anEmitter->mParticleList.mSize = 0;
        anEmitter->mParticleList.mHead = nullptr;
        anEmitter->mParticleList.mTail = nullptr;
    }
}

void InitNodePool(TodAllocator *thePool)
{
    thePool->mFreeList = nullptr;
    thePool->mBlockList = nullptr;
    thePool->mGrowCount = 1024;
    thePool->mTotalItems = 0;
    thePool->mItemSize = sizeof(DataArrayIdNode);
    TodAllocatorGrow(thePool);
}

} // namespace

TodParticleHolder::TodParticleHolder() = default;

void TodParticleHolder::InitializeHolder()
{
    mParticleSystems.DataArrayInitialize(1024, "particle systems");
    mParticleSystems.mNextKey = 53618;

    mEmitters.DataArrayInitialize(1024, "emitters");
    mEmitters.mNextKey = 56681;

    mParticles.DataArrayInitialize(1024, "particles");
    mParticles.mNextKey = 53618;

    InitNodePool(&mParticleNodes);
    InitNodePool(&mEmitterNodes);
}

bool TodParticleHolder::IsOverLoaded()
{
    return mParticleSystems.mSize > 0x384u || mEmitters.mSize > 0x384u || mParticles.mSize > 0x384u;
}

void TodParticleHolder::DisposeHolder()
{
    mParticleSystems.DataArrayDispose();
    ReleaseEmitterLists(&mEmitters);
    mEmitters.DataArrayDispose();
    mParticles.DataArrayDispose();
    FreeAllocatorBlocks(&mParticleNodes);
    FreeAllocatorBlocks(&mEmitterNodes);
}

TodParticleHolder::~TodParticleHolder()
{
    DisposeHolder();
}

TodParticleSystem *TodParticleHolder::AllocParticleSystemFromDef(
    float theX, float theY, int theRenderOrder, TodParticleDefinition *theDefinition, ParticleEffect theParticleEffect)
{
    if (mParticleSystems.mSize == mParticleSystems.mMaxSize ||
        mEmitters.mSize + (unsigned int)theDefinition->mEmitterDefCount > mEmitters.mMaxSize)
        return nullptr;

    TodParticleSystem *aSystem = mParticleSystems.DataArrayAlloc();
    aSystem->mParticleHolder = this;
    aSystem->TodParticleInitializeFromDef(theX, theY, theRenderOrder, theDefinition, theParticleEffect);
    return aSystem;
}

TodParticleSystem *TodParticleHolder::AllocParticleSystem(
    float theX, float theY, int theRenderOrder, ParticleEffect theParticleEffect)
{
    return AllocParticleSystemFromDef(
        theX, theY, theRenderOrder, gParticleDefArray + theParticleEffect, theParticleEffect);
}
