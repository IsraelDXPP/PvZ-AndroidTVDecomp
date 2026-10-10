#include "TodParticleSystem.h"
#include "TodParticleEmitter.h"
#include "DataArrayMethods.h"

#include "Graphics.h"
#include "Image.h"
#include "ReconFreeFunctions.h"
#include "TodAllocator.h"
#include "TodParticleDefinition.h"
#include <cstring>

namespace
{

using EmitterListNode = DataArrayIdNode;

TodParticleEmitter *AllocParticleEmitter(TodParticleHolder *theHolder)
{
    return theHolder->mEmitters.DataArrayAlloc();
}

TodParticleEmitter *EmitterAt(TodParticleHolder *theHolder, unsigned int theIndex)
{
    ParticleEmitterSlot *aSlots = theHolder->mEmitters.mBlock;
    return &aSlots[theIndex & DATA_ARRAY_INDEX_MASK].mItem;
}

unsigned int EmitterSlotId(TodParticleHolder *theHolder, TodParticleEmitter *theEmitter)
{
    ParticleEmitterSlot *aSlots = theHolder->mEmitters.mBlock;
    unsigned int aCount = theHolder->mEmitters.mMaxUsedCount;
    for (unsigned int anIndex = 0; anIndex < aCount; ++anIndex)
    {
        if (&aSlots[anIndex].mItem == theEmitter)
            return aSlots[anIndex].mID;
    }
    return 0;
}

void FreeListNodes(EmitterListNode *theHead, TodAllocator *theAllocator)
{
    for (EmitterListNode *aNode = theHead; aNode != nullptr;)
    {
        EmitterListNode *aNext = aNode->mNext;
        TodAllocatorWriteLink(aNode, theAllocator->mFreeList);
        --theAllocator->mTotalItems;
        theAllocator->mFreeList = aNode;
        aNode = aNext;
    }
}

void AppendEmitter(TodParticleSystem::EmitterIdList *theList, unsigned int theIndex)
{
    TodAllocator *anAllocator = theList->mpAllocator;
    if (anAllocator == nullptr)
    {
        anAllocator = FindGlobalAllocator(sizeof(DataArrayIdNode));
        theList->mpAllocator = anAllocator;
    }

    EmitterListNode *aNode = static_cast<EmitterListNode *>(TodAllocatorAlloc(anAllocator));
    aNode->mPrev = static_cast<EmitterListNode *>(theList->mTail);
    aNode->mValue = theIndex;
    aNode->mNext = nullptr;
    if (theList->mTail != nullptr)
        static_cast<EmitterListNode *>(theList->mTail)->mNext = aNode;
    else
        theList->mHead = aNode;
    theList->mTail = aNode;
    ++theList->mSize;
}

bool EmitterNameMatches(const char *theName, TodEmitterDefinition *theDefinition)
{
    return theName == nullptr || (theDefinition->mName != nullptr && strcasecmp(theName, theDefinition->mName) == 0);
}

} // namespace

void TodParticleSystem::ParticleSystemDie()
{
    EmitterListNode *aNode = static_cast<EmitterListNode *>(mEmitterList.mHead);
    if (aNode != nullptr)
    {
        do
        {
            unsigned int anIndex = aNode->mValue;
            aNode = aNode->mNext;
            TodParticleEmitter *anEmitter = EmitterAt(mParticleHolder, anIndex);
            anEmitter->DeleteAll();
            mParticleHolder->mEmitters.DataArrayFree(anEmitter);
        } while (aNode != nullptr);

        FreeListNodes(mEmitterList.mHead, mEmitterList.mpAllocator);
    }

    mEmitterList.mSize = 0;
    mEmitterList.mHead = nullptr;
    mEmitterList.mTail = nullptr;
    mDead = true;
}

void TodParticleSystem::Draw(Sexy::Graphics *theGraphics)
{
    for (EmitterListNode *aNode = static_cast<EmitterListNode *>(mEmitterList.mHead); aNode != nullptr;
        aNode = aNode->mNext)
    {
        EmitterAt(mParticleHolder, aNode->mValue)->Draw(theGraphics);
    }
}

