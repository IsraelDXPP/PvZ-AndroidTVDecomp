#include "TodParticleEmitter.h"

#include "DataArrayMethods.h"
#include "Graphics.h"
#include "ReconFreeFunctions.h"
#include "ReconHelpers.h"
#include "SexyAppBase.h"
#include "TodParticleHolder.h"
#include "TodParticleSystem.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {

using ParticleListNode = DataArrayIdNode;

float EvaluatePrevious(FloatParameterTrack& theTrack, float theTime, float theSeed)
{
    return theTime < 0.0f ? 0.0f : FloatTrackEvaluate(theTrack, theTime, theSeed);
}

bool IsConstantZero(const FloatParameterTrack& theTrack)
{
    if (theTrack.mCountNodes == 0)
        return true;
    return theTrack.mCountNodes == 1 && theTrack.mNodes[0].mLowValue == 0.0f
        && theTrack.mNodes[0].mHighValue == 0.0f;
}

int RoundToInt(float theValue)
{
    return static_cast<int>(theValue > 0.0f ? theValue + 0.5f : theValue - 0.5f);
}

void ReturnNode(ParticleListNode* theNode, TodAllocator* theAllocator)
{
    TodAllocatorWriteLink(theNode, theAllocator->mFreeList);
    theAllocator->mFreeList = theNode;
    --theAllocator->mTotalItems;
}

// 0x207F7C uses a particle's address as an identity for shake noise. A slot ID
// supplies stable identity here without converting object pointers to integers.
SexyVector2 ShakeNoise(unsigned int theID, int theAge)
{
    uint64_t aValue = (static_cast<uint64_t>(theID * static_cast<unsigned int>(theAge))
        * 48271) % 2147483647;
    float aNoiseX = static_cast<float>(aValue) * 9.3132e-10f - 1.0f;
    aValue = (aValue * 48271) % 2147483647;
    return SexyVector2(aNoiseX, static_cast<float>(aValue) * 9.3132e-10f - 1.0f);
}

void BlendParam(float& theValue, bool& theSet, float theSource, bool theSourceSet, float theTime)
{
    if (theSourceSet)
        theValue = theSet ? theSource + theTime * (theValue - theSource) : theSource;
    theSet = theSet || theSourceSet;
}

} // namespace

TodParticleEmitter::TodParticleEmitter()
    : mEmitterDef(nullptr), mParticleSystem(nullptr), mParticleList{}, mSyncState{},
      mTrackInterp{}, mSystemFieldInterp{}
{
    SyncBlockInfo aBlock;
    aBlock.mData = &mSyncState;
    aBlock.mSize = sizeof(mSyncState);
    mSyncBlocks.push_back(aBlock);
}

// 0x20A42C: initialize persistent samples, then run the first update/burst.
void TodParticleEmitter::TodEmitterInitialize(float theX, float theY,
    TodParticleSystem* theSystem, TodEmitterDefinition* theDefinition)
{
    mEmitterDef = theDefinition;
    mParticleSystem = theSystem;
    mParticleList.mpAllocator = &theSystem->mParticleHolder->mParticleNodes;
    mSyncState.mSystemCenter = SexyVector2(theX, theY);
    mSyncState.mSystemAge = -1;
    mSyncState.mSystemTimeValue = -1.0f;
    mSyncState.mSystemLastTimeValue = -1.0f;
    mSyncState.mColorOverride = Sexy::Color::White;
    mSyncState.mScaleOverride = 1.0f;
    mSyncState.mFrameOverride = -1;
    mSyncState.mSystemDuration = std::max(1, static_cast<int>(
        FloatTrackIsSet(&theDefinition->mSystemDuration)
        ? FloatTrackEvaluate(theDefinition->mSystemDuration, 0.0f, Rand(1.0f))
        : FloatTrackEvaluate(theDefinition->mParticleDuration, 0.0f, 1.0f)));
    for (int anIndex = 0; anIndex < std::min(4, theDefinition->mSystemFieldCount); ++anIndex)
        for (float& aSample : mSystemFieldInterp[anIndex])
            aSample = Rand(1.0f);
    for (float& aSample : mTrackInterp)
        aSample = Rand(1.0f);
    Update();
}

float TodParticleEmitter::SystemTrackEvaluate(FloatParameterTrack& theTrack, ParticleSystemTracks theIndex)
{
    return FloatTrackEvaluate(theTrack, mSyncState.mSystemTimeValue, mTrackInterp[theIndex]);
}

float TodParticleEmitter::ParticleTrackEvaluate(FloatParameterTrack& theTrack,
    TodParticle* theParticle, ParticleTracks theIndex)
{
    return FloatTrackEvaluate(theTrack, theParticle->mSyncState.mParticleTimeValue,
        theParticle->mSyncState.mTrackInterp[theIndex]);
}

