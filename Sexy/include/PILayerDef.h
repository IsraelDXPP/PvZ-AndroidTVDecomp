// One particle layer: the emitters placed on it, the deflectors, blockers and
// forces that affect them, the layer offset curve, the cached offset the
// reader stores from that curve, and the layer spin curve.

#pragma once

#include "ReconTypes.h"
#include "PIBlocker.h"
#include "PIDeflector.h"
#include "PIEmitterInstanceDef.h"
#include "PIForce.h"
#include "PIValue.h"
#include "PIValue2D.h"

#include <vector>

namespace Sexy {

class PILayerDef
{
public:
    Sexy::String mName;
    std::vector<Sexy::PIEmitterInstanceDef> mEmitters;
    std::vector<Sexy::PIDeflector> mDeflectors;
    std::vector<Sexy::PIBlocker> mBlockers;
    std::vector<Sexy::PIForce> mForces;
    Sexy::PIValue2D mValue2D;
    float mOffsetX;
    float mOffsetY;
    Sexy::PIValue mSpin;

    PILayerDef()
        : mName(), mEmitters(), mDeflectors(), mBlockers(), mForces(), mValue2D(),
          mOffsetX(0.0f), mOffsetY(0.0f), mSpin()
    {
    }
    PILayerDef(Sexy::PILayerDef const& other);
};

} // namespace Sexy

using Sexy::PILayerDef;
