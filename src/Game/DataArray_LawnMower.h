// DataArray_LawnMower -- the DataArray<LawnMower> instance Board holds.
//
// This tree has no DataArray<T> template -- only the flattened DataArray_Xxx
// classes -- so the instance is spelled out here with the template's own member
// names, from PvZ\TodLib\Common\DataArray.h.
//
// The element stride is **sizeof(LawnMower) + 4 = 0x58**, because the template
// stores
//     struct DataArrayItem { T mItem; uint32_t mID; };
// and LawnMower is 0x54 (its own header records the proof). The translation unit
// never allocates the array itself with a size that distinguishes 0x1C from 0x20,
// so the trailing word the allocation sites ask for keeps an offset-based name.

#pragma once

#include "ReconTypes.h"
#include "LawnMower.h"
#include <cstdint>

class DataArray_LawnMower
{
public:
    void *mBlock;            // +0x00, DataArrayItem *, the array storage
    uint32_t mMaxUsedCount;  // +0x04
    uint32_t mMaxSize;       // +0x08
    uint32_t mFreeListHead;  // +0x0C
    uint32_t mSize;          // +0x10
    uint32_t mNextKey;       // +0x14
    const char *mName;       // +0x18
};