// 0x209BF0. Time is in hundredths of a second; angles use the TV literals.
TodParticle* TodParticleEmitter::SpawnParticle(int theIndex, int theCount)
{
    auto& aParticles = mParticleSystem->mParticleHolder->mParticles;
    if (aParticles.mSize == aParticles.mMaxSize)
        return nullptr;
    TodParticle* aParticle = aParticles.DataArrayAlloc();
    auto& aState = aParticle->mSyncState;
    aParticle->mParticleEmitter = this;
    for (int anIndex = 0; anIndex < std::min(4, mEmitterDef->mParticleFieldCount); ++anIndex)
        for (float& aSample : aState.mFieldInterp[anIndex])
            aSample = Rand(1.0f);
    for (float& aSample : aState.mTrackInterp)
        aSample = Rand(1.0f);
    float aDurationSample = Rand(1.0f);
    float aSpeedSample = Rand(1.0f);
    float anOffsetXSample = Rand(1.0f);
    float anOffsetYSample = Rand(1.0f);
    float aSystemTime = mSyncState.mSystemTimeValue;
    aState.mParticleDuration = std::max(1, static_cast<int>(
        FloatTrackEvaluate(mEmitterDef->mParticleDuration, aSystemTime, aDurationSample)));
    aState.mParticleTimeValue = -1.0f;
    aState.mParticleLastTimeValue = -1.0f;
    if (mEmitterDef->mFlags & PARTICLE_RANDOM_START_TIME)
        aState.mParticleAge = Rand(aState.mParticleDuration);
    float aSpeed = FloatTrackEvaluate(mEmitterDef->mLaunchSpeed, aSystemTime, aSpeedSample) * 0.01f;
    float anAngleSample = Rand(1.0f);
    float anAngle = 0.0f;
    if (mEmitterDef->mEmitterType == EMITTER_CIRCLE_PATH)
        anAngle = SystemTrackEvaluate(mEmitterDef->mEmitterPath, SYSTEM_TRACK_EMITTER_PATH) * 6.2832f;
    else if (mEmitterDef->mEmitterType == EMITTER_CIRCLE_EVEN_SPACING)
        anAngle = static_cast<float>(theIndex) * 6.2832f / static_cast<float>(theCount);
    else if (IsConstantZero(mEmitterDef->mLaunchAngle))
        anAngle = Rand(6.2831853f);
    if (mEmitterDef->mEmitterType == EMITTER_CIRCLE_PATH
        || mEmitterDef->mEmitterType == EMITTER_CIRCLE_EVEN_SPACING
        || !IsConstantZero(mEmitterDef->mLaunchAngle))
        anAngle += FloatTrackEvaluate(mEmitterDef->mLaunchAngle, aSystemTime, anAngleSample) * 0.017453f;

    float aPositionX = 0.0f;
    float aPositionY = 0.0f;
    if (mEmitterDef->mEmitterType == EMITTER_BOX)
    {
        float aBoxXSample = Rand(1.0f);
        float aBoxYSample = Rand(1.0f);
        aPositionX = FloatTrackEvaluate(mEmitterDef->mEmitterBoxX, aSystemTime, aBoxXSample);
        aPositionY = FloatTrackEvaluate(mEmitterDef->mEmitterBoxY, aSystemTime, aBoxYSample);
    }
    else if (mEmitterDef->mEmitterType == EMITTER_BOX_PATH)
    {
        float aPath = SystemTrackEvaluate(mEmitterDef->mEmitterPath, SYSTEM_TRACK_EMITTER_PATH);
        float aLeft = FloatTrackEvaluate(mEmitterDef->mEmitterBoxX, aSystemTime, 0.0f);
        float aRight = FloatTrackEvaluate(mEmitterDef->mEmitterBoxX, aSystemTime, 1.0f);
        float aTop = FloatTrackEvaluate(mEmitterDef->mEmitterBoxY, aSystemTime, 0.0f);
        float aBottom = FloatTrackEvaluate(mEmitterDef->mEmitterBoxY, aSystemTime, 1.0f);
        float aWidth = aRight - aLeft;
        float aHeight = aBottom - aTop;
        float aDistance = (2.0f * aWidth + 2.0f * aHeight) * aPath;
        if (aDistance < aHeight)
        {
            aPositionX = aLeft;
            aPositionY = aTop + aDistance;
        }
        else if (aDistance < aHeight + aWidth)
        {
            aPositionX = aLeft + aDistance - aHeight;
            aPositionY = aBottom;
        }
        else if (aDistance < 2.0f * aHeight + aWidth)
        {
            aPositionX = aRight;
            aPositionY = aBottom - (aDistance - aHeight - aWidth);
        }
        else
        {
            aPositionX = aRight - (aDistance - 2.0f * aHeight - aWidth);
            aPositionY = aTop;
        }
    }
    else
    {
        float aRadius = FloatTrackEvaluate(mEmitterDef->mEmitterRadius, aSystemTime, Rand(1.0f));
        aPositionX = std::sin(anAngle) * aRadius;
        aPositionY = std::cos(anAngle) * aRadius;
    }
    float aSkewXSample = Rand(1.0f);
    float aSkewYSample = Rand(1.0f);
    float aSkewX = FloatTrackEvaluate(mEmitterDef->mEmitterSkewX, aSystemTime, aSkewXSample);
    float aSkewY = FloatTrackEvaluate(mEmitterDef->mEmitterSkewY, aSystemTime, aSkewYSample);
    aState.mPosition = SexyVector2(aPositionX + mSyncState.mSystemCenter.mX + aSkewX * aPositionY,
        aPositionY + mSyncState.mSystemCenter.mY + aSkewY * aPositionX);
    aState.mVelocity = SexyVector2(std::sin(anAngle) * aSpeed, std::cos(anAngle) * aSpeed);
    aState.mPosition.mX += FloatTrackEvaluate(mEmitterDef->mEmitterOffsetX, aSystemTime, anOffsetXSample);
    aState.mPosition.mY += FloatTrackEvaluate(mEmitterDef->mEmitterOffsetY, aSystemTime, anOffsetYSample);
    if (mEmitterDef->mAnimated == 0 && !FloatTrackIsSet(&mEmitterDef->mAnimationRate))
        aState.mImageFrame = mEmitterDef->mImageFrames > 0 ? Rand(mEmitterDef->mImageFrames) : 0;
    if (mEmitterDef->mFlags & PARTICLE_RANDOM_LAUNCH_SPIN)
        aState.mSpinPosition = Rand(6.2831853f);
    else if (mEmitterDef->mFlags & PARTICLE_ALIGN_LAUNCH_SPIN)
        aState.mSpinPosition = anAngle;

    ParticleListNode* aNode = static_cast<ParticleListNode*>(TodAllocatorAlloc(mParticleList.mpAllocator));
    aNode->mValue = aParticles.DataArrayGetID(aParticle);
    aNode->mPrev = nullptr;
    aNode->mNext = static_cast<ParticleListNode*>(mParticleList.mHead);
    if (aNode->mNext != nullptr)
        aNode->mNext->mPrev = aNode;
    else
        mParticleList.mTail = aNode;
    mParticleList.mHead = aNode;
    ++mParticleList.mSize;
    ++mSyncState.mParticlesSpawned;
    UpdateParticle(aParticle);
    return aParticle;
}

