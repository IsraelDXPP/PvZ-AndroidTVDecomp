// DataArray<GridItem> -- reconstructed from libGameMain.so.
//
// This file is the transcription of one instantiation of the TodLib container
// `template <typename T> class DataArray` (authoritative source
// PvZ\TodLib\Common\DataArray.h:36). The instantiation has no vtable and no base
// class.
//
// Size 0x1C: the template holds seven words -- block pointer, maxUsedCount,
// maxSize, freeListHead, size, nextKey and a debug name. Board's own member
// table agrees: `mGridItems` spans ordinals "105 ~ 111", i.e. seven words.
// The recorder's 0x20 was a rounding artifact of its offset heuristic.
//
// The element stride is sizeof(GridItem) + 4 == 0x12C + 4 == 0x130, because
// `DataArray<T>::DataArrayItem` is `{ T mItem; uint32_t mID; }`. Board.cpp:6587 /
// 6595 walk `mGridItems` with that 304-byte stride, and Board.cpp:6593 reads the
// trailing id at `+302`.

#pragma once

#include "ReconTypes.h"

class DataArray_GridItem
{
public:
    // `DataArrayItem *mBlock` -- the element array, stride 0x130.
    void *mBlock;               // +0x00
    unsigned int mMaxUsedCount; // +0x04
    unsigned int mMaxSize;      // +0x08
    unsigned int mFreeListHead; // +0x0C
    unsigned int mSize;         // +0x10
    unsigned int mNextKey;      // +0x14
    const char *mName;          // +0x18
    // 0x1C
};


// The element stride, stated where the container is defined.
inline constexpr int DATA_ARRAY_GRIDITEM_STRIDE = 0x130;
