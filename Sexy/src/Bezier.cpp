// Piecewise cubic Bezier. The shared result buffer is what Evaluate, Velocity
// and Acceleration hand back; callers copy the two floats before the next call.

#include "Bezier.h"
#include "ReconExterns.h"

#include <cmath>
#include <cstring>

namespace
{

float gBezierPointResult[2];

const float kControlScale = 0.33333f;

float CubicAt(float theStart, float theControlA, float theControlB, float theEnd, float theU)
{
    const float aFromStart = 3.0f * theControlA - 3.0f * theStart;
    const float aMid = 3.0f * theStart + 3.0f * theControlB - 6.0f * theControlA;
    const float aEnd = 3.0f * theControlA - theStart + theEnd - 3.0f * theControlB;
    return theStart + ((aFromStart + (aMid + theU * aEnd) * theU) * theU);
}

float CubicSpeed(float theStart, float theControlA, float theControlB, float theEnd, float theU)
{
    const float aFromStart = 3.0f * theControlA - 3.0f * theStart;
    const float aMid = 6.0f * theControlB + 6.0f * theStart - 12.0f * theControlA;
    const float aEnd = 3.0f * (3.0f * theControlA - theStart + theEnd - 3.0f * theControlB);
    return aFromStart + (aMid + aEnd * theU) * theU;
}

float CubicAccel(float theStart, float theControlA, float theControlB, float theEnd, float theU)
{
    const float aMid = 6.0f * theControlB + 6.0f * theStart - 12.0f * theControlA;
    const float aEnd = 6.0f * (3.0f * theControlA - theStart + theEnd - 3.0f * theControlB);
    return aMid + aEnd * theU;
}

float PointDistance(const Sexy::TPoint<float> &theA, const Sexy::TPoint<float> &theB)
{
    const float aDx = theB.mX - theA.mX;
    const float aDy = theB.mY - theA.mY;
    return sqrtf(aDx * aDx + aDy * aDy);
}

struct CurveSpan
{
    int mIndex;
    float mLocal;
    const float *mTime;
    float mTimeEnd;
};

CurveSpan FindSpan(const float *theTimes, int theCount, float theTime)
{
    CurveSpan aSpan;
    aSpan.mIndex = 0;
    aSpan.mLocal = 0.0f;
    aSpan.mTime = theTimes;
    aSpan.mTimeEnd = theTimes[1];
    if (theCount < 2)
        return aSpan;
    if (theTime >= theTimes[theCount - 1])
    {
        aSpan.mIndex = theCount - 2;
        aSpan.mTime = theTimes + aSpan.mIndex;
        aSpan.mTimeEnd = theTimes[theCount - 1];
        aSpan.mLocal = 1.0f;
        return aSpan;
    }
    if (theTime < theTimes[1])
        return aSpan;

    const int aLastSegment = theCount - 2;
    int aIndex = 0;
    const float *aCursor = theTimes + 2;
    while (aIndex < aLastSegment)
    {
        ++aIndex;
        const float aNext = *aCursor++;
        if (aNext > theTime)
        {
            aSpan.mIndex = aIndex;
            aSpan.mTime = theTimes + aIndex;
            aSpan.mTimeEnd = aNext;
            return aSpan;
        }
    }
    aSpan.mIndex = aLastSegment;
    aSpan.mTime = theTimes + aLastSegment;
    aSpan.mTimeEnd = theTimes[theCount - 1];
    return aSpan;
}

void CopyPoint(float *theDest, const float *thePoints, int thePointIndex)
{
    theDest[0] = thePoints[2 * thePointIndex];
    theDest[1] = thePoints[2 * thePointIndex + 1];
}

} // namespace

Sexy::Bezier::Bezier()
{
    mTimes = nullptr;
    mSegmentLengths = nullptr;
    mTotalLength = 0.0f;
    mPointCount = 0;
    mDerivativePoints = nullptr;
    mPoints = nullptr;
    mUnusedFlag = -1;
}

Sexy::Bezier::~Bezier()
{
    Clean();
}

void Sexy::Bezier::Clean()
{
    delete[] mTimes;
    delete[] mSegmentLengths;
    delete[] mDerivativePoints;
    delete[] mPoints;
    mTimes = nullptr;
    mSegmentLengths = nullptr;
    mDerivativePoints = nullptr;
    mPoints = nullptr;
    mPointCount = 0;
    mTotalLength = 0.0f;
}