// 0x20A204: fractional accumulation, active count limits and launch limits.
void TodParticleEmitter::UpdateSpawning()
{
    auto& aHolder = *mParticleSystem->mParticleHolder;
    TodParticleEmitter* aFadeEmitter = aHolder.mEmitters.DataArrayTryToGet(mSyncState.mCrossFadeEmitterID);
    TodParticleEmitter* aSource = aFadeEmitter != nullptr ? aFadeEmitter : this;
    auto& aDefinition = *aSource->mEmitterDef;
    float anAccum = mSyncState.mSpawnAccum
        + aSource->SystemTrackEvaluate(aDefinition.mSpawnRate, SYSTEM_TRACK_SPAWN_RATE) * 0.01f;
    int aCount = static_cast<int>(anAccum);
    mSyncState.mSpawnAccum = anAccum - static_cast<float>(aCount);
    int aMinimum = static_cast<int>(aSource->SystemTrackEvaluate(aDefinition.mSpawnMinActive, SYSTEM_TRACK_SPAWN_MIN_ACTIVE));
    int aMaximum = static_cast<int>(aSource->SystemTrackEvaluate(aDefinition.mSpawnMaxActive, SYSTEM_TRACK_SPAWN_MAX_ACTIVE));
    if (aMinimum >= 0)
        aCount = std::max(aCount, aMinimum - mParticleList.mSize);
    if (aMaximum >= 0)
        aCount = std::min(aCount, aMaximum - mParticleList.mSize);
    if (FloatTrackIsSet(&aDefinition.mSpawnMaxLaunched))
    {
        int aLaunchLimit = static_cast<int>(aSource->SystemTrackEvaluate(
            aDefinition.mSpawnMaxLaunched, SYSTEM_TRACK_SPAWN_MAX_LAUNCHED));
        aCount = std::min(aCount, aLaunchLimit - mSyncState.mParticlesSpawned);
    }
    for (int anIndex = 0; anIndex < aCount; ++anIndex)
    {
        TodParticle* aParticle = SpawnParticle(anIndex, aCount);
        if (aParticle == nullptr)
            break;
        if (aFadeEmitter != nullptr)
            CrossFadeParticle(aParticle, aFadeEmitter);
    }
}

// 0x20A310.
void TodParticleEmitter::Update()
{
    if (mSyncState.mDead)
        return;
    bool anExpired = false;
    if (++mSyncState.mSystemAge >= mSyncState.mSystemDuration)
    {
        if (mEmitterDef->mFlags & PARTICLE_SYSTEM_LOOPS)
            mSyncState.mSystemAge = 0;
        else
        {
            mSyncState.mSystemAge = mSyncState.mSystemDuration - 1;
            anExpired = true;
        }
        mSyncState.mSystemTimeValue = anExpired ? 1.0f : 0.0f;
    }
    else
        mSyncState.mSystemTimeValue = mSyncState.mSystemDuration > 1
            ? static_cast<float>(mSyncState.mSystemAge) / (mSyncState.mSystemDuration - 1) : 0.0f;
    if (mSyncState.mEmitterCrossFadeCountDown > 0 && --mSyncState.mEmitterCrossFadeCountDown == 0)
        anExpired = true;
    if (mSyncState.mCrossFadeEmitterID != 0)
    {
        TodParticleEmitter* aTarget = mParticleSystem->mParticleHolder->mEmitters.DataArrayTryToGet(mSyncState.mCrossFadeEmitterID);
        if (aTarget == nullptr || aTarget->mSyncState.mDead)
            anExpired = true;
    }
    for (int anIndex = 0; anIndex < std::min(4, mEmitterDef->mSystemFieldCount); ++anIndex)
        UpdateSystemField(mEmitterDef->mSystemFields[anIndex], anIndex);
    auto& aParticles = mParticleSystem->mParticleHolder->mParticles;
    for (auto* aNode = static_cast<ParticleListNode*>(mParticleList.mHead); aNode != nullptr;)
    {
        auto* aNext = aNode->mNext;
        TodParticle* aParticle = aParticles.DataArrayGet(aNode->mValue);
        if (!UpdateParticle(aParticle))
            DeleteParticle(aParticle);
        aNode = aNext;
    }
    UpdateSpawning();
    if (anExpired)
    {
        for (auto* aNode = static_cast<ParticleListNode*>(mParticleList.mHead); aNode != nullptr;)
        {
            auto* aNext = aNode->mNext;
            TodParticle* aParticle = aParticles.DataArrayGet(aNode->mValue);
            if (aParticle->mSyncState.mCrossFadeDuration <= 0)
                DeleteParticle(aParticle);
            aNode = aNext;
        }
        if (mParticleList.mSize == 0)
            mSyncState.mDead = true;
    }
    if (!mSyncState.mDead)
        mSyncState.mSystemLastTimeValue = mSyncState.mSystemTimeValue;
}

