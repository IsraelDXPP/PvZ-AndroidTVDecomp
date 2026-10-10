#include "PIEmitterInstanceDef.h"

Sexy::PIEmitterInstanceDef::PIEmitterInstanceDef(const Sexy::PIEmitterInstanceDef &other) :
    mCurve(other.mCurve),
    mValue2D(other.mValue2D),
    mCurves(other.mCurves)
{
    for (int index = 0; index < 19; ++index)
        mValues[index] = other.mValues[index];
}