void TodParticleSystem::SystemMove(float theX, float theY)
{
    for (EmitterListNode *aNode = static_cast<EmitterListNode *>(mEmitterList.mHead); aNode != nullptr;
        aNode = aNode->mNext)
    {
        EmitterAt(mParticleHolder, aNode->mValue)->SystemMove(theX, theY);
    }
}

void TodParticleSystem::OverrideColor(const char *theEmitterName, const Sexy::Color &theColor)
{
    for (EmitterListNode *aNode = static_cast<EmitterListNode *>(mEmitterList.mHead); aNode != nullptr;
        aNode = aNode->mNext)
    {
        TodParticleEmitter *anEmitter = EmitterAt(mParticleHolder, aNode->mValue);
        if (EmitterNameMatches(theEmitterName, anEmitter->mEmitterDef))
            anEmitter->mSyncState.mColorOverride = theColor;
    }
}

void TodParticleSystem::OverrideExtraAdditiveDraw(const char *theEmitterName, bool theEnableExtraAdditiveDraw)
{
    for (EmitterListNode *aNode = static_cast<EmitterListNode *>(mEmitterList.mHead); aNode != nullptr;
        aNode = aNode->mNext)
    {
        TodParticleEmitter *anEmitter = EmitterAt(mParticleHolder, aNode->mValue);
        if (EmitterNameMatches(theEmitterName, anEmitter->mEmitterDef))
            anEmitter->mSyncState.mExtraAdditiveDrawOverride = theEnableExtraAdditiveDraw;
    }
}

void TodParticleSystem::OverrideImage(const char *theEmitterName, Sexy::Image *theImage)
{
    for (EmitterListNode *aNode = static_cast<EmitterListNode *>(mEmitterList.mHead); aNode != nullptr;
        aNode = aNode->mNext)
    {
        TodParticleEmitter *anEmitter = EmitterAt(mParticleHolder, aNode->mValue);
        if (EmitterNameMatches(theEmitterName, anEmitter->mEmitterDef))
            anEmitter->mSyncState.mImageOverride = theImage;
    }
}

void TodParticleSystem::OverrideFrame(const char *theEmitterName, int theFrame)
{
    for (EmitterListNode *aNode = static_cast<EmitterListNode *>(mEmitterList.mHead); aNode != nullptr;
        aNode = aNode->mNext)
    {
        TodParticleEmitter *anEmitter = EmitterAt(mParticleHolder, aNode->mValue);
        if (EmitterNameMatches(theEmitterName, anEmitter->mEmitterDef))
            anEmitter->mSyncState.mFrameOverride = theFrame;
    }
}

void TodParticleSystem::OverrideScale(const char *theEmitterName, float theScale)
{
    for (EmitterListNode *aNode = static_cast<EmitterListNode *>(mEmitterList.mHead); aNode != nullptr;
        aNode = aNode->mNext)
    {
        TodParticleEmitter *anEmitter = EmitterAt(mParticleHolder, aNode->mValue);
        if (EmitterNameMatches(theEmitterName, anEmitter->mEmitterDef))
            anEmitter->mSyncState.mScaleOverride = theScale;
    }
}

TodParticleEmitter *TodParticleSystem::FindEmitterByName(const char *theEmitterName)
{
    if (theEmitterName == nullptr)
        return nullptr;
    for (EmitterListNode *aNode = static_cast<EmitterListNode *>(mEmitterList.mHead); aNode != nullptr;
        aNode = aNode->mNext)
    {
        TodParticleEmitter *anEmitter = EmitterAt(mParticleHolder, aNode->mValue);
        if (EmitterNameMatches(theEmitterName, anEmitter->mEmitterDef))
            return anEmitter;
    }
    return nullptr;
}

TodEmitterDefinition *TodParticleSystem::FindEmitterDefByName(const char *theEmitterName)
{
    if (theEmitterName == nullptr)
        return nullptr;
    for (int anIndex = 0; anIndex < mParticleDef->mEmitterDefCount; ++anIndex)
    {
        TodEmitterDefinition *aDefinition = &mParticleDef->mEmitterDefs[anIndex];
        if (EmitterNameMatches(theEmitterName, aDefinition))
            return aDefinition;
    }
    return nullptr;
}