// 0x209A40.
bool TodParticleEmitter::UpdateParticle(TodParticle* theParticle)
{
    auto& aState = theParticle->mSyncState;
    if (aState.mParticleAge >= aState.mParticleDuration)
    {
        if (mEmitterDef->mFlags & PARTICLE_PARTICLE_LOOPS)
            aState.mParticleAge = 0;
        else if (aState.mCrossFadeDuration > 0)
            aState.mParticleAge = aState.mParticleDuration - 1;
        else if (mEmitterDef->mOnDuration == nullptr || mEmitterDef->mOnDuration[0] == 0
            || !CrossFadeParticleToName(theParticle, mEmitterDef->mOnDuration))
            return false;
    }
    if (aState.mCrossFadeParticleID != 0
        && mParticleSystem->mParticleHolder->mParticles.DataArrayTryToGet(aState.mCrossFadeParticleID) == nullptr)
        return false;
    aState.mParticleTimeValue = aState.mParticleDuration > 1
        ? static_cast<float>(aState.mParticleAge) / (aState.mParticleDuration - 1) : 0.0f;
    for (int anIndex = 0; anIndex < std::min(4, mEmitterDef->mParticleFieldCount); ++anIndex)
        UpdateParticleField(theParticle, mEmitterDef->mParticleFields[anIndex], anIndex);
    aState.mPosition.mX += aState.mVelocity.mX;
    aState.mPosition.mY += aState.mVelocity.mY;
    float aSpinSpeed = ParticleTrackEvaluate(mEmitterDef->mParticleSpinSpeed, theParticle, PARTICLE_TRACK_SPIN_SPEED);
    float aSpinAngle = ParticleTrackEvaluate(mEmitterDef->mParticleSpinAngle, theParticle, PARTICLE_TRACK_SPIN_ANGLE);
    float aLastAngle = EvaluatePrevious(mEmitterDef->mParticleSpinAngle,
        aState.mParticleLastTimeValue, aState.mTrackInterp[PARTICLE_TRACK_SPIN_ANGLE]);
    aState.mSpinPosition += aState.mSpinVelocity + (aSpinSpeed * 0.01f + aSpinAngle - aLastAngle) * 0.017453f;
    if (FloatTrackIsSet(&mEmitterDef->mAnimationRate))
    {
        aState.mAnimationTimeValue += ParticleTrackEvaluate(mEmitterDef->mAnimationRate,
            theParticle, PARTICLE_TRACK_ANIMATION_RATE) * 0.01f;
        aState.mAnimationTimeValue -= std::floor(aState.mAnimationTimeValue);
    }
    ++aState.mParticleAge;
    aState.mParticleLastTimeValue = aState.mParticleTimeValue;
    return true;
}

// 0x2085A0: unlink and release both the particle and its cross-fade source.
void TodParticleEmitter::DeleteParticle(TodParticle* theParticle)
{
    auto& aParticles = mParticleSystem->mParticleHolder->mParticles;
    TodParticle* aSource = aParticles.DataArrayTryToGet(theParticle->mSyncState.mCrossFadeParticleID);
    theParticle->mSyncState.mCrossFadeParticleID = 0;
    if (aSource != nullptr)
        aSource->mParticleEmitter->DeleteParticle(aSource);
    unsigned int anID = aParticles.DataArrayGetID(theParticle);
    auto* aNode = static_cast<ParticleListNode*>(mParticleList.mHead);
    while (aNode != nullptr && aNode->mValue != anID)
        aNode = aNode->mNext;
    if (aNode == nullptr)
        return;
    if (aNode->mPrev != nullptr)
        aNode->mPrev->mNext = aNode->mNext;
    else
        mParticleList.mHead = aNode->mNext;
    if (aNode->mNext != nullptr)
        aNode->mNext->mPrev = aNode->mPrev;
    else
        mParticleList.mTail = aNode->mPrev;
    --mParticleList.mSize;
    ReturnNode(aNode, mParticleList.mpAllocator);
    aParticles.DataArrayFree(theParticle);
}

// 0x2083C0 deletes an emitter's own list without recursively deleting sources.
void TodParticleEmitter::DeleteAll()
{
    auto& aParticles = mParticleSystem->mParticleHolder->mParticles;
    for (auto* aNode = static_cast<ParticleListNode*>(mParticleList.mHead); aNode != nullptr;)
    {
        auto* aNext = aNode->mNext;
        aParticles.DataArrayFree(aParticles.DataArrayGet(aNode->mValue));
        ReturnNode(aNode, mParticleList.mpAllocator);
        aNode = aNext;
    }
    mParticleList.mHead = nullptr;
    mParticleList.mTail = nullptr;
    mParticleList.mSize = 0;
}

