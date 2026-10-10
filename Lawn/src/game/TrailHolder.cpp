#include "TrailHolder.h"

#include "Trail.h"
#include "DataArrayMethods.h"
#include "TrailDefinition.h"
#include <cstring>

TrailHolder::TrailHolder() = default;

int TrailHolder::InitializeHolder()
{
    mTrails.DataArrayInitialize(1024, "trails");
    mTrails.mNextKey = 53857;
    // EffectSystem::Update returns this value. No caller reads the result.
    return 0;
}

void TrailHolder::DisposeHolder()
{
    mTrails.DataArrayDispose();
}

TrailHolder::~TrailHolder()
{
    // The binary calls DisposeHolder and then DataArrayDispose on the same
    // address. With no separate base, the second pass is a no-op.
    DisposeHolder();
    DisposeHolder();
}

// theRenderOrder is the TV allocator's first argument. The body never stores
// it, so the trail's mRenderOrder stays at the constructor's 0.
Trail *TrailHolder::AllocTrailFromDef(int /*theRenderOrder*/, TrailDefinition *theDefinition)
{

    if (mTrails.mSize == mTrails.mMaxSize)
        return nullptr;

    Trail *aTrail = mTrails.DataArrayAlloc();
    aTrail->mTrailHolder = this;
    aTrail->mDefinition = theDefinition;

    float aDuration = FloatTrackEvaluate(theDefinition->mTrailDuration, 0.0f, Rand(1.0f));
    aTrail->mTrailDuration = (int)aDuration;
    return aTrail;
}

Trail *TrailHolder::AllocTrail(int theRenderOrder, TrailType theTrailType)
{
    return AllocTrailFromDef(theRenderOrder, gTrailDefArray + theTrailType);
}
