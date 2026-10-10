// Bezier is a piecewise cubic curve. Each span is two points plus two control
// points. Evaluate, Velocity and Acceleration return a shared two-float buffer
// that the next call overwrites. Arc length is measured by adaptive subdivision.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_BEZIER_H
#define LAWNPROJECTTV_SEXY_INCLUDE_BEZIER_H

#include "ReconTypes.h"

namespace Sexy
{

class Bezier
{
public:
    float *mTimes;
    float *mSegmentLengths;
    float mTotalLength;
    int mPointCount;
    float *mDerivativePoints;
    float *mPoints;
    int mUnusedFlag;

    Bezier();
    ~Bezier();
    Bezier(Sexy::Bezier const &theOther);
    Bezier &operator=(Sexy::Bezier const &theOther);
    void Clean();
    float *Evaluate(float theTime);
    float *Velocity(float theTime, int theClampToEnd);
    float *Acceleration(float theTime);
    float SubdivideLength(TPoint<float> const &theStart, TPoint<float> const &theControlA,
        TPoint<float> const &theControlB, TPoint<float> const &theEnd);
    float SegmentArcLength(int theSegment, float theStart, float theEnd);
    int Init(TPoint<float> const *thePoints, TPoint<float> const *theControls, float const *theTimes, int theCount);
    int Init(TPoint<float> const *thePoints, float const *theTimes, int theCount);
    float ArcLength(float theStart, float theEnd);
};

} // namespace Sexy

using Sexy::Bezier;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_BEZIER_H