// 0x2090C8.
void TodParticleEmitter::SystemMove(float theX, float theY)
{
    float aDeltaX = theX - mSyncState.mSystemCenter.mX;
    float aDeltaY = theY - mSyncState.mSystemCenter.mY;
    if (std::abs(aDeltaX) < 0.000001f && std::abs(aDeltaY) < 0.000001f)
        return;
    mSyncState.mSystemCenter = SexyVector2(theX, theY);
    if (mEmitterDef->mFlags & PARTICLE_PARTICLES_DONT_FOLLOW)
        return;
    auto& aParticles = mParticleSystem->mParticleHolder->mParticles;
    for (auto* aNode = static_cast<ParticleListNode*>(mParticleList.mHead); aNode != nullptr; aNode = aNode->mNext)
    {
        auto& aPosition = aParticles.DataArrayGet(aNode->mValue)->mSyncState.mPosition;
        aPosition.mX += aDeltaX;
        aPosition.mY += aDeltaY;
    }
}

// 0x208338.
void TodParticleEmitter::UpdateSystemField(ParticleField& theField, int theIndex)
{
    if (theField.mFieldType != FIELD_SYSTEM_POSITION)
        return;
    float aTime = mSyncState.mSystemTimeValue;
    float aLastTime = mSyncState.mSystemLastTimeValue;
    float aSampleX = mSystemFieldInterp[theIndex][0];
    float aSampleY = mSystemFieldInterp[theIndex][1];
    mSyncState.mSystemCenter.mX += FloatTrackEvaluate(theField.mX, aTime, aSampleX)
        - EvaluatePrevious(theField.mX, aLastTime, aSampleX);
    mSyncState.mSystemCenter.mY += FloatTrackEvaluate(theField.mY, aTime, aSampleY)
        - EvaluatePrevious(theField.mY, aLastTime, aSampleY);
}

// 0x207F7C.
void TodParticleEmitter::UpdateParticleField(TodParticle* theParticle, ParticleField& theField, int theIndex)
{
    auto& aState = theParticle->mSyncState;
    float aSampleX = aState.mFieldInterp[theIndex][0];
    float aSampleY = aState.mFieldInterp[theIndex][1];
    float aFieldX = FloatTrackEvaluate(theField.mX, aState.mParticleTimeValue, aSampleX);
    float aFieldY = FloatTrackEvaluate(theField.mY, aState.mParticleTimeValue, aSampleY);
    auto& aPosition = aState.mPosition;
    auto& aVelocity = aState.mVelocity;
    switch (theField.mFieldType)
    {
    case FIELD_FRICTION:
        aVelocity.mX *= 1.0f - aFieldX;
        aVelocity.mY *= 1.0f - aFieldY;
        break;
    case FIELD_ACCELERATION:
        aVelocity.mX += aFieldX * 0.01f;
        aVelocity.mY += aFieldY * 0.01f;
        break;
    case FIELD_ATTRACTOR:
        aVelocity.mX += (mSyncState.mSystemCenter.mX - aPosition.mX + aFieldX) * 0.01f;
        aVelocity.mY += (mSyncState.mSystemCenter.mY - aPosition.mY + aFieldY) * 0.01f;
        break;
    case FIELD_MAX_VELOCITY:
        aVelocity.mX = std::max(-aFieldX, std::min(aVelocity.mX, aFieldX));
        aVelocity.mY = std::max(-aFieldY, std::min(aVelocity.mY, aFieldY));
        break;
    case FIELD_VELOCITY:
        aPosition.mX += aFieldX * 0.01f;
        aPosition.mY += aFieldY * 0.01f;
        break;
    case FIELD_POSITION:
        aPosition.mX += aFieldX - EvaluatePrevious(theField.mX, aState.mParticleLastTimeValue, aSampleX);
        aPosition.mY += aFieldY - EvaluatePrevious(theField.mY, aState.mParticleLastTimeValue, aSampleY);
        break;
    case FIELD_GROUND_CONSTRAINT:
        if (aPosition.mY > mSyncState.mSystemCenter.mY + aFieldY)
        {
            aPosition.mY = mSyncState.mSystemCenter.mY + aFieldY;
            float aReflect = ParticleTrackEvaluate(mEmitterDef->mCollisionReflect,
                theParticle, PARTICLE_TRACK_COLLISION_REFLECT);
            float aSpin = ParticleTrackEvaluate(mEmitterDef->mCollisionSpin,
                theParticle, PARTICLE_TRACK_COLLISION_SPIN);
            aState.mSpinVelocity = aVelocity.mY * 0.001f * aSpin;
            aVelocity.mX *= aReflect;
            aVelocity.mY *= -aReflect;
        }
        break;
    case FIELD_SHAKE:
    {
        unsigned int anID = mParticleSystem->mParticleHolder->mParticles.DataArrayGetID(theParticle);
        int aLastAge = aState.mParticleAge == 0 ? aState.mParticleDuration - 1 : aState.mParticleAge - 1;
        SexyVector2 aNoise = ShakeNoise(anID, aState.mParticleAge);
        SexyVector2 aLastNoise = ShakeNoise(anID, aLastAge);
        aPosition.mX += aFieldX * aNoise.mX
            - EvaluatePrevious(theField.mX, aState.mParticleLastTimeValue, aSampleX) * aLastNoise.mX;
        aPosition.mY += aFieldY * aNoise.mY
            - EvaluatePrevious(theField.mY, aState.mParticleLastTimeValue, aSampleY) * aLastNoise.mY;
        break;
    }
    case FIELD_CIRCLE:
    case FIELD_AWAY:
    {
        float aDeltaX = aPosition.mX - mSyncState.mSystemCenter.mX;
        float aDeltaY = aPosition.mY - mSyncState.mSystemCenter.mY;
        float aDistance = std::sqrt(aDeltaX * aDeltaX + aDeltaY * aDeltaY);
        float aSpeed = (aFieldX + aFieldY * aDistance) * 0.01f;
        if (aDistance != 0.0f)
        {
            aDeltaX /= aDistance;
            aDeltaY /= aDistance;
        }
        if (theField.mFieldType == FIELD_CIRCLE)
        {
            aPosition.mX -= aDeltaY * aSpeed;
            aPosition.mY += aDeltaX * aSpeed;
        }
        else
        {
            aPosition.mX += aDeltaX * aSpeed;
            aPosition.mY += aDeltaY * aSpeed;
        }
        break;
    }
    default:
        break;
    }
}