Sexy::Bezier &Sexy::Bezier::operator=(Sexy::Bezier const &theOther)
{
    if (this == &theOther)
        return *this;

    Clean();
    mUnusedFlag = theOther.mUnusedFlag;
    mPointCount = theOther.mPointCount;
    mTotalLength = theOther.mTotalLength;
    if (mPointCount <= 0)
        return *this;

    mTimes = new float[mPointCount];
    mPoints = new float[2 * mPointCount];
    memcpy(mTimes, theOther.mTimes, sizeof(float) * mPointCount);
    memcpy(mPoints, theOther.mPoints, sizeof(float) * 2 * mPointCount);

    const int aSegments = mPointCount - 1;
    if (aSegments > 0)
    {
        mSegmentLengths = new float[aSegments];
        mDerivativePoints = new float[4 * aSegments];
        if (theOther.mSegmentLengths != nullptr)
            memcpy(mSegmentLengths, theOther.mSegmentLengths, sizeof(float) * aSegments);
        if (theOther.mDerivativePoints != nullptr)
            memcpy(mDerivativePoints, theOther.mDerivativePoints, sizeof(float) * 4 * aSegments);
    }
    return *this;
}

Sexy::Bezier::Bezier(Sexy::Bezier const &theOther)
{
    mTimes = nullptr;
    mSegmentLengths = nullptr;
    mDerivativePoints = nullptr;
    mPoints = nullptr;
    mPointCount = 0;
    mTotalLength = 0.0f;
    mUnusedFlag = -1;
    *this = theOther;
}

float *Sexy::Bezier::Evaluate(float theTime)
{
    float *aResult = gBezierPointResult;
    if (mPointCount <= 1)
    {
        aResult[0] = 0.0f;
        aResult[1] = 0.0f;
        return aResult;
    }
    if (theTime <= mTimes[0])
    {
        CopyPoint(aResult, mPoints, 0);
        return aResult;
    }
    if (theTime >= mTimes[mPointCount - 1])
    {
        CopyPoint(aResult, mPoints, mPointCount - 1);
        return aResult;
    }

    const CurveSpan aSpan = FindSpan(mTimes, mPointCount, theTime);
    const float aDenom = aSpan.mTimeEnd - *aSpan.mTime;
    const float aU = (aDenom != 0.0f) ? (theTime - *aSpan.mTime) / aDenom : 0.0f;
    const float *aStart = mPoints + 2 * aSpan.mIndex;
    const float *aEnd = aStart + 2;
    const float *aControls = mDerivativePoints + 4 * aSpan.mIndex;
    aResult[0] = CubicAt(aStart[0], aControls[0], aControls[2], aEnd[0], aU);
    aResult[1] = CubicAt(aStart[1], aControls[1], aControls[3], aEnd[1], aU);
    return aResult;
}

float *Sexy::Bezier::Velocity(float theTime, int theClampToEnd)
{
    float *aResult = gBezierPointResult;
    if (mPointCount <= 1 || theTime <= mTimes[0] || theTime >= mTimes[mPointCount - 1])
    {
        if (theClampToEnd != 0 && mPointCount > 1)
        {
            const int aIndex = (theTime <= mTimes[0]) ? 0 : mPointCount - 1;
            CopyPoint(aResult, mPoints, aIndex);
            return aResult;
        }
        aResult[0] = 0.0f;
        aResult[1] = 0.0f;
        return aResult;
    }

    const CurveSpan aSpan = FindSpan(mTimes, mPointCount, theTime);
    const float aDenom = aSpan.mTimeEnd - *aSpan.mTime;
    const float aU = (aDenom != 0.0f) ? (theTime - *aSpan.mTime) / aDenom : 0.0f;
    const float *aStart = mPoints + 2 * aSpan.mIndex;
    const float *aEnd = aStart + 2;
    const float *aControls = mDerivativePoints + 4 * aSpan.mIndex;
    aResult[0] = CubicSpeed(aStart[0], aControls[0], aControls[2], aEnd[0], aU);
    aResult[1] = CubicSpeed(aStart[1], aControls[1], aControls[3], aEnd[1], aU);
    return aResult;
}

