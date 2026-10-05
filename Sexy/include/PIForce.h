// A force field: a named polygon that pushes particles that fall inside it.
// mEnabled is the CForce bool. mStrength, mDirection, mActive, mSpin, mExtra
// and mAngle are the six curves the reader fills, in that order. mArrowOrigin
// is the point debug draw adds to the rotated arrow tip.

#pragma once

#include "ReconTypes.h"
#include "Bezier.h"
#include "PIValue.h"
#include "PIValue2D.h"

namespace Sexy {

class PIForce
{
public:
    Sexy::String mName;
    bool mEnabled;
    Sexy::PIValue2D mValue2D;
    Sexy::PIValue mStrength;
    Sexy::PIValue mDirection;
    Sexy::PIValue mActive;
    Sexy::PIValue mSpin;
    Sexy::PIValue mExtra;
    Sexy::PIValue mAngle;
    TPoint<float> mCorners[4];
    int mArrowOriginX;
    int mArrowOriginY;

    PIForce()
        : mName(), mEnabled(false), mValue2D(), mStrength(), mDirection(), mActive(),
          mSpin(), mExtra(), mAngle(), mArrowOriginX(0), mArrowOriginY(0)
    {
    }
    PIForce(Sexy::PIForce const& other);
};

} // namespace Sexy

using Sexy::PIForce;
