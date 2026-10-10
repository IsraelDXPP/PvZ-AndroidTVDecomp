#include "PIDeflector.h"

#include "Bezier.h"
#include "PIValue.h"
#include "PIValue2D.h"

Sexy::PIDeflector::PIDeflector(Sexy::PIDeflector const &other) :
    mName(other.mName),
    mBounce(other.mBounce),
    mHitChance(other.mHitChance),
    mThickness(other.mThickness),
    mVisible(other.mVisible),
    mOffset(other.mOffset),
    mActive(other.mActive),
    mSpin(other.mSpin),
    mPointCurves(other.mPointCurves),
    mPoints(other.mPoints)
{
}
