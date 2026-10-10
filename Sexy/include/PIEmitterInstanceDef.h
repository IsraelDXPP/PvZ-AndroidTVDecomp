// A placed emitter: the free-spawn type indices, the position curve, the
// nineteen motion curves, and the extra path points.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_PIEMITTERINSTANCEDEF_H
#define LAWNPROJECTTV_SEXY_INCLUDE_PIEMITTERINSTANCEDEF_H

#include "PIValue.h"
#include "PIValue2D.h"

#include <vector>

namespace Sexy
{

class PIEmitterInstanceDef
{
public:
    std::vector<int> mCurve;
    Sexy::PIValue2D mValue2D;
    Sexy::PIValue mValues[19];
    std::vector<Sexy::PIValue2D> mCurves;

    PIEmitterInstanceDef() :
        mCurve(),
        mValue2D(),
        mCurves()
    {
    }
    PIEmitterInstanceDef(const PIEmitterInstanceDef &other);
};

} // namespace Sexy

using Sexy::PIEmitterInstanceDef;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_PIEMITTERINSTANCEDEF_H
