// A sampled 2D curve. A bezier, when present, replaces the key list.
// Otherwise the value is a linear blend of the two keys that bracket the
// query, and the velocity is that segment's delta.

#include "PIValue2D.h"

Sexy::PIValue2D::PIValue2D(const PIValue2D& other)
    : mKeyframes(other.mKeyframes)
    , mBezier(other.mBezier)
    , mLastValueTime(other.mLastValueTime)
    , mLastValueX(other.mLastValueX)
    , mLastValueY(other.mLastValueY)
    , mLastVelocityTime(other.mLastVelocityTime)
    , mLastVelocityX(other.mLastVelocityX)
    , mLastVelocityY(other.mLastVelocityY)
{
}

float* Sexy::PIValue2D::GetValueAt(float theTime)
{
    if (mLastValueTime == theTime)
        return &mLastValueX;

    mLastValueTime = theTime;
    const int keyCount = (int)mKeyframes.size();

    if (keyCount == 1)
    {
        mLastValueX = mKeyframes[0].mX;
        mLastValueY = mKeyframes[0].mY;
        return &mLastValueX;
    }

    if (mBezier.mPointCount > 0)
    {
        const float* evaluated = mBezier.Evaluate(theTime);
        mLastValueX = evaluated[0];
        mLastValueY = evaluated[1];
        return &mLastValueX;
    }

    if (keyCount <= 1)
    {
        mLastValueX = 0.0f;
        mLastValueY = 0.0f;
        return &mLastValueX;
    }

    int index = 1;
    float startTime = 0.0f;
    float endTime = 0.0f;
    for (;;)
    {
        startTime = mKeyframes[(size_t)index - 1].mTime;
        if (theTime >= startTime)
        {
            endTime = mKeyframes[(size_t)index].mTime;
            if (theTime <= endTime)
                break;
        }
        if (index == keyCount - 1)
        {
            endTime = mKeyframes[(size_t)index].mTime;
            break;
        }
        ++index;
        if (index == keyCount)
        {
            mLastValueX = 0.0f;
            mLastValueY = 0.0f;
            return &mLastValueX;
        }
    }

    float mix = (theTime - startTime) / (endTime - startTime);
    if (mix > 1.0f)
        mix = 1.0f;
    mLastValueX = mKeyframes[(size_t)index - 1].mX
        + mix * (mKeyframes[(size_t)index].mX - mKeyframes[(size_t)index - 1].mX);
    mLastValueY = mKeyframes[(size_t)index - 1].mY
        + mix * (mKeyframes[(size_t)index].mY - mKeyframes[(size_t)index - 1].mY);
    return &mLastValueX;
}

float* Sexy::PIValue2D::GetVelocityAt(float theTime)
{
    if (mLastVelocityTime == theTime)
        return &mLastVelocityX;

    mLastVelocityTime = theTime;
    const int keyCount = (int)mKeyframes.size();

    if (keyCount <= 1)
    {
        mLastVelocityX = 0.0f;
        mLastVelocityY = 0.0f;
        return &mLastVelocityX;
    }

    if (mBezier.mPointCount > 0)
    {
        const float* evaluated = mBezier.Velocity(theTime, 0);
        mLastVelocityX = evaluated[0];
        mLastVelocityY = evaluated[1];
        return &mLastVelocityX;
    }

    int index = 1;
    while (theTime < mKeyframes[(size_t)index - 1].mTime
        || theTime > mKeyframes[(size_t)index].mTime)
    {
        if (index == keyCount - 1)
            break;
        ++index;
        if (index == keyCount)
        {
            mLastVelocityX = 0.0f;
            mLastVelocityY = 0.0f;
            return &mLastVelocityX;
        }
    }

    mLastVelocityX = mKeyframes[(size_t)index].mX - mKeyframes[(size_t)index - 1].mX;
    mLastVelocityY = mKeyframes[(size_t)index].mY - mKeyframes[(size_t)index - 1].mY;
    return &mLastVelocityX;
}
