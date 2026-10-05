#include "TodParticleSystem.h"
#include "TodParticleEmitter.h"
#include "DataArrayMethods.h"

#include "Graphics.h"
#include "Image.h"
#include "RawObject.h"
#include "ReconFreeFunctions.h"
#include "TodAllocator.h"
#include "TodParticleDefinition.h"
#include "TodParticleEmitter.h"
#include <cstring>

namespace {

struct EmitterListNode
{
    unsigned int mValue;
    EmitterListNode *mNext;
    EmitterListNode *mPrev;
};

TodParticleEmitter *AllocParticleEmitter(TodParticleHolder *theHolder)
{
    DataArray<TodParticleEmitter, ParticleEmitterSlot> &anArray = theHolder->mEmitters;
    unsigned int anIndex = anArray.mFreeListHead;
    ParticleEmitterSlot *aSlots = anArray.mBlock;
    ParticleEmitterSlot *aSlot = aSlots + anIndex;

    unsigned int aNextFree;
    if (anIndex == anArray.mMaxUsedCount)
    {
        aNextFree = anIndex + 1;
        anArray.mMaxUsedCount = aNextFree;
    }
    else
    {
        aNextFree = aSlot->mID;
    }
    anArray.mFreeListHead = aNextFree;

    std::memset(aSlot, 0, 0xBC);

    unsigned int aKey = anArray.mNextKey;
    aSlot->mID = anIndex | (aKey << 16);
    anArray.mNextKey = (aKey == 0xFFFFu) ? 1u : aKey + 1u;
    ++anArray.mSize;

    return ConstructRaw<TodParticleEmitter>(&aSlot->mItem);
}

TodParticleEmitter *EmitterAt(TodParticleHolder *theHolder, unsigned int theIndex)
{
    ParticleEmitterSlot *aSlots = theHolder->mEmitters.mBlock;
    return &aSlots[theIndex].mItem;
}

unsigned int EmitterSlotId(TodParticleHolder *theHolder, TodParticleEmitter *theEmitter)
{
    ParticleEmitterSlot *aSlots = theHolder->mEmitters.mBlock;
    unsigned int aCount = theHolder->mEmitters.mMaxSize;
    for (unsigned int anIndex = 0; anIndex < aCount; ++anIndex)
    {
        if (&aSlots[anIndex].mItem == theEmitter)
            return aSlots[anIndex].mID;
    }
    return 0;
}

void FreeListNodes(void *theHead, TodAllocator *theAllocator)
{
    for (void *aNode = theHead; aNode != nullptr; )
    {
        void *aNext = TodAllocatorReadLink(static_cast<char *>(aNode) + sizeof(void *));
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
        anAllocator = FindGlobalAllocator(12);
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
    return theName == nullptr || strcasecmp(theName, theDefinition->mName) == 0;
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
            TodParticleEmitter::DeleteAll(anEmitter);

            FreeListNodes(anEmitter->mParticleList.mHead, anEmitter->mParticleList.mpAllocator);
            anEmitter->mParticleList.mSize = 0;
            anEmitter->mParticleList.mHead = nullptr;
            anEmitter->mParticleList.mTail = nullptr;
            anEmitter->~TodParticleEmitter();

            unsigned int aFreeHead = mParticleHolder->mEmitters.mFreeListHead;
            ParticleEmitterSlot *aSlots = mParticleHolder->mEmitters.mBlock;
            ParticleEmitterSlot *aSlot = aSlots;
            for (unsigned int anIndex = 0; anIndex < mParticleHolder->mEmitters.mMaxSize; ++anIndex)
            {
                if (&aSlots[anIndex].mItem == anEmitter)
                {
                    aSlot = &aSlots[anIndex];
                    break;
                }
            }
            mParticleHolder->mEmitters.mFreeListHead = aSlot->mID;
            aSlot->mID = aFreeHead;
            --mParticleHolder->mEmitters.mSize;
        }
        while (aNode != nullptr);

        FreeListNodes(mEmitterList.mHead, mEmitterList.mpAllocator);
    }

