// DataArray_Plant -- `DataArray<Plant>` instantiated for the board's plant array.
//
// No base class and no vtable: DataArray<T> is a plain container (see
// PvZ\TodLib\Common\DataArray.h) and the binary has no vtable symbol for this
// instantiation. So nothing is virtual here.
//
// SIZE 0x1C, NOT 0x20.
//   DataArray<T> is seven words. The IDA type export agrees field for field:
//   libGameMain115.h:7929 `struct DataArray { int *mBlock; unsigned int
//   mMaxUsedCount; unsigned int mMaxSize; unsigned int mFreeListHead; unsigned int
//   mSize; unsigned int mNextKey; const char *mName; };`
//   and the strides of the six arrays Board embeds prove it: they sit at +280
//   (Zombie), +308 (Plant), +336 (Projectile), +364 (Coin), +392 (LawnMower) and
//   +420 (GridItem) in Board.cpp:19865-19870, i.e. exactly 28 (0x1C) bytes apart.
//   The transcriber's 0x20 was that extent rounded up.
//
// ELEMENT STRIDE. `DataArray<T>::DataArrayItem` is `{ T mItem; uint32_t mID; }`, so
// the array's stride is sizeof(T) + 4. For T = Plant the binary uses a stride of
// 352 (0x160): DataArray<Plant>::DataArrayDispose walks the block with
// `v2 += 176` on an `unsigned __int16 *` (176 * 2 = 352), and it reads the element
// id at `v2[174]` / writes it at `*((_DWORD *)v2 + 87)`, i.e. byte +0x15C.
// Therefore sizeof(Plant) = 0x15C and sizeof(DataArrayItem<Plant>) = 0x160 -- the
// same pair Game\Plant.h documents.
//
// The template itself lives only in the reference tree
// (PvZ\TodLib\Common\DataArray.h), which is not on this project's include path, so
// the member table is expanded in place. Nothing in this tree names this class --
// only the project file lists it -- so the expansion is safe.

#pragma once

#include "ReconTypes.h"

struct PlantSlot;

class DataArray_Plant
{
public:
    // PlantSlot storage, stride 0x160. Plant.h includes this header before Plant
    // is complete, so the slot type is completed at the end of Plant.h.
    PlantSlot * mBlock;         // +0x00
    unsigned int mMaxUsedCount; // +0x04
    unsigned int mMaxSize;      // +0x08  Board initialises it to 1024
    unsigned int mFreeListHead; // +0x0C
    unsigned int mSize;         // +0x10
    unsigned int mNextKey;      // +0x14
    const char * mName;         // +0x18  "plants"
};

