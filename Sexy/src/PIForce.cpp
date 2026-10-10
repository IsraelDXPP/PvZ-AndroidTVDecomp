#include "PIForce.h"

#include "Bezier.h"
#include "PIValue.h"

Sexy::PIForce::PIForce(Sexy::PIForce const &other) :
    mName(other.mName),
    mEnabled(other.mEnabled),
    mValue2D(other.mValue2D),
    mStrength(other.mStrength),
    mDirection(other.mDirection),
    mActive(other.mActive),
    mSpin(other.mSpin),
    mExtra(other.mExtra),
    mAngle(other.mAngle)
{
    for (int corner = 0; corner < 4; ++corner)
        mCorners[corner] = other.mCorners[corner];

    mArrowOriginX = other.mArrowOriginX;
    mArrowOriginY = other.mArrowOriginY;
}