// 0x20A0F8 / 0x20A17C / 0x209930.
bool TodParticleEmitter::CrossFadeParticle(TodParticle* theParticle, TodParticleEmitter* theEmitter)
{
    if (theParticle->mSyncState.mCrossFadeDuration > 0
        || !FloatTrackIsSet(&theEmitter->mEmitterDef->mCrossFadeDuration))
        return false;
    TodParticle* aTarget = theEmitter->SpawnParticle(0, 1);
    if (aTarget == nullptr)
        return false;
    int aDuration = mSyncState.mEmitterCrossFadeCountDown;
    if (aDuration <= 0)
        aDuration = std::max(1, static_cast<int>(FloatTrackEvaluate(
            theEmitter->mEmitterDef->mCrossFadeDuration, mSyncState.mSystemTimeValue, Rand(1.0f))));
    theParticle->mSyncState.mCrossFadeDuration = aDuration;
    if (!FloatTrackIsSet(&theEmitter->mEmitterDef->mParticleDuration))
        aTarget->mSyncState.mParticleDuration = aDuration;
    aTarget->mSyncState.mCrossFadeParticleID =
        mParticleSystem->mParticleHolder->mParticles.DataArrayGetID(theParticle);
    return true;
}

void TodParticleEmitter::CrossFadeEmitter(TodParticleEmitter* theEmitter)
{
    if (mSyncState.mEmitterCrossFadeCountDown > 0
        || !FloatTrackIsSet(&theEmitter->mEmitterDef->mCrossFadeDuration))
        return;
    mSyncState.mEmitterCrossFadeCountDown = std::max(1, static_cast<int>(FloatTrackEvaluate(
        theEmitter->mEmitterDef->mCrossFadeDuration, mSyncState.mSystemTimeValue, Rand(1.0f))));
    mSyncState.mCrossFadeEmitterID = mParticleSystem->mParticleHolder->mEmitters.DataArrayGetID(theEmitter);
    if (!FloatTrackIsSet(&theEmitter->mEmitterDef->mSystemDuration))
        theEmitter->mSyncState.mSystemDuration = mSyncState.mEmitterCrossFadeCountDown;
    auto& aParticles = mParticleSystem->mParticleHolder->mParticles;
    for (auto* aNode = static_cast<ParticleListNode*>(mParticleList.mHead); aNode != nullptr; aNode = aNode->mNext)
        CrossFadeParticle(aParticles.DataArrayGet(aNode->mValue), theEmitter);
}

bool TodParticleEmitter::CrossFadeParticleToName(TodParticle* theParticle, const char* theName)
{
    TodEmitterDefinition* aDefinition = mParticleSystem->FindEmitterDefByName(theName);
    auto& aHolder = *mParticleSystem->mParticleHolder;
    if (aDefinition == nullptr || aHolder.mEmitters.mSize == aHolder.mEmitters.mMaxSize)
        return false;
    TodParticleEmitter* anEmitter = aHolder.mEmitters.DataArrayAlloc();
    anEmitter->TodEmitterInitialize(mSyncState.mSystemCenter.mX, mSyncState.mSystemCenter.mY,
        mParticleSystem, aDefinition);
    auto& aList = mParticleSystem->mEmitterList;
    auto* aNode = static_cast<ParticleListNode*>(TodAllocatorAlloc(aList.mpAllocator));
    aNode->mValue = aHolder.mEmitters.DataArrayGetID(anEmitter);
    aNode->mNext = nullptr;
    aNode->mPrev = static_cast<ParticleListNode*>(aList.mTail);
    if (aNode->mPrev != nullptr)
        aNode->mPrev->mNext = aNode;
    else
        aList.mHead = aNode;
    aList.mTail = aNode;
    ++aList.mSize;
    return CrossFadeParticle(theParticle, anEmitter);
}