float *Sexy::Bezier::Acceleration(float theTime)
{
    float *aResult = gBezierPointResult;
    if (mPointCount <= 1)
    {
        aResult[0] = 0.0f;
        aResult[1] = 0.0f;
        return aResult;
    }
    if (theTime <= mTimes[0])
    {
        CopyPoint(aResult, mPoints, 0);
        return aResult;
    }
    if (theTime >= mTimes[mPointCount - 1])
    {
        CopyPoint(aResult, mPoints, mPointCount - 1);
        return aResult;
    }

    const CurveSpan aSpan = FindSpan(mTimes, mPointCount, theTime);
    const float aDenom = aSpan.mTimeEnd - *aSpan.mTime;
    const float aU = (aDenom != 0.0f) ? (theTime - *aSpan.mTime) / aDenom : 0.0f;
    const float *aStart = mPoints + 2 * aSpan.mIndex;
    const float *aEnd = aStart + 2;
    const float *aControls = mDerivativePoints + 4 * aSpan.mIndex;
    aResult[0] = CubicAccel(aStart[0], aControls[0], aControls[2], aEnd[0], aU);
    aResult[1] = CubicAccel(aStart[1], aControls[1], aControls[3], aEnd[1], aU);
    return aResult;
}

float Sexy::Bezier::SubdivideLength(TPoint<float> const &theStart, TPoint<float> const &theControlA,
    TPoint<float> const &theControlB, TPoint<float> const &theEnd)
{
    const float aChord = PointDistance(theStart, theEnd);
    const float aPolyline = PointDistance(theStart, theControlA) + PointDistance(theControlA, theControlB) +
        PointDistance(theControlB, theEnd);
    const float aError = aChord - aPolyline;
    if (aError * aError < 0.001f)
        return (aPolyline + aChord) * 0.5f;

    const TPoint<float> aLeftA = (theStart + theControlA) * 0.5f;
    const TPoint<float> aMidControl = (theControlA + theControlB) * 0.5f;
    const TPoint<float> aRightC = (theControlB + theEnd) * 0.5f;
    const TPoint<float> aLeftB = (aLeftA + aMidControl) * 0.5f;
    const TPoint<float> aRightB = (aMidControl + aRightC) * 0.5f;
    const TPoint<float> aMid = (aLeftB + aRightB) * 0.5f;
    return SubdivideLength(theStart, aLeftA, aLeftB, aMid) + SubdivideLength(aMid, aRightB, aRightC, theEnd);
}

float Sexy::Bezier::SegmentArcLength(int theSegment, float theStart, float theEnd)
{
    if (theEnd <= theStart)
        return 0.0f;
    return MeasureCurvePiece(this, theSegment, theStart, theEnd);
}

int Sexy::Bezier::Init(
    TPoint<float> const *thePoints, TPoint<float> const *theControls, float const *theTimes, int theCount)
{
    if (mPointCount != 0)
        return 0;
    if (thePoints == nullptr || theControls == nullptr || theTimes == nullptr || theCount <= 1)
        return 0;

    const int aSegments = theCount - 1;
    mPoints = new float[2 * theCount];
    mDerivativePoints = new float[4 * aSegments];
    mTimes = new float[theCount];
    mPointCount = theCount;

    for (int i = 0; i < theCount; ++i)
    {
        mPoints[2 * i] = thePoints[i].mX;
        mPoints[2 * i + 1] = thePoints[i].mY;
        mTimes[i] = theTimes[i];
    }
    for (int i = 0; i < 2 * aSegments; ++i)
    {
        mDerivativePoints[2 * i] = theControls[i].mX;
        mDerivativePoints[2 * i + 1] = theControls[i].mY;
    }

    mSegmentLengths = new float[aSegments];
    mTotalLength = 0.0f;
    for (int i = 0; i < aSegments; ++i)
    {
        mSegmentLengths[i] = SegmentArcLength(i, 0.0f, 1.0f);
        mTotalLength += mSegmentLengths[i];
    }
    return 1;
}

