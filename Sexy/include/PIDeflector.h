// A particle deflector: a named polyline that bounces particles that cross it.
// mBounce, mHitChance and mThickness are the three floats the reader stores
// first. mActive gates the bounce, mSpin is the second curve, mOffset is the
// value curve, mPointCurves the per-vertex curves and mPoints the polyline.

#pragma once

#include "ReconTypes.h"
#include "Bezier.h"
#include "PIValue.h"
#include "PIValue2D.h"

#include <vector>

namespace Sexy {

class PIDeflector
{
public:
    Sexy::String mName;
    float mBounce;
    float mHitChance;
    float mThickness;
    bool mVisible;
    Sexy::PIValue2D mOffset;
    Sexy::PIValue mActive;
    Sexy::PIValue mSpin;
    std::vector<Sexy::PIValue2D> mPointCurves;
    std::vector<TPoint<float>> mPoints;

    PIDeflector()
        : mName(), mBounce(0.0f), mHitChance(0.0f), mThickness(0.0f), mVisible(false),
          mOffset(), mActive(), mSpin(), mPointCurves(), mPoints()
    {
    }
    PIDeflector(Sexy::PIDeflector const& other);
};

} // namespace Sexy

using Sexy::PIDeflector;