// 0x208684. Source channels only participate when explicitly keyed/overridden.
void TodParticleEmitter::GetRenderParams(TodParticle* theParticle, ParticleRenderParams& theParams)
{
    auto& aDef = *mEmitterDef;
    auto& aColor = mSyncState.mColorOverride;
    auto& aState = theParticle->mSyncState;
    theParams.mRedSet = FloatTrackIsSet(&aDef.mSystemRed) || FloatTrackIsSet(&aDef.mParticleRed) || aColor.mRed != 1;
    theParams.mGreenSet = FloatTrackIsSet(&aDef.mSystemGreen) || FloatTrackIsSet(&aDef.mParticleGreen) || aColor.mGreen != 1;
    theParams.mBlueSet = FloatTrackIsSet(&aDef.mSystemBlue) || FloatTrackIsSet(&aDef.mParticleBlue) || aColor.mBlue != 1;
    theParams.mAlphaSet = FloatTrackIsSet(&aDef.mSystemAlpha) || FloatTrackIsSet(&aDef.mParticleAlpha) || aColor.mAlpha != 1;
    theParams.mScaleSet = FloatTrackIsSet(&aDef.mParticleScale) || mSyncState.mScaleOverride != 1.0f;
    theParams.mStretchSet = FloatTrackIsSet(&aDef.mParticleStretch);
    theParams.mSpinSet = FloatTrackIsSet(&aDef.mParticleSpinSpeed) || FloatTrackIsSet(&aDef.mParticleSpinAngle)
        || (aDef.mFlags & (PARTICLE_RANDOM_LAUNCH_SPIN | PARTICLE_ALIGN_LAUNCH_SPIN)) != 0;
    theParams.mPositionSet = aDef.mParticleFieldCount > 0 || FloatTrackIsSet(&aDef.mEmitterRadius)
        || FloatTrackIsSet(&aDef.mEmitterOffsetX) || FloatTrackIsSet(&aDef.mEmitterOffsetY)
        || FloatTrackIsSet(&aDef.mEmitterBoxX) || FloatTrackIsSet(&aDef.mEmitterBoxY);
    float aBrightness = SystemTrackEvaluate(aDef.mSystemBrightness, SYSTEM_TRACK_BRIGHTNESS)
        * ParticleTrackEvaluate(aDef.mParticleBrightness, theParticle, PARTICLE_TRACK_BRIGHTNESS);
    theParams.mRed = SystemTrackEvaluate(aDef.mSystemRed, SYSTEM_TRACK_RED)
        * ParticleTrackEvaluate(aDef.mParticleRed, theParticle, PARTICLE_TRACK_RED) * aColor.mRed * aBrightness;
    theParams.mGreen = SystemTrackEvaluate(aDef.mSystemGreen, SYSTEM_TRACK_GREEN)
        * ParticleTrackEvaluate(aDef.mParticleGreen, theParticle, PARTICLE_TRACK_GREEN) * aColor.mGreen * aBrightness;
    theParams.mBlue = SystemTrackEvaluate(aDef.mSystemBlue, SYSTEM_TRACK_BLUE)
        * ParticleTrackEvaluate(aDef.mParticleBlue, theParticle, PARTICLE_TRACK_BLUE) * aColor.mBlue * aBrightness;
    theParams.mAlpha = SystemTrackEvaluate(aDef.mSystemAlpha, SYSTEM_TRACK_ALPHA)
        * ParticleTrackEvaluate(aDef.mParticleAlpha, theParticle, PARTICLE_TRACK_ALPHA) * aColor.mAlpha;
    theParams.mScale = ParticleTrackEvaluate(aDef.mParticleScale, theParticle, PARTICLE_TRACK_SCALE) * mSyncState.mScaleOverride;
    theParams.mStretch = ParticleTrackEvaluate(aDef.mParticleStretch, theParticle, PARTICLE_TRACK_STRETCH);
    theParams.mSpin = aState.mSpinPosition;
    theParams.mPositionX = aState.mPosition.mX;
    theParams.mPositionY = aState.mPosition.mY;
    TodParticle* aSource = mParticleSystem->mParticleHolder->mParticles.DataArrayTryToGet(aState.mCrossFadeParticleID);
    if (aSource == nullptr)
        return;
    ParticleRenderParams aSourceParams;
    aSource->mParticleEmitter->GetRenderParams(aSource, aSourceParams);
    float aTime = aSource->mSyncState.mCrossFadeDuration > 1
        ? static_cast<float>(aState.mParticleAge) / (aSource->mSyncState.mCrossFadeDuration - 1) : 1.0f;
    BlendParam(theParams.mRed, theParams.mRedSet, aSourceParams.mRed, aSourceParams.mRedSet, aTime);
    BlendParam(theParams.mGreen, theParams.mGreenSet, aSourceParams.mGreen, aSourceParams.mGreenSet, aTime);
    BlendParam(theParams.mBlue, theParams.mBlueSet, aSourceParams.mBlue, aSourceParams.mBlueSet, aTime);
    BlendParam(theParams.mAlpha, theParams.mAlphaSet, aSourceParams.mAlpha, aSourceParams.mAlphaSet, aTime);
    BlendParam(theParams.mScale, theParams.mScaleSet, aSourceParams.mScale, aSourceParams.mScaleSet, aTime);
    BlendParam(theParams.mStretch, theParams.mStretchSet, aSourceParams.mStretch, aSourceParams.mStretchSet, aTime);
    BlendParam(theParams.mSpin, theParams.mSpinSet, aSourceParams.mSpin, aSourceParams.mSpinSet, aTime);
    bool aPositionSet = theParams.mPositionSet;
    BlendParam(theParams.mPositionX, aPositionSet, aSourceParams.mPositionX, aSourceParams.mPositionSet, aTime);
    BlendParam(theParams.mPositionY, theParams.mPositionSet, aSourceParams.mPositionY, aSourceParams.mPositionSet, aTime);
}

