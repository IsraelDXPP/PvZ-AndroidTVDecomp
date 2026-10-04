// DataArray_Zombie -- the `DataArray<Zombie>` instantiation, reconstructed from
// libGameMain.so.
//
// A plain data record: no vtable, no base class, nothing virtual.
//
// SIZE 0x1C -- seven 32-bit fields -- not the 0x20 the transcription wrote.
// Three independent sources agree on the field list and therefore on the size:
//   * the IDA type export libGameMain115.h:7929 spells the whole template out:
//     `struct DataArray { int *mBlock; unsigned int mMaxUsedCount;
//      unsigned int mMaxSize; unsigned int mFreeListHead; unsigned int mSize;
//      unsigned int mNextKey; const char *mName; };`
//   * the authoritative template PvZ\TodLib\Common\DataArray.h:44 declares exactly
//     those seven members, in that order;
//   * the binary's own bodies address the receiver as words 0..6:
//     _recon/ida/DataArray_Zombie.cpp uses a1[0] (mBlock), a1[1] (mMaxUsedCount),
//     a1[3] (mFreeListHead), a1[4] (mSize), a1[5] (mNextKey) and `a1 + 24`
//     (mName), and DataArrayDispose clears `a1 + 8` and `a1 + 12` as well.
// The type is only ever embedded -- Board holds its `DataArray<Zombie> mZombies` at
// Board + 280 (Board.cpp:14029) -- so no allocation site pins a different size.
//
// The element type is `Zombie`: the binary's DataArrayAlloc memsets 0x184 bytes per
// slot and constructs a Zombie in place. `struct DataArrayItem { T mItem;
// uint32_t mID; }` (PvZ\TodLib\Common\DataArray.h:39) makes the array stride
// sizeof(Zombie) + 4 == 0x188, which is the `392 * index` the same body uses.
//
// TODO(recon): the element type is not spelled out here on purpose -- including
// Zombie.h from this header would create a cycle, since Zombie.h is what the array
// holds. A real `template <class T> class DataArray` in the tree would replace this
// per-instantiation file, as build\map-lawn.md's SaveGame section notes.

#pragma once

#include "ReconTypes.h"

class DataArray_Zombie
{
public:
    void*        mBlock;         // +0x00  base of the item block
    unsigned int mMaxUsedCount;  // +0x04  one past the highest slot ever handed out
    unsigned int mMaxSize;       // +0x08  capacity in items
    unsigned int mFreeListHead;  // +0x0C  head of the free list, by slot index
    unsigned int mSize;          // +0x10  live item count
    unsigned int mNextKey;       // +0x14  next id; wrapped at DATA_ARRAY_MAX_SIZE
    const char*  mName;          // +0x18  diagnostic name passed to DataArrayInitialize
    // Size 0x1C.
};

