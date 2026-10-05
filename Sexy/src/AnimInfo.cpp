// AnimInfo picks which cel an image shows for a tick.
//
// Per-frame delays are optional. An empty delay list means every cel lasts
// mFrameDelay. A frame map, when present, remaps that cel. Ping-pong builds
// the map as the cels forward and then back, skipping the two end cels.

#include "AnimInfo.h"

Sexy::AnimInfo::AnimInfo()
    : mAnimType(AnimType_None)
    , mFrameDelay(1)
    , mNumCels(1)
    , mTotalAnimTime(0)
{
}

int Sexy::AnimInfo::GetPerFrameCel(int theTick)
{
    if (mNumCels <= 0 || mPerFrameDelay.empty())
        return mNumCels - 1;

    int frameTime = theTick - mPerFrameDelay[0];
    if (frameTime < 0)
        return 0;

    const int delayCount = (int)mPerFrameDelay.size();
    for (int frame = 1; frame < mNumCels; ++frame)
    {
        if (frame >= delayCount)
            return frame;
        frameTime -= mPerFrameDelay[(size_t)frame];
        if (frameTime < 0)
            return frame;
    }
    return mNumCels - 1;
}

int Sexy::AnimInfo::GetCel(int theTick)
{
    if (mAnimType == AnimType_Once && mTotalAnimTime <= theTick)
    {
        if (mFrameMap.empty())
            return mNumCels - 1;
        return mFrameMap.back();
    }

    int cel = 0;
    if (mTotalAnimTime > 0)
        cel = theTick % mTotalAnimTime;
    if (mPerFrameDelay.empty())
    {
        if (mFrameDelay > 0 && mNumCels > 0)
            cel = cel / mFrameDelay % mNumCels;
    }
    else
    {
        cel = GetPerFrameCel(cel);
    }

    if (cel >= 0 && cel < (int)mFrameMap.size())
        return mFrameMap[(size_t)cel];
    return cel;
}

unsigned int Sexy::AnimInfo::SetPerFrameDelay(int theCel, int theDelay)
{
    const unsigned int oldSize = (unsigned int)mPerFrameDelay.size();
    if (theCel < 0)
        return oldSize;
    if (theCel >= (int)oldSize)
        mPerFrameDelay.resize((size_t)theCel + 1, 0);
    mPerFrameDelay[(size_t)theCel] = theDelay;
    return oldSize;
}

void Sexy::AnimInfo::Compute(int theNumCels, int theBeginFrameTime, int theEndFrameTime)
{
    if (theNumCels <= 0)
        theNumCels = 1;
    mNumCels = theNumCels;
    if (mFrameDelay <= 0)
        mFrameDelay = 1;

    if (mAnimType == AnimType_PingPong && mNumCels > 1)
    {
        const int numFrames = 2 * (mNumCels - 1);
        mFrameMap.assign((size_t)numFrames, 0);
        for (int i = 0; i < mNumCels; ++i)
            mFrameMap[(size_t)i] = i;
        for (int i = 0; i < mNumCels - 2; ++i)
            mFrameMap[(size_t)(mNumCels + i)] = mNumCels - 2 - i;
    }

    if (!mFrameMap.empty())
        mNumCels = (int)mFrameMap.size();

    if (theBeginFrameTime > 0)
        SetPerFrameDelay(0, theBeginFrameTime);
    if (theEndFrameTime > 0)
        SetPerFrameDelay(mNumCels - 1, theEndFrameTime);

    if (mPerFrameDelay.empty())
    {
        mTotalAnimTime = mNumCels * mFrameDelay;
    }
    else
    {
        if ((int)mPerFrameDelay.size() != mNumCels)
            mPerFrameDelay.resize((size_t)mNumCels, 0);
        mTotalAnimTime = 0;
        for (int i = 0; i < mNumCels; ++i)
        {
            if (mPerFrameDelay[(size_t)i] <= 0)
                mPerFrameDelay[(size_t)i] = mFrameDelay;
            mTotalAnimTime += mPerFrameDelay[(size_t)i];
        }
    }

    if (!mFrameMap.empty() && (int)mFrameMap.size() != mNumCels)
        mFrameMap.resize((size_t)mNumCels, 0);
}
