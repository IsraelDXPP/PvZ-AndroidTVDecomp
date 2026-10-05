#pragma once

#include "ReconTypes.h"
#include "Trail.h"
#include "TrailDefinition.h"
#include "DataArray.h"

// One DataArray<Trail> and nothing else. The destructor disposes the array
// twice because the binary calls DisposeHolder and then DataArrayDispose on
// the same address.
class TrailHolder
{
public:
    DataArray<Trail, TrailSlot> mTrails;

    TrailHolder();
    int InitializeHolder();
    void DisposeHolder();
    ~TrailHolder();
    Trail* AllocTrailFromDef(int theRenderOrder, TrailDefinition* theDefinition);
    Trail* AllocTrail(int theRenderOrder, TrailType theTrailType);
};

// Trail (0xF8) plus the DataArray id word.
inline constexpr int DATA_ARRAY_TRAIL_STRIDE = 0xFC;