TodParticleSystem::TodParticleSystem()
{
    const size_t aSyncBlockSize = MakeSyncBlock(mEffectType, mDontUpdate).mSize;

    SyncBlockInfo aBlock;
    aBlock.mData = &mEffectType;
    aBlock.mSize = aSyncBlockSize;
    mSyncBlocks.push_back(aBlock);
    mEffectType = PARTICLE_MELONSPLASH;
    mParticleDef = nullptr;
    mParticleHolder = nullptr;
    mEmitterList.mHead = nullptr;
    mEmitterList.mTail = nullptr;
    mEmitterList.mSize = 0;
    mEmitterList.mpAllocator = nullptr;
    mDead = false;
    mIsAttachment = false;
    mRenderOrder = 0;
    mDontUpdate = false;
}

void TodParticleSystem::TodParticleInitializeFromDef(
    float theX, float theY, int theRenderOrder, TodParticleDefinition *theDefinition, ParticleEffect theEffectType)
{
    mRenderOrder = theRenderOrder;
    mEffectType = theEffectType;
    mEmitterList.mpAllocator = &mParticleHolder->mEmitterNodes;
    mParticleDef = theDefinition;

    for (int i = 0; i < theDefinition->mEmitterDefCount; ++i)
    {
        TodEmitterDefinition *aDefinition = &theDefinition->mEmitterDefs[i];
        if (FloatTrackIsSet(&aDefinition->mCrossFadeDuration))
            continue;

        if ((aDefinition->mFlags & PARTICLE_DIE_IF_OVERLOADED) != 0 && mParticleHolder->IsOverLoaded())
        {
            ParticleSystemDie();
            return;
        }

        TodParticleEmitter *anEmitter = AllocParticleEmitter(mParticleHolder);
        anEmitter->TodEmitterInitialize(theX, theY, this, aDefinition);
        AppendEmitter(&mEmitterList, EmitterSlotId(mParticleHolder, anEmitter));
    }
}

void TodParticleSystem::Update()
{
    if (mDontUpdate)
        return;

    bool aKeepAlive = false;
    for (EmitterListNode *aNode = mEmitterList.mHead; aNode != nullptr;)
    {
        TodParticleEmitter *anEmitter = EmitterAt(mParticleHolder, aNode->mValue);
        aNode = aNode->mNext;
        anEmitter->Update();
        if (FloatTrackIsSet(&anEmitter->mEmitterDef->mCrossFadeDuration))
        {
            if (anEmitter->mParticleList.mSize > 0)
                aKeepAlive = true;
        }
        else if (!anEmitter->mSyncState.mDead)
        {
            aKeepAlive = true;
        }
    }

    if (!aKeepAlive)
        mDead = true;
}

void TodParticleSystem::CrossFade(const char *theEmitterName)
{
    TodEmitterDefinition *aDefinition = FindEmitterDefByName(theEmitterName);
    if (aDefinition == nullptr || !FloatTrackIsSet(&aDefinition->mCrossFadeDuration))
        return;

    if ((unsigned int)mEmitterList.mSize + mParticleHolder->mEmitters.mSize > mParticleHolder->mEmitters.mMaxSize)
    {
        ParticleSystemDie();
        return;
    }

    for (EmitterListNode *aNode = mEmitterList.mHead; aNode != nullptr;)
    {
        TodParticleEmitter *anEmitter = EmitterAt(mParticleHolder, aNode->mValue);
        aNode = aNode->mNext;
        if (aDefinition == anEmitter->mEmitterDef)
            continue;

        TodParticleEmitter *aNewEmitter = AllocParticleEmitter(mParticleHolder);
        aNewEmitter->TodEmitterInitialize(
            anEmitter->mSyncState.mSystemCenter.mX, anEmitter->mSyncState.mSystemCenter.mY, this, aDefinition);
        AppendEmitter(&mEmitterList, EmitterSlotId(mParticleHolder, aNewEmitter));
        anEmitter->CrossFadeEmitter(aNewEmitter);
    }
}

TodParticleSystem::~TodParticleSystem()
{
    ParticleSystemDie();
}