    mEmitterList.mSize = 0;
    mEmitterList.mHead = nullptr;
    mEmitterList.mTail = nullptr;
    mDead = true;
}

void TodParticleSystem::Draw(Sexy::Graphics *theGraphics)
{
    for (EmitterListNode *aNode = static_cast<EmitterListNode *>(mEmitterList.mHead);
         aNode != nullptr; aNode = aNode->mNext)
    {
        TodParticleEmitter::Draw(EmitterAt(mParticleHolder, aNode->mValue), theGraphics);
    }
}

void TodParticleSystem::SystemMove(float theX, float theY)
{
    for (EmitterListNode *aNode = static_cast<EmitterListNode *>(mEmitterList.mHead);
         aNode != nullptr; aNode = aNode->mNext)
    {
        TodParticleEmitter::SystemMove(EmitterAt(mParticleHolder, aNode->mValue), theX, theY);
    }
}

void TodParticleSystem::OverrideColor(const char *theEmitterName, const Sexy::Color &theColor)
{
    for (EmitterListNode *aNode = static_cast<EmitterListNode *>(mEmitterList.mHead);
         aNode != nullptr; aNode = aNode->mNext)
    {
        TodParticleEmitter *anEmitter = EmitterAt(mParticleHolder, aNode->mValue);
        if (EmitterNameMatches(theEmitterName, anEmitter->mEmitterDef))
            anEmitter->mSyncState.mColorOverride = theColor;
    }
}

void TodParticleSystem::OverrideExtraAdditiveDraw(const char *theEmitterName, bool theEnableExtraAdditiveDraw)
{
    for (EmitterListNode *aNode = static_cast<EmitterListNode *>(mEmitterList.mHead);
         aNode != nullptr; aNode = aNode->mNext)
    {
        TodParticleEmitter *anEmitter = EmitterAt(mParticleHolder, aNode->mValue);
        if (EmitterNameMatches(theEmitterName, anEmitter->mEmitterDef))
            anEmitter->mSyncState.mExtraAdditiveDrawOverride = theEnableExtraAdditiveDraw;
    }
}

void TodParticleSystem::OverrideImage(const char *theEmitterName, Sexy::Image *theImage)
{
    for (EmitterListNode *aNode = static_cast<EmitterListNode *>(mEmitterList.mHead);
         aNode != nullptr; aNode = aNode->mNext)
    {
        TodParticleEmitter *anEmitter = EmitterAt(mParticleHolder, aNode->mValue);
        if (EmitterNameMatches(theEmitterName, anEmitter->mEmitterDef))
            anEmitter->mSyncState.mImageOverride = theImage;
    }
}

void TodParticleSystem::OverrideFrame(const char *theEmitterName, int theFrame)
{
    for (EmitterListNode *aNode = static_cast<EmitterListNode *>(mEmitterList.mHead);
         aNode != nullptr; aNode = aNode->mNext)
    {
        TodParticleEmitter *anEmitter = EmitterAt(mParticleHolder, aNode->mValue);
        if (EmitterNameMatches(theEmitterName, anEmitter->mEmitterDef))
            anEmitter->mSyncState.mFrameOverride = theFrame;
    }
}

void TodParticleSystem::OverrideScale(const char *theEmitterName, float theScale)
{
    for (EmitterListNode *aNode = static_cast<EmitterListNode *>(mEmitterList.mHead);
         aNode != nullptr; aNode = aNode->mNext)
    {
        TodParticleEmitter *anEmitter = EmitterAt(mParticleHolder, aNode->mValue);
        if (EmitterNameMatches(theEmitterName, anEmitter->mEmitterDef))
            anEmitter->mSyncState.mScaleOverride = theScale;
    }
}

