#include "PILayerDef.h"

#include "PIBlocker.h"
#include "PIDeflector.h"
#include "PIEmitterInstanceDef.h"
#include "PIForce.h"
#include "PIValue.h"
#include "PIValue2D.h"

Sexy::PILayerDef::PILayerDef(Sexy::PILayerDef const& other)
    : mName(other.mName),
      mEmitters(other.mEmitters),
      mDeflectors(other.mDeflectors),
      mBlockers(other.mBlockers),
      mForces(other.mForces),
      mValue2D(other.mValue2D),
      mOffsetX(other.mOffsetX),
      mOffsetY(other.mOffsetY),
      mSpin(other.mSpin)
{
}
