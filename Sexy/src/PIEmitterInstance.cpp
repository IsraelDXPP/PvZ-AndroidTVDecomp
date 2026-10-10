#include "PIEmitterInstance.h"

Sexy::PIEmitterInstance::PIEmitterInstance() :
    mDef(nullptr),
    mTint(),
    mImage(),
    mOffsetX(0.0f),
    mOffsetY(0.0f),
    mRateScale(1.0f),
    mActive(true)
{
    mTransform.LoadIdentity();
}

Sexy::PIEmitterInstance::PIEmitterInstance(const Sexy::PIEmitterInstance &other) :
    mDef(other.mDef),
    mTint(other.mTint),
    mImage(other.mImage),
    mTransform(other.mTransform),
    mOffsetX(other.mOffsetX),
    mOffsetY(other.mOffsetY),
    mRateScale(other.mRateScale),
    mActive(other.mActive)
{
}