TodParticleEmitter *TodParticleSystem::FindEmitterByName(const char *theEmitterName)
{
    for (EmitterListNode *aNode = static_cast<EmitterListNode *>(mEmitterList.mHead);
         aNode != nullptr; aNode = aNode->mNext)
    {
        TodParticleEmitter *anEmitter = EmitterAt(mParticleHolder, aNode->mValue);
        if (strcasecmp(anEmitter->mEmitterDef->mName, theEmitterName) == 0)
            return anEmitter;
    }
    return nullptr;
}

TodEmitterDefinition *TodParticleSystem::FindEmitterDefByName(const char *theEmitterName)
{
    int aCount = mParticleDef->mEmitterDefCount;
    if (aCount <= 0)
        return nullptr;

    TodEmitterDefinition *aDefinition = mParticleDef->mEmitterDefs;
    for (int i = 0; i + 1 < aCount; ++i)
    {
        if (strcasecmp(aDefinition->mName, theEmitterName) == 0)
            return aDefinition;
        ++aDefinition;
    }
    if (strcasecmp(aDefinition->mName, theEmitterName) == 0)
        return aDefinition;
    return nullptr;
}

TodParticleSystem::TodParticleSystem()
{
    const int aSyncBlockSize = 0x25;

    SyncBlockInfo aBlock;
    aBlock.mData = &mEffectType;
    aBlock.mSize = aSyncBlockSize;
    mSyncBlocks.push_back(aBlock);
    std::memset(&mEffectType, 0, aSyncBlockSize);

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
    mActive = false;
}

void TodParticleSystem::TodParticleInitializeFromDef(
    float theX, float theY, int theRenderOrder,
    TodParticleDefinition *theDefinition, ParticleEffect theEffectType)
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

        if ((aDefinition->mFlags & 0x80) != 0 && mParticleHolder->IsOverLoaded())
        {
            ParticleSystemDie();
            return;
        }

        TodParticleEmitter *anEmitter = AllocParticleEmitter(mParticleHolder);
        anEmitter->TodEmitterInitialize(theX, theY, this, aDefinition);
        AppendEmitter(&mEmitterList, EmitterSlotId(mParticleHolder, anEmitter) & 0xFFFFu);
    }
}

void TodParticleSystem::Update()
{
    if (mDontUpdate)
        return;

    bool aKeepAlive = mActive;
    for (EmitterListNode *aNode = static_cast<EmitterListNode *>(mEmitterList.mHead);
         aNode != nullptr; aNode = aNode->mNext)
    {
        TodParticleEmitter *anEmitter = EmitterAt(mParticleHolder, aNode->mValue);
        TodParticleEmitter::Update(anEmitter);
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

    if ((unsigned int)mEmitterList.mSize + mParticleHolder->mEmitters.mSize
        > mParticleHolder->mEmitters.mMaxSize)
    {
        ParticleSystemDie();
        return;
    }

    for (EmitterListNode *aNode = static_cast<EmitterListNode *>(mEmitterList.mHead);
         aNode != nullptr; aNode = aNode->mNext)
    {
        TodParticleEmitter *anEmitter = EmitterAt(mParticleHolder, aNode->mValue);
        if (aDefinition == anEmitter->mEmitterDef)
            continue;

        TodParticleEmitter *aNewEmitter = AllocParticleEmitter(mParticleHolder);
        aNewEmitter->TodEmitterInitialize(
            anEmitter->mSyncState.mSystemCenter.mX,
            anEmitter->mSyncState.mSystemCenter.mY,
            this, aDefinition);
        AppendEmitter(&mEmitterList, EmitterSlotId(mParticleHolder, aNewEmitter) & 0xFFFFu);
        TodParticleEmitter::CrossFadeEmitter(anEmitter, aNewEmitter);
    }
}

TodParticleSystem::~TodParticleSystem()
{
    ParticleSystemDie();
}