int Sexy::Bezier::Init(TPoint<float> const *thePoints, float const *theTimes, int theCount)
{
    if (mPointCount != 0)
        return 0;
    if (thePoints == nullptr || theTimes == nullptr || theCount <= 1)
        return 0;

    const int aSegments = theCount - 1;
    mPoints = new float[2 * theCount];
    mDerivativePoints = new float[4 * aSegments];
    mTimes = new float[theCount];
    mPointCount = theCount;
    for (int i = 0; i < 4 * aSegments; ++i)
        mDerivativePoints[i] = 0.0f;

    for (int i = 0; i < theCount; ++i)
    {
        mPoints[2 * i] = thePoints[i].mX;
        mPoints[2 * i + 1] = thePoints[i].mY;
        mTimes[i] = theTimes[i];
    }

    for (int i = 1; i <= theCount - 2; ++i)
    {
        const float aDx = (mPoints[2 * (i + 1)] - mPoints[2 * (i - 1)]) * kControlScale;
        const float aDy = (mPoints[2 * (i + 1) + 1] - mPoints[2 * (i - 1) + 1]) * kControlScale;
        const int aIncoming = 4 * (i - 1) + 2;
        mDerivativePoints[aIncoming] = mPoints[2 * i] - aDx;
        mDerivativePoints[aIncoming + 1] = mPoints[2 * i + 1] - aDy;
        const int aOutgoing = 4 * i;
        mDerivativePoints[aOutgoing] = mPoints[2 * i] + aDx;
        mDerivativePoints[aOutgoing + 1] = mPoints[2 * i + 1] + aDy;
    }

    const float aFirstDx = (mPoints[2] - mPoints[0]) * kControlScale;
    const float aFirstDy = (mPoints[3] - mPoints[1]) * kControlScale;
    mDerivativePoints[0] = mDerivativePoints[2] - aFirstDx;
    mDerivativePoints[1] = mDerivativePoints[3] - aFirstDy;

    const int aLast = aSegments - 1;
    const float aLastDx = (mPoints[2 * (theCount - 1)] - mPoints[2 * (theCount - 2)]) * kControlScale;
    const float aLastDy = (mPoints[2 * (theCount - 1) + 1] - mPoints[2 * (theCount - 2) + 1]) * kControlScale;
    mDerivativePoints[4 * aLast + 2] = mDerivativePoints[4 * aLast] + aLastDx;
    mDerivativePoints[4 * aLast + 3] = mDerivativePoints[4 * aLast + 1] + aLastDy;

    mSegmentLengths = new float[aSegments];
    mTotalLength = 0.0f;
    for (int i = 0; i < aSegments; ++i)
    {
        mSegmentLengths[i] = SegmentArcLength(i, 0.0f, 1.0f);
        mTotalLength += mSegmentLengths[i];
    }
    return 1;
}

float Sexy::Bezier::ArcLength(float theStart, float theEnd)
{
    if (theEnd <= theStart || mPointCount < 2)
        return 0.0f;

    float aStart = theStart;
    if (aStart < mTimes[0])
        aStart = mTimes[0];
    float aEnd = mTimes[mPointCount - 1];
    if (aEnd > theEnd)
        aEnd = theEnd;
    if (aEnd <= aStart)
        return 0.0f;

    const CurveSpan aStartSpan = FindSpan(mTimes, mPointCount, aStart);
    const CurveSpan aEndSpan = FindSpan(mTimes, mPointCount, aEnd);
    const float aStartDenom = aStartSpan.mTimeEnd - *aStartSpan.mTime;
    const float aEndDenom = aEndSpan.mTimeEnd - *aEndSpan.mTime;
    const float aStartU = (aStartDenom != 0.0f) ? (aStart - *aStartSpan.mTime) / aStartDenom : 0.0f;
    const float aEndU = (aEndDenom != 0.0f) ? (aEnd - *aEndSpan.mTime) / aEndDenom : 0.0f;

    if (aEndSpan.mIndex == aStartSpan.mIndex)
        return SegmentArcLength(aStartSpan.mIndex, aStartU, aEndU);

    float aLength = SegmentArcLength(aStartSpan.mIndex, aStartU, 1.0f);
    for (int i = aStartSpan.mIndex + 1; i < aEndSpan.mIndex; ++i)
        aLength += mSegmentLengths[i];
    return aLength + SegmentArcLength(aEndSpan.mIndex, 0.0f, aEndU);
}
