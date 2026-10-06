#ifndef __DATAARRAY_H__
#define __DATAARRAY_H__

#include <cstddef>

enum
{
    DATA_ARRAY_INDEX_MASK = 65535,
    DATA_ARRAY_KEY_MASK = -65536,
    DATA_ARRAY_KEY_SHIFT = 16,
    DATA_ARRAY_MAX_SIZE = 65536,
    DATA_ARRAY_KEY_FIRST = 1
};

// The slot records. Each is `{ T mItem; unsigned int mID; }`, completed at the
// end of T's own header. Naming them here lets a holder embed DataArray<T>
// while T is still incomplete: the array stores only a pointer.
struct ZombieSlot;
struct PlantSlot;
struct ProjectileSlot;
struct CoinSlot;
struct LawnMowerSlot;
struct GridItemSlot;
struct ReanimationSlot;
struct ParticleSystemSlot;
struct ParticleEmitterSlot;
struct ParticleSlot;
struct TrailSlot;
struct AttachmentSlot;

struct DataArrayIdNode
{
    unsigned int mValue;
    DataArrayIdNode* mNext;
    DataArrayIdNode* mPrev;
};

// One array of T. Slot is the record named above. The method bodies are not
// in this header: a method that constructs or destroys T can only be compiled
// after T is complete, and each item header includes DataArrayMethods.h once
// its class and its slot are finished.
//
// The block is raw bytes. new Slot[N] would construct every slot, and
// DataArrayAlloc then constructs the one slot it hands out a second time.
// operator delete also does not match a new[] cookie, so DataArrayDispose
// destroys the live items and releases the block with operator delete[].
template <typename T, typename Slot>
class DataArray
{
public:
    Slot *mBlock;
    unsigned int mMaxUsedCount;
    unsigned int mMaxSize;
    unsigned int mFreeListHead;
    unsigned int mSize;
    unsigned int mNextKey;
    const char *mName;

public:
    DataArray()
    {
        mBlock = nullptr;
        mMaxUsedCount = 0;
        mMaxSize = 0;
        mFreeListHead = 0;
        mSize = 0;
        mNextKey = DATA_ARRAY_KEY_FIRST;
        mName = nullptr;
    }

    // Defined with the other methods, after T is complete. A holder that is
    // destroyed while T is still incomplete must call DataArrayDispose itself.
    void DataArrayInitialize(unsigned int theMaxSize, const char *theName);
    void DataArrayDispose();
    void DataArrayFree(T *theItem);
    void DataArrayFreeAll();
    unsigned int DataArrayGetID(T *theItem);
    bool IterateNext(T *&theItem);
    T *DataArrayAlloc();
    T *DataArrayTryToGet(unsigned int theId);
    T *DataArrayGet(unsigned int theId);
};

#endif
