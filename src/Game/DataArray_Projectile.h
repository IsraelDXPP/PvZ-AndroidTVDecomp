// DataArray_Projectile -- the binary's `DataArray<Projectile>` instantiation.
//
// DataArray<T> itself is the template in PvZ\TodLib\Common\DataArray.h. Its
// layout is seven words (0x1C bytes):
//     DataArrayItem<T> *mBlock;   uint32_t mMaxUsedCount; uint32_t mMaxSize;
//     uint32_t mFreeListHead;     uint32_t mSize;         uint32_t mNextKey;
//     const char *mName;
// and its element is
//     template <typename T> struct DataArrayItem { T mItem; uint32_t mID; };
// so the ARRAY STRIDE is sizeof(T) + 4. For this instantiation Projectile is
// 0xA0 bytes (the authoritative PvZ\Lawn\Board\Projectile.h and the allocation
// site agree), which makes the stride 0xA4.
//
// The transcriber's opaque 0x20-byte placeholder was the stride arithmetic, not
// sizeof(DataArray<T>).

#pragma once

#include "ReconTypes.h"
// Needed for the stride assertion below, which requires a complete Projectile.
#include "Projectile.h"

class DataArray_Projectile
{
public:
    Projectile   *mBlock;         // +0x00
    unsigned int  mMaxUsedCount;  // +0x04
    unsigned int  mMaxSize;       // +0x08
    unsigned int  mFreeListHead;  // +0x0C
    unsigned int  mSize;          // +0x10
    unsigned int  mNextKey;       // +0x14
    const char   *mName;          // +0x18
    // size 0x1C
};

// Layout check: seven words, exactly as the template.

// The element stride the binary uses for this array.
