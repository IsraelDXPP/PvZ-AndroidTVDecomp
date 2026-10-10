// Handle the effect queries receive. FindLive matches it to the playing
// emitter by the definition pointer. Tint, image, transform and the offset
// are the values LoadEffect used to publish on the instance itself.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_PIEMITTERINSTANCE_H
#define LAWNPROJECTTV_SEXY_INCLUDE_PIEMITTERINSTANCE_H

#include "SexyTransform2D.h"
#include "SharedImageRef.h"

namespace Sexy
{

class PIEmitterInstanceDef;

class PIEmitterInstance
{
public:
    PIEmitterInstanceDef *mDef;
    Color mTint;
    SharedImageRef mImage;
    SexyTransform2D mTransform;
    float mOffsetX;
    float mOffsetY;
    float mRateScale;
    bool mActive;

    PIEmitterInstance();
    PIEmitterInstance(const PIEmitterInstance &other);
};

} // namespace Sexy

using Sexy::PIEmitterInstance;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_PIEMITTERINSTANCE_H