// 0x208BA0: source cels, clipping, transforms, full-screen and additive passes.
void TodParticleEmitter::DrawParticle(Sexy::Graphics* theGraphics, TodParticle* theParticle)
{
    if (theParticle->mSyncState.mCrossFadeDuration > 0)
        return;
    ParticleRenderParams aParams;
    GetRenderParams(theParticle, aParams);
    Sexy::Color aColor(std::clamp(RoundToInt(aParams.mRed), 0, 255),
        std::clamp(RoundToInt(aParams.mGreen), 0, 255),
        std::clamp(RoundToInt(aParams.mBlue), 0, 255),
        std::clamp(RoundToInt(aParams.mAlpha), 0, 255));
    if (aColor.mAlpha == 0)
        return;
    TodParticle* anImageParticle = theParticle;
    if (mSyncState.mImageOverride == nullptr && mEmitterDef->mImage == nullptr)
    {
        anImageParticle = mParticleSystem->mParticleHolder->mParticles.DataArrayTryToGet(
            theParticle->mSyncState.mCrossFadeParticleID);
        if (anImageParticle == nullptr)
            return;
    }
    auto& anEmitter = *anImageParticle->mParticleEmitter;
    auto& aDefinition = *anEmitter.mEmitterDef;
    auto& aState = anImageParticle->mSyncState;
    Sexy::Image* anImage = anEmitter.mSyncState.mImageOverride;
    if (anImage == nullptr)
        anImage = aDefinition.mImage;
    if (anImage == nullptr)
        return;
    int aWidth = anImage->GetCelWidth();
    int aHeight = anImage->GetCelHeight();
    int aFrame = anEmitter.mSyncState.mFrameOverride;
    if (aFrame < 0 || aFrame >= anImage->GetCelCount())
    {
        aFrame = aState.mImageFrame;
        if (FloatTrackIsSet(&aDefinition.mAnimationRate))
            aFrame = static_cast<int>(aDefinition.mImageFrames * aState.mAnimationTimeValue);
        else if (aDefinition.mAnimated != 0)
            aFrame = static_cast<int>(aDefinition.mImageFrames * aState.mParticleTimeValue);
        aFrame = std::clamp(aFrame, 0, std::max(0, aDefinition.mImageFrames - 1));
    }
    Sexy::Rect aSource(std::min(aFrame + aDefinition.mImageCol, anImage->mNumCols - 1) * aWidth,
        std::min(aDefinition.mImageRow, anImage->mNumRows - 1) * aHeight, aWidth, aHeight);
    float aClipTop = anEmitter.ParticleTrackEvaluate(aDefinition.mClipTop, anImageParticle, PARTICLE_TRACK_CLIP_TOP);
    float aClipBottom = anEmitter.ParticleTrackEvaluate(aDefinition.mClipBottom, anImageParticle, PARTICLE_TRACK_CLIP_BOTTOM);
    float aClipLeft = anEmitter.ParticleTrackEvaluate(aDefinition.mClipLeft, anImageParticle, PARTICLE_TRACK_CLIP_LEFT);
    float aClipRight = anEmitter.ParticleTrackEvaluate(aDefinition.mClipRight, anImageParticle, PARTICLE_TRACK_CLIP_RIGHT);
    float aPositionX = aParams.mPositionX + theGraphics->mTransX + aClipLeft * aWidth;
    float aPositionY = aParams.mPositionY + theGraphics->mTransY + aClipTop * aHeight;
    aSource.mX += RoundToInt(aClipLeft * aWidth);
    aSource.mY += RoundToInt(aClipTop * aHeight);
    aSource.mWidth -= RoundToInt((aClipLeft + aClipRight) * aWidth);
    aSource.mHeight -= RoundToInt((aClipTop + aClipBottom) * aHeight);
    if (aDefinition.mFlags & PARTICLE_PIXEL_ALIGNED)
    {
        aPositionX = static_cast<float>(RoundToInt(aPositionX));
        aPositionY = static_cast<float>(RoundToInt(aPositionY));
    }
    int aDrawMode = (aDefinition.mFlags & PARTICLE_ADDITIVE)
        ? Sexy::Graphics::DRAWMODE_ADDITIVE : theGraphics->mDrawMode;
    if (aDefinition.mFlags & PARTICLE_FULL_SCREEN)
    {
        Sexy::Color anOldColor = theGraphics->mColor;
        int anOldMode = theGraphics->mDrawMode;
        theGraphics->SetColor(aColor);
        theGraphics->SetDrawMode(aDrawMode);
        theGraphics->FillRect(static_cast<int>(-theGraphics->mTransX),
            static_cast<int>(-theGraphics->mTransY), 1280, 720);
        theGraphics->SetColor(anOldColor);
        theGraphics->SetDrawMode(anOldMode);
        return;
    }
    Sexy::SexyMatrix3 aMatrix;
    TodScaleRotateTransformMatrix(aMatrix, aPositionX, aPositionY, aParams.mSpin,
        aParams.mScale, aParams.mScale * aParams.mStretch);
    TodBltMatrix(theGraphics, anImage, aMatrix, theGraphics->mClipRect, aColor, aDrawMode, aSource);
    if (anEmitter.mSyncState.mExtraAdditiveDrawOverride)
        TodBltMatrix(theGraphics, anImage, aMatrix, theGraphics->mClipRect,
            aColor, Sexy::Graphics::DRAWMODE_ADDITIVE, aSource);
}

// 0x209024.
void TodParticleEmitter::Draw(Sexy::Graphics* theGraphics)
{
    bool anAccelerated = gSexyAppBase != nullptr && gSexyAppBase->Is3DAccelerated();
    if ((mEmitterDef->mFlags & PARTICLE_SOFTWARE_ONLY) && anAccelerated)
        return;
    if ((mEmitterDef->mFlags & PARTICLE_HARDWARE_ONLY) && !anAccelerated)
        return;
    auto& aParticles = mParticleSystem->mParticleHolder->mParticles;
    for (auto* aNode = static_cast<ParticleListNode*>(mParticleList.mHead); aNode != nullptr; aNode = aNode->mNext)
        DrawParticle(theGraphics, aParticles.DataArrayGet(aNode->mValue));
}
